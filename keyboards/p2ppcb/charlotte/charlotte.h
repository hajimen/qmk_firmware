#pragma once

#include "quantum.h"

// Set / unset this variable to on / off LEDs.
extern volatile bool MATRIX_LED_ONOFF[MATRIX_LROWS][N_LCOLS];

/*
 * P2PPCB doesn't use LAYOUT macro.
*/
#define REP_COLS(x) (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x)
#define REP_ROWS(x) x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x

/**
 * @brief Gets LED connection of LED_Sn and LED_Dn. Must be called only before matrix_init_custom().
*/
bool is_led_connected(int led_s, int led_d);
