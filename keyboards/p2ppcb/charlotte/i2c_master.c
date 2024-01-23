/* Copyright 2022 NAKAZATO Hajime
 * Copyright 2018 Jack Humbert
 * Copyright 2018 Yiancar
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This library is only valid for RP2040 processors.
 * This library follows the convention of the AVR i2c_master library (as far as possible).
 * As a result addresses are expected to be already shifted (addr << 1).
 * I2CD0 is the default driver which corresponds to pins GP2 and GP3. This
 * can be changed. I2C0_SDA_PIN and I2C0_SCL_PIN defines them.
 * Please ensure that CHARLOTTE_USE_I2C is TRUE in the halconf.h file and that
 * RP_I2C_USE_I2C1 is TRUE in the mcuconf.h file.
 */
#include "quantum.h"
#include "i2c_master.h"
#include <string.h>
#include <ch.h>
#include <hal.h>

#ifndef I2C_MASTER_DRIVER
#    define I2C_MASTER_DRIVER I2CD0
#endif

static const I2CConfig i2cconfig = {
    I2C1_CLOCK_SPEED,
};

/**
 * @brief Handles any I2C error condition by stopping the I2C peripheral and
 * aborting any ongoing transactions. Furthermore ChibiOS status codes are
 * converted into QMK codes.
 *
 * @param status ChibiOS specific I2C status code
 * @return i2c_status_t QMK specific I2C status code
 */
static i2c_status_t i2c_epilogue(const msg_t status) {
    if (status == MSG_OK) {
        return I2C_STATUS_SUCCESS;
    }

    // From ChibiOS HAL: "After a timeout the driver must be stopped and
    // restarted because the bus is in an uncertain state." We also issue that
    // hard stop in case of any error.
    i2cStop(&I2C_MASTER_DRIVER);

    return status == MSG_TIMEOUT ? I2C_STATUS_TIMEOUT : I2C_STATUS_ERROR;
}

__attribute__((weak)) void i2c_init(void) {
}

i2c_status_t i2c_transmit(uint8_t i2c_address, const uint8_t* data, uint16_t length, uint16_t timeout) {
    i2cStart(&I2C_MASTER_DRIVER, &i2cconfig);
    msg_t status = i2cMasterTransmitTimeout(&I2C_MASTER_DRIVER, (i2c_address >> 1), data, length, 0, 0, TIME_MS2I(timeout));
    return i2c_epilogue(status);
}

i2c_status_t i2c_writeReg(uint8_t i2c_address, uint8_t regaddr, const uint8_t* data, uint16_t length, uint16_t timeout) {
    i2cStart(&I2C_MASTER_DRIVER, &i2cconfig);

    uint8_t complete_packet[length + 1];
    for (uint16_t i = 0; i < length; i++) {
        complete_packet[i + 1] = data[i];
    }
    complete_packet[0] = regaddr;

    msg_t status = i2cMasterTransmitTimeout(&I2C_MASTER_DRIVER, (i2c_address >> 1), complete_packet, length + 1, 0, 0, TIME_MS2I(timeout));
    return i2c_epilogue(status);
}

i2c_status_t i2c_writeReg16(uint8_t i2c_address, uint16_t regaddr, const uint8_t* data, uint16_t length, uint16_t timeout) {
    i2cStart(&I2C_MASTER_DRIVER, &i2cconfig);

    uint8_t complete_packet[length + 2];
    for (uint16_t i = 0; i < length; i++) {
        complete_packet[i + 2] = data[i];
    }
    complete_packet[0] = regaddr >> 8;
    complete_packet[1] = regaddr & 0xFF;

    msg_t status = i2cMasterTransmitTimeout(&I2C_MASTER_DRIVER, (i2c_address >> 1), complete_packet, length + 2, 0, 0, TIME_MS2I(timeout));
    return i2c_epilogue(status);
}

i2c_status_t i2c_readReg(uint8_t i2c_address, uint8_t regaddr, uint8_t* data, uint16_t length, uint16_t timeout) {
    i2cStart(&I2C_MASTER_DRIVER, &i2cconfig);
    msg_t status = i2cMasterTransmitTimeout(&I2C_MASTER_DRIVER, (i2c_address >> 1), &regaddr, 1, data, length, TIME_MS2I(timeout));
    return i2c_epilogue(status);
}

i2c_status_t i2c_readReg16(uint8_t i2c_address, uint16_t regaddr, uint8_t* data, uint16_t length, uint16_t timeout) {
    i2cStart(&I2C_MASTER_DRIVER, &i2cconfig);
    uint8_t register_packet[2] = {regaddr >> 8, regaddr & 0xFF};
    msg_t   status = i2cMasterTransmitTimeout(&I2C_MASTER_DRIVER, (i2c_address >> 1), register_packet, 2, data, length, TIME_MS2I(timeout));
    return i2c_epilogue(status);
}
