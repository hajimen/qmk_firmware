/* Copyright 2022 NAKAZATO Hajime
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

#include "analog.h"
#include <hal.h>

#if (HAL_USE_ADC != TRUE)
#    error "You need to set HAL_USE_ADC to TRUE in your halconf.h to use the ADC."
#endif

#if (RP_ADC_USE_ADC1 != TRUE)
#    error "You need to set 'RP_ADC_USE_ADC1' settings to TRUE in your mcuconf.h to use the ADC."
#endif

#define ADC_NUM_CHANNELS 4
#define ADC_BUFFER_DEPTH 1
#define ADC_PIN_BASE     26U

static ADCConfig   adcCfg = {};
static adcsample_t sampleBuffer[ADC_NUM_CHANNELS * ADC_BUFFER_DEPTH];
static bool        adcInitialized = false;

static ADCConversionGroup adcConversionGroup = {
    .circular = FALSE,
    .end_cb   = NULL,
    .error_cb = NULL,
    // .channel_mask and .num_channels should be set before read
};

/**
 * @brief                Read multi ch at once.
 *
 * @param mask           LSB is GP26, and so on.
 * @return adcsample_t*  The max is 4095. The order is LSB to MSB of mask. [GP26, GP27, GP28] for 0b111, for example.
 */
adcsample_t* analogReadMask(uint8_t mask) {
    ADCDriver* targetDriver = &ADCD1;
    adcConversionGroup.channel_mask = mask;
    uint8_t num_ch = 0U;
    for (int i = 0; i < ADC_NUM_CHANNELS; i++) {  // believe it will be unrolled by compiler
        if (mask & (1U << i)) {
            palSetLineMode(i + ADC_PIN_BASE, PAL_MODE_INPUT_ANALOG);
            num_ch++;
        }
    }
    adcConversionGroup.num_channels = num_ch;

    if (!adcInitialized) {
        adcStart(targetDriver, &adcCfg);
        adcInitialized = true;
    }
    if (adcConvert(targetDriver, &adcConversionGroup, &sampleBuffer[0], ADC_BUFFER_DEPTH) != MSG_OK) {
        return NULL;
    }

    return sampleBuffer;
}

/**
 * @brief          Read a pin.
 *
 * @param pin      should be GP26, GP27, or GP28.
 * @return int16_t The max is 4095.
 */
int16_t analogReadPin(pin_t pin) {
    adcsample_t* buf = analogReadMask(1U << (pin - ADC_PIN_BASE));
    if (!buf) {
        return 0;
    }
    return (int16_t)buf[0];
}
