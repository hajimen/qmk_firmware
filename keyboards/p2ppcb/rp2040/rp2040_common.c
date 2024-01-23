#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "matrix.h"
#include QMK_KEYBOARD_H
#include "rp2040_test.h"
#include "rp2040_common.h"

static virtual_timer_t flicker_vt;

#define P2PPCB_NOP() asm volatile("nop")

void wait_200ns(void) {
    // 125 MHz
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();

    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();

    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
    P2PPCB_NOP();
}

static bool detect_led_driver(pin_t led_pin, pin_t drive_pin) {
    bool ret;
    while (true) {
        palSetLineMode(led_pin, PAL_MODE_INPUT_PULLUP & (~PAL_RP_PAD_SCHMITT));
        setPinOutput(drive_pin);
        writePinLow(drive_pin);
        chThdSleep(TIME_US2I(10));
        if (!readPin(led_pin)) {
            ret = false;
            break;
        }
        writePinHigh(drive_pin);
        chThdSleep(TIME_US2I(10));
        ret = !readPin(led_pin);
        break;
    }

    // high-z
    setPinInput(led_pin);
    setPinInput(drive_pin);

    return ret;
}

board_type detect_board(void) {
    int c = 0;
    for (int i = 0; i < N_LED_COLS; i ++) {
        c += detect_led_driver(LED_COLS[i], DRIVE_LEDS[i]);
    }
    if (c == 0) {
        return BOARD_TYPE_BOB;
    } else if (c == N_LED_COLS) {
        return BOARD_TYPE_CHARLOTTE;
    } else {
        return BOARD_TYPE_ERROR;
    }
}

// You can live without test code
__attribute__((weak)) bool test_by_tester_if_connected() {
    return false;
}

bool check_board_type(board_type bt) {
    if (detect_board() != bt) {
        flicker_led();
        return false;
    }
    return !test_by_tester_if_connected();
}

static bool last_led_onoff = false;
static void flicker_callback(virtual_timer_t *vtp, void *p) {
    writePin(GP25, last_led_onoff);
    last_led_onoff = !last_led_onoff;
}

void flicker_led(void) {
    chVTObjectInit(&flicker_vt);
    chVTSetContinuous(&flicker_vt, TIME_MS2I(200), flicker_callback, NULL);
}
