/**
* MIT License
*
* Copyright (c) 2021 Infineon Technologies AG
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE
*
*
* \file
*
* \brief This file implements the platform abstraction layer(pal) specific defines,
* 		 pins, etc.
*
* \ingroup  grPAL
* @{
*/

#ifndef PAL_PSOC6_CONFIG_H
#define PAL_PSOC6_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cyhal.h"
#include "pal.h"

/* Define the SDA and SCL pins */
#define I2C_SDA_PIN (P6_1)
#define I2C_SCL_PIN (P6_0)

/* Pins used for vdd and reset */
#define PIN_VDD 	(P6_5)
#define PIN_RESET 	(P9_0)

/* GPIO interface data */
typedef struct pal_gpio_itf
{
    uint8_t pin;
    bool_t init_state;
}pal_gpio_itf_t;

/* I2C interface data */
typedef struct pal_i2c_itf
{
    cyhal_i2c_t * i2c_master_obj;
    uint8_t sda_pin;
    uint8_t scl_pin;
}pal_i2c_itf_t;

#ifdef __cplusplus
}
#endif

#endif /* PAL_PSOC6_CONFIG_H */
/**
* @}
*/
