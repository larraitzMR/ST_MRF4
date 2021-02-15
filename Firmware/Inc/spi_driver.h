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
 *  @brief SPI driver declaration file
 *
 *  The spi driver provides basic functionality for sending and receiving
 *  data via SPI interface. All four common SPI modes are supported.
 *  Moreover, only master mode and 8 bytes blocks are supported.
 *
 *  API:
 *  - Transmit data: #spiTxRx
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup SPI
  * @{
  */

#ifndef SPI_H
#define SPI_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#define SPI_HANDLE      hspi1

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
/**
 *****************************************************************************
 *  @brief  Transfer a buffer of given length
 *
 *  This function is used to shift out \a length bytes of \a outbuf and
 *  write latched in data to \a inbuf.
 *  @note Due to the nature of SPI \a inbuf has to have the same size as
 *  \a outbuf, i.e. the number of bytes shifted out equals the number of
 *  bytes latched in again.
 *
 *  @param [in] txData: Buffer of size \a length to be transmitted.
 *  @param [out] rxData: Buffer of size \a length where received data is
 *              written to OR NULL in order to perform a write-only operation.
 *  @param [in] length: Number of bytes to be transfered.
 *
 *  @return ERR_IO : Error during SPI communication, returned data may be invalid.
 *  @return ERR_NONE : No error, all \a length bytes transmitted/received.
 *
 *****************************************************************************
 */
void spiTxRx(uint8_t *txData, uint8_t *rxData, uint16_t length);

/**
 *****************************************************************************
 *  Clears Chip Select line
 *****************************************************************************
 */
#define spiSelect()     RESET_GPIO(GPIO_SPI_CS_PORT,GPIO_SPI_CS_PIN)

/**
 *****************************************************************************
 *  Sets Chip Select line
 *****************************************************************************
 */
#define spiUnselect()   SET_GPIO(GPIO_SPI_CS_PORT,GPIO_SPI_CS_PIN)

/**
 *****************************************************************************
 *  Initializes SPI
 *****************************************************************************
 */
#define spiInitialize() __HAL_SPI_ENABLE(&SPI_HANDLE)

/**
 *****************************************************************************
 *  Abort outstanding SPI operation
 *****************************************************************************
 */
#define spiAbort() HAL_SPI_Abort(&SPI_HANDLE)

#endif /* SPI_H */

/**
  * @}
  */
/**
  * @}
  */
