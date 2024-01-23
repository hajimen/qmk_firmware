/*
    Copyright (C) 2022 NAKAZATO Hajime
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

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
 * @file    charlotte_i2c.c
 * @brief   I2C Driver code. Slave mode is available.
 * @note    This module is one of HAL community part in terms of ChibiOS.
 *
 * @addtogroup I2C
 * @{
 */
#include "hal.h"

#if (CHARLOTTE_USE_I2C == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   ChibiOS HAL initialization (community part). Invoked by @p halInit().
 *
 * @init
 */
void halCommunityInit(void) {
  i2cInit();
}

/**
 * @brief   I2C Driver initialization.
 *
 * @init
 */
void i2cInit(void) {

  i2c_lld_init();
}

/**
 * @brief   Initializes @p I2CDriver structure.
 *
 * @param[out] i2cp     pointer to the @p I2CDriver object
 *
 * @init
 */
void i2cObjectInit(I2CDriver *i2cp) {

  i2cp->state  = I2C_STOP;
  i2cp->config = NULL;
  i2cp->slave_active = false;

#if I2C_USE_MUTUAL_EXCLUSION == TRUE
  osalMutexObjectInit(&i2cp->mutex);
#endif

#if defined(I2C_DRIVER_EXT_INIT_HOOK)
  I2C_DRIVER_EXT_INIT_HOOK(i2cp);
#endif
}

/**
 * @brief   Configures and activates the I2C peripheral as master-active.
 * @note    Master-active and slave-active are mutual exclusive.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] config    pointer to the @p I2CConfig object
 * @return              The operation status.
 *
 * @api
 */
msg_t i2cStart(I2CDriver *i2cp, const I2CConfig *config) {

  osalDbgCheck((i2cp != NULL) && (config != NULL));

  osalSysLock();
  osalDbgAssert((i2cp->state == I2C_STOP) || (i2cp->state == I2C_READY && !i2cp->slave_active) ||
                (i2cp->state == I2C_LOCKED && !i2cp->slave_active), "invalid state");

  i2cp->config = config;
  i2cp->slave_active = false;

  i2c_lld_start(i2cp);
  i2cp->state = I2C_READY;

  osalSysUnlock();

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Configures and activates the I2C peripheral as slave-active.
 * @note    Master-active and slave-active are mutual exclusive.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] config    pointer to the @p I2CConfig object
 * @return              The operation status.
 *
 * @api
 */
msg_t i2cSlaveStart(I2CDriver *i2cp, const I2CConfig *config, i2c_slave_handler_t slave_handler, uint8_t slave_address) {

  osalDbgCheck((i2cp != NULL) && (config != NULL));

  osalSysLock();
  osalDbgAssert((i2cp->state == I2C_STOP) || (i2cp->state == I2C_READY && i2cp->slave_active) ||
                (i2cp->state == I2C_LOCKED && i2cp->slave_active), "invalid state");

  i2cp->config = config;
  i2cp->slave_active = true;
  i2cp->slave_address = slave_address;
  i2cp->slave_handler = slave_handler;

  i2c_lld_start(i2cp);
  i2cp->state = I2C_READY;

  osalSysUnlock();

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the I2C peripheral.
 * @note    Master-active and slave-active both.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @api
 */
void i2cStop(I2CDriver *i2cp) {

  osalDbgCheck(i2cp != NULL);

  osalSysLock();

  i2c_lld_stop(i2cp);
  i2cp->config = NULL;
  i2cp->state  = I2C_STOP;
  i2cp->slave_active = false;

  osalSysUnlock();
}

/**
 * @brief   Returns the errors mask associated to the previous operation.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @return              The errors mask.
 *
 * @api
 */
i2cflags_t i2cGetErrors(I2CDriver *i2cp) {

  osalDbgCheck(i2cp != NULL);

  return i2c_lld_get_errors(i2cp);
}

/**
 * @brief   Sends data via the I2C bus.
 * @details Function designed to realize "read-through-write" transfer
 *          paradigm. If you want transmit data without any further read,
 *          than set @b rxbytes field to 0.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address (7 bits) without R/W bit
 * @param[in] txbuf     pointer to transmit buffer
 * @param[in] txbytes   number of bytes to be transmitted
 * @param[out] rxbuf    pointer to receive buffer
 * @param[in] rxbytes   number of bytes to be received, set it to 0 if
 *                      you want transmit only
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end.
 *
 * @api
 */
msg_t i2cMasterTransmitTimeout(I2CDriver *i2cp,
                               i2caddr_t addr,
                               const uint8_t *txbuf,
                               size_t txbytes,
                               uint8_t *rxbuf,
                               size_t rxbytes,
                               sysinterval_t timeout) {
  msg_t rdymsg;

  osalDbgCheck((i2cp != NULL) &&
               (txbytes > 0U) && (txbuf != NULL) &&
               ((rxbytes == 0U) || ((rxbytes > 0U) && (rxbuf != NULL))) &&
               (timeout != TIME_IMMEDIATE));

  osalDbgAssert(i2cp->state == I2C_READY, "not ready");
  osalDbgAssert(!i2cp->slave_active, "slave mode is active");

  osalSysLock();
  i2cp->errors = I2C_NO_ERROR;
  rdymsg = i2c_lld_master_transmit_timeout(i2cp, addr, txbuf, txbytes,
                                           rxbuf, rxbytes, timeout);
  osalSysUnlock();
  return rdymsg;
}

/**
 * @brief   Receives data from the I2C bus.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address (7 bits) without R/W bit
 * @param[out] rxbuf    pointer to receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end.
 *
 * @api
 */
msg_t i2cMasterReceiveTimeout(I2CDriver *i2cp,
                              i2caddr_t addr,
                              uint8_t *rxbuf,
                              size_t rxbytes,
                              sysinterval_t timeout) {

  msg_t rdymsg;

  osalDbgCheck((i2cp != NULL) && (addr != 0U) &&
               (rxbytes > 0U) && (rxbuf != NULL) &&
               (timeout != TIME_IMMEDIATE));

  osalDbgAssert(i2cp->state == I2C_READY, "not ready");
  osalDbgAssert(!i2cp->slave_active, "slave mode is active");

  osalSysLock();
  i2cp->errors = I2C_NO_ERROR;
  rdymsg = i2c_lld_master_receive_timeout(i2cp, addr, rxbuf, rxbytes, timeout);
  osalSysUnlock();
  return rdymsg;
}

#if (I2C_USE_MUTUAL_EXCLUSION == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Gains exclusive access to the I2C bus.
 * @details This function tries to gain ownership to the I2C bus, if the bus
 *          is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option @p I2C_USE_MUTUAL_EXCLUSION
 *          must be enabled.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @api
 */
void i2cAcquireBus(I2CDriver *i2cp) {

  osalDbgCheck(i2cp != NULL);

  osalMutexLock(&i2cp->mutex);
}

/**
 * @brief   Releases exclusive access to the I2C bus.
 * @pre     In order to use this function the option @p I2C_USE_MUTUAL_EXCLUSION
 *          must be enabled.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @api
 */
void i2cReleaseBus(I2CDriver *i2cp) {

  osalDbgCheck(i2cp != NULL);

  osalMutexUnlock(&i2cp->mutex);
}
#endif /* I2C_USE_MUTUAL_EXCLUSION == TRUE */

#endif /* CHARLOTTE_USE_I2C == TRUE */

/** @} */
