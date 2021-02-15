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
 *  @brief serial output log declaration file
 *
 *  This driver provides a printf-like way to output log messages
 *  via the UART interface. It makes use of the uart driver.
 *
 *  API:
 *  - Write a log message to UART output: #LOG
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup Logging
  * @{
  */

#ifndef LOGGER_H
#define LOGGER_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include "platform.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#define LOGGER_ON   1
#define LOGGER_OFF  0

#if ELANCE
#define LOG_HANDLE  hlpuart1

#elif EVAL
#define LOG_HANDLE  huart3

#elif JIGEN && defined(STM32WB55xx)
#define LOG_HANDLE  huart1

#else
#define LOG_HANDLE  huart3

#endif

/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/
#if(USE_LOGGER == LOGGER_ON)
#define LOG loggerTx        /**< macro used for printing debug messages */
#define LOGDUMP loggerTxHex /**< macro used for dumping buffers */
#define LOGINIT loggerInitialize
#define LOGFLUSH loggerTxFlush
#else
#define LOG(...)        /**< macro used for printing debug messages if USE_LOGGER is set */
#define LOGDUMP(...)    /**< macro used for dumping buffers if USE_LOGGER is set */
#define LOGINIT(...)
#define LOGFLUSH(...)
#endif

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
#if (USE_LOGGER == LOGGER_ON)

#define LOG_BUFFER_SIZE         (2*1024)
#define LOG_BUFFER_SIZE_DMA     (2*1024)


void loggerInitialize(void);
void loggerTxFlush(void);

/**
 *****************************************************************************
 *  @brief  Writes out a formated string via UART interface
 *
 *  This function is used to write a formated string via the UART interface.
 *  @note This function shall not be called directly. Instead the #LOG
 *  macro should be used.
 *
 *  @param [in] format: 0 terminated string to be written out to UART interface.
 *                      the following format tags are supported:
 *                      - %x
 *  @param [in] ... :   variable parameter which depends on \a format.
 *
 *****************************************************************************
 */
void loggerTx(const char *format, ...);

/**
 *****************************************************************************
 *  @brief  dumps a buffer to console, 8 values per line are dumped.
 *
 *  Buffer is hexdumped to console using dbgPrint(). Please note that the
 *  buffer size must be a multiple of 8.
 *
 *  @param [in] buffer: pointer to buffer to be dumped.
 *  @param [in] length: buffer length (multiple of 8)
 *
 *****************************************************************************
 */
void loggerTxHex(const unsigned char *buffer, uint16_t length);
#endif

#endif /* LOGGER_H */

/**
  * @}
  */
/**
  * @}
  */
