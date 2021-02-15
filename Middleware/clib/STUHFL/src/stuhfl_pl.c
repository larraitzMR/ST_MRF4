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
 *  @brief
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup PC_Communication
  * @{
  */

// dllmain.cpp

#include "stuhfl.h"
#include "stuhfl_dl.h"
#include "stuhfl_pl.h"
#include "stuhfl_err.h"
#include "stuhfl_platform.h"
#include "stuhfl_helpers.h"
#include "stuhfl_log.h"

#if defined(WIN32) || defined(WIN64)
#include "stuhfl_bl_win32.h"
#elif defined(POSIX)
#include "stuhfl_bl_posix.h"
#endif

uint16_t sndID = 0;
uint16_t rcvID = 0;

static uint8_t *snd = NULL;
static uint16_t gSndMaxLen = 0;
static uint8_t *rcv = NULL;
static uint16_t gRcvMaxLen = 0;

#define DIRECTION_FROM_BOARD                    0x0000
#define DIRECTION_TO_BOARD                      0x8000
#define SIGNATURE_NONE                          0x00
#define SIGNATURE_XOR_BCC                       0x01

uint16_t mode = DIRECTION_TO_BOARD | SIGNATURE_NONE;

void encodeSndFrame(uint8_t *sndData, uint16_t *sndDataLen, uint16_t mode, uint16_t id, uint16_t status, uint16_t cmd, uint8_t *payloadData, uint16_t payloadDataLen);
//
typedef STUHFL_T_RET_CODE(*STUHFL_T_Reset)(STUHFL_T_DEVICE_CTX *device, STUHFL_T_RESET resetType);
typedef STUHFL_T_RET_CODE(*STUHFL_T_Disconnect)(STUHFL_T_DEVICE_CTX *device);
typedef STUHFL_T_RET_CODE(*STUHFL_T_SndRaw)(STUHFL_T_DEVICE_CTX *device, uint8_t *data, uint16_t dataLen);
typedef STUHFL_T_RET_CODE(*STUHFL_T_RcvRaw)(STUHFL_T_DEVICE_CTX *device, uint8_t *data, uint16_t *dataLen);
typedef STUHFL_T_RET_CODE(*STUHFL_T_SetDTR)(STUHFL_T_DEVICE_CTX *device, uint8_t dtrValue);
typedef STUHFL_T_RET_CODE(*STUHFL_T_GetDTR)(STUHFL_T_DEVICE_CTX *device, uint8_t *dtrValue);
typedef STUHFL_T_RET_CODE(*STUHFL_T_SetRTS)(STUHFL_T_DEVICE_CTX *device, uint8_t rtsValue);
typedef STUHFL_T_RET_CODE(*STUHFL_T_GetRTS)(STUHFL_T_DEVICE_CTX *device, uint8_t *rtsValue);
typedef STUHFL_T_RET_CODE(*STUHFL_T_SetTimeouts)(STUHFL_T_DEVICE_CTX *device, uint32_t rdTimeout, uint32_t wrTimeout);
typedef STUHFL_T_RET_CODE(*STUHFL_T_GetTimeouts)(STUHFL_T_DEVICE_CTX *device, uint32_t *rdTimeout, uint32_t *wrTimeout);



static STUHFL_T_Reset STUHFL_F_PlatformReset = NULL;
static STUHFL_T_Disconnect STUHFL_F_PlatformDisconnect = NULL;
static STUHFL_T_SndRaw STUHFL_F_PlatformSndRaw = NULL;
static STUHFL_T_RcvRaw STUHFL_F_PlatformRcvRaw = NULL;

static STUHFL_T_SetDTR STUHFL_F_PlatformSetDTR = NULL;
static STUHFL_T_GetDTR STUHFL_F_PlatformGetDTR = NULL;
static STUHFL_T_SetRTS STUHFL_F_PlatformSetRTS = NULL;
static STUHFL_T_GetRTS STUHFL_F_PlatformGetRTS = NULL;
static STUHFL_T_SetTimeouts STUHFL_F_PlatformSetTimeouts = NULL;
static STUHFL_T_GetTimeouts STUHFL_F_PlatformGetTimeouts = NULL;

#define TRACE_PL_LOG_CLEAR()    { STUHFL_F_LogClear(LOG_LEVEL_TRACE_PL); }
#define TRACE_PL_LOG_APPEND(...){ STUHFL_F_LogAppend(LOG_LEVEL_TRACE_PL, __VA_ARGS__); }
#define TRACE_PL_LOG_FLUSH()    { STUHFL_F_LogFlush(LOG_LEVEL_TRACE_PL); }

#define TRACE_PL_LOG_START()    { STUHFL_F_LogClear(LOG_LEVEL_TRACE_PL); }
#define TRACE_PL_LOG(...)       { STUHFL_F_LogAppend(LOG_LEVEL_TRACE_PL, __VA_ARGS__); STUHFL_F_LogFlush(LOG_LEVEL_TRACE_PL); }

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_Connect_Dispatcher(STUHFL_T_DEVICE_CTX *device, uint8_t *sndBuffer, uint16_t sndBufferLen, uint8_t *rcvBuffer, uint16_t rcvBufferLen)
{
    STUHFL_T_RET_CODE ret = ERR_NONE;


    STUHFL_T_ParamTypeConnectionPort port = NULL;
    uint32_t br;
    ret = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_PORT, &port);
    ret |= STUHFL_F_GetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_BR, &br);

    if (ret != ERR_NONE) {
        return ret;
    }
#if defined(WIN32) || defined(WIN64)
    // Connect
    ret = STUHFL_F_Connect_Win32(device, port, br);

    // register operation handlers
    STUHFL_F_PlatformReset = STUHFL_F_Reset_Win32;
    STUHFL_F_PlatformDisconnect = STUHFL_F_Disconnect_Win32;

    STUHFL_F_PlatformSndRaw = STUHFL_F_SndRaw_Win32;
    STUHFL_F_PlatformRcvRaw = STUHFL_F_RcvRaw_Win32;

    STUHFL_F_PlatformSetDTR = STUHFL_F_SetDTR_Win32;
    STUHFL_F_PlatformGetDTR = STUHFL_F_GetDTR_Win32;
    STUHFL_F_PlatformSetRTS = STUHFL_F_SetRTS_Win32;
    STUHFL_F_PlatformGetRTS = STUHFL_F_GetRTS_Win32;
    STUHFL_F_PlatformSetTimeouts = STUHFL_F_SetTimeouts_Win32;
    STUHFL_F_PlatformGetTimeouts = STUHFL_F_GetTimeouts_Win32;

#elif defined(POSIX)
    // Connect
    ret = STUHFL_F_Connect_Posix(device, port, br);

    // register operation handlers
    STUHFL_F_PlatformReset = STUHFL_F_Reset_Posix;
    STUHFL_F_PlatformDisconnect = STUHFL_F_Disconnect_Posix;

    STUHFL_F_PlatformSndRaw = STUHFL_F_SndRaw_Posix;
    STUHFL_F_PlatformRcvRaw = STUHFL_F_RcvRaw_Posix;

    STUHFL_F_PlatformSetDTR = STUHFL_F_SetDTR_Posix;
    STUHFL_F_PlatformGetDTR = STUHFL_F_GetDTR_Posix;
    STUHFL_F_PlatformSetRTS = STUHFL_F_SetRTS_Posix;
    STUHFL_F_PlatformGetRTS = STUHFL_F_GetRTS_Posix;
    STUHFL_F_PlatformSetTimeouts = STUHFL_F_SetTimeouts_Posix;
    STUHFL_F_PlatformGetTimeouts = STUHFL_F_GetTimeouts_Posix;

#else
    return ERR_PARAM;
#endif

    snd = sndBuffer;
    gSndMaxLen = sndBufferLen;
    rcv = rcvBuffer;
    gRcvMaxLen = rcvBufferLen;

    sndID = 0;
    rcvID = 0;
    mode = DIRECTION_TO_BOARD | SIGNATURE_NONE;
    return ret;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_Reset_Dispatcher(STUHFL_T_DEVICE_CTX device, STUHFL_T_RESET resetType)
{
    return STUHFL_F_PlatformReset(device, resetType);
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_Disconnect_Dispatcher(STUHFL_T_DEVICE_CTX device)
{
    return STUHFL_F_PlatformDisconnect(device);
}

// --------------------------------------------------------------------------
uint8_t *STUHFL_F_Get_SndPayloadPtr()
{
    return &snd[COMM_PAYLOAD_POS];
}

// --------------------------------------------------------------------------
uint16_t STUHFL_F_Get_RcvCmd()
{
    return COMM_GET_CMD(rcv);
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_Get_RcvStatus()
{
    return (STUHFL_T_RET_CODE)(int16_t)COMM_GET_STATUS(rcv);    // Keep status sign
}

// --------------------------------------------------------------------------
uint8_t *STUHFL_F_Get_RcvPayloadPtr()
{
    return &rcv[COMM_PAYLOAD_POS];
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_Snd_Dispatcher(STUHFL_T_DEVICE_CTX device, uint16_t cmd, uint16_t status, uint8_t *payloadData, uint16_t payloadDataLen)
{
    uint16_t sndLen = 0;

    TRACE_PL_LOG_START();
    // prepare frame
    encodeSndFrame(snd, &sndLen, mode, sndID, status, cmd, payloadData, payloadDataLen);
    sndID++;

#define TB_SIZE    1024
    char tb[TB_SIZE];
    TRACE_PL_LOG("Tx >>> (%04d) 0x%s", sndLen, byteArray2HexString(tb, TB_SIZE, snd, sndLen));
#undef TB_SIZE

    // finally send
    return STUHFL_F_SndRaw_Dispatcher(device, snd, sndLen);
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_Rcv_Dispatcher(STUHFL_T_DEVICE_CTX device, uint8_t *payloadData, uint16_t *payloadDataLen)
{
    uint16_t expectedLen;
    *payloadDataLen = 0;

    // NOTE: receive operation is split into 2 parts
    // 1) Read fixed data (header + status + cmd + payloadLength) and get the payload length information from the header
    // 2) Read the payload itself

    // 1) Header + status + CMD + payloadLen
    //
    uint16_t rcvLen = COMM_PAYLOAD_POS;

    TRACE_PL_LOG_START();
    STUHFL_T_RET_CODE ret = STUHFL_F_PlatformRcvRaw(device, rcv, &rcvLen);
    if ((ret != ERR_NONE) || (rcvLen != COMM_PAYLOAD_POS)) {
        TRACE_PL_LOG("Rx <<< (%04d) .. no data packet received", rcvLen);
        return ERR_TIMEOUT;
    }
    *payloadDataLen = 0;

    // Check cmd is a valid one
    uint16_t cmd = COMM_GET_CMD(rcv);
    if (((cmd >> 8) > STUHFL_CG_TS) || ((cmd & 0xFF) >= 0x30)) {
        TRACE_PL_LOG("Rx <<< (%04d) .. unknown command (%04x), ignoring data ...", COMM_GET_PAYLOAD_LENGTH(rcv), cmd);
        return ERR_IO;
    }

    // 2) Payload
    expectedLen = COMM_GET_PAYLOAD_LENGTH(rcv);
    if (expectedLen > gRcvMaxLen-COMM_PAYLOAD_POS) {
        // Clamp to max size of Rx buffer
        expectedLen = (uint16_t)(gRcvMaxLen - COMM_PAYLOAD_POS);
    }
    uint16_t len = expectedLen;
    ret = STUHFL_F_PlatformRcvRaw(device, &rcv[COMM_PAYLOAD_POS], &len);
#define TB_SIZE    1024
    char tb[TB_SIZE];
    if ((ret != ERR_NONE) || (len < expectedLen)) {
        TRACE_PL_LOG("Rx <<< (%04d) .. receive length mismatch error (%d), ignoring RX data: 0x%s", (COMM_PAYLOAD_POS+len), expectedLen, byteArray2HexString(tb, TB_SIZE, rcv, (uint16_t)(COMM_PAYLOAD_POS+len)));
        return ERR_IO;
    }
    TRACE_PL_LOG("Rx <<< (%04d) 0x%s", (COMM_PAYLOAD_POS+len), byteArray2HexString(tb, TB_SIZE, rcv, (uint16_t)(COMM_PAYLOAD_POS+len)));
#undef TB_SIZE

    rcvLen = (uint16_t)(rcvLen + expectedLen);
    *payloadDataLen = expectedLen;

    if (&rcv[COMM_PAYLOAD_POS] != payloadData) {
        memcpy(payloadData, &rcv[COMM_PAYLOAD_POS], expectedLen);
    }


    // 3) Verify signature
    uint16_t mode = COMM_GET_PREAMBLE_MODE(rcv);
    if (mode & SIGNATURE_XOR_BCC) {
        uint8_t bcc = 0;
        for (int i = 0; i < *payloadDataLen; i++) {
            bcc ^= payloadData[i];
        }

        // invalid signature
        if (bcc != 0) {
            ret = ERR_PROTO;
        }

        // remove signature from payload
        (*payloadDataLen)--;
    }

    return ret;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_SndRaw_Dispatcher(STUHFL_T_DEVICE_CTX device, uint8_t *data, uint16_t dataLen)
{
    //
    return STUHFL_F_PlatformSndRaw(device, data, dataLen);
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_RcvRaw_Dispatcher(STUHFL_T_DEVICE_CTX device, uint8_t *data, uint16_t *dataLen)
{
    //
    return STUHFL_F_PlatformRcvRaw(device, data, dataLen);
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_SetParam_Dispatcher(STUHFL_T_DEVICE_CTX device, STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE value)
{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    STUHFL_T_PARAM type = param & STUHFL_PARAM_TYPE_MASK;
    param &= STUHFL_PARAM_KEY_MASK;

    TRACE_PL_LOG_START();
    switch (type) {

    case STUHFL_PARAM_TYPE_CONNECTION: {
        switch (param) {
        case STUHFL_PARAM_KEY_RD_TIMEOUT_MS: {
            uint32_t rdTimeout = 0;
            uint32_t wrTimeout = 0;
            ret = STUHFL_F_PlatformGetTimeouts(device, &rdTimeout, &wrTimeout);
            memcpy(&rdTimeout, value, sizeof(uint32_t));
            ret = STUHFL_F_PlatformSetTimeouts(device, rdTimeout, wrTimeout);
            TRACE_PL_LOG_APPEND("SetParam: RdTimeout:%d ", rdTimeout);
            break;
        }
        case STUHFL_PARAM_KEY_WR_TIMEOUT_MS: {
            uint32_t rdTimeout = 0;
            uint32_t wrTimeout = 0;
            ret = STUHFL_F_PlatformGetTimeouts(device, &rdTimeout, &wrTimeout);
            memcpy(&wrTimeout, value, sizeof(uint32_t));
            ret = STUHFL_F_PlatformSetTimeouts(device, rdTimeout, wrTimeout);
            TRACE_PL_LOG_APPEND("SetParam: WrTimeout:%d ", wrTimeout);
            break;
        }
        case STUHFL_PARAM_KEY_DTR: {
            uint8_t on = 0;
            memcpy(&on, value, sizeof(uint8_t));
            ret = STUHFL_F_PlatformSetDTR(device, on);
            TRACE_PL_LOG_APPEND("SetParam: DTR:%d ", on);
            break;
        }
        case STUHFL_PARAM_KEY_RTS: {
            uint8_t on = 0;
            memcpy(&on, value, sizeof(uint8_t));
            ret = STUHFL_F_PlatformSetRTS(device, on);
            TRACE_PL_LOG_APPEND("SetParam: RTS:%d ", on);
            break;
        }
        default:
            ret = ERR_PARAM;
            break;
        }
        break;
    }

    default:
        ret = ERR_PARAM;
        break;
    }

    TRACE_PL_LOG_FLUSH();
    return ret;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_GetParam_Dispatcher(STUHFL_T_DEVICE_CTX device, STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE value)
{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    STUHFL_T_PARAM type = param & STUHFL_PARAM_TYPE_MASK;
    param &= STUHFL_PARAM_KEY_MASK;

    TRACE_PL_LOG_START();
    switch (type) {

    case STUHFL_PARAM_TYPE_CONNECTION: {
        switch (param) {

        case STUHFL_PARAM_KEY_RD_TIMEOUT_MS: {
            uint32_t rdTimeout = 0;
            uint32_t wrTimeout = 0;
            ret = STUHFL_F_PlatformGetTimeouts(device, &rdTimeout, &wrTimeout);
            TRACE_PL_LOG_APPEND("GetParam: RdTimeout:%d ", rdTimeout);
            memcpy(&value, &rdTimeout, sizeof(uint32_t));
            break;
        }
        case STUHFL_PARAM_KEY_WR_TIMEOUT_MS: {
            uint32_t rdTimeout = 0;
            uint32_t wrTimeout = 0;
            ret = STUHFL_F_PlatformGetTimeouts(device, &rdTimeout, &wrTimeout);
            TRACE_PL_LOG_APPEND("GetParam: WrTimeout:%d ", wrTimeout);
            memcpy(&value, &wrTimeout, sizeof(uint32_t));
            break;
        }
        case STUHFL_PARAM_KEY_DTR: {
            uint8_t dtrValue = 0;
            ret = STUHFL_F_PlatformGetDTR(device, &dtrValue);
            TRACE_PL_LOG_APPEND("GetParam: DTR:%d ", dtrValue);
            memcpy(&value, &dtrValue, sizeof(uint8_t));
            break;
        }
        case STUHFL_PARAM_KEY_RTS: {
            uint8_t rtsValue = 0;
            ret = STUHFL_F_PlatformGetRTS(device, &rtsValue);
            TRACE_PL_LOG_APPEND("GetParam: RTS:%d ", rtsValue);
            memcpy(&value, &rtsValue, sizeof(uint8_t));
            break;
        }
        default:
            ret = ERR_PARAM;
            break;
        }
        break;
    }

    default:
        ret = ERR_PARAM;
        break;
    }

    TRACE_PL_LOG_FLUSH();
    return ret;
}




void encodeSndFrame(uint8_t *sndData, uint16_t *sndDataLen, uint16_t mode, uint16_t id, uint16_t status, uint16_t cmd, uint8_t *payloadData, uint16_t payloadDataLen)
{
    // 1. build header
    COMM_SET_PREAMBLE_MODE(sndData, mode);
    COMM_SET_PREAMBLE_ID(sndData, id);
    COMM_SET_STATUS(sndData, status);

    // 2. CMD + PayloadLen
    COMM_SET_CMD(sndData, cmd);
    payloadDataLen = (uint16_t)(payloadDataLen + ((mode & SIGNATURE_XOR_BCC) ? 1U : 0U));
    COMM_SET_PAYLOAD_LENGTH(sndData, payloadDataLen);

    // set sndDataLen to at least HeaderLen + CmdLen + PayloadLen
    if (payloadDataLen > gSndMaxLen-COMM_PAYLOAD_POS) {
        // Clamp to max size of Tx buffer
        payloadDataLen = (uint16_t)(gSndMaxLen - COMM_PAYLOAD_POS);
    }
    *sndDataLen = (uint16_t)(COMM_PAYLOAD_POS + payloadDataLen);

    // 3. payload (copy if needed)
    if (&sndData[COMM_PAYLOAD_POS] != payloadData) {
        memcpy(&sndData[COMM_PAYLOAD_POS], payloadData, payloadDataLen);
        *sndDataLen = (uint16_t)(*sndDataLen + payloadDataLen);
    }


    // 4. checksum (if needed)
    if (mode & SIGNATURE_XOR_BCC) {
        sndData[COMM_PAYLOAD_POS + payloadDataLen] = 0;
        for (int i = 0; i < payloadDataLen; i++) {
            sndData[COMM_PAYLOAD_POS + payloadDataLen] ^= payloadData[i];
        }
        // add signature to length
        (*sndDataLen) ++;
    }
}

/**
  * @}
  */
/**
  * @}
  */
