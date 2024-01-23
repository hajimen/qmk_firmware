#include QMK_KEYBOARD_H
#include <stdio.h>

/*
 * VIA has 4 layers.
 */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = {
        {KC_6, KC_5, KC_4, KC_3, KC_2, KC_1, KC_ESCAPE, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_BACKSLASH, KC_T, KC_R, KC_E, KC_W, KC_Q, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_QUOTE, KC_G, KC_F, KC_D, KC_S, KC_A, KC_TAB, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_RIGHT_GUI, KC_B, KC_V, KC_C, KC_X, KC_Z, KC_DELETE, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_LEFT_SHIFT, KC_NO, KC_SPACE, KC_F4, KC_F3, KC_F2, KC_F1, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_LEFT_ALT, KC_LEFT_CTRL, KC_NO, KC_F8, KC_F7, KC_F6, KC_F5, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_F12, KC_F11, KC_F10, KC_F9, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_LANGUAGE_2, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },

        {KC_BACKSPACE, KC_EQUAL, KC_MINUS, KC_0, KC_9, KC_8, KC_7, KC_GRAVE, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_RIGHT_BRACKET, KC_LEFT_BRACKET, KC_P, KC_O, KC_I, KC_U, KC_Y, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_ENTER, KC_NO, KC_L, KC_K, KC_J, KC_H, KC_SEMICOLON, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_COMMA, KC_M, KC_N, KC_SLASH, MO(1), KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_PAGE_UP, KC_HOME, KC_DOT, KC_LEFT, KC_UP, KC_SPACE, KC_RIGHT_SHIFT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_PAGE_DOWN, KC_END, KC_NO, KC_RIGHT, KC_DOWN, KC_RIGHT_CTRL, KC_RIGHT_ALT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_LANGUAGE_1, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
    },
    [1] = {
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_RIGHT_GUI, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_INSERT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_LEFT_SHIFT, KC_NO, KC_NO, KC_NO, KC_PAUSE, KC_PRINT_SCREEN, KC_APPLICATION, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_LEFT_ALT, KC_LEFT_CTRL, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_CAPS_LOCK, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },

        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_RIGHT_SHIFT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_RIGHT_CTRL, KC_RIGHT_ALT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_SCROLL_LOCK, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
        {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, },
    },
    [2] = {REP_ROWS({REP_COLS(KC_NO)}), REP_ROWS({REP_COLS(KC_NO)})},
    [3] = {REP_ROWS({REP_COLS(KC_NO)}), REP_ROWS({REP_COLS(KC_NO)})}
};

bool led_update_user(led_t led_state) {
    if (is_keyboard_left()) {
        MATRIX_LED_ONOFF[0][0] = led_state.caps_lock;
    } else {
        MATRIX_LED_ONOFF[0][1] = led_state.scroll_lock;
    }
    return false;
}

bool is_keyboard_left(void) {
    return is_led_connected(0, 0);
}
