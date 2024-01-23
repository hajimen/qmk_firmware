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

/**
 * @file    bitbang_i2c.c
 * @brief   I2C by bitbang
 *
 * @{
 */

#include "hal.h"
#include "bitbang_i2c.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define CHECK_ERROR(msg)                                                    \
  if ((msg) < (msg_t)0) {                                                   \
    palSetLine(i2cp->config->sda);                                          \
    palSetLine(i2cp->config->scl);                                          \
    return MSG_TIMEOUT;                                                     \
  }

/**
 * @brief   Open-drain with pull up resistor.
 */
#define PAL_MODE_OPEN_DRAIN_PULLUP_L    (PAL_RP_IOCTRL_FUNCSEL_SIO       |   \
                                         PAL_RP_GPIO_OE                  |   \
                                         PAL_RP_PAD_PUE                  |   \
                                         PAL_RP_PAD_IE                   |   \
                                         PAL_RP_PAD_SCHMITT)

#define PAL_MODE_OPEN_DRAIN_PULLUP_H    (PAL_RP_IOCTRL_FUNCSEL_SIO       |   \
                                         PAL_RP_PAD_PUE                  |   \
                                         PAL_RP_PAD_IE                   |   \
                                         PAL_RP_PAD_SCHMITT)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief I2C1 driver identifier.*/
BitbangI2CDriver BitbangI2CD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static inline void clear_open_drain(ioline_t pin) {
  palSetLineMode(pin, PAL_MODE_OPEN_DRAIN_PULLUP_L);
}

static inline void set_open_drain(ioline_t pin) {
  palSetLineMode(pin, PAL_MODE_OPEN_DRAIN_PULLUP_H);
}

static inline void write_open_drain(ioline_t pin, bool bit) {
  palSetLineMode(pin, bit ? PAL_MODE_OPEN_DRAIN_PULLUP_H : PAL_MODE_OPEN_DRAIN_PULLUP_L);
}

static msg_t i2c_write_stop(BitbangI2CDriver *i2cp);

static inline void i2c_delay(BitbangI2CDriver *i2cp) {

  osalThreadSleepS(i2cp->config->ticks);
}

static msg_t i2c_check_arbitration(BitbangI2CDriver *i2cp) {

  if (palReadLine(i2cp->config->sda) == PAL_LOW) {
    i2cp->errors |= BITBANG_I2C_ARBITRATION_LOST;
    return MSG_RESET;
  }

  return MSG_OK;
}

static inline msg_t i2c_check_timeout(BitbangI2CDriver *i2cp) {

  if ((i2cp->start != i2cp->end) &&
      (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), i2cp->start, i2cp->end))) {
    i2c_write_stop(i2cp);
    return MSG_TIMEOUT;
  }

  return MSG_OK;
}

static msg_t i2c_wait_clock(BitbangI2CDriver *i2cp) {

  while (palReadLine(i2cp->config->scl) == PAL_LOW) {
    if ((i2cp->start != i2cp->end) &&
        (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), i2cp->start, i2cp->end))) {
      return MSG_TIMEOUT;
    }
    i2c_delay(i2cp);
  }

  return MSG_OK;
}

static inline msg_t i2c_write_start(BitbangI2CDriver *i2cp) {

  /* Arbitration check.*/
  CHECK_ERROR(i2c_check_arbitration(i2cp));

  clear_open_drain(i2cp->config->sda);
  i2c_delay(i2cp);
  clear_open_drain(i2cp->config->scl);
  i2c_delay(i2cp);

  return MSG_OK;
}

static msg_t i2c_write_restart(BitbangI2CDriver *i2cp) {

  set_open_drain(i2cp->config->sda);
  i2c_delay(i2cp);
  set_open_drain(i2cp->config->scl);

  /* Clock stretching.*/
  CHECK_ERROR(i2c_wait_clock(i2cp));

  i2c_delay(i2cp);
  i2c_write_start(i2cp);

  return MSG_OK;
}

static msg_t i2c_write_stop(BitbangI2CDriver *i2cp) {

  clear_open_drain(i2cp->config->sda);
  i2c_delay(i2cp);
  set_open_drain(i2cp->config->scl);

  /* Clock stretching.*/
  CHECK_ERROR(i2c_wait_clock(i2cp));

  i2c_delay(i2cp);
  set_open_drain(i2cp->config->sda);
  i2c_delay(i2cp);

  /* Arbitration check.*/
  CHECK_ERROR(i2c_check_arbitration(i2cp));

  i2c_delay(i2cp);

  return MSG_OK;
}

static msg_t i2c_write_bit(BitbangI2CDriver *i2cp, unsigned bit) {

  write_open_drain(i2cp->config->sda, bit);
  i2c_delay(i2cp);
  set_open_drain(i2cp->config->scl);
  i2c_delay(i2cp);

  /* Clock stretching.*/
  CHECK_ERROR(i2c_wait_clock(i2cp));

  /* Arbitration check.*/
  if (palReadLine(i2cp->config->sda) != (bit ? PAL_HIGH : PAL_LOW)) {
    i2cp->errors |= BITBANG_I2C_ARBITRATION_LOST;
    return MSG_RESET;
  }

  clear_open_drain(i2cp->config->scl);

  return MSG_OK;
}

static msg_t i2c_read_bit(BitbangI2CDriver *i2cp) {
  msg_t bit;

  set_open_drain(i2cp->config->sda);
  i2c_delay(i2cp);
  set_open_drain(i2cp->config->scl);

  /* Clock stretching.*/
  CHECK_ERROR(i2c_wait_clock(i2cp));

  i2c_delay(i2cp);
  bit = palReadLine(i2cp->config->sda);
  clear_open_drain(i2cp->config->scl);

  return bit;
}

static msg_t i2c_write_byte(BitbangI2CDriver *i2cp, uint8_t byte) {
  msg_t msg;
  uint8_t mask;

  CHECK_ERROR(i2c_check_timeout(i2cp));

  for (mask = 0x80U; mask > 0U; mask >>= 1U) {
    CHECK_ERROR(i2c_write_bit(i2cp, (byte & mask) != 0));
  }

  msg = i2c_read_bit(i2cp);
  CHECK_ERROR(msg);

  /* Checking for NACK.*/
  if (msg == PAL_HIGH) {
    i2cp->errors |= BITBANG_I2C_ACK_FAILURE;
    return MSG_RESET;
  }

  return MSG_OK;
}

static msg_t i2c_read_byte(BitbangI2CDriver *i2cp, unsigned nack) {
  msg_t byte;
  unsigned i;

  CHECK_ERROR(i2c_check_timeout(i2cp));

  byte = 0U;
  for (i = 0; i < 8; i++) {
    msg_t msg = i2c_read_bit(i2cp);
    CHECK_ERROR(msg);
    byte = (byte << 1U) | msg;
  }

  CHECK_ERROR(i2c_write_bit(i2cp, nack));

  return byte;
}

static msg_t i2c_write_header(BitbangI2CDriver *i2cp, bitbang_i2caddr_t addr, bool rw) {

  /* Check for 10 bits addressing.*/
  if (i2cp->config->addr10) {
    /* It is 10 bits.*/
    uint8_t b1, b2;

    b1 = 0xF0U | ((addr >> 8U) << 1U);
    b2 = (uint8_t)(addr & 255U);
    if (rw) {
      b1 |= 1U;
    }
    CHECK_ERROR(i2c_write_byte(i2cp, b1));
    CHECK_ERROR(i2c_write_byte(i2cp, b2));
  }
  else {
    /* It is 7 bits.*/
    if (rw) {
      /* Read.*/
      CHECK_ERROR(i2c_write_byte(i2cp, (addr << 1U) | 1U));
    }
    else {
      /* Write.*/
      CHECK_ERROR(i2c_write_byte(i2cp, (addr << 1U) | 0U));
    }
  }

  return MSG_OK;
}


/**
 * @brief   Low level I2C driver initialization.
 *
 * @notapi
 */
static void bitbang_i2c_lld_init(void) {

  bitbangI2cObjectInit(&BitbangI2CD1);
}

/**
 * @brief   Configures and activates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p BitbangI2CDriver object
 *
 * @notapi
 */
static void bitbang_i2c_lld_start(BitbangI2CDriver *i2cp) {

  ioline_t sda_pin = i2cp->config->sda;
  ioline_t scl_pin = i2cp->config->scl;

  palClearLine(scl_pin);
  palClearLine(sda_pin);
  palSetLineMode(scl_pin, PAL_MODE_OPEN_DRAIN_PULLUP_H);
  palSetLineMode(sda_pin, PAL_MODE_OPEN_DRAIN_PULLUP_H);
  i2c_delay(i2cp);

  if (!palReadLine(sda_pin)) {
    bool recovered = false;
    set_open_drain(scl_pin);
    osalThreadSleepS(TIME_US2I(5));  // clock 100 kHz
    for (int i = 0; i < 9; i++) {
      clear_open_drain(scl_pin);
      osalThreadSleepS(TIME_US2I(5));
      set_open_drain(scl_pin);
      osalThreadSleepS(TIME_US2I(5));
      if (palReadLine(sda_pin)) {
        recovered = true;
        break;
      }
    }
    if (!recovered) {
      i2cp->state = BITBANG_I2C_LOCKED;
      return;
    }

    // send STOP
    clear_open_drain(sda_pin);
    clear_open_drain(scl_pin);
    osalThreadSleepS(TIME_US2I(5));
    clear_open_drain(sda_pin);
    osalThreadSleepS(TIME_US2I(5));
    set_open_drain(scl_pin);
    osalThreadSleepS(TIME_US2I(5));
    set_open_drain(sda_pin);
    osalThreadSleepS(TIME_US2I(5));
  }
}

/**
 * @brief   Deactivates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p BitbangI2CDriver object
 *
 * @notapi
 */
static void bitbang_i2c_lld_stop(BitbangI2CDriver *i2cp) {

  /* Does nothing.*/
  (void)i2cp;
}

/**
 * @brief   Receives data via the I2C bus as master.
 *
 * @param[in] i2cp      pointer to the @p BitbangI2CDriver object
 * @param[in] addr      slave device address
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end. <b>After a
 *                      timeout the driver must be stopped and restarted
 *                      because the bus is in an uncertain state</b>.
 *
 * @notapi
 */
static msg_t bitbang_i2c_lld_master_receive_timeout(BitbangI2CDriver *i2cp, bitbang_i2caddr_t addr,
                                     uint8_t *rxbuf, size_t rxbytes,
                                     sysinterval_t timeout) {

  /* Setting timeout fields.*/
  i2cp->start = osalOsGetSystemTimeX();
  i2cp->end = i2cp->start;
  if (timeout != TIME_INFINITE) {
    i2cp->end = osalTimeAddX(i2cp->start, timeout);
  }

  CHECK_ERROR(i2c_write_start(i2cp));

  /* Sending address and mode.*/
  CHECK_ERROR(i2c_write_header(i2cp, addr, true));

  do {
    /* Last byte sends a NACK.*/
    msg_t msg = i2c_read_byte(i2cp, rxbytes > 1U ? 0U : 1U);
    CHECK_ERROR(msg);
    *rxbuf++ = (uint8_t)msg;
  } while (--rxbytes);

  return i2c_write_stop(i2cp);
}

/**
 * @brief   Transmits data via the I2C bus as master.
 * @details Number of receiving bytes must be 0 or more than 1 on STM32F1x.
 *          This is hardware restriction.
 *
 * @param[in] i2cp      pointer to the @p BitbangI2CDriver object
 * @param[in] addr      slave device address
 * @param[in] txbuf     pointer to the transmit buffer
 * @param[in] txbytes   number of bytes to be transmitted
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end. <b>After a
 *                      timeout the driver must be stopped and restarted
 *                      because the bus is in an uncertain state</b>.
 *
 * @notapi
 */
static msg_t bitbang_i2c_lld_master_transmit_timeout(BitbangI2CDriver *i2cp, bitbang_i2caddr_t addr,
                                      const uint8_t *txbuf, size_t txbytes,
                                      uint8_t *rxbuf, size_t rxbytes,
                                      sysinterval_t timeout) {

  /* Setting timeout fields.*/
  i2cp->start = osalOsGetSystemTimeX();
  i2cp->end = i2cp->start;
  if (timeout != TIME_INFINITE) {
    i2cp->end = osalTimeAddX(i2cp->start, timeout);
  }

  /* Sending start condition.*/
  CHECK_ERROR(i2c_write_start(i2cp));

  /* Sending address and mode.*/
  CHECK_ERROR(i2c_write_header(i2cp, addr, false));

  do {
    CHECK_ERROR(i2c_write_byte(i2cp, *txbuf++));
  } while (--txbytes);

  /* Is there a read phase? */
  if (rxbytes > 0U) {

    /* Sending restart condition.*/
    CHECK_ERROR(i2c_write_restart(i2cp));
    /* Sending address and mode.*/
    CHECK_ERROR(i2c_write_header(i2cp, addr, true));

    do {
      /* Last byte sends a NACK.*/
      msg_t msg = i2c_read_byte(i2cp, rxbytes > 1U ? 0U : 1U);
      CHECK_ERROR(msg);
      *rxbuf++ = (uint8_t)msg;
    } while (--rxbytes);
  }

  return i2c_write_stop(i2cp);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   I2C Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void bitbangI2cInit(void) {

  bitbang_i2c_lld_init();
}

/**
 * @brief   Initializes the standard part of a @p BitbangI2CDriver structure.
 *
 * @param[out] i2cp     pointer to the @p BitbangI2CDriver object
 *
 * @init
 */
void bitbangI2cObjectInit(BitbangI2CDriver *i2cp) {

  i2cp->state  = BITBANG_I2C_STOP;
  i2cp->config = NULL;

}

/**
 * @brief   Configures and activates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p BitbangI2CDriver object
 * @param[in] config    pointer to the @p I2CConfig object
 * @return              The operation status.
 *
 * @api
 */
msg_t bitbangI2cStart(BitbangI2CDriver *i2cp, const BitbangI2CConfig *config) {
  msg_t msg;

  osalDbgCheck((i2cp != NULL) && (config != NULL));

  osalSysLock();
  osalDbgAssert((i2cp->state == BITBANG_I2C_STOP) || (i2cp->state == BITBANG_I2C_READY) ||
                (i2cp->state == BITBANG_I2C_LOCKED), "invalid state");

  i2cp->config = config;
  i2c_delay(i2cp);

  bitbang_i2c_lld_start(i2cp);
  msg = HAL_RET_SUCCESS;
  if (msg == HAL_RET_SUCCESS) {
    i2cp->state = BITBANG_I2C_READY;
  }
  else {
    i2cp->state = BITBANG_I2C_STOP;
  }

  osalSysUnlock();

  return msg;
}

/**
 * @brief   Deactivates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p BitbangI2CDriver object
 *
 * @api
 */
void bitbangI2cStop(BitbangI2CDriver *i2cp) {

  osalDbgCheck(i2cp != NULL);

  osalSysLock();

  osalDbgAssert((i2cp->state == BITBANG_I2C_STOP) || (i2cp->state == BITBANG_I2C_READY) ||
                (i2cp->state == BITBANG_I2C_LOCKED), "invalid state");

  bitbang_i2c_lld_stop(i2cp);
  i2cp->config = NULL;
  i2cp->state  = BITBANG_I2C_STOP;

  osalSysUnlock();
}

/**
 * @brief   Returns the errors mask associated to the previous operation.
 *
 * @param[in] i2cp      pointer to the @p BitbangI2CDriver object
 * @return              The errors mask.
 *
 * @api
 */
bitbang_i2cflags_t bitbangI2cGetErrors(BitbangI2CDriver *i2cp) {

  osalDbgCheck(i2cp != NULL);

  return i2c_lld_get_errors(i2cp);
}

/**
 * @brief   Sends data via the I2C bus.
 * @details Function designed to realize "read-through-write" transfer
 *          paradigm. If you want transmit data without any further read,
 *          than set @b rxbytes field to 0.
 *
 * @param[in] i2cp      pointer to the @p BitbangI2CDriver object
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
msg_t bitbangI2cMasterTransmitTimeout(BitbangI2CDriver *i2cp,
                               bitbang_i2caddr_t addr,
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

  osalDbgAssert(i2cp->state == BITBANG_I2C_READY, "not ready");

  osalSysLock();
  i2cp->errors = BITBANG_I2C_NO_ERROR;
  i2cp->state = BITBANG_I2C_ACTIVE_TX;
  rdymsg = bitbang_i2c_lld_master_transmit_timeout(i2cp, addr, txbuf, txbytes,
                                           rxbuf, rxbytes, timeout);
  if (rdymsg == MSG_TIMEOUT) {
    i2cp->state = BITBANG_I2C_LOCKED;
  }
  else {
    i2cp->state = BITBANG_I2C_READY;
  }
  osalSysUnlock();
  return rdymsg;
}

/**
 * @brief   Receives data from the I2C bus.
 *
 * @param[in] i2cp      pointer to the @p BitbangI2CDriver object
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
msg_t bitbangI2cMasterReceiveTimeout(BitbangI2CDriver *i2cp,
                              bitbang_i2caddr_t addr,
                              uint8_t *rxbuf,
                              size_t rxbytes,
                              sysinterval_t timeout) {

  msg_t rdymsg;

  osalDbgCheck((i2cp != NULL) && (addr != 0U) &&
               (rxbytes > 0U) && (rxbuf != NULL) &&
               (timeout != TIME_IMMEDIATE));

  osalDbgAssert(i2cp->state == BITBANG_I2C_READY, "not ready");

  osalSysLock();
  i2cp->errors = BITBANG_I2C_NO_ERROR;
  i2cp->state = BITBANG_I2C_ACTIVE_RX;
  rdymsg = bitbang_i2c_lld_master_receive_timeout(i2cp, addr, rxbuf, rxbytes, timeout);
  if (rdymsg == MSG_TIMEOUT) {
    i2cp->state = BITBANG_I2C_LOCKED;
  }
  else {
    i2cp->state = BITBANG_I2C_READY;
  }
  osalSysUnlock();
  return rdymsg;
}

/** @} */
