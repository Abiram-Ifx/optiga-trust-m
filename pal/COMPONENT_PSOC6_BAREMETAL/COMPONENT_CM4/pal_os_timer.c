/**
* \copyright
* MIT License
*
* Copyright (c) 2020 Infineon Technologies AG
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
* \file pal_os_timer.c
*
* \brief   This file implements the platform abstraction layer APIs for timer.
*
* \ingroup  grPAL
*
* @{
*/

#include "cyhal.h"
#include "optiga/pal/pal_os_timer.h"

/// @cond hidden

/* Interrrupt priority for the timer module */
#define TIMER_ISR_PRIORITY (7U)

/* struct for storing total time elapsed since the timer was started */
static uint32_t seconds_count;

/* Timer object used for accessing the timer */
static cyhal_timer_t timer_obj;
/* Timer configurtion parameters */
const cyhal_timer_cfg_t timer_cfg =
    {
        .compare_value = 0,                 /* Timer compare value, not used */
        .period = 1000000,                  /* Timer period is set to 1 second */
        .direction = CYHAL_TIMER_DIR_UP,    /* Timer counts up */
        .is_compare = false,                /* Don't use compare mode */
        .is_continuous = true,              /* Timer runs without stopping */
        .value = 0                          /* Initial value of counter */
    };

/* Declare the ISR */
static void isr_timer(void *callback_arg, cyhal_timer_event_t event);

/// @endcond


uint32_t pal_os_timer_get_time_in_microseconds(void)
{
    /* Convert the total timer tick value to microseconds */
    uint32_t time_in_microseconds = (cyhal_timer_read((cyhal_timer_t *) &timer_obj) + (seconds_count * 1000000));
    
    return (time_in_microseconds);
}

uint32_t pal_os_timer_get_time_in_milliseconds(void)
{
    /* Convert the total timer tick value to milliseconds */
    uint32_t time_in_milliseconds = (((uint32_t) (cyhal_timer_read((cyhal_timer_t *) &timer_obj) / 1000)) + (seconds_count * 1000));

    return (time_in_milliseconds);
}

void pal_os_timer_delay_in_milliseconds(uint16_t milliseconds)
{
    uint32_t start_time;
    uint32_t current_time;
    uint32_t time_stamp_diff;

    start_time = pal_os_timer_get_time_in_milliseconds();
    current_time = start_time;
    time_stamp_diff = current_time - start_time;
    while (time_stamp_diff <= (uint32_t)milliseconds)
    {
        current_time = pal_os_timer_get_time_in_milliseconds();
        time_stamp_diff = current_time - start_time;
        if (start_time > current_time)
        {
            time_stamp_diff = (0xFFFFFFFF + (current_time - start_time)) + 0x01;
        }        
    }
}

pal_status_t pal_timer_init(void)
{
    pal_status_t return_status = PAL_STATUS_FAILURE;

    /* Configure and start the timer only if initialization is successful */
    if (CY_RSLT_SUCCESS == cyhal_timer_init(&timer_obj, NC, NULL))
    {
    	/* Configure the timer with the provided timer config parameters */
    	if (CY_RSLT_SUCCESS == cyhal_timer_configure(&timer_obj, &timer_cfg))
    	{
            /* Register the isr for the timer interrupt */
            cyhal_timer_register_callback(&timer_obj, &isr_timer, NULL);

            /* Set the event for which the interrupt occurs and enable the interrupt */
            cyhal_timer_enable_event(&timer_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT, TIMER_ISR_PRIORITY, true);

            /* Start the timer with the configured parameters */
            if (CY_RSLT_SUCCESS == cyhal_timer_start(&timer_obj))
            {
                /* Set the return status */
            	return_status = PAL_STATUS_SUCCESS;
            }
    	}
    }
    return return_status;
}

pal_status_t pal_timer_deinit(void)
{
    /* Free the timer */
    cyhal_timer_free((cyhal_timer_t *) &timer_obj);

    return PAL_STATUS_SUCCESS;
}

/*******************************************************************************
* Function Name: isr_timer
********************************************************************************
* Summary:
* This is the interrupt handler function for the timer interrupt.
*
* Parameters:
*    callback_arg    Arguments passed to the interrupt callback
*    event            Timer/counter interrupt triggers
*
*******************************************************************************/
static void isr_timer(void *callback_arg, cyhal_timer_event_t event)
{
    (void) callback_arg;
    (void) event;

    /* Increment the timer seconds counter */
    seconds_count++;
}

/**
* @}
*/
