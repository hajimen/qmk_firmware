#pragma once

// Common constants of Bob and Charlotte
static const pin_t    DS                = 0;
static const pin_t    CP                = 1;
static const pin_t    COLS[]            = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static const int      ROWS[]            = {6, 4, 2, 0, 1, 3, 5, 7, 14, 12, 10, 8, 9, 11, 13, 15};

// Constants of Charlotte
static const pin_t    LED_COLS[]        = {26, 27, 28};
static const pin_t    DRIVE_LEDS[]      = {22, 21, 20};
static const pin_t    ENABLE_LEDS[]     = {18, 19};
static const pin_t    LED_SCAN          = 17;
static const int      N_LED_COLS        = 3;
static const int      N_LED_ROWS        = 2;
static const int      N_NORMAL_COLS     = 12;
static const int      N_NORMAL_ROWS     = 16;

typedef int board_type;
static const board_type BOARD_TYPE_ERROR      = 0;
static const board_type BOARD_TYPE_BOB        = 1;
static const board_type BOARD_TYPE_CHARLOTTE  = 2;

/**
 * @brief Waits 200 ns by NOP.
*/
void wait_200ns(void);

/**
 * @brief Returns board type. BOARD_TYPE_ERROR, BOARD_TYPE_BOB, or BOARD_TYPE_CHARLOTTE.
*/
board_type detect_board(void);

/**
 * @brief Returns false is bt is not the board.
*/
bool check_board_type(board_type bt);

/**
 * @brief Flickers LED on RPi Pico.
*/
void flicker_led(void);
