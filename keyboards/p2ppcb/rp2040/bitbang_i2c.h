/*
    Copyright (C) 2022 NAKAZATO Hajime

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
/*
   Concepts and parts of this file have been contributed by Uladzimir Pylinsky
   aka barthess.
 */

/**
 * @file    bitbang_i2c.h
 * @brief   I2C Driver by bitbang.
 *
 * @{
 */

#pragma once

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/* TODO: To be reviewed, too STM32-centric.*/
/**
 * @name    I2C bus error conditions
 * @{
 */
#define BITBANG_I2C_NO_ERROR               0x00    /**< @brief No error.            */
#define BITBANG_I2C_BUS_ERROR              0x01    /**< @brief Bus Error.           */
#define BITBANG_I2C_ARBITRATION_LOST       0x02    /**< @brief Arbitration Lost.    */
#define BITBANG_I2C_ACK_FAILURE            0x04    /**< @brief Acknowledge Failure. */
#define BITBANG_I2C_OVERRUN                0x08    /**< @brief Overrun/Underrun.    */
#define BITBANG_I2C_PEC_ERROR              0x10    /**< @brief PEC Error in
                                                reception.                  */
#define BITBANG_I2C_TIMEOUT                0x20    /**< @brief Hardware timeout.    */
#define BITBANG_I2C_SMB_ALERT              0x40    /**< @brief SMBus Alert.         */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  BITBANG_I2C_UNINIT = 0,                           /**< @brief Not initialized.    */
  BITBANG_I2C_STOP = 1,                             /**< @brief Stopped.            */
  BITBANG_I2C_READY = 2,                            /**< @brief Ready.              */
  BITBANG_I2C_ACTIVE_TX = 3,                        /**< @brief Transmitting.       */
  BITBANG_I2C_ACTIVE_RX = 4,                        /**< @brief Receiving.          */
  BITBANG_I2C_LOCKED = 5                            /**< @brief Bus locked.         */
} bitbang_i2cstate_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Get errors from I2C driver.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
#define i2c_lld_get_errors(i2cp) ((i2cp)->errors)

/**
 * @brief   Wakes up the waiting thread notifying no errors.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
#define _i2c_wakeup_isr(i2cp) do {                                          \
  osalSysLockFromISR();                                                     \
  osalThreadResumeI(&(i2cp)->thread, MSG_OK);                               \
  osalSysUnlockFromISR();                                                   \
} while (0)

/**
 * @brief   Wakes up the waiting thread notifying errors.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
#define _i2c_wakeup_error_isr(i2cp) do {                                    \
  osalSysLockFromISR();                                                     \
  osalThreadResumeI(&(i2cp)->thread, MSG_RESET);                            \
  osalSysUnlockFromISR();                                                   \
} while (0)

/**
 * @brief   Wrap i2cMasterTransmitTimeout function with TIME_INFINITE timeout.
 * @api
 */
#define bitbangI2cMasterTransmit(i2cp, addr, txbuf, txbytes, rxbuf, rxbytes)       \
  (bitbangI2cMasterTransmitTimeout(i2cp, addr, txbuf, txbytes, rxbuf, rxbytes,     \
                            TIME_INFINITE))

/**
 * @brief   Wrap i2cMasterReceiveTimeout function with TIME_INFINITE timeout.
 * @api
 */
#define bitbangI2cMasterReceive(i2cp, addr, rxbuf, rxbytes)                        \
  (bitbangI2cMasterReceiveTimeout(i2cp, addr, rxbuf, rxbytes, TIME_INFINITE))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/
/**
 * @brief   Type representing an I2C address.
 */
typedef uint16_t bitbang_i2caddr_t;

/**
 * @brief   Type of I2C driver condition flags.
 */
typedef uint8_t bitbang_i2cflags_t;

/**
 * @brief   I2C driver configuration structure.
 */
struct bitbang_hal_i2c_config {
  /**
   * @brief   10 bits addressing switch.
   */
  bool                      addr10;
  /**
   * @brief   I2C clock line.
   */
  ioline_t                  scl;
  /**
   * @brief   I2C data line.
   */
  ioline_t                  sda;
  /**
   * @brief   Delay of an half bit time in system ticks.
   */
  systime_t                 ticks;
};

/**
 * @brief   Type of a structure representing an I2C configuration.
 */
typedef struct bitbang_hal_i2c_config BitbangI2CConfig;

/**
 * @brief   Type of a structure representing an I2C driver.
 */
typedef struct bitbang_hal_i2c_driver BitbangI2CDriver;

/**
 * @brief   Structure representing an I2C driver.
 */
struct bitbang_hal_i2c_driver {
  /**
   * @brief   Driver state.
   */
  bitbang_i2cstate_t        state;
  /**
   * @brief   Current configuration data.
   */
  const BitbangI2CConfig    *config;
  /**
   * @brief   Error flags.
   */
  bitbang_i2cflags_t        errors;
  /* End of the mandatory fields.*/
  /**
   * @brief   Time of operation begin.
   */
  systime_t                 start;
  /**
   * @brief   Time of operation timeout.
   */
  systime_t                 end;
};

#ifdef __cplusplus
extern "C" {
#endif
  extern BitbangI2CDriver BitbangI2CD1;
  void bitbangI2cInit(void);
  void bitbangI2cObjectInit(BitbangI2CDriver *i2cp);
  msg_t bitbangI2cStart(BitbangI2CDriver *i2cp, const BitbangI2CConfig *config);
  void bitbangI2cStop(BitbangI2CDriver *i2cp);
  bitbang_i2cflags_t bitbangI2cGetErrors(BitbangI2CDriver *i2cp);
  msg_t bitbangI2cMasterTransmitTimeout(BitbangI2CDriver *i2cp,
                                        bitbang_i2caddr_t addr,
                                        const uint8_t *txbuf, size_t txbytes,
                                        uint8_t *rxbuf, size_t rxbytes,
                                        sysinterval_t timeout);
  msg_t bitbangI2cMasterReceiveTimeout(BitbangI2CDriver *i2cp,
                                       bitbang_i2caddr_t addr,
                                       uint8_t *rxbuf, size_t rxbytes,
                                       sysinterval_t timeout);
#ifdef __cplusplus
}
#endif

/** @} */
