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
 *  @brief Streaming driver interface declarations.
 *
 *  The defines allow switching between different stream drivers,
 *  currently implemented are:
 *  - UART
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup PC_Communication
  * @{
  */

#ifndef STREAM_DRIVER_H
#define STREAM_DRIVER_H
/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "uart_stream_driver.h"

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
/* redirect according to underlying communication protocol */
#define StreamInitialize       uartStreamInitialize
#define StreamConnect          uartStreamConnect
#define StreamDisconnect       uartStreamDisconnect
#define StreamReady            uartStreamReady
#define StreamHasAnotherPacket uartStreamHasAnotherPacket
#define StreamPacketProcessed  uartStreamPacketProcessed
#define StreamReceive          uartStreamReceive
#define StreamTransmit         uartStreamTransmit

#endif /* STREAM_DRIVER_H */

/**
  * @}
  */
/**
  * @}
  */
