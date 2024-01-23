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

#pragma once

/**
 * @brief I2C slave event types.
 */
typedef enum i2c_slave_event_t
{
    I2C_SLAVE_RECEIVE_FINISHED, /* The master has been finished to transmit a series of data. Now the master is requesting data from the slave, or made stop condition. */
    I2C_SLAVE_REQUEST, /* The master is requesting data. The slave must write into i2cp->txptr. */
    I2C_SLAVE_TRANSACTION_FINISHED, /* The transaction is over. Prepare next transaction. */
} i2c_slave_event_t;

struct I2CDriver;
/**
 * @brief I2C slave event handler
 *
 * The event handler will run from the I2C ISR, so it should return quickly (under 25 us at 400 kb/s).
 * Avoid blocking inside the handler and split large data transfers across multiple calls for best results.
 * To prepare for requesting data from master, store the data to i2cp->txptr. The buffer size is txbytes.
 * When receiving data from master, the data is stored in i2cp->slave_rx_buf. The size is received_rxbytes.
 *
 * @param i2cp Slave I2C instance.
 * @param event Event type.
 */
typedef void (*i2c_slave_handler_t)(struct I2CDriver *i2cp, i2c_slave_event_t event);

#ifndef RP_I2C_RX_BUFSIZE
#  define RP_I2C_RX_BUFSIZE 256
#endif

#define I2C_DRIVER_EXT_FIELDS \
    bool slave_active; \
    i2c_slave_handler_t slave_handler; \
    uint8_t slave_address; \
    uint8_t slave_rx_buf[RP_I2C_RX_BUFSIZE]; \
    size_t requested_rxbytes; \
    size_t received_rxbytes; \
    bool stop_det_raised; \
    bool restart_det_raised;

#undef HAL_USE_I2C
#define HAL_USE_I2C CHARLOTTE_USE_I2C
#undef HAL_I2C_H

#include "hal_i2c.h"

#undef HAL_USE_I2C
#define HAL_USE_I2C FALSE

msg_t i2cSlaveStart(I2CDriver *i2cp, const I2CConfig *config, i2c_slave_handler_t slave_handler, uint8_t slave_address);
