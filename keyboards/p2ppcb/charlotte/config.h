#pragma once

/* key matrix size */
#ifdef SPLIT_KEYBOARD
    #define MATRIX_ROWS 18 * 2
#else
    #define MATRIX_ROWS 18
#endif
#define MATRIX_NROWS 16
#define MATRIX_LROWS 2
#define MATRIX_COLS 12
#define N_LCOLS 3

/* Split keyboard is I2C */
#ifdef SPLIT_KEYBOARD
# define USE_I2C
# define SPLIT_LED_STATE_ENABLE
#endif

#define I2C_MASTER_DRIVER I2CD1
#define I2C_SLAVE_DRIVER I2CD1
#define I2C1_SDA_PIN GP14
#define I2C1_SCL_PIN GP15
