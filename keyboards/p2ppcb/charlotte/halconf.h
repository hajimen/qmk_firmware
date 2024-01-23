#pragma once
#define HAL_USE_ADC TRUE
#undef HAL_USE_I2C
#define HAL_USE_I2C FALSE  // Charlotte uses CHARLOTTE_USE_I2C for I2C slave.
#define I2C1_CLOCK_SPEED 100000  // 100000 Hz (100 kHz) Fast Mode is upper limit for TRRS cable. You can redefine I2C1_CLOCK_SPEED if you don't use TRRS cable.
#include_next <halconf.h>

#ifndef CHARLOTTE_USE_I2C
# define CHARLOTTE_USE_I2C TRUE
#endif
#define HAL_USE_COMMUNITY CHARLOTTE_USE_I2C  //Community HAL is Charlotte I2C
