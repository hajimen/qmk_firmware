#pragma once

#include "quantum.h"

extern volatile bool MATRIX_LED_ONOFF[MATRIX_LROWS][N_LCOLS];

#define REP_COLS(x) (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x), (x)
#define REP_ROWS(x) x, x, x, x, x, x, x, x, x, x
