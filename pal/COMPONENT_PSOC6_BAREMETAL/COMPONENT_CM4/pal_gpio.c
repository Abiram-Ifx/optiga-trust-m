/**
* \copyright
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
* \endcopyright
*
* \author Infineon Technologies AG
*
* \file pal_gpio.c
*
* \brief   This file implements the platform abstraction layer APIs for GPIO.
*
* \ingroup  grPAL
*
* @{
*/

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "optiga/pal/pal_gpio.h"
#include "optiga/pal/pal_ifx_i2c_config.h"
#include "pal_psoc6_config.h"

pal_status_t pal_gpio_init(const pal_gpio_t * p_gpio_context)
{
    pal_status_t return_status = PAL_STATUS_FAILURE;

    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL))
    {
        /* Initialize the pin provided by the application as output with initial value = false (low) */
        if (CY_RSLT_SUCCESS == cyhal_gpio_init(((pal_gpio_itf_t *)p_gpio_context->p_gpio_hw)->pin, 
                                               CYHAL_GPIO_DIR_OUTPUT, 
                                               CYHAL_GPIO_DRIVE_STRONG, 
                                               ((pal_gpio_itf_t *)p_gpio_context->p_gpio_hw)->init_state))
        {
        	return_status = PAL_STATUS_SUCCESS;
        }
    }

    return return_status;
}

pal_status_t pal_gpio_deinit(const pal_gpio_t * p_gpio_context)
{
	if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL))
	{
	    /* De-initialize the pin provided by the application */
	    cyhal_gpio_free(((pal_gpio_itf_t *)p_gpio_context->p_gpio_hw)->pin);
	}

    return PAL_STATUS_SUCCESS;
}

void pal_gpio_set_high(const pal_gpio_t * p_gpio_context)
{
    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL))
    {
        cyhal_gpio_write(((pal_gpio_itf_t *)p_gpio_context->p_gpio_hw)->pin, true);
    }
}

void pal_gpio_set_low(const pal_gpio_t * p_gpio_context)
{
    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL))
    {
        cyhal_gpio_write(((pal_gpio_itf_t *)p_gpio_context->p_gpio_hw)->pin, false);
    }
}

/**
* @}
*/
