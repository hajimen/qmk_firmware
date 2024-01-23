/**
 * matrix.c
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "quantum.h"
#include "matrix.h"
#include QMK_KEYBOARD_H

static void wait_200ns(void) {
    // 48MHz
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}

static const uint16_t LED_SW_THREASHOLD = 3100;
static const pin_t    LDRIVE1           = C14;
static const pin_t    LSCAN             = C15;
static const pin_t    PWM               = A6;  // Unused pin. Always H.
static const pin_t    RGB_DI            = A7;
static const pin_t    NROWS[]           = { C13, B9, B8, B7, B6, B5, B4, B3 };
static const pin_t    nCOL_PL           = B11;
static const pin_t    COL_CP            = B10;
static const int      N_COL_PER_CG      = 8;
static const pin_t    CGS[]             = {B2, B1, B0};
static const int      N_CG              = 3;
static const ioline_t LD_SDI            = A8;
static const ioline_t LD_LE             = B12;
static const ioline_t LD_CLK            = B15;

#define PAL_PORT_LCOL_MASK (PAL_PORT_BIT(0) | PAL_PORT_BIT(1) | PAL_PORT_BIT(2) | PAL_PORT_BIT(3) | PAL_PORT_BIT(4) | PAL_PORT_BIT(5))
#define ADC_CHSELR_0_5 (ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL2 | ADC_CHSELR_CHSEL3 | ADC_CHSELR_CHSEL4 | ADC_CHSELR_CHSEL5)

// Create buffer to store ADC results. This is one-dimensional interleaved array
#define ADC_BUF_DEPTH 6  // depth of buffer
#define ADC_CH_NUM    1  // number of used ADC channels
static volatile adcsample_t samples_buf[ADC_BUF_DEPTH * ADC_CH_NUM];  // results buffer array
static volatile bool now_scanning = false;
static volatile int  i_lrow = 0;
static volatile bool matrix_led_sw[MATRIX_LROWS][N_LCOLS] = {0};
volatile bool MATRIX_LED_ONOFF[MATRIX_LROWS][N_LCOLS] = {0};
static volatile bool suspended = false;
static volatile virtual_timer_t repeat_vt;
static volatile virtual_timer_t one_shot_vt;

static void set_led_col(void) {
    writePinLow(LD_CLK);
    writePinLow(LD_SDI);
    for (int i = N_COL_PER_CG - 1; i >= 0; i--) {
        wait_200ns();
        if (i < N_LCOLS && MATRIX_LED_ONOFF[i_lrow][i]) {
            writePinHigh(LD_SDI);
        }
        if (i == 0) {
            writePinHigh(LD_LE);
        }
        writePinHigh(LD_CLK);
        wait_200ns();
        writePinLow(LD_CLK);
        if (i == 0) {
            writePinLow(LD_LE);
        }
        writePinLow(LD_SDI);
    }
}

static void turn_off_led_col(void) {
    writePinLow(LD_CLK);
    writePinLow(LD_SDI);
    for (int i = N_COL_PER_CG - 1; i >= 0; i--) {
        wait_200ns();
        if (i == 0) {
            writePinHigh(LD_LE);
        }
        writePinHigh(LD_CLK);
        wait_200ns();
        writePinLow(LD_CLK);
        if (i == 0) {
            writePinLow(LD_LE);
        }
    }
}

static void adc_callback(ADCDriver *adcp) {
    if (now_scanning) {
        for (int i = 0; i < N_LCOLS; i++) {
            matrix_led_sw[i_lrow][i] = (samples_buf[i] > LED_SW_THREASHOLD);
        }
        i_lrow = (i_lrow + 1) % MATRIX_LROWS;

        writePinLow(LSCAN);
        writePin(LDRIVE1, i_lrow == 0);
        palSetGroupMode(GPIOA, PAL_PORT_LCOL_MASK, 0, PAL_MODE_OUTPUT_OPENDRAIN);
        palWriteGroup(GPIOA, PAL_PORT_LCOL_MASK, 0, 0b111111U);  // high-z
        if (!suspended) {
            set_led_col();
        }
        now_scanning = false;
    }
}

static ADCConfig adccfg = {};
static ADCConversionGroup adccg = {
    TRUE,          // set to TRUE if need circular buffer, set FALSE otherwise
    ADC_CH_NUM,    // number of channels
    adc_callback,  // callback function, set to NULL for begin
    NULL,          // error callback function

    ADC_CFGR1_RES_12BIT, /* CFGR1 */
    ADC_TR(0, 0),        /* TR */
    ADC_SMPR_SMP_1P5,    /* SMPR */
    ADC_CHSELR_0_5       /* CHSELR */
};

static void one_shot_callback(virtual_timer_t *vtp, void *arg) {
    if (now_scanning) {
        chSysLockFromISR();
        adcStartConversionI(&ADCD1, &adccg, (adcsample_t *)samples_buf, ADC_BUF_DEPTH);
        chSysUnlockFromISR();
    }
}

static void repeat_callback(virtual_timer_t *vtp, void *arg) {
    if (now_scanning) {  // should not be occur, but to make sure
        (void)now_scanning;
    } else {
        now_scanning = true;
        writePinHigh(LSCAN);
        palSetGroupMode(GPIOA, PAL_PORT_LCOL_MASK, 0, PAL_MODE_INPUT_ANALOG);
        chSysLockFromISR();
        chVTSetI((virtual_timer_t *)(&one_shot_vt), TIME_US2I(1), one_shot_callback, NULL);  // wait 1 us and start ADC
        chSysUnlockFromISR();
    }
    chSysLockFromISR();
    chVTSetI((virtual_timer_t *)(&repeat_vt), TIME_MS2I(10), repeat_callback, NULL);
    chSysUnlockFromISR();
}

static void init_pins(void) {
    setPinOutput(LD_CLK);
    writePinLow(LD_CLK);

    setPinOutput(LD_SDI);
    writePinLow(LD_SDI);

    writePinLow(LD_LE);

    setPinOutput(nCOL_PL);
    writePinHigh(nCOL_PL);

    setPinOutput(COL_CP);
    writePinLow(COL_CP);

    setPinOutput(LDRIVE1);
    writePinHigh(LDRIVE1);

    setPinOutput(LSCAN);
    writePinLow(LSCAN);

    setPinOutput(PWM);
    writePinHigh(PWM);

    setPinInput(RGB_DI);  // high-Z

    for (int i = 0; i < MATRIX_NROWS; i++) {
        pin_t row_pin = NROWS[i];
        setPinOutput(row_pin);
        writePinLow(row_pin);
    }
    for (int i = 0; i < N_CG; i++) {
        setPinInput(CGS[i]);
    }
}

static void init_adc_interval(void) {
    adcStart(&ADCD1, &adccfg);
    palSetGroupMode(GPIOA, PAL_PORT_LCOL_MASK, 0, PAL_MODE_OUTPUT_OPENDRAIN);
    palWriteGroup(GPIOA, PAL_PORT_LCOL_MASK, 0, 0b111111U);

    chVTObjectInit((virtual_timer_t *)(&repeat_vt));
    chVTObjectInit((virtual_timer_t *)(&one_shot_vt));
    chVTSet((virtual_timer_t *)(&repeat_vt), TIME_MS2I(10), repeat_callback, NULL);
}

static bool read_cols_on_row(matrix_row_t matrix[], uint8_t i_row) {
    // Start with a clear matrix row
    matrix_row_t current_row = 0;

    // Select row and wait for row selecton to stabilize
    if (i_row >= MATRIX_NROWS) {
        for (int i = 0; i < N_LCOLS; i++) {
            current_row |= matrix_led_sw[i_row - MATRIX_NROWS][i] ? (MATRIX_ROW_SHIFTER << i) : 0;
        }
    } else {
        pin_t row_pin = NROWS[i_row];
        // select row
        writePinHigh(row_pin);

        chThdSleep(TIME_US2I(1));

        // load cols to shift registers
        writePinLow(COL_CP);
        writePinLow(nCOL_PL);  // sampling start
        wait_200ns();
        writePinHigh(nCOL_PL);  // sampling finished

        // unselect row
        writePinLow(row_pin);

        // For each col group...
        for (int i = 0; i < N_COL_PER_CG; i++) {
            wait_200ns();
            for (int ii = 0; ii < N_CG; ii++) {
                // active high
                uint8_t pin_state = readPin(CGS[ii]);
                // Populate the matrix row with the state of the col pin
                current_row |= pin_state ? (MATRIX_ROW_SHIFTER << (ii * N_COL_PER_CG + N_COL_PER_CG - i - 1)) : 0;
            }
            writePinHigh(COL_CP);
            wait_200ns();
            writePinLow(COL_CP);
        }
    }

    // If the row has changed, store the row and return the changed flag.
    if (matrix[i_row] != current_row) {
        matrix[i_row] = current_row;
        return true;
    }

    return false;
}

void matrix_init_custom(void) {
    init_pins();
    turn_off_led_col();
    init_adc_interval();
}

bool matrix_scan_custom(matrix_row_t matrix[]) {
    bool changed = false;

    for (uint8_t i_row = 0; i_row < MATRIX_ROWS; i_row++) {
        changed |= read_cols_on_row(matrix, i_row);
    }

    return changed;
}

void suspend_wakeup_init_user(void) {
    if (suspended) {
        i_lrow = 0;
        suspended = false;
    }
}

void suspend_power_down_user(void) {
    if (!suspended) {
        suspended = true;
    }
}
