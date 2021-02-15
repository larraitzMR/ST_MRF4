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
 *  @brief Interface for stream packet handling.
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup PC_Communication
  * @{
  */

#ifndef STREAM_DISPATCHER_H
#define STREAM_DISPATCHER_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include "stream_driver.h"

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/

/** Returns the last error that occured and clears the error
  */
uint8_t StreamDispatcherGetLastError(void);
uint16_t StreamDispatcherGetLastCmd(void);
/** Initialisation of the stream dispatcher (no connect)
  */
void StreamDispatcherInit(void);

/** @brief Main entry point into the stream dispatcher must be called cyclic.
  *
  * This function checks the stream driver for received data. If new data is
  * available it is processed and forwarded to the application
  * functions.
  */
void ProcessIO(void);

#endif /* STREAM_DISPATCHER */

/**
  * @}
  */
/**
  * @}
  */
