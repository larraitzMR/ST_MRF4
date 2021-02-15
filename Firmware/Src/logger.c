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
 *  @brief debug log output utility.
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup Logging
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "stdio.h"
#include <stdarg.h>
#include <string.h>
#include "logger.h"
#include "platform.h"
#include "usart.h"
#include "stdbool.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#define LOG_UART_WITH_DMA 0

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
#if (USE_LOGGER == LOGGER_ON)

#if LOG_UART_WITH_DMA
static uint16_t id = 0;
static uint16_t tx_count = 0;

uint8_t loggerTxBuffer[LOG_BUFFER_SIZE];
uint8_t loggerTxBufferDma[LOG_BUFFER_SIZE_DMA];
#endif
/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void loggerInitialize(void)
{
#if LOG_UART_WITH_DMA
    memset(loggerTxBuffer, 0, LOG_BUFFER_SIZE);
    memset(loggerTxBufferDma, 0, LOG_BUFFER_SIZE_DMA);

    // create ID
    tx_count = snprintf((char*)loggerTxBuffer, LOG_BUFFER_SIZE, "%04x:>", id++);

    // initialize
    HAL_UART_Init(&LOG_HANDLE);
#endif    
}

/*------------------------------------------------------------------------- */
void loggerTxFlush(void)
{
#if LOG_UART_WITH_DMA
    if(tx_count > 6){

//        if((HAL_UART_GetState(&UART_HANDLE) == HAL_UART_STATE_BUSY_TX) || (HAL_UART_GetState(&UART_HANDLE) == HAL_UART_STATE_BUSY_TX_RX)){
//            // transmit in progress..
//            return;
//        }

        // we need to wait here, otherwise data will be lost/overwritten
        while((HAL_UART_GetState(&UART_HANDLE) == HAL_UART_STATE_BUSY_TX) || (HAL_UART_GetState(&UART_HANDLE) == HAL_UART_STATE_BUSY_TX_RX)){
            __NOP();	// Avoid code optimization
        }
        
        memcpy(loggerTxBufferDma, loggerTxBuffer, tx_count);
        HAL_UART_Transmit_DMA(&LOG_HANDLE, loggerTxBufferDma, tx_count);
        // create ID
        tx_count = snprintf((char*)loggerTxBuffer, LOG_BUFFER_SIZE, "%04x:>", id++);
    }
#endif
}

/*------------------------------------------------------------------------- */
void loggerTx(const char *format, ...)
{
//    uint8_t tmp[LOG_BUFFER_SIZE];
//    snprintf((char*)(tmp), 7, "%04x:>", id++);
//    
//    va_list args;
//    va_start(args, format);
//    uint16_t len = vsnprintf((char*)(tmp + 6), (LOG_BUFFER_SIZE - 6), format, args) + 6;
//    va_end(args);

    uint8_t tmp[LOG_BUFFER_SIZE];
    
    va_list args;
    va_start(args, format);
    uint16_t len = vsnprintf((char*)(tmp), (LOG_BUFFER_SIZE), format, args);
    va_end(args);

#if LOG_ITM
    SWO_PrintString(tmp, 0);
#elif LOG_UART_WITH_DMA

    
    if((tx_count + len) < LOG_BUFFER_SIZE){
        memcpy(loggerTxBuffer + tx_count, tmp, len);
        tx_count += len;
    }else{
        // copy what fits into log buffer
        uint16_t fillSize = LOG_BUFFER_SIZE - tx_count;        
        memcpy(loggerTxBuffer + tx_count, tmp, fillSize);

        // copy to DMA
        memcpy(loggerTxBufferDma, loggerTxBuffer, LOG_BUFFER_SIZE_DMA);
        
        // the rest of the tmp could be copied into the beginning of the intermediate log txBuffer
        memcpy(loggerTxBuffer, &tmp[fillSize], len - fillSize);
        tx_count = len - fillSize;

        // finally start the transmit..        
        while((HAL_UART_GetState(&UART_HANDLE) == HAL_UART_STATE_BUSY_TX) || (HAL_UART_GetState(&UART_HANDLE) == HAL_UART_STATE_BUSY_TX_RX))
            ;    // we need to wait here, otherwise data will be lost/overwritten
        HAL_UART_Transmit_DMA(&LOG_HANDLE, loggerTxBufferDma, LOG_BUFFER_SIZE_DMA);
    }

#else
    HAL_UART_Transmit(&LOG_HANDLE, tmp, len, 100);
#endif
}

/*------------------------------------------------------------------------- */
void loggerTxHex(const unsigned char *buffer, uint16_t length)
{
    uint8_t tmp[256];
    uint16_t i, rest;
    uint16_t len = 0;
    rest = length%8;

    if(length >= 8)
    {
        for(i=0 ; i<(length/8) ; i++)
        {
        
            len += sprintf((char*)(tmp + len), "%02x%02x%02x%02x%02x%02x%02x%02x\n",
            //len += sprintf((char*)(tmp + len), "%hhx%hhx%hhx%hhx%hhx%hhx%hhx%hhx\n",
                   buffer[i*8],
                   buffer[i*8+1],
                   buffer[i*8+2],
                   buffer[i*8+3],
                   buffer[i*8+4],
                   buffer[i*8+5],
                   buffer[i*8+6],
                   buffer[i*8+7]);
            if(len > (256-20))
                break;
        }
    }
    if(rest>0)
    {
        for(i=(length/8)*8 ; i<length ; i++)
        {
            len += sprintf((char*)(tmp + len), "%02x", buffer[i]);            
            if(len > (256-20))
                break;
        }
    }
    loggerTx("%s\n", tmp);
}




/*!
 * \brief Sends a character over the SWO channel
 * \param c Character to be sent
 * \param portNo SWO channel number, value in the range of 0 to 31
 */
void SWO_PrintChar(char c, char c1, uint8_t portNo) {
  volatile int timeout;
 
  /* Check if Trace Control Register (ITM->TCR at 0xE0000E80) is set */
  if ((ITM->TCR&ITM_TCR_ITMENA_Msk) == 0) { /* check Trace Control Register if ITM trace is enabled*/
    return; /* not enabled? */
  }
  /* Check if the requested channel stimulus port (ITM->TER at 0xE0000E00) is enabled */
  if ((ITM->TER & (1ul<<portNo))==0) { /* check Trace Enable Register if requested port is enabled */
    return; /* requested port not enabled? */
  }
  timeout = 5000; /* arbitrary timeout value */
  while (ITM->PORT[0].u32 == 0) {
    /* Wait until STIMx is ready, then send data */
    timeout--;
    if (timeout==0) {
      return; /* not able to send */
    }
  }
  //ITM->PORT[0].u16 = 0x20 | (c<<8);
  ITM->PORT[0].u16 = c | (c1<<8);
  
}

/*!
 * \brief Sends a string over SWO to the host
 * \param s String to send
 * \param portNumber Port number, 0-31, use 0 for normal debug strings
 */
void SWO_PrintString(const char *s, uint8_t portNumber) {
  while ((*s!='\0')&&(*(s+1)!='\0')) {
    SWO_PrintChar(*s, *(s+1), portNumber);
    s += 2;
  }
}

// -------------------------
//    va_list args;
//    va_start(args, format);
//    vsnprintf((char*)(loggerTxBuffer), LOG_BUFFER_SIZE, format, args);
//    va_end(args);
//    SWO_PrintString((char*)loggerTxBuffer, 0);



#endif
/**
  * @}
  */
/**
  * @}
  */
