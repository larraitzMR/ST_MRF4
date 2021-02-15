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
 *  @brief UART driver declaration file
 *
 *  The UART driver provides basic functionalty for sending and receiving
 *  data via serial interface.\n
 *
 *  API:
 *  - Transmit data:          #uartTxNBytes()
 *  - size of received data:  #uartRxNumBytesAvailable()
 *  - get received data:      #uartRxNBytes()
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup UART
  * @{
  */

#ifndef UART_DRIVER_H__
#define UART_DRIVER_H__

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
#define UART_RX_BUFFER_SIZE     (2*1024)
#define UART_TX_BUFFER_SIZE     (4*1024)

#if ELANCE
#define UART_HANDLE huart3

#elif JIGEN && defined(STM32WB55xx)
#define UART_HANDLE  hlpuart1

#else
#define UART_HANDLE huart2

#endif
typedef enum
{
    UART_RX_STATE_HEADER,
    UART_RX_STATE_DATA
} uartRxState_t;

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
/** Initializes the UART
  */
void uartInitialize(void);

/** @brief  Transmit a given number of bytes
  *
  * This function is used to transmit \a length bytes via the UART interface.
  * @note blocking implementation, i.e. Function doesn't return before all
  * data has been sent.
  *
  * @param [in] buffer: Buffer of size \a length to be transmitted.
  * @param [in] length: Number of bytes to be transmitted.
  */
void uartTxNBytes(const uint8_t *buffer, const uint16_t length);

/** @brief Transmits next packet
  *
  * Triggers HAL UART Tx via DMA
  */
void uartTxNextPacket(void);

void uartRxNextPacket(void);

/** @brief Receives all pending UART data
  *
  * @param [out] buffer: packet payload data
  * @return 1 if a packet has been fully received, 0 if not
  */  
uint8_t uartRxRead(uint8_t *buffer);

/* */
void uartTxBufferDirect(uint16_t status, uint16_t cmd, uint8_t *txData, uint16_t txSize);

#endif /* UART_DRIVER_H__ */

/**
  * @}
  */
/**
  * @}
  */
