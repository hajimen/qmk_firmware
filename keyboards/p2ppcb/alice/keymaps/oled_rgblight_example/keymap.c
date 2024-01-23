#include QMK_KEYBOARD_H
#include "i2c_master.h"

#define CHECK_ROWS(x) {x,  x + 1, x + 2, x + 3, x + 4, x + 5, x + 6, x + 7, x + 8, x + 9, x + 10, x + 11, x + 12, x + 13, x + 14, x + 15, x + 16, x + 17, x + 18, x + 19, x + 20, x + 21, x + 22, x + 23}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = {
        CHECK_ROWS(4),
        CHECK_ROWS(MATRIX_COLS + 4),
        CHECK_ROWS(MATRIX_COLS * 2 + 4),
        CHECK_ROWS(MATRIX_COLS * 3 + 4),
        CHECK_ROWS(MATRIX_COLS * 4 + 4),
        CHECK_ROWS(MATRIX_COLS * 5 + 4),
        CHECK_ROWS(4),
        CHECK_ROWS(MATRIX_COLS + 4),
        CHECK_ROWS(MATRIX_COLS * 2 + 4),
        CHECK_ROWS(MATRIX_COLS * 3 + 4),
    },
    [1] = {REP_ROWS({REP_COLS(KC_NO)})},
    [2] = {REP_ROWS({REP_COLS(KC_NO)})},
    [3] = {REP_ROWS({REP_COLS(KC_NO)})}
};

static void render_logo(void) {
    static const char PROGMEM qmk_logo[] = {
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94,
        0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4,
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0x00
    };

    oled_write_P(qmk_logo, false);
}

void keyboard_post_init_user(void) {
    rgblight_enable_noeeprom();
    rgblight_sethsv_noeeprom(180, 10, 10);
    rgblight_mode_noeeprom(RGBLIGHT_MODE_BREATHING + 3);
    i2c_init();
    if (is_oled_on()) {
        render_logo();
    }
}
