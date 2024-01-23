#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "matrix.h"
#include QMK_KEYBOARD_H
#include "bitbang_i2c.h"
#include "rp2040_common.h"

static bool tester_connected = false;
static bool test_error = false;

// MCP23018 registers
const uint8_t GPIOA = 0x12;
const uint8_t GPIOB = 0x13;

const bitbang_i2caddr_t I2C_ADDR = 0b100000;  // MCP23018
const int connected_col_left[] = {4, 5, 6, 7, 1};
const int connected_col_right[] = {8, 9, 10, 11, 2};
const int n_connected_col = 5;

static inline bool check_col_error(uint32_t col_error) {
    int n_err = 0;
    for (int i = 0; i < n_connected_col; i++) {
        if (col_error & (1U << i)) {
            n_err ++;
        }
    }
    switch (n_err) {
        case 0:
            test_error = false;
            return true;
        case 1:
            test_error = true;
            return true;
        default:
            test_error = false;
            return false;
    }
}

volatile bool read_stat[100];
static bool is_tester_connected(void) {
    uint32_t col_error = 0U;
    for (int i = 0; i < n_connected_col; i++) {
        setPinInputHigh(COLS[connected_col_left[i]]);
        setPinOutput(COLS[connected_col_right[i]]);
        writePinHigh(COLS[connected_col_right[i]]);
    }
    chThdSleep(TIME_US2I(1));
    for (int i = 0; i < n_connected_col; i++) {
        if (!readPin(COLS[connected_col_left[i]])) {
            col_error |= (1U << i);
        }
    }
    if (!check_col_error(col_error)) {
        return false;
    }
    for (int i = 0; i < n_connected_col; i++) {
        setPinInputLow(COLS[connected_col_left[i]]);
    }
    chThdSleep(TIME_US2I(1));
    for (int i = 0; i < n_connected_col; i++) {
        if (!readPin(COLS[connected_col_left[i]])) {
            col_error |= (1U << i);
        }
    }
    if (!check_col_error(col_error)) {
        return false;
    }
    for (int i = 0; i < n_connected_col; i++) {
        writePinLow(COLS[connected_col_right[i]]);
    }
    chThdSleep(TIME_US2I(1));
    for (int i = 0; i < n_connected_col; i++) {
        if (readPin(COLS[connected_col_left[i]])) {
            col_error |= (1U << i);
        }
    }
    return check_col_error(col_error);
}

/**
 * @brief Read an entire control register
 *
 * @param address The address of the particular register
 * @return The data read from the particular register
 */
static uint8_t readFromRegister(uint8_t address) {
    uint8_t txbuf[] = {address};
    uint8_t rxbuf[1];
    msg_t msg = bitbangI2cMasterTransmit(&BitbangI2CD1, I2C_ADDR, txbuf, 1, rxbuf, 1);
    if (msg != MSG_OK) {
        test_error = true;
        return 0U;
    }
    return rxbuf[0];
}

static void run_tester(void) {
    tester_connected = is_tester_connected();
    if (!tester_connected || test_error) {
        return;
    }

    setPinOutput(GP25);  // on-board LED
    writePinLow(GP25);

    // init shift register 74164
    setPinOutput(DS);
    writePinLow(DS);
    setPinOutput(CP);
    writePinLow(CP);
    for (int i = 0; i < N_NORMAL_ROWS; i ++) {
        wait_200ns();
        writePinHigh(CP);
        wait_200ns();
        writePinLow(CP);
    }

    for (int i = 0; i < N_NORMAL_COLS; i ++) {
        setPinInputLow(COLS[i]);
    }

    // connect to I/O Expander by bitbang I2C
    bitbangI2cInit();
    BitbangI2CConfig config = {
        .addr10 = false,
        .scl = GP5,
        .sda = GP2,
        .ticks = TIME_US2I(50)
    };
    bitbangI2cStart(&BitbangI2CD1, &config);  // always success

    uint32_t row_error = 0U;

    for (int i = 0; i < N_NORMAL_ROWS; i++) {
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

        int current_row = ROWS[i];
        uint32_t gpio_status = readFromRegister(GPIOA) + (readFromRegister(GPIOB) << 8);
        if (gpio_status != (1U << current_row)) {
            row_error |= (1U << current_row);
        }
    }
    wait_200ns();
    writePinHigh(CP);
    wait_200ns();
    writePinLow(CP);

    if (row_error) {
        test_error = true;
    }

    switch (detect_board()) {
        case BOARD_TYPE_BOB:
            return;
        case BOARD_TYPE_ERROR:
            test_error = true;
            return;
    }

    setPinOutput(LED_SCAN);
    writePinLow(LED_SCAN);
    for (int i = 0; i < N_LED_COLS; i ++) {
        setPinOutput(DRIVE_LEDS[i]);
        writePinHigh(DRIVE_LEDS[i]);
    }
    for (int i = 0; i < N_LED_ROWS; i ++) {
        setPinOutput(ENABLE_LEDS[i]);
        writePinHigh(ENABLE_LEDS[i]);
    }
}

bool test_by_tester_if_connected(void) {
    run_tester();
    if (!tester_connected) {
        return false;
    }
    if (!test_error) {
        writePinHigh(GP25);
        return true;
    }

    flicker_led();
    return true;
}

#ifdef OLED_ENABLE
static void render_logo(void) {
    static const char PROGMEM qmk_logo[] = {
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94,
        0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4,
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0x00
    };

    oled_write_P(qmk_logo, false);
}

void keyboard_post_init_user(void) {
    if (tester_connected) {
        if (is_oled_on()) {
            render_logo();
        } else {
            flicker_led();
        }
    }
}
#endif
