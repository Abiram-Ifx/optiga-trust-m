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
#include "pal_platform.h"
#include "cyhal.h"

/// @cond hidden

/* Define I2C master frequency */
#define I2C_MASTER_FREQUENCY 100000U
/* i2c master sda and scl pins */
#define PIN_SDA (P6_1)
#define PIN_SCL (P6_0)

/* Define the I2C master configuration structure */
static cyhal_i2c_cfg_t i2c_master_config =
{
	.is_slave = CYHAL_I2C_MODE_MASTER,			// Master mode
	.address = 0,								// Slave address set as 0 when configured as master
	.frequencyhal_hz = I2C_MASTER_FREQUENCY		// I2C master frequency
};

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
 * API IMPLEMENTATION
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
    /* Declare variables */
    cy_rslt_t rslt;
    pal_status_t pal_status = PAL_STATUS_FAILURE;
    
    if ((p_i2c_context != NULL) && (p_i2c_context->p_i2c_hw_config != NULL))
    {      
        /* Initialize I2C master, set the SDA and SCL pins and assign a new clock */
        if (CY_RSLT_SUCCESS == cyhal_i2c_init((cyhal_i2c_t *) &p_i2c_context->p_i2c_hw_config, PIN_SDA, PIN_SCL, NULL))
	    {
			/* Configure the I2C resource to be master */
        	rslt = cyhal_i2c_configure((cyhal_i2c_t *) &p_i2c_context->p_i2c_hw_config, &i2c_master_config);
			if (rslt == CY_RSLT_SUCCESS)
				pal_status = PAL_STATUS_SUCCESS;
	    }
        else
        {
        	/* Extract the error code from the result variable and perform the desired operation */
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

    if ((p_i2c_context != NULL) && (p_i2c_context->p_i2c_hw_config != NULL))
    {
        /* Free the I2C interface */
        cyhal_i2c_free((cyhal_i2c_t *) &p_i2c_context->p_i2c_hw_config);

	    pal_status = PAL_STATUS_SUCCESS;
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

    if ((p_i2c_context != NULL) && (p_i2c_context->p_i2c_hw_config != NULL))
    {
    	rslt = cyhal_i2c_master_transfer_async((cyhal_i2c_t *) p_i2c_context->p_i2c_hw_config, p_i2c_context->slave_address, p_data, length, NULL, 0);

		if (rslt == CY_RSLT_SUCCESS)
		{
			pal_status = PAL_STATUS_SUCCESS;
		}
		else
		{
			/* Find the error code and determine the status of the operation */
			pal_status = get_status(&rslt);
			/* Call the upper layer error handler */
		}
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

  	if ((p_i2c_context != NULL) && (p_i2c_context->p_i2c_hw_config != NULL))
  	{
	    rslt = cyhal_i2c_master_transfer_async((cyhal_i2c_t *)(p_i2c_context->p_i2c_hw_config), p_i2c_context->slave_address, NULL, 0, p_data, length);

		if (rslt == CY_RSLT_SUCCESS)
		{
			pal_status = PAL_STATUS_SUCCESS;
		}
		else
		{
			/* Find the error code and determine the status of the operation */
			pal_status = get_status(&rslt);
			/* Call the upper layer error handler */
		}
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
  	pal_status_t pal_status = PAL_STATUS_FAILURE;
	cy_rslt_t rslt;

  	if ((p_i2c_context != NULL) && (p_i2c_context->p_i2c_hw_config != NULL))
  	{
		/* Set the required bitrate in the i2c master configuration */
		i2c_master_config.frequencyhal_hz = (uint32_t) bitrate;

		/* Configure the i2c with the new bitrate */
		rslt = cyhal_i2c_configure((cyhal_i2c_t *) &p_i2c_context->p_i2c_hw_config, &i2c_master_config);
		
		if (rslt == CY_RSLT_SUCCESS)
			pal_status = PAL_STATUS_SUCCESS;
		else
		{
			/* Find the error code and determine the status of the operation */
			pal_status = get_status(&rslt);
			/* Call the upper layer error handler */
		}
  	}
  	return pal_status;
}

/**********************************************************************************************************************
 * API IMPLEMENTATION - END
 *********************************************************************************************************************/

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

/**********************************************************************************************************************
 * PAL INTERNAL FUNCTIONS - END
 *********************************************************************************************************************/

/**
* @}
*/
