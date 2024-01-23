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

#include "quantum.h"

#include "i2c_slave.h"
#include "i2c_master.h"
#include "timer.h"

volatile static uint32_t last_transaction_time;

static const I2CConfig i2cconfig = {
    I2C1_CLOCK_SPEED,
};

// The attribute below is fatally important because gcc doesn't align static variable when "-Os".
volatile uint8_t i2c_slave_reg[I2C_SLAVE_REG_COUNT] __attribute__ ((aligned));

// The slave implements I2C_SLAVE_REG_COUNT byte memory. To write a series of bytes, the master first
// writes the memory address, followed by the data. The address is automatically incremented
// for each byte transferred, looping back to 0 upon reaching the end. Reading is done
// sequentially from the current memory address.

// Our handler is called from the I2C ISR, so it must complete quickly. Blocking calls /
// printing to stdio may interfere with interrupt handling.
static void __not_in_flash_func(i2c_slave_handler)(I2CDriver *i2cp, i2c_slave_event_t event) {
    static uint32_t offset = 0U;  // assumes this handler handles only one I2CDriver instance.
    switch (event) {
    case I2C_SLAVE_RECEIVE_FINISHED: // master has written some data
        i2cp->txptr = NULL;
        if (i2cp->received_rxbytes == 0U) {
            break;
        }
        offset = i2cp->slave_rx_buf[0];
        for (uint32_t i = 1; i < i2cp->received_rxbytes; i++) {
            i2c_slave_reg[(offset + i - 1) % I2C_SLAVE_REG_COUNT] = i2cp->slave_rx_buf[i];
        }
        offset = (offset + i2cp->received_rxbytes - 1) % I2C_SLAVE_REG_COUNT;
        i2cp->received_rxbytes = 0U;
        // fall through. slave-receive to slave-transmit transaction is common. By doing this,
        // function call in ISR can be reduced.
    case I2C_SLAVE_REQUEST: // master is requesting data
        if (i2cp->txptr != NULL) {
          // looping back or continue
          offset = (i2cp->txptr - i2c_slave_reg) % I2C_SLAVE_REG_COUNT;
        }
        i2cp->txptr = (uint8_t*)i2c_slave_reg + offset;
        i2cp->txbytes = I2C_SLAVE_REG_COUNT - offset;
        break;
    case I2C_SLAVE_TRANSACTION_FINISHED: // prepare next transaction
        offset = 0;  // This is a design intended to avoid hidden and confusing state of I2C slave.
        break;
    default:
        break;
    }
    last_transaction_time = timer_read32();
}

void i2c_slave_init(uint8_t address) {
    i2c_init();
    i2cSlaveStart(&I2C_SLAVE_DRIVER, &i2cconfig, i2c_slave_handler, address >> 1);
    last_transaction_time = timer_read32();
}

void i2c_slave_stop(void) {
    i2cStop(&I2C_SLAVE_DRIVER);
}

uint32_t i2c_slave_last_transaction_time(void) {
    return last_transaction_time;
}
