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
 *  @brief UART streaming driver implementation
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup PC_Communication
  * @{
  */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "stream_driver.h"
#include "uart_stream_driver.h"

//#include "st_stream_clib.h"
#include "stream_dispatcher.h"
#include "evalAPI_commands.h"
#include "stuhfl_pl.h"

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */
extern uint16_t txID;
/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */
static uint8_t *rxBuffer;    /** hook to rx buffer provided by uartStreamInitialize */
static uint8_t *txBuffer;    /** hook to tx buffer provided by uartStreamInitialize */

static uint16_t packetsToProcess;
static uint8_t txUartBuffer[UART_TX_BUFFER_SIZE];

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */
void uartStreamInitialize(uint8_t *rxBuf, uint8_t *txBuf)
{
    // rxBuf = rxBuffer in stream_dispatcher.c
    // txBuf = txBuffer in stream_dispatcher.c

    // setup pointers to buffers
    rxBuffer = rxBuf;
    txBuffer = txBuf;

    packetsToProcess = 0;
}

/*------------------------------------------------------------------------- */
void uartStreamConnect(void)
{
}

/*------------------------------------------------------------------------- */
void uartStreamDisconnect(void)
{
}

/*------------------------------------------------------------------------- */
uint8_t uartStreamReady(void)
{
    return 1;
}

/*------------------------------------------------------------------------- */
void uartStreamPacketProcessed(uint8_t rxed)
{
    packetsToProcess = (packetsToProcess > 0 ? packetsToProcess - 1 : 0);
}

/*------------------------------------------------------------------------- */
uint8_t uartStreamHasAnotherPacket(void)
{
    return packetsToProcess;
}

/*------------------------------------------------------------------------- */
uint8_t uartStreamReceive(void)
{
    packetsToProcess += uartRxRead(rxBuffer);
    return packetsToProcess;
}

/*------------------------------------------------------------------------- */
void uartStreamTransmit(uint16_t totalTxSize)
{
    // mode, status and payload_length
    COMM_SET_PREAMBLE_MODE(txUartBuffer, 0);
    COMM_SET_PREAMBLE_ID(txUartBuffer, txID++);    
    COMM_SET_STATUS(txUartBuffer, (uint16_t)((int8_t)StreamDispatcherGetLastError()));      // Keep sign
    COMM_SET_CMD(txUartBuffer, StreamDispatcherGetLastCmd());
    COMM_SET_PAYLOAD_LENGTH(txUartBuffer, totalTxSize);
    // payload
    memcpy((txUartBuffer + COMM_PAYLOAD_POS), (txBuffer + COMM_PAYLOAD_POS), totalTxSize);
    uartTxNBytes(txUartBuffer,(COMM_PAYLOAD_POS + totalTxSize));
}

void uartStreamTransmitBuffer(uint16_t status, uint16_t cmd, uint8_t *txData, uint16_t txSize)
{
    uartTxBufferDirect(status, cmd, txData, txSize);
}


/**
  * @}
  */
/**
  * @}
  */
