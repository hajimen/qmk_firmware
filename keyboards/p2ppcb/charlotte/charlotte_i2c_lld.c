/*
    Copyright (C) 2022 NAKAZATO Hajime
    ChibiOS - Copyright (C) 2022 Stefan Kerkmann
    ChibiOS - Copyright (C) 2021 Hanya
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

/**
 * @file    charlotte_i2c_lld.c
 * @brief   PLATFORM I2C subsystem low level driver source. Slave mode is available.
 * @note    This module is one of HAL community part in terms of ChibiOS.
 *
 * @addtogroup I2C
 * @{
 */


#include "hal.h"
#include "gpio.h"
#include "chibios_config.h"

#if (CHARLOTTE_USE_I2C == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if (RP_I2C_USE_I2C0 == TRUE)
#  ifndef I2C0_SCL_PIN
#    define I2C0_SCL_PIN GP1
#  endif
#  ifndef I2C0_SDA_PIN
#    define I2C0_SDA_PIN GP0
#  endif
#endif

#if (RP_I2C_USE_I2C1 == TRUE)
#  ifndef I2C1_SCL_PIN
#    define I2C1_SCL_PIN GP3
#  endif
#  ifndef I2C1_SDA_PIN
#    define I2C1_SDA_PIN GP2
#  endif
#endif

#if (RP_I2C_USE_I2C0 == TRUE) && (RP_I2C_USE_I2C1 == TRUE)
#  define CHOOSE_BY_I2CP(i2cp, v1, v2) (i2cp == &I2CD0 ? v1 : (i2cp == &I2CD1 ? v2 : -1))
#elif (RP_I2C_USE_I2C0 == TRUE)
#  define CHOOSE_BY_I2CP(i2cp, v1, v2) (v1)
#elif (RP_I2C_USE_I2C1 == TRUE)
#  define CHOOSE_BY_I2CP(i2cp, v1, v2) (v2)
#endif

#define I2C_SCL_PIN(i2cp) CHOOSE_BY_I2CP(i2cp, I2C0_SCL_PIN, I2C1_SCL_PIN)
#define I2C_SDA_PIN(i2cp) CHOOSE_BY_I2CP(i2cp, I2C0_SDA_PIN, I2C1_SDA_PIN)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (RP_I2C_USE_I2C0 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   I2C1 driver identifier.
 */
I2CDriver I2CD0;
#endif

#if (RP_I2C_USE_I2C1 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   I2C2 driver identifier.
 */
I2CDriver I2CD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#define I2C_OVERRUN_ERRORS \
  (I2C_IC_INTR_STAT_M_RX_OVER | I2C_IC_INTR_STAT_M_RX_UNDER | \
   I2C_IC_INTR_STAT_M_TX_OVER)

#define I2C_ERROR_INTERRUPTS \
  (I2C_IC_INTR_MASK_M_TX_ABRT | I2C_IC_INTR_MASK_M_TX_OVER | \
   I2C_IC_INTR_MASK_M_RX_OVER | I2C_IC_INTR_MASK_M_RX_UNDER)

#define I2C_SLAVE_ACTIVE_INTERRUPTS \
  (I2C_ERROR_INTERRUPTS      | I2C_IC_INTR_MASK_M_RX_FULL | \
   I2C_IC_INTR_MASK_M_RD_REQ | I2C_IC_INTR_MASK_M_STOP_DET | \
   I2C_IC_INTR_MASK_M_START_DET)

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Tries to clear SDA line stuck if master mode. Sets PAL mode and makes I2C_STOP if cleared or slave mode.
 *
 * @note    It follows Synopsys DesignWare DW_apb_i2c Databook, 3.11.1 SDA Line Stuck at LOW Recovery.
 */
static bool i2c_lld_prepare_line(I2CDriver *i2cp) {
  pin_t scl_pin = I2C_SCL_PIN(i2cp);
  pin_t sda_pin = I2C_SDA_PIN(i2cp);

  // The "I2C1_S??_PAL_MODE" name looks bizarre for I2C0, but chibios_config.h has the name only.
  // BTW the value of I2C0_S??_PAL_MODE and I2C1_S??_PAL_MODE are the same.
  uint32_t scl_pal_mode = I2C1_SCL_PAL_MODE;
  uint32_t sda_pal_mode = I2C1_SDA_PAL_MODE;

  i2c_lld_stop(i2cp);

  if (!i2cp->slave_active) {
    // clear SDA line stuck
    // slave should not clear SDA line.

    palSetLineMode(sda_pin, PAL_MODE_INPUT_PULLUP);
    if (!readPin(sda_pin)) {
      bool recovered = false;
      palSetLineMode(scl_pin, PAL_MODE_OUTPUT_PUSHPULL);
      writePinHigh(scl_pin);
      chThdSleepS(TIME_US2I(5));  // clock 100 kHz
      for (int i = 0; i < 9; i++) {
        writePinLow(scl_pin);
        chThdSleepS(TIME_US2I(5));
        writePinHigh(scl_pin);
        chThdSleepS(TIME_US2I(5));
        if (readPin(sda_pin)) {
          recovered = true;
          break;
        }
      }
      if (!recovered) {
        i2cp->state = I2C_LOCKED;
        return false;
      }

      // send STOP
      writePinLow(sda_pin);
      palSetLineMode(sda_pin, PAL_MODE_OUTPUT_PUSHPULL);
      writePinLow(scl_pin);
      chThdSleepS(TIME_US2I(5));
      writePinLow(sda_pin);
      chThdSleepS(TIME_US2I(5));
      writePinHigh(scl_pin);
      chThdSleepS(TIME_US2I(5));
      writePinHigh(sda_pin);
      chThdSleepS(TIME_US2I(5));
    }
  }

  // set PAL mode
  palSetLineMode(scl_pin, scl_pal_mode);
  palSetLineMode(sda_pin, sda_pal_mode);
  chThdSleepS(TIME_US2I(5));

  return true;
}

/**
 * @brief   Sets interrupt mask.
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] enable    temporary interrupt mask
 */
static inline void i2c_lld_set_interrupt_mask(I2CDriver *i2cp) {
  i2cp->i2c->INTRMASK = i2cp->slave_active ? I2C_SLAVE_ACTIVE_INTERRUPTS : 0U;
}

/**
 * @brief    Calculates the waiting time which is 10 times the SCL period duration.
 */
static inline sysinterval_t i2c_lld_get_wait_time(I2CDriver *i2cp){
  uint32_t baudrate = i2cp->config->baudrate;
  if (baudrate <= 100000U) {
    return OSAL_US2I(100);
  } else if (baudrate <= 400000U) {
    return OSAL_US2I(25);
  } else {
    return OSAL_US2I(10);
  }
}

/**
 * @brief    Waits with timeout until @p formula becomes false.
 *
 * @param[in] formula      waits to be false
 */
#define _i2c_wait_status(formula) \
  /* Calculating the time window for the timeout on the enable condition.*/ \
  msg_t ok = MSG_OK; \
  systime_t start, end; \
  start = osalOsGetSystemTimeX(); \
  end = osalTimeAddX(start, i2c_lld_get_wait_time(i2cp)); \
  \
  while (formula) { \
    if (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), start, end)) { \
      ok = MSG_TIMEOUT; \
      break; \
    } \
    chThdSleepS(TIME_US2I(1)); \
  } \

/**
 * @brief    Tries to disable the I2C peripheral.
 */
static msg_t i2c_lld_disableS(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;

  dp->ENABLE &= ~I2C_IC_ENABLE_ENABLE;

  _i2c_wait_status(dp->ENABLESTATUS & I2C_IC_ENABLE_STATUS_IC_EN);

  return ok;
}

/**
 * @brief    Aborts any ongoing transmission of the I2C peripheral.
 */
static msg_t i2c_lld_abort_transmissionS(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;

  /* Disable all interrupts as a pre-caution. */
  dp->INTRMASK = 0U;
  /* Enable peripheral to issue abort. */
  dp->ENABLE |= I2C_IC_ENABLE_ENABLE;
  /* Issue abort. */
  dp->ENABLE |= I2C_IC_ENABLE_ABORT;

  _i2c_wait_status((dp->RAWINTRSTAT & I2C_IC_INTR_STAT_M_TX_ABRT) == 0U);

  (void)dp->CLRTXABRT;
  return ok;
}

/**
 * @brief    Calculates and set up clock settings.
 */
static void i2c_lld_setup_frequency(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;
  /*                [us]    SS   FS  FS+
   *  MIN_SCL_LOWtime_FS:  4.7  1.3  0.5
   * MIN_SCL_HIGHtime_FS:  4.0  0.6  0.26
   *             max tSP:    -  0.05 0.05
   *         min tSU:DAT: 0.25  0.1  0.05
   *             tHD:DAT:  0    0    0
   */

  uint32_t minLowfs, minHighfs;
  uint32_t baudrate = i2cp->config->baudrate;
  if (baudrate <= 100000U) {
    // ns
    minLowfs = 4700U;
    minHighfs = 4000U;
  } else if (baudrate <= 400000U) {
    minLowfs = 1300U;
    minHighfs = 600U;
  } else {
    minLowfs = 500U;
    minHighfs = 260U;
  }

  halfreq_t sys_clk = halClockGetPointX(clk_sys) / 100000;
  uint32_t hcntfs = (minHighfs * sys_clk) / 10000U + 1U;
  uint32_t lcntfs = (minLowfs * hcntfs) / ((10000000U / baudrate) * 100U - minLowfs) + 1U;

  uint32_t sdahd = 0U;
  if (baudrate < 1000000U) {
    sdahd = (sys_clk * 3U) / 100U + 1U;
  } else {
    sdahd = (sys_clk * 3U) / 250U + 1U;
  }

  /* Always Fast Mode */
  dp->FSSCLHCNT = hcntfs - 7U;
  dp->FSSCLLCNT = lcntfs - 1U;
  dp->FSSPKLEN = 2U;

  dp->SDAHOLD |= sdahd & I2C_IC_SDA_HOLD_IC_SDA_TX_HOLD;
}

static void __not_in_flash_func(i2c_lld_transaction_finish)(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;
  if (i2cp->slave_active) {
    if (i2cp->state == I2C_ACTIVE_RX) {
      i2cp->slave_handler(i2cp, I2C_SLAVE_RECEIVE_FINISHED);
    }
    i2cp->slave_handler(i2cp, I2C_SLAVE_TRANSACTION_FINISHED);
    i2cp->state = I2C_READY;
  } else {
    dp->INTRMASK = 0U;
    (void)dp->CLRINTR;
    _i2c_wakeup_isr(i2cp);
  }
}

/**
 * @brief   I2C shared ISR code.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 */
static void __not_in_flash_func(i2c_lld_serve_interrupt)(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;
  uint32_t intr = dp->INTRSTAT;

  uint32_t tx_abrt_source = dp->TXABRTSOURCE;
  if (tx_abrt_source) {
    /* Abort detected. */
    if (tx_abrt_source & (I2C_IC_TX_ABRT_SOURCE_ARB_LOST |
                          I2C_IC_TX_ABRT_SOURCE_ABRT_SLV_ARBLOST)) {
      i2cp->errors |= I2C_ARBITRATION_LOST;
    }
    if (tx_abrt_source & I2C_IC_TX_ABRT_SOURCE_ABRT_USER_ABRT) {
      // raised by RX buffer overrun. i2c_lld_abort_transmissionS() also makes I2C_IC_ENABLE_ABORT, but the interrupt is disabled.
      i2cp->errors |= I2C_OVERRUN;
    }
    if (tx_abrt_source & (I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK |
                          I2C_IC_TX_ABRT_SOURCE_ABRT_10ADDR1_NOACK |
                          I2C_IC_TX_ABRT_SOURCE_ABRT_10ADDR2_NOACK |
                          I2C_IC_TX_ABRT_SOURCE_ABRT_GCALL_NOACK |
                          I2C_IC_TX_ABRT_SOURCE_ABRT_TXDATA_NOACK)) {
      i2cp->errors |= I2C_ACK_FAILURE;
    }

    if (!i2cp->errors) {
      i2cp->errors |= I2C_BUS_ERROR;
    }
  }

  if (intr & I2C_OVERRUN_ERRORS) {
    i2cp->errors |= I2C_OVERRUN;
  }

  if (i2cp->errors) {
    if (i2cp->slave_active) {
      i2cp->slave_handler(i2cp, I2C_SLAVE_TRANSACTION_FINISHED);
    }

    /* Restore interrupts. */
    dp->INTRMASK = 0U;
    (void)dp->CLRINTR;
    i2c_lld_set_interrupt_mask(i2cp);
    if (!i2cp->slave_active) {
      _i2c_wakeup_error_isr(i2cp);
    }
    return;
  }

  if (intr & I2C_IC_INTR_STAT_R_START_DET) {
    // always slave mode
    // repeat start can be raised before RX FIFO has been cleared.
    if (i2cp->state == I2C_ACTIVE_RX) {
      // Repeat start from slave-receive. There is no handler for repeat start from slave-transmit.
      if (dp->STATUS & I2C_IC_STATUS_RFNE) {
        i2cp->restart_det_raised = true;
      } else {
        i2cp->slave_handler(i2cp, I2C_SLAVE_RECEIVE_FINISHED);
      }
    } else if (i2cp->state == I2C_READY) {
      // first start
      i2cp->state = I2C_LOCKED;

      i2cp->rxbytes = 0U;
      i2cp->received_rxbytes = 0U;
      i2cp->txptr = NULL;
      i2cp->txbytes = 0U;
      i2cp->stop_det_raised = false;
      i2cp->restart_det_raised = false;

      i2cp->errors = I2C_NO_ERROR;
    }
    (void)dp->CLRSTARTDET;

    // fall-through. maybe other interrupts raised.
  }

  if (intr & I2C_IC_INTR_STAT_R_STOP_DET) {
    // STOP_DET can be raised before RX FIFO has been cleared.
    if (dp->STATUS & I2C_IC_STATUS_RFNE) {
      i2cp->stop_det_raised = true;
      (void)dp->CLRSTOPDET;
    } else {
      // RX FIFO is empty
      i2c_lld_transaction_finish(i2cp);
      (void)dp->CLRSTOPDET;
      return;
    }

    // fall-through. maybe other interrupts raised.
  }

  if (intr & I2C_IC_INTR_STAT_R_RD_REQ) {
    // slave-transmit requested
    i2cp->state = I2C_ACTIVE_TX;
    if (i2cp->txbytes == 0U) {
      i2cp->slave_handler(i2cp, I2C_SLAVE_REQUEST);
    }
    osalDbgAssert(i2cp->txbytes > 0U, "slave handler must fill TX buffer");
    osalDbgAssert(dp->STATUS & I2C_IC_STATUS_TFNF, "TX buffer is full but RD_REQ interrupt raised. Something failed.");

    dp->DATACMD = (uint32_t)*i2cp->txptr;
    i2cp->txptr++;
    i2cp->txbytes--;
    (void)dp->CLRRDREQ;

    // fall-through. maybe RX_FULL raised.
  }

  if (intr & I2C_IC_INTR_STAT_R_RX_FULL) {
    // RX FIFO is full. Should be read.
    if (i2cp->slave_active) {
      // slave-receive
      i2cp->state = I2C_ACTIVE_RX;
      while (dp->STATUS & I2C_IC_STATUS_RFNE) {
        if (i2cp->received_rxbytes == sizeof(i2cp->slave_rx_buf)) {
          osalDbgAssert(false, "RX buffer overrun. 256 bytes in a transaction is max.");
          dp->ENABLE |= I2C_IC_ENABLE_ABORT;
          return;
        }
        i2cp->slave_rx_buf[i2cp->received_rxbytes] = (uint8_t)dp->DATACMD;
        i2cp->received_rxbytes++;
      }
      if (i2cp->stop_det_raised) {
        i2c_lld_transaction_finish(i2cp);
        return;
      }
      if (i2cp->restart_det_raised) {
        i2cp->slave_handler(i2cp, I2C_SLAVE_RECEIVE_FINISHED);
        i2cp->restart_det_raised = false;
      }
    } else {
      // master-receive
      while (dp->STATUS & I2C_IC_STATUS_RFNE) {
        if (i2cp->received_rxbytes == i2cp->rxbytes) {
          osalDbgAssert(false, "RX FIFO has more than requested. Serious bug.");
          dp->ENABLE |= I2C_IC_ENABLE_ABORT;
          return;
        }
        i2cp->rxptr[i2cp->received_rxbytes] = (uint8_t)dp->DATACMD;
        i2cp->received_rxbytes++;
      }

      if (i2cp->rxbytes == i2cp->received_rxbytes) {
        /* Everything is received. */
        dp->RXTL = 0U;
        if (i2cp->stop_det_raised) {
          // RX_FULL can be raised after STOP_DET
          i2c_lld_transaction_finish(i2cp);
          return;
        }
      } else {
        /* Enable TX FIFO empty IRQ to request more data. */
        dp->SET.INTRMASK = I2C_IC_INTR_MASK_M_TX_EMPTY;
      }
    }

    // fall-through. maybe STOP_DET raised.
  }

  if (intr & I2C_IC_INTR_STAT_R_TX_EMPTY) {
    // always master mode. Maybe first interrupt in a transaction.
    // TX FIFO is empty.
    if (i2cp->state == I2C_ACTIVE_TX) {
      // master-transmit
      while (i2cp->txbytes > 0U && dp->STATUS & I2C_IC_STATUS_TFNF) {
        uint32_t data_cmd = (uint32_t)*i2cp->txptr;

        /* Send STOP after last byte or prepare RESTART in next RX. */
        if (i2cp->txbytes == 1U) {
          if (i2cp->rxbytes == 0U) {
            data_cmd |= I2C_IC_DATA_CMD_STOP;
          } else {
            i2cp->send_restart = true;
          }
        }
        dp->DATACMD = data_cmd;

        i2cp->txptr++;
        i2cp->txbytes--;
      }

      if (i2cp->txbytes == 0U) {
        if (i2cp->rxbytes == 0U) {
          /* Nothing more to send or receive, disable TX FIFO empty interrupt. */
          dp->CLR.INTRMASK = I2C_IC_INTR_MASK_M_TX_EMPTY;
        } else {
          // TX - RX transaction
          i2cp->state = I2C_ACTIVE_RX;
        }
      }
    } else if (i2cp->state == I2C_ACTIVE_RX) {
      // master-receive
      /* RP2040 Designware I2C peripheral has FIFO depth of 16 elements. As we
       * specify that the TX_EMPTY interrupt only fires if the TX FIFO is
       * truly empty we don't need to check the current fill level. */
      uint32_t batch = MIN(16U, i2cp->rxbytes - i2cp->requested_rxbytes);

      /* Setup RX FIFO trigger level to only trigger when the batch has been
       * completely received. Therefore we don't need to check if there are any
       * bytes still pending to be received. */
      dp->RXTL = batch > 1U ? batch - 1U : 0U;

      while (batch > 0U) {
        uint32_t cmd = I2C_IC_DATA_CMD_CMD;

        /* RESTART occurs in TX - RX transaction. */
        if (i2cp->send_restart) {
          // always first byte of RX
          cmd |= I2C_IC_DATA_CMD_RESTART;
          i2cp->send_restart = false;
        }

        /* Send STOP after last byte. RX - TX transaction never occurs in this code at master mode. */
        if (i2cp->rxbytes - i2cp->requested_rxbytes == 1U) {
          cmd |= I2C_IC_DATA_CMD_STOP;
        }

        dp->DATACMD = cmd;

        batch--;
        i2cp->requested_rxbytes++;
      }

      /* Clear TX FIFO empty interrupt, it will be re-activated when data has been
       * received. */
      dp->CLR.INTRMASK = I2C_IC_INTR_MASK_M_TX_EMPTY;
    } else {
      osalDbgAssert(false, "i2cp->state is wrong when TX_EMPTY raised.");
    }
    return;
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if (RP_I2C_USE_I2C0 == TRUE) || defined(__DOXYGEN__)

OSAL_IRQ_HANDLER(RP_I2C0_IRQ_HANDLER) {
  OSAL_IRQ_PROLOGUE();

  i2c_lld_serve_interrupt(&I2CD0);

  OSAL_IRQ_EPILOGUE();
}

#endif

#if (RP_I2C_USE_I2C1 == TRUE) || defined(__DOXYGEN__)

OSAL_IRQ_HANDLER(RP_I2C1_IRQ_HANDLER) {
  OSAL_IRQ_PROLOGUE();

  i2c_lld_serve_interrupt(&I2CD1);

  OSAL_IRQ_EPILOGUE();
}

#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level I2C driver initialization.
 *
 * @notapi
 */
void i2c_lld_init(void) {

#if RP_I2C_USE_I2C0 == TRUE
  i2cObjectInit(&I2CD0);
  I2CD0.i2c = I2C0;
  I2CD0.thread = NULL;
  I2CD0.state = I2C_UNINIT;

  /* Reset I2C */
  hal_lld_peripheral_reset(RESETS_ALLREG_I2C0);
#endif

#if RP_I2C_USE_I2C1 == TRUE
  i2cObjectInit(&I2CD1);
  I2CD1.i2c = I2C1;
  I2CD1.thread = NULL;
  I2CD1.state = I2C_UNINIT;

  /* Reset I2C */
  hal_lld_peripheral_reset(RESETS_ALLREG_I2C1);
#endif
}

/**
 * @brief   Addresses of the form 000 0xxx or 111 1xxx are reserved. No slave should have these addresses.
 */
static inline bool i2c_lld_reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

/**
 * @brief   Configures and activates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_start(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;

  if (i2cp->state == I2C_READY) {
    return;
  }

  if (i2cp->state != I2C_STOP) {
    bool prepared = i2c_lld_prepare_line(i2cp);
    osalDbgAssert(prepared, "i2c_lld_prepare_line() failed");
  }

  hal_lld_peripheral_unreset(CHOOSE_BY_I2CP(i2cp, RESETS_ALLREG_I2C0, RESETS_ALLREG_I2C1));
  nvicEnableVector(CHOOSE_BY_I2CP(i2cp, RP_I2C0_IRQ_NUMBER, RP_I2C1_IRQ_NUMBER),
                  CHOOSE_BY_I2CP(i2cp, RP_IRQ_I2C0_PRIORITY, RP_IRQ_I2C1_PRIORITY));

  /* Disable i2c peripheral for setup phase. */
  if (i2c_lld_disableS(i2cp) != MSG_OK) {
    osalDbgAssert(false, "i2c_lld_disableS() failed");
    return;
  }

  if (i2cp->slave_active) {
    osalDbgAssert(i2cp->slave_address < 0x80, "I2C slave address is 7-bit only. RP2040 can do 10-bit address. Modify the code if you need it.");
    osalDbgAssert(!i2c_lld_reserved_addr(i2cp->slave_address), "The I2C address is reserved by system.");

    dp->CON = I2C_IC_CON_STOP_DET_IFADDRESSED |
              I2C_IC_CON_RX_FIFO_FULL_HLD_CTRL |
              I2C_IC_CON_IC_RESTART_EN |
              I2C_IC_CON_TX_EMPTY_CTRL |
              (2U << I2C_IC_CON_SPEED_Pos); // Always Fast Mode
    dp->SAR = i2cp->slave_address;
  } else {
    // master active
    dp->CON = I2C_IC_CON_IC_SLAVE_DISABLE |
              (2U << I2C_IC_CON_SPEED_Pos) | // Always Fast Mode
              I2C_IC_CON_IC_RESTART_EN |
#if RP_I2C_ADDRESS_MODE_10BIT == TRUE
              I2C_IC_CON_IC_10BITADDR_MASTER |
#endif
              I2C_IC_CON_MASTER_MODE |
              I2C_IC_CON_STOP_DET_IF_MASTER_ACTIVE |
              I2C_IC_CON_TX_EMPTY_CTRL;
  }

  dp->RXTL = 0U;
  dp->TXTL = 0U;

  i2c_lld_setup_frequency(i2cp);

  /* Clear interrupt mask. */
  dp->INTRMASK = 0U;

  /* Enable peripheral */
  dp->ENABLE = I2C_IC_ENABLE_ENABLE;

  /* Clear interrupts. */
  (void)dp->CLRINTR;

  /* If slave is enabled, interrupt should be handled. */
  i2c_lld_set_interrupt_mask(i2cp);
}

/**
 * @brief   Deactivates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_stop(I2CDriver *i2cp) {

  if (i2cp->state != I2C_STOP) {
    if (i2cp->slave_active) {
      // if slave is active, ignore about the bus status and disable/reset.
    } else if (RESETS->RESET & CHOOSE_BY_I2CP(i2cp, RESETS_ALLREG_I2C0, RESETS_ALLREG_I2C1)) {
      // if I2C peripheral is in reset state, nothing to do.
    } else if (i2c_lld_disableS(i2cp) != MSG_OK) {
      if (i2c_lld_abort_transmissionS(i2cp) != MSG_OK || i2c_lld_disableS(i2cp) != MSG_OK) {
        osalDbgAssert(false, "failed to abort transaction");
      }
    }

    nvicDisableVector(CHOOSE_BY_I2CP(i2cp, RP_I2C0_IRQ_NUMBER, RP_I2C1_IRQ_NUMBER));
    hal_lld_peripheral_reset(CHOOSE_BY_I2CP(i2cp, RESETS_ALLREG_I2C0, RESETS_ALLREG_I2C1));

    i2cp->state = I2C_STOP;
  }
}

/**
 * @brief   Clear activity and disable I2C with timeout.
 */
static msg_t i2c_lld_clear_activity_disable(I2C_TypeDef *dp) {
  systime_t start, end;
  bool activity_going = true;

  /* Calculating the time window for the timeout on the busy bus condition.*/
  start = osalOsGetSystemTimeX();
  end = osalTimeAddX(start, OSAL_MS2I(RP_I2C_BUSY_TIMEOUT));

  while (true) {
    if (activity_going) {
      if ((dp->STATUS & I2C_IC_STATUS_ACTIVITY) == 0U) {
        dp->ENABLE &= ~I2C_IC_ENABLE_ENABLE;
        activity_going = false;
      } else {
        (void)dp->CLRACTIVITY;
      }
    }
    if (!activity_going) {
      if ((dp->ENABLESTATUS & I2C_IC_ENABLE_STATUS_IC_EN) == 0U) {
        break;
      }
    }

    /* If the system time went outside the allowed window then a timeout
       condition is returned. */
    if (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), start, end)) {
      return MSG_TIMEOUT;
    }
    chThdSleepS(TIME_US2I(1));
  }
  return MSG_OK;
}

/**
 * @brief   Receives data via the I2C bus as master.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
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
msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                     uint8_t *rxbuf, size_t rxbytes,
                                     sysinterval_t timeout) {

  return i2c_lld_master_transmit_timeout(i2cp, addr,
                                         NULL, 0U, rxbuf, rxbytes, timeout);
}

/**
 * @brief   Transmits data via the I2C bus as master.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
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
msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                      const uint8_t *txbuf, size_t txbytes,
                                      uint8_t *rxbuf, size_t rxbytes,
                                      sysinterval_t timeout) {

  osalDbgAssert(i2cp->state == I2C_READY, "Prepare I2CDriver first.");

  msg_t msg;
  I2C_TypeDef *dp = i2cp->i2c;

  msg = i2c_lld_clear_activity_disable(dp);
  if (msg != MSG_OK) {
    return msg;
  }

  if (txbytes > 0) {
    i2cp->state = I2C_ACTIVE_TX;
  } else if (rxbytes > 0) {
    i2cp->state = I2C_ACTIVE_RX;
  } else {
    osalDbgAssert(false, "No transmit/receive is not a valid I2C transaction.");
    return MSG_OK;
  }
  i2cp->txbytes = txbytes;
  i2cp->txptr = txbuf;
  i2cp->rxbytes = rxbytes;
  i2cp->requested_rxbytes = 0U;
  i2cp->received_rxbytes = 0U;
  i2cp->rxptr = rxbuf;
  i2cp->stop_det_raised = false;
  i2cp->send_restart = false;

  /* Set target address. */
  dp->TAR = addr & I2C_IC_TAR_IC_TAR;

  /* Clear interrupt mask. */
  dp->INTRMASK = 0U;

  /* Clear interrupts. */
  (void)dp->CLRINTR;

  /* Set interrupt mask. */
  dp->INTRMASK = I2C_IC_INTR_MASK_M_RX_FULL |
                 I2C_IC_INTR_MASK_M_STOP_DET |
                 I2C_IC_INTR_MASK_M_TX_EMPTY |
                 I2C_ERROR_INTERRUPTS;

  /* Enable peripheral */
  dp->ENABLE = I2C_IC_ENABLE_ENABLE;

  /* Waits for the operation completion or a timeout.*/
  msg = osalThreadSuspendTimeoutS(&i2cp->thread, timeout);

  if (msg == MSG_TIMEOUT) {
    /* Disable and clear interrupts. */
    dp->INTRMASK = 0U;
    (void)dp->CLRINTR;
    i2cp->state = I2C_LOCKED;
  }
  if (msg != MSG_OK && !readPin(I2C_SDA_PIN(i2cp))) {
    // SDA line stuck LOW
    i2cp->state = I2C_LOCKED;
    i2c_lld_prepare_line(i2cp);
  }
  if (msg == MSG_OK) {
    i2cp->state = I2C_READY;
  }

  return msg;
}

#endif /* CHARLOTTE_USE_I2C == TRUE */

/** @} */
