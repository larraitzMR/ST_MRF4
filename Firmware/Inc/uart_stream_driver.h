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
 *  @brief UART streaming driver declarations
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup PC_Communication
  * @{
  */

#ifndef _UART_STREAM_DRIVER_H
#define _UART_STREAM_DRIVER_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include <stdint.h>
#include "uart_driver.h"

/*
 ******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************
 */
/**
 *****************************************************************************
 *  for compatibility to USB HID, does nothing for UART
 *
 *  @return n/a
 *****************************************************************************
 */
void uartStreamConnect(void);

/**
 *****************************************************************************
 *  for compatibility to USB HID, does nothing for UART
 *
 *  @return 1
 *****************************************************************************
 */
uint8_t uartStreamReady(void);

/**
 *****************************************************************************
 *  @brief  initializes the UART Stream driver variables
 *
 *  This function takes care for proper initialisation of buffers, variables, etc.
 *
 *  @param rxBuf : buffer where received packets will be copied into
 *  @param txBuf : buffer where to be transmitted packets will be copied into
 *  @return n/a
 *****************************************************************************
 */
void uartStreamInitialize(uint8_t *rxBuf, uint8_t *txBuf);

/**
 *****************************************************************************
 *  @brief  tells the stream driver that the packet has been processed and can
 *   be moved from the rx buffer
 *
 *  @param rxed : number of bytes which have been processed
 *  @return n/a
 *****************************************************************************
 */
void uartStreamPacketProcessed(uint8_t rxed);

/**
 *****************************************************************************
 *  @brief  returns 1 if another packet is available in buffer
 *
 *  @return 0=no packet available in buffer, 1=another packet available
 *****************************************************************************
 */
uint8_t uartStreamHasAnotherPacket(void);

/**
 *****************************************************************************
 *  @brief checks if there is data received on UART from the host
 *  and copies the received data into a local buffer
 *
 *  Checks if UART has data received from the host, copies this
 *  data into a local buffer. The data in the local buffer is than interpreted
 *  as a packet (with header, rx-length and tx-length). As soon as a full
 *  packet is received the function returns non-null.
 *
 *  @return 0 = nothing to process, >0 at least 1 packet to be processed
 *****************************************************************************
 */
uint8_t uartStreamReceive(void);

/**
 *****************************************************************************
 *  @brief checks if there is data to be transmitted from the device to
 *  the host.
 *
 *  Checks if there is data waiting to be transmitted to the host. Copies this
 *  data from a local buffer to the UART buffer and transmits this UART buffer.
 *
 *  @param [in] totalTxSize: the size of the data to be transmitted (the Stream
 *  driver header is not included)
 *****************************************************************************
 */
void uartStreamTransmit(uint16_t totalTxSize);
void uartStreamTransmitBuffer(uint16_t status, uint16_t cmd, uint8_t *txData, uint16_t totalTxSize);

#endif // _UART_STREAM_DRIVER_H

/**
  * @}
  */
/**
  * @}
  */
