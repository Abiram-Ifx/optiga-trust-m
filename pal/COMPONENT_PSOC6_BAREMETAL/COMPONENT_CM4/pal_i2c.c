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
* \brief This file implements the platform abstraction layer(pal) APIs for I2C.
*
* \ingroup  grPAL
* @{
*/

/**********************************************************************************************************************
 * HEADER FILES
 *********************************************************************************************************************/
#include "optiga/pal/pal_i2c.h"
#include "optiga/ifx_i2c/ifx_i2c.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cyhal_scb_common.h"
#include "pal_psoc6_config.h"

/// @cond hidden

/* Maximum bit rate of the I2C master */
#define PAL_I2C_MASTER_MAX_BITRATE  (400U)
/* Define I2C master frequency */
#define I2C_MASTER_FREQUENCY_HZ     (PAL_I2C_MASTER_MAX_BITRATE * (1000U))
/* Interrupt priority for i2c */
#define PAL_I2C_MASTER_INTR_PRIO    (3U)

/* i2c master sda and scl pins */
#define PIN_SDA (CYBSP_I2C_SDA) //P6_1
#define PIN_SCL (CYBSP_I2C_SCL) //P6_0

/* Initialization status of the i2c interface */
static bool pal_i2c_init_status = false;

/* Current context of i2c object */
_STATIC_H const pal_i2c_t * gp_pal_i2c_current_ctx;

/* Get the error code from the result value and returns the status
 * Parameters:
 * rslt [in] - result value returned by the HAL API
 *
 * Return value:
 * returns the status based on the result value
 * */
static pal_status_t get_status(cy_rslt_t * rslt);

/// @endcond

/**********************************************************************************************************************
 * PAL INTERNAL FUNCTIONS - START
 *********************************************************************************************************************/

static pal_status_t get_status(cy_rslt_t * result)
{
	pal_status_t return_status = PAL_STATUS_FAILURE;
	uint16_t error_code = CY_RSLT_GET_CODE(*result);

	/*
	 * Error codes provided by the i2c HAL API
	 * 0: CYHAL_I2C_RSLT_ERR_INVALID_PIN
	 * 1: CYHAL_I2C_RSLT_ERR_CAN_NOT_REACH_DR
	 * 2: CYHAL_I2C_RSLT_ERR_INVALID_ADDRESS_SIZE
	 * 3: CYHAL_I2C_RSLT_ERR_TX_RX_BUFFERS_ARE_EMPTY
	 * 4: CYHAL_I2C_RSLT_ERR_PREVIOUS_ASYNCH_PENDING
	 * 5: CYHAL_I2C_RSLT_ERR_PM_CALLBACK
	 *
	 * Error codes 0,1,2,3,5 are considered as failure, while 4 means the i2c bus is busy.
	 * */
	if ( (0 == error_code) || (1 == error_code) || (2 == error_code) || (3 == error_code) || (5 == error_code))
		return_status = PAL_STATUS_FAILURE;
	else if (4 == error_code)
		return_status = PAL_STATUS_I2C_BUSY;

	return return_status;
}

_STATIC_H void invoke_upper_layer_callback (const pal_i2c_t * p_pal_i2c_ctx,
                                            optiga_lib_status_t event)
{
    upper_layer_callback_t upper_layer_handler;
    //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
    upper_layer_handler = (upper_layer_callback_t)p_pal_i2c_ctx->upper_layer_event_handler;

    upper_layer_handler(p_pal_i2c_ctx->p_upper_layer_ctx, event);

    //Release I2C Bus
    //pal_i2c_release(p_pal_i2c_ctx->p_upper_layer_ctx);
}

_STATIC_H void i2c_master_error_detected_callback(const pal_i2c_t * p_pal_i2c_ctx)
{
    cyhal_i2c_abort_async(((pal_i2c_itf_t *)(p_pal_i2c_ctx->p_i2c_hw_config))->i2c_master_obj);
    invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_ERROR);
}

/* Defining master callback handler */
void i2c_master_event_handler(void *callback_arg, cyhal_i2c_event_t event)
{
    if (0UL != (CYHAL_I2C_MASTER_ERR_EVENT & event))
    {
        /* In case of error abort transfer */
        i2c_master_error_detected_callback(gp_pal_i2c_current_ctx);
    }
    /* Check write complete event */
    else if (0UL != (CYHAL_I2C_MASTER_WR_CMPLT_EVENT & event))
    {
        /* Perform the required functions */
        invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_SUCCESS);
    }
    /* Check read complete event */
    else if (0UL != (CYHAL_I2C_MASTER_RD_CMPLT_EVENT & event))
    {
        /* Perform the required functions */
        invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_SUCCESS);
    }
}

/**********************************************************************************************************************
 * PAL INTERNAL FUNCTIONS - END
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * API IMPLEMENTATION - START
 *********************************************************************************************************************/

/**
 * API to initialize the i2c master with the given context.
 * <br>
 *
 *<b>API Details:</b>
 * - The platform specific initialization of I2C master has to be implemented as part of this API, if required.<br>
 * - If the target platform does not demand explicit initialization of i2c master
 *   (Example: If the platform driver takes care of init after the reset), it would not be required to implement.<br>
 * - The implementation must take care the following scenarios depending upon the target platform selected.
 *   - The implementation must handle the acquiring and releasing of the I2C bus before initializing the I2C master to
 *     avoid interrupting the ongoing slave I2C transactions using the same I2C master.
 *   - If the I2C bus is in busy state, the API must not initialize and return #PAL_STATUS_I2C_BUSY status.
 *   - Repeated initialization must be taken care with respect to the platform requirements. (Example: Multiple users/applications  
 *     sharing the same I2C master resource)
 *
 *<b>User Input:</b><br>
 * - The input #pal_i2c_t p_i2c_context must not be NULL.<br>
 *
 * \param[in] p_i2c_context   Pal i2c context to be initialized
 *
 * \retval  #PAL_STATUS_SUCCESS  Returns when the I2C master init it successfull
 * \retval  #PAL_STATUS_FAILURE  Returns when the I2C init fails.
 */
pal_status_t pal_i2c_init(const pal_i2c_t* p_i2c_context)
{
    pal_status_t pal_status = PAL_STATUS_FAILURE;
    /* Define the I2C master configuration structure */
    static cyhal_i2c_cfg_t i2c_master_cfg =
    {
    	.is_slave = CYHAL_I2C_MODE_MASTER,			    // Master mode
    	.address = 0,								    // Slave address set as 0 when configured as master
    	.frequencyhal_hz = I2C_MASTER_FREQUENCY_HZ		// I2C master frequency
    };
    
    /* Check if the i2c bus is already initialized */
    if (false == pal_i2c_init_status)
    {
        if ((p_i2c_context != NULL) && (p_i2c_context->p_i2c_hw_config != NULL))
        {      
            /* Initialize I2C master, set the SDA and SCL pins and assign a new clock */
            if (CY_RSLT_SUCCESS == cyhal_i2c_init(((pal_i2c_itf_t *)(p_i2c_context->p_i2c_hw_config))->i2c_master_obj, 
                                                  ((pal_i2c_itf_t *)(p_i2c_context->p_i2c_hw_config))->sda_pin, 
                                                  ((pal_i2c_itf_t *)(p_i2c_context->p_i2c_hw_config))->scl_pin, 
                                                  NULL))
	        {
			    /* Configure the I2C resource to be master */
			    if (CY_RSLT_SUCCESS == cyhal_i2c_configure(((pal_i2c_itf_t *)(p_i2c_context->p_i2c_hw_config))->i2c_master_obj, &i2c_master_cfg))
                {
                    /* Register i2c master callback */
			        cyhal_i2c_register_callback(((pal_i2c_itf_t *)(p_i2c_context->p_i2c_hw_config))->i2c_master_obj,
						                        (cyhal_i2c_event_callback_t) i2c_master_event_handler,
						                        NULL);

			        /* Enable interrupts for I2C master */
			        cyhal_i2c_enable_event(((pal_i2c_itf_t *)(p_i2c_context->p_i2c_hw_config))->i2c_master_obj,
						                   (cyhal_i2c_event_t)(CYHAL_I2C_MASTER_WR_CMPLT_EVENT \
						                                       | CYHAL_I2C_MASTER_RD_CMPLT_EVENT \
						                                       | CYHAL_I2C_MASTER_ERR_EVENT),    \
						                                       PAL_I2C_MASTER_INTR_PRIO ,
						                                       true);
                    /* Set the initialization status of the i2c interface  */
                    pal_i2c_init_status = true;
                    pal_status = PAL_STATUS_SUCCESS;
                }				
	        }
        }
    }
    return pal_status;
}

/**
 * API to de-initialize the I2C master with the specified context.
 * <br>
 *
 *<b>API Details:</b>
 * - The platform specific de-initialization of I2C master has to be implemented as part of this API, if required.<br>
 * - If the target platform does not demand explicit de-initialization of i2c master
 *   (Example: If the platform driver takes care of init after the reset), it would not be required to implement.<br>
 * - The implementation must take care the following scenarios depending upon the target platform selected.
 *   - The implementation must handle the acquiring and releasing of the I2C bus before de-initializing the I2C master to
 *     avoid interrupting the ongoing slave I2C transactions using the same I2C master.
 *   - If the I2C bus is in busy state, the API must not de-initialize and return #PAL_STATUS_I2C_BUSY status.
 *	 - This API must ensure that multiple users/applications sharing the same I2C master resource is not impacted.
 *
 *<b>User Input:</b><br>
 * - The input #pal_i2c_t p_i2c_context must not be NULL.<br>
 *
 * \param[in] p_i2c_context   I2C context to be de-initialized
 *
 * \retval  #PAL_STATUS_SUCCESS  Returns when the I2C master de-init it successfull
 * \retval  #PAL_STATUS_FAILURE  Returns when the I2C de-init fails.
 */
pal_status_t pal_i2c_deinit(const pal_i2c_t* p_i2c_context)
{
    pal_status_t pal_status = PAL_STATUS_FAILURE;

    if (true == pal_i2c_init_status)
    {
        if ((p_i2c_context != NULL) && (p_i2c_context->p_i2c_hw_config != NULL))
        {
            /* Free the I2C interface */
            cyhal_i2c_free(((pal_i2c_itf_t *)(p_i2c_context->p_i2c_hw_config))->i2c_master_obj);

            /* Set the i2c initialization status */
            pal_i2c_init_status = false;

	        pal_status = PAL_STATUS_SUCCESS;
        }
    }
    return pal_status;
}

/**
 * Platform abstraction layer API to write the data to I2C slave.
 * <br>
 * <br>
 * \image html pal_i2c_write.png "pal_i2c_write()" width=20cm
 *
 *
 *<b>API Details:</b>
 * - The API attempts to write if the I2C bus is free, else it returns busy status #PAL_STATUS_I2C_BUSY<br>
 * - The bus is released only after the completion of transmission or after completion of error handling.<br>
 * - The API invokes the upper layer handler with the respective event status as explained below.
 *   - #PAL_I2C_EVENT_BUSY when I2C bus in busy state
 *   - #PAL_I2C_EVENT_ERROR when API fails
 *   - #PAL_I2C_EVENT_SUCCESS when operation is successfully completed asynchronously
 *<br>
 *
 *<b>User Input:</b><br>
 * - The input #pal_i2c_t p_i2c_context must not be NULL.<br>
 * - The upper_layer_event_handler must be initialized in the p_i2c_context before invoking the API.<br>
 *
 *<b>Notes:</b><br> 
 *  - Otherwise the below implementation has to be updated to handle different bitrates based on the input context.<br>
 *  - The caller of this API must take care of the guard time based on the slave's requirement.<br>
 *
 * \param[in] p_i2c_context  Pointer to the pal I2C context #pal_i2c_t
 * \param[in] p_data         Pointer to the data to be written
 * \param[in] length         Length of the data to be written
 *
 * \retval  #PAL_STATUS_SUCCESS  Returns when the I2C write is invoked successfully
 * \retval  #PAL_STATUS_FAILURE  Returns when the I2C write fails.
 * \retval  #PAL_STATUS_I2C_BUSY Returns when the I2C bus is busy. 
 */
pal_status_t pal_i2c_write(const pal_i2c_t *p_i2c_context, uint8_t *p_data, uint16_t length)
{
    pal_status_t pal_status = PAL_STATUS_FAILURE;
	cy_rslt_t rslt;

    if ((p_i2c_context != NULL) && (p_i2c_context->p_i2c_hw_config != NULL))//Acquire the i2c bus if needed
    {
    	gp_pal_i2c_current_ctx = p_i2c_context;

    	rslt = cyhal_i2c_master_transfer_async(((pal_i2c_itf_t *) (p_i2c_context->p_i2c_hw_config))->i2c_master_obj, 
                                                p_i2c_context->slave_address, 
                                                p_data, 
                                                length, 
                                                NULL, 
                                                0);

		if (rslt == CY_RSLT_SUCCESS)
		{
			pal_status = PAL_STATUS_SUCCESS;
		}
		else
		{
            /* Relese the i2c bus */

			/* Find the error code and determine the status of the operation */
			pal_status = get_status(&rslt);
			/* Invoke the upper layer error handler */
            //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
            ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_ERROR);
		}
	}
    else
    {
        pal_status = PAL_STATUS_I2C_BUSY;
        /* Invoke the upper layer error handler */
        //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
        ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                    (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_BUSY);
    }
    return pal_status;
}

/**
 * Platform abstraction layer API to read the data from I2C slave.
 * <br>
 * <br>
 * \image html pal_i2c_read.png "pal_i2c_read()" width=20cm
 *
 *<b>API Details:</b>
 * - The API attempts to read if the I2C bus is free, else it returns busy status #PAL_STATUS_I2C_BUSY<br>
 * - The bus is released only after the completion of reception or after completion of error handling.<br>
 * - The API invokes the upper layer handler with the respective event status as explained below.
 *   - #PAL_I2C_EVENT_BUSY when I2C bus in busy state
 *   - #PAL_I2C_EVENT_ERROR when API fails
 *   - #PAL_I2C_EVENT_SUCCESS when operation is successfully completed asynchronously
 *<br>
 *
 *<b>User Input:</b><br>
 * - The input #pal_i2c_t p_i2c_context must not be NULL.<br>
 * - The upper_layer_event_handler must be initialized in the p_i2c_context before invoking the API.<br>
 *
 *<b>Notes:</b><br> 
 *  - Otherwise the below implementation has to be updated to handle different bitrates based on the input context.<br>
 *  - The caller of this API must take care of the guard time based on the slave's requirement.<br>
 *
 * \param[in]  p_i2c_context  pointer to the PAL i2c context #pal_i2c_t
 * \param[in]  p_data         Pointer to the data buffer to store the read data
 * \param[in]  length         Length of the data to be read
 *
 * \retval  #PAL_STATUS_SUCCESS  Returns when the I2C read is invoked successfully
 * \retval  #PAL_STATUS_FAILURE  Returns when the I2C read fails.
 * \retval  #PAL_STATUS_I2C_BUSY Returns when the I2C bus is busy.
 */
pal_status_t pal_i2c_read(const pal_i2c_t* p_i2c_context , uint8_t* p_data , uint16_t length)
{
	cy_rslt_t rslt;
  	pal_status_t pal_status = PAL_STATUS_FAILURE;

  	if ((p_i2c_context != NULL) && (p_i2c_context->p_i2c_hw_config != NULL))//Acquire the i2c bus if needed
  	{
  		gp_pal_i2c_current_ctx = p_i2c_context;

	    rslt = cyhal_i2c_master_transfer_async(((pal_i2c_itf_t *) (p_i2c_context->p_i2c_hw_config))->i2c_master_obj, 
                                               p_i2c_context->slave_address, 
                                               NULL, 
                                               0, 
                                               p_data, 
                                               length);

		if (rslt == CY_RSLT_SUCCESS)
		{
			pal_status = PAL_STATUS_SUCCESS;
		}
		else
		{
            /* Release the i2c bus */

			/* Find the error code and determine the status of the operation */
			pal_status = get_status(&rslt);
			/* Call the upper layer error handler */
            //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
            ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                       (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_ERROR);
		}
  	}
    else
    {
        pal_status = PAL_STATUS_I2C_BUSY;
        /* Invoke the upper layer error handler */
        //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
        ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler))
                                                    (p_i2c_context->p_upper_layer_ctx , PAL_I2C_EVENT_BUSY);
    }
  	return pal_status;
}
   
/**
 * Platform abstraction layer API to set the bitrate/speed(KHz) of I2C master.
 * <br>
 *
 *<b>API Details:</b>
 * - Sets the bitrate of I2C master if the I2C bus is free, else it returns busy status #PAL_STATUS_I2C_BUSY<br>
 * - The bus is released after the setting the bitrate.<br>
 * - This API must take care of setting the bitrate to I2C master's maximum supported value. 
 * - Eg. In XMC4500, the maximum supported bitrate is 400 KHz. If the supplied bitrate is greater than 400KHz, the API will 
 *   set the I2C master's bitrate to 400KHz.
 * - Use the #PAL_I2C_MASTER_MAX_BITRATE macro to specify the maximum supported bitrate value for the target platform.
 * - If upper_layer_event_handler is initialized, the upper layer handler is invoked with the respective event 
 *   status listed below.
 *   - #PAL_I2C_EVENT_BUSY when I2C bus in busy state
 *   - #PAL_I2C_EVENT_ERROR when API fails to set the bit rate 
 *   - #PAL_I2C_EVENT_SUCCESS when operation is successful
 *<br>
 *
 *<b>User Input:</b><br>
 * - The input #pal_i2c_t  p_i2c_context must not be NULL.<br>
 *
 * \param[in] p_i2c_context  Pointer to the pal i2c context
 * \param[in] bitrate        Bitrate to be used by i2c master in KHz
 *
 * \retval  #PAL_STATUS_SUCCESS  Returns when the setting of bitrate is successfully completed
 * \retval  #PAL_STATUS_FAILURE  Returns when the setting of bitrate fails.
 * \retval  #PAL_STATUS_I2C_BUSY Returns when the I2C bus is busy.
 */
pal_status_t pal_i2c_set_bitrate(const pal_i2c_t* p_i2c_context , uint16_t bitrate)
{
    cyhal_i2c_t * i2c_master_obj = (cyhal_i2c_t * )(((pal_i2c_itf_t *)(p_i2c_context->p_i2c_hw_config))->i2c_master_obj);
  	pal_status_t pal_status = PAL_STATUS_FAILURE;
    optiga_lib_status_t event = PAL_I2C_EVENT_ERROR;
    uint32_t setDataRate;

  	if ((p_i2c_context != NULL) && (p_i2c_context->p_i2c_hw_config != NULL))
  	{
        /* Acquire the i2c bus if needed */
        
        /* If the selected bitrate is higher than the bitrate supported by the hardware
         * set the bitrate to the maximum bitrate supported by the hardware
         */
        if (bitrate > PAL_I2C_MASTER_MAX_BITRATE)
        {
            bitrate = PAL_I2C_MASTER_MAX_BITRATE;
        }

        setDataRate = _cyhal_i2c_set_peri_divider(i2c_master_obj->base,
                                                  i2c_master_obj->resource.block_num,
                                                  &(i2c_master_obj->clock),
                                                  (bitrate * 1000),
                                                  false);

		if (0 == setDataRate)
        {
            pal_status = PAL_STATUS_FAILURE;
        }
        else
        {
            pal_status = PAL_STATUS_SUCCESS;
            event = PAL_I2C_EVENT_SUCCESS;
        }

        if (0 != p_i2c_context->upper_layer_event_handler)
        {
            //lint --e{611} suppress "void* function pointer is type casted to upper_layer_callback_t type"
            ((callback_handler_t)(p_i2c_context->upper_layer_event_handler))(p_i2c_context->p_upper_layer_ctx , event);
        }
        /* Release the i2c bus if it was acquired */

  	}
  	return pal_status;
}


/**********************************************************************************************************************
 * API IMPLEMENTATION - END
 *********************************************************************************************************************/

/**
* @}
*/
