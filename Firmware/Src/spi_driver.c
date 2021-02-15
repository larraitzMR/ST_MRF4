/******************************************************************************
  * \attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2020 STMicroelectronics</center></h2>
  *
  * Licensed under ST MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        www.st.com/myliberty
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/
/** @file
 *
 *  @author ST Microelectronics
 *
 *  @brief SPI driver for STM32L4
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup SPI
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "spi.h"
#include "platform.h"
#include "st25RU3993_config.h"
#include "errno_application.h"
#include "spi_driver.h"
#include "timer.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#define SPI_BUFFER_SIZE 256
#define SPI_TIMEOUT     0xFFFFFFFF 

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
uint8_t buffer_temp[SPI_BUFFER_SIZE];

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

/*------------------------------------------------------------------------- */
__inline void spiTxRx(uint8_t *txData, uint8_t *rxData, uint16_t length)
{
    txData = (txData == 0 ? buffer_temp : txData);
    rxData = (rxData == 0 ? buffer_temp : rxData);
    HAL_SPI_TransmitReceive(&SPI_HANDLE,txData,rxData,length,SPI_TIMEOUT);    
}

/**
  * @}
  */
/**
  * @}
  */
