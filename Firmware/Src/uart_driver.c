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
 *  @brief UART driver implementation for STM32
 *
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup UART
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include <string.h>
#include "usart.h"
#include "uart_driver.h"
#include "logger.h"
#include "evalAPI_commands.h"
#include "stuhfl_pl.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */
uint16_t txID = 0;
/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
static uint16_t elementsToReceive = 0;
static uint16_t rxCount = 0;
static uint16_t txCount = 0;

static uint8_t uartRxBuffer[UART_RX_BUFFER_SIZE];      // intermediate buffer after data is received via DMA
static uint8_t uartTxBuffer[UART_TX_BUFFER_SIZE];      // intermediate buffer until data is ready to be transmitted via DMA
static uint8_t uartRxBufferDma[UART_RX_BUFFER_SIZE];   // DMA data buffer to receive
static uint8_t uartTxBufferDma[UART_TX_BUFFER_SIZE];   // DMA data buffer to transmit, stays unchanged until transmission is finished

#if !defined(STM32WB55xx)
volatile bool uartTxInProgress = false;
#endif
/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

/*------------------------------------------------------------------------- */
void uartInitialize(void)
{
    // clear buffers
    rxCount = 0;
    txCount = 0;
    memset(uartRxBuffer, 0, UART_RX_BUFFER_SIZE);
    memset(uartTxBuffer, 0, UART_TX_BUFFER_SIZE);
    memset(uartRxBufferDma, 0, UART_RX_BUFFER_SIZE);
    memset(uartTxBufferDma, 0, UART_TX_BUFFER_SIZE);

    // initialize
    HAL_UART_Init(&UART_HANDLE);

    // trigger UART Rx
    uartRxNextPacket();
}

void uartTxBufferDirect(uint16_t status, uint16_t cmd, uint8_t *txData, uint16_t txSize)
{
#if defined(STM32WB55xx)
    // wait for last transmission to finish
    while((HAL_UART_GetState(&UART_HANDLE) == HAL_UART_STATE_BUSY_TX) || (HAL_UART_GetState(&UART_HANDLE) == HAL_UART_STATE_BUSY_TX_RX)){
    // in Half Duplex mode it is not possible to transmit before receiption is finished
        __NOP();	// Avoid code optimization
    }
#else
    while(uartTxInProgress){
        __NOP();	// Avoid code optimization
    }
#endif


    // transmit can now be started ..
    // mode, status and payload_length
    COMM_SET_PREAMBLE_MODE(uartTxBufferDma, 0);
    COMM_SET_PREAMBLE_ID(uartTxBufferDma, txID++);
    COMM_SET_STATUS(uartTxBufferDma, status);
    COMM_SET_CMD(uartTxBufferDma, cmd);
    COMM_SET_PAYLOAD_LENGTH(uartTxBufferDma, txSize);
    // payload
    if (txSize) {
        memcpy((uartTxBufferDma + COMM_PAYLOAD_POS), txData, txSize);
    }
    HAL_UART_Transmit_DMA(&UART_HANDLE, uartTxBufferDma, txSize + COMM_PAYLOAD_POS);
#if !defined(STM32WB55xx)
    uartTxInProgress = true;
#endif
}

/*------------------------------------------------------------------------- */
void uartTxNBytes(const uint8_t *buffer, const uint16_t length)
{
    if(length > UART_TX_BUFFER_SIZE)
    {
        return;
    }
    if(!length)
    {
        return;
    }
    
    memcpy(uartTxBuffer + txCount, buffer, length);
    txCount = length;
    
    uartTxNextPacket();
}

/*------------------------------------------------------------------------- */
void uartTxNextPacket(void)
{
#if defined(STM32WB55xx)
    // wait for last transmission to finish
    while((HAL_UART_GetState(&UART_HANDLE) == HAL_UART_STATE_BUSY_TX) || (HAL_UART_GetState(&UART_HANDLE) == HAL_UART_STATE_BUSY_TX_RX)){
    // in Half Duplex mode it is not possible to transmit before receiption is finished
        __NOP();	// Avoid code optimization
    }
#else
    while(uartTxInProgress){
        __NOP();	// Avoid code optimization
    }
#endif    
    // transmit can be started and we have something to transmit
    memcpy(uartTxBufferDma, uartTxBuffer, txCount);
    HAL_UART_Transmit_DMA(&UART_HANDLE, uartTxBufferDma, txCount);
#if !defined(STM32WB55xx)
    uartTxInProgress = true;    
#endif
    txCount = 0;
}

/*------------------------------------------------------------------------- */
void uartRxNextPacket(void)
{
    HAL_UART_Receive_DMA(&UART_HANDLE, uartRxBufferDma, UART_RX_BUFFER_SIZE);
}

/*------------------------------------------------------------------------- */
uint8_t uartRxRead(uint8_t *buffer)
{
    static uartRxState_t state = UART_RX_STATE_HEADER;
    static uint16_t elementsReceivedOld = 0;
    uint16_t elementsReceived = (UART_RX_BUFFER_SIZE - UART_HANDLE.hdmarx->Instance->CNDTR);
    uint8_t numPackets = 0;
    uint8_t packetFound = 0;

    if(elementsReceivedOld != elementsReceived){
        if(elementsReceived<elementsReceivedOld){
            // Overflow occurred
            if((rxCount + UART_RX_BUFFER_SIZE - elementsReceivedOld) < UART_RX_BUFFER_SIZE){
                memcpy((uartRxBuffer + rxCount), (uartRxBufferDma + elementsReceivedOld), (UART_RX_BUFFER_SIZE - elementsReceivedOld));
                rxCount += (UART_RX_BUFFER_SIZE - elementsReceivedOld);
                elementsReceivedOld = 0;
            }
            
            if((rxCount + elementsReceived) < UART_RX_BUFFER_SIZE){
                memcpy((uartRxBuffer + rxCount), uartRxBufferDma, elementsReceived);
                rxCount += elementsReceived;
                elementsReceivedOld = elementsReceived;
            }
        }
        else{
            if((rxCount + elementsReceived - elementsReceivedOld) < UART_RX_BUFFER_SIZE){
                memcpy((uartRxBuffer + rxCount), (uartRxBufferDma + elementsReceivedOld), (elementsReceived - elementsReceivedOld));
                rxCount += (elementsReceived - elementsReceivedOld);
                elementsReceivedOld = elementsReceived;
            }
        }    
    }

    do
    {
        packetFound = 0;
        if(state == UART_RX_STATE_HEADER){
            if(rxCount >= COMM_PAYLOAD_POS){
                elementsToReceive = COMM_GET_PAYLOAD_LENGTH(uartRxBuffer);
                state = UART_RX_STATE_DATA;
            }
        }

        if(state == UART_RX_STATE_DATA){
            if(rxCount >= (COMM_PAYLOAD_POS + elementsToReceive)){
                // REMINDER: analyze preamble and status here..
                // REMINDER: handle signature as well here ..
                memcpy(buffer, uartRxBuffer, (COMM_PAYLOAD_POS + elementsToReceive));
                memcpy(uartRxBuffer, (uartRxBuffer + COMM_PAYLOAD_POS + elementsToReceive), (UART_RX_BUFFER_SIZE - COMM_PAYLOAD_POS - elementsToReceive));
                rxCount -= (COMM_PAYLOAD_POS + elementsToReceive);
                state = UART_RX_STATE_HEADER;
                numPackets++;
                packetFound = 1;
            }
        }                

    } while(packetFound);
    return numPackets;
}

/**
  * @}
  */
/**
  * @}
  */
