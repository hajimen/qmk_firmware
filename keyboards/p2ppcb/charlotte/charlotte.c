/*
    Copyright (C) 2022 NAKAZATO Hajime

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "matrix.h"
#include QMK_KEYBOARD_H
#include "rp2040_common.h"

#ifdef SPLIT_KEYBOARD
#include "timer.h"
#include "usb_util.h"
#include "i2c_slave.h"
static const uint32_t SLAVE_SUSPEND_THREASHOLD = 500;
static volatile uint32_t last_i2c_transaction = 0U;
#endif // SPLIT_KEYBOARD

static bool cols_enabled[N_NORMAL_COLS] = {};

static const uint16_t LED_SW_THREASHOLD = 3000;
static const uint16_t LED_CONNECTION_THREASHOLD = 160;

static virtual_timer_t repeat_vt;
static virtual_timer_t one_shot_vt;
static const time_conv_t ONE_SHOT_US    = 1;
static const time_conv_t REPEAT_MS      = 4;

volatile bool MATRIX_LED_ONOFF[MATRIX_LROWS][N_LCOLS];
static int i_lrow = -1;
static volatile bool matrix_led_sw[MATRIX_LROWS][N_LCOLS];
static volatile bool suspended = false;

static const uint8_t ADC_CH_MASK        = 0b111;  // GP26 to GP28
#define ADC_BUFFER_DEPTH                1U
static volatile bool now_scanning = false;
static adcsample_t adc_samples_buf[ADC_BUFFER_DEPTH * N_LCOLS];

static bool no_scan = false;

static void __not_in_flash_func(adc_callback)(ADCDriver *adcp) {
    if (now_scanning) {
        for (int i = 0; i < N_LCOLS; i++) {
            matrix_led_sw[i_lrow][i] = (adc_samples_buf[i] > LED_SW_THREASHOLD);
        }

        writePinLow(LED_SCAN);

        // drive leds
        if (!suspended) {
            for (int i = 0; i < N_LCOLS; i++) {
                writePin(DRIVE_LEDS[i], MATRIX_LED_ONOFF[i_lrow][i]);
            }
        }

        i_lrow = (i_lrow + 1) % MATRIX_LROWS;

        now_scanning = false;
    }
}

static void adc_error_callback(ADCDriver *adcp, adcerror_t err) {
    if (now_scanning) {
        i_lrow = (i_lrow + 1) % MATRIX_LROWS;
        now_scanning = false;
    }
}

static ADCConfig adccfg = {};
static ADCConversionGroup adccg = {
    .circular = false,                   // set to TRUE if need circular buffer, set FALSE otherwise
    .num_channels = N_LCOLS,             // number of channels to convert
    .channel_mask = ADC_CH_MASK,         // channels to convert
    .end_cb = adc_callback,              // callback function
    .error_cb = adc_error_callback,      // error callback function
};

static void __not_in_flash_func(one_shot_callback)(virtual_timer_t *vtp, void *p) {
    if (now_scanning) {
        // I think these lines below should be in adc_lld_start_conversion() of ChibiOS HAL.
        ADCD1.current_buffer_position = 0;
        ADCD1.current_channel = 0;
        ADCD1.current_iteration = 0;

        chSysLockFromISR();
        adcStartConversionI(&ADCD1, &adccg, adc_samples_buf, ADC_BUFFER_DEPTH);
        chSysUnlockFromISR();
    }
}

static void __not_in_flash_func(repeat_callback)(virtual_timer_t *vtp, void *p) {
    if (now_scanning) {  // should not occur, but to make sure
        (void)now_scanning;
    } else {
        now_scanning = true;
        // scan keys
        for (int i = 0; i < MATRIX_LROWS; i++) {
            writePin(ENABLE_LEDS[i], i == i_lrow);
        }
        for (int i = 0; i < N_LCOLS; i++) {
            writePinLow(DRIVE_LEDS[i]);
        }
        writePinHigh(LED_SCAN);

        chSysLockFromISR();
        // wait for switching / discharging
        chVTSetI(&one_shot_vt, TIME_US2I(ONE_SHOT_US), one_shot_callback, NULL);
        chSysUnlockFromISR();
    }
}

void matrix_init_custom(void) {
    no_scan = !check_board_type(BOARD_TYPE_CHARLOTTE);
    if (no_scan) {
        return;
    }

    setPinOutput(GP25);  // on-board LED
    writePinLow(GP25);

    suspended = false;

    setPinOutput(DS);
    writePinLow(DS);
    setPinOutput(CP);
    writePinLow(CP);
    for (int i = 0; i < MATRIX_NROWS; i ++) {
        wait_200ns();
        writePinHigh(CP);
        wait_200ns();
        writePinLow(CP);
    }
    for (int i = 0; i < MATRIX_COLS; i ++) {
        cols_enabled[i] = true;
#ifdef RESERVED_D_WIRES
        for (int ii = 0; ii < sizeof(RESERVED_D_WIRES) / sizeof(pin_t); ii++) {
            if (COLS[i] == RESERVED_D_WIRES[ii]) {
                cols_enabled[i] = false;
                break;
            }
        }
#endif
        if (cols_enabled[i]) {
            setPinInputLow(COLS[i]);
        }
    }

    memset((void *)MATRIX_LED_ONOFF, 0, sizeof(MATRIX_LED_ONOFF));
    memset((void *)matrix_led_sw, 0, sizeof(matrix_led_sw));

    adcStart(&ADCD1, &adccfg);
    for (int i = 0; i < N_LCOLS; i ++) {
        palSetLineMode(LED_COLS[i], PAL_MODE_INPUT_ANALOG);
        setPinOutput(DRIVE_LEDS[i]);
        writePinLow(DRIVE_LEDS[i]);
    }
    for (int i = 0; i < MATRIX_LROWS; i ++) {
        setPinOutput(ENABLE_LEDS[i]);
        writePinLow(ENABLE_LEDS[i]);
    }
    setPinOutput(LED_SCAN);
    writePinLow(LED_SCAN);

    chVTObjectInit(&repeat_vt);
    chVTObjectInit(&one_shot_vt);
    now_scanning = false;
    i_lrow = 0;
    chVTSetContinuous(&repeat_vt, TIME_MS2I(REPEAT_MS), repeat_callback, NULL);
}

bool matrix_scan_custom(matrix_row_t raw_matrix[]) {
    if (no_scan) {
        return false;
    }

    matrix_row_t matrix[MATRIX_ROWS] = {0};
    for (int i = 0; i < MATRIX_NROWS; i++) {
        if (i == 0) {
            writePinHigh(DS);
        }
        wait_200ns();
        writePinHigh(CP);
        wait_200ns();
        if (i == 0) {
            writePinLow(DS);
        }
        writePinLow(CP);

        // wait for switching/discharging
        chThdSleep(TIME_US2I(1));

        matrix_row_t r = 0U;
        uint32_t all = palReadPort(0);
        for (int ii = 0; ii < MATRIX_COLS; ii++) {
            if (cols_enabled[ii] && ((1U << COLS[ii]) & all)) {
                r |= (1U << ii);
            }
        }
        matrix[ROWS[i]] = r;

        if (i == MATRIX_NROWS - 1) {
            wait_200ns();
            writePinHigh(CP);
            wait_200ns();
            writePinLow(CP);
        }
    }
    for (int i = 0; i < MATRIX_LROWS; i++) {
        matrix_row_t r = 0U;
        for (int ii = 0; ii < N_LCOLS; ii++) {
            if (matrix_led_sw[i][ii]) {
                r |= (1U << ii);
            }
        }
        matrix[i + MATRIX_NROWS] = r;
    }

    bool changed = memcmp(raw_matrix, matrix, sizeof(matrix)) != 0;
    if (changed) memcpy(raw_matrix, matrix, sizeof(matrix));

#ifdef SPLIT_KEYBOARD
    // From slave, QMK cannot detect USB suspend.
    uint32_t t = i2c_slave_last_transaction_time();
    if (suspended) {
        if (t != last_i2c_transaction) {
            suspend_wakeup_init();
        }
    } else if (!usb_connected_state()) {
        if (timer_read32() - t > SLAVE_SUSPEND_THREASHOLD) {
            suspend_power_down();
        }
    }
    last_i2c_transaction = t;
#endif // SPLIT_KEYBOARD

    return changed;
}

void suspend_wakeup_init_kb(void) {
    suspended = false;
    suspend_wakeup_init_user();
}

void suspend_power_down_kb(void) {
    suspended = true;
    suspend_power_down_user();
}

bool is_led_connected(int led_s, int led_d) {
    static bool did_led_connection_scan[MATRIX_LROWS] = {false};
    static bool led_connected[MATRIX_LROWS][N_LCOLS];

    if (!did_led_connection_scan[led_s]) {
        // scan led connection
        for (int i = 0; i < N_LCOLS; i ++) {
            palSetLineMode(LED_COLS[i], PAL_MODE_INPUT_ANALOG);
            setPinOutput(DRIVE_LEDS[i]);
            writePinHigh(DRIVE_LEDS[i]);
        }
        for (int i = 0; i < MATRIX_LROWS; i ++) {
            setPinOutput(ENABLE_LEDS[i]);
            writePin(ENABLE_LEDS[i], i == led_s);
        }
        setPinOutput(LED_SCAN);
        writePinLow(LED_SCAN);

        chThdSleep(TIME_US2I(100));

        adcStart(&ADCD1, &adccfg);

        adcConvert(&ADCD1, &adccg, adc_samples_buf, ADC_BUFFER_DEPTH);

        for (int i = 0; i < N_LCOLS; i++) {
            led_connected[led_s][i] = (adc_samples_buf[i] > LED_CONNECTION_THREASHOLD);
        }
        did_led_connection_scan[led_s] = true;
    }

    return led_connected[led_s][led_d];
}

#ifdef SPLIT_KEYBOARD
bool is_keyboard_master(void) {
    // Without this, suspended PC -> connect KB -> wakeup PC sequence fails because
    // the implementation in quantum/split_common/split_util.c memorizes
    // initial state and disconnects USB.
    return usb_connected_state();
}
#endif // SPLIT_KEYBOARD
