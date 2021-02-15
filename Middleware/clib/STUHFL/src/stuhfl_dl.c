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
#include "stuhfl_al.h"
#include "stuhfl_sl.h"
#include "stuhfl_sl_gen2.h"
#include "stuhfl_sl_gb29768.h"
#include "stuhfl_dl.h"
#include "stuhfl_dl_ST25RU3993.h"
#include "stuhfl_pl.h"
#include "stuhfl_helpers.h"
#include "stuhfl_err.h"
#include "stuhfl_log.h"
#if defined(WIN32) || defined(WIN64)
#include "stuhfl_bl_win32.h"
#endif

//
#define TRACE_DL_LOG_CLEAR()    { STUHFL_F_LogClear(LOG_LEVEL_TRACE_DL); }
#define TRACE_DL_LOG_APPEND(...){ STUHFL_F_LogAppend(LOG_LEVEL_TRACE_DL, __VA_ARGS__); }
#define TRACE_DL_LOG_FLUSH()    { STUHFL_F_LogFlush(LOG_LEVEL_TRACE_DL); }
//
#define TRACE_DL_LOG_START()    { STUHFL_F_LogClear(LOG_LEVEL_TRACE_DL); }
#define TRACE_DL_LOG(...)       { STUHFL_F_LogAppend(LOG_LEVEL_TRACE_DL, __VA_ARGS__); STUHFL_F_LogFlush(LOG_LEVEL_TRACE_DL); }

//
#define STUHFL_D_ST25RU3993_HID_VID     0x1234
#define STUHFL_D_ST25RU3993_HID_PID     0x1234

//
static STUHFL_T_DEVICE_CTX deviceCtx = NULL;
static STUHFL_T_ParamTypeConnectionPort comPort = NULL;
//static STUHFL_T_ParamTypeConnectionBR br = 115200;
//static STUHFL_T_ParamTypeConnectionBR br = 230400;
//static STUHFL_T_ParamTypeConnectionBR br = 460800;
//static STUHFL_T_ParamTypeConnectionBR br = 500000;
//static STUHFL_T_ParamTypeConnectionBR br = 576000;
//static STUHFL_T_ParamTypeConnectionBR br = 921600;
//static STUHFL_T_ParamTypeConnectionBR br = 1000000;
//static STUHFL_T_ParamTypeConnectionBR br = 1152000;
//static STUHFL_T_ParamTypeConnectionBR br = 1500000;
//static STUHFL_T_ParamTypeConnectionBR br = 2000000;
//static STUHFL_T_ParamTypeConnectionBR br = 2500000;
static STUHFL_T_ParamTypeConnectionBR br = 3000000;     // highest baudrate supported by FT231X on EVAL board
//static STUHFL_T_ParamTypeConnectionBR br = 3500000;
//static STUHFL_T_ParamTypeConnectionBR br = 4000000;

static bool gIgnoreInventoryData = true;


// - Internal implementation helpers ----------------------------------------
STUHFL_T_RET_CODE SetParam_SndPayload_TYPE_ST25RU3993(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE *values, uint16_t *valuesOffset, uint8_t *sndPayload, uint16_t *sndPayloadOffset);
STUHFL_T_RET_CODE SetParam_RcvPayload_TYPE_ST25RU3993(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE *values, uint16_t *valuesOffset, uint8_t *rcvPayload, uint16_t *rcvPayloadOffset);
STUHFL_T_RET_CODE GetParam_SndPayload_TYPE_ST25RU3993(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE *values, uint16_t *valuesOffset, uint8_t *sndPayload, uint16_t *sndPayloadOffset);
STUHFL_T_RET_CODE GetParam_RcvPayload_TYPE_ST25RU3993(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE *values, uint16_t *valuesOffset, uint8_t *rcvPayload, uint16_t rcvPayloadAvailable, uint16_t *rcvPayloadOffset);



// --------------------------------------------------------------------------
STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetRawData(uint8_t *data, uint16_t *dataLen)
{
#define TB_SIZE    1024
    char tb[TB_SIZE];
    TRACE_DL_LOG_START();
    STUHFL_T_RET_CODE ret = STUHFL_F_RcvRaw_Dispatcher(deviceCtx, data, dataLen);
    TRACE_DL_LOG("STUHFL_F_GetRawData(*data = 0x%s, dataLen = %d) = %d", byteArray2HexString(tb, TB_SIZE, data, min(16, *dataLen)), *dataLen, ret);
#undef TB_SIZE
    return ret;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Connect(STUHFL_T_DEVICE_CTX *device, uint8_t *sndBuffer, uint16_t sndBufferLen, uint8_t *rcvBuffer, uint16_t rcvBufferLen)
{
    STUHFL_T_RET_CODE ret = STUHFL_F_Connect_Dispatcher(device, sndBuffer, sndBufferLen, rcvBuffer, rcvBufferLen);
    deviceCtx = device;
    TRACE_DL_LOG_START();
    TRACE_DL_LOG("STUHFL_F_Connect(device = 0x%x, *sndBuffer = 0x%x, sndBufferLen = %d, *rcvBuffer = 0x%x, rcvBufferLen = %d) = %d", *device, sndBuffer, sndBufferLen, rcvBuffer, rcvBufferLen, ret);
    return ret;
}


// --------------------------------------------------------------------------
STUHFL_DLL_API STUHFL_T_DEVICE_CTX CALL_CONV STUHFL_F_GetCtx(void)
{
    TRACE_DL_LOG_START();
    TRACE_DL_LOG("STUHFL_F_GetCtx() .. deviceCtx = 0x%x)", deviceCtx);
    return deviceCtx;
}

// --------------------------------------------------------------------------
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Reset(STUHFL_T_RESET resetType)
{
    TRACE_DL_LOG_START();
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    switch (resetType) {
    case STUHFL_RESET_TYPE_SOFT:
        break;

    case STUHFL_RESET_TYPE_HARD:
        break;

    case STUHFL_RESET_TYPE_CLEAR_COMM:
        ret = STUHFL_F_Reset_Dispatcher(deviceCtx, resetType);
        break;

    default:
        break;
    }

    TRACE_DL_LOG("STUHFL_F_Reset(resetType = %d) = %d", resetType, ret);
    return ret;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Disconnect()
{
    TRACE_DL_LOG_START();
    STUHFL_T_RET_CODE ret = STUHFL_F_Disconnect_Dispatcher(deviceCtx);
    TRACE_DL_LOG("STUHFL_F_Disconnect(deviceCtx = 0x%x) = %d", deviceCtx, ret);
    return ret;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE CALL_CONV STUHFL_F_SetParam(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE value)
{
    //TRACE_DL_LOG_START();
    STUHFL_T_RET_CODE ret = STUHFL_F_SetMultipleParams(1, &param, value);
    //TRACE_DL_LOG("STUHFL_F_SetParam(param = 0x%x, *value = 0x%x) = %d", param, value, ret);
    return ret;
}
STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetParam(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE value)
{
    //TRACE_DL_LOG_START();
    STUHFL_T_RET_CODE ret = STUHFL_F_GetMultipleParams(1, &param, value);
    //TRACE_DL_LOG("STUHFL_F_GetParam(param = 0x%x, *value = 0x%x) = %d", param, value, ret);
    return ret;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetParamInfo(STUHFL_T_PARAM param, STUHFL_T_Param_Info info)
{
    TRACE_DL_LOG_START();
    STUHFL_T_RET_CODE ret = ERR_PARAM;

    // by default most params have the following configuration
    info.type = (STUHFL_T_TYPE)STUHFL_TYPE_UINT32;
    info.size_of = sizeof(STUHFL_T_ParamTypeUINT32);
    info.canRd = true;
    info.canWr = true;

    STUHFL_T_PARAM type = param & STUHFL_PARAM_TYPE_MASK;
    param &= STUHFL_PARAM_KEY_MASK;

    switch (type) {
    case STUHFL_PARAM_TYPE_CONNECTION: {
        switch (param) {
        // params with default param info
        case STUHFL_PARAM_KEY_PORT:
        case STUHFL_PARAM_KEY_BR:
        case STUHFL_PARAM_KEY_RD_TIMEOUT_MS:
        case STUHFL_PARAM_KEY_WR_TIMEOUT_MS:
            break;
        case STUHFL_PARAM_KEY_DTR:
        case STUHFL_PARAM_KEY_RTS:
            info.type = (STUHFL_T_TYPE)STUHFL_TYPE_UINT8;
            info.size_of = sizeof(STUHFL_T_ParamTypeUINT8);
            break;

        default:
            ret = ERR_PARAM;
            break;
        }
        break;
    }

    case STUHFL_PARAM_TYPE_ST25RU3993: {
        switch (param) {
        //
        case STUHFL_PARAM_KEY_VERSION_FW:
        case STUHFL_PARAM_KEY_VERSION_HW:
            info.canWr = false;
            break;

        case STUHFL_PARAM_KEY_RWD_REGISTER: {
            info.type = (STUHFL_T_TYPE)STUHFL_TYPE_ST25RU3993_ACCESS_REGISTER;
            info.size_of = sizeof(STUHFL_T_ST25RU3993_Register);
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

    TRACE_DL_LOG("STUHFL_F_GetParamInfo(param = 0x%x, info.type = %d, info.size_of = %d, info.canRd = %d, info.canWr = %d) = %d", param, info.type, info.size_of, info.canRd, info.canWr, ret);
    return ret;
}

// --------------------------------------------------------------------------
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_SetMultipleParams(STUHFL_T_PARAM_CNT paramCnt, STUHFL_T_PARAM *params, STUHFL_T_PARAM_VALUE *values)
{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    uint8_t *sndPayload = STUHFL_F_Get_SndPayloadPtr();
    uint16_t sndPayloadLen = 0;
    uint8_t *rcvPayload = STUHFL_F_Get_RcvPayloadPtr();
    uint16_t rcvPayloadLen = 0;
    uint16_t valuesOffset = 0;
    bool hostParam = false;

    TRACE_DL_LOG_CLEAR();
    TRACE_DL_LOG_APPEND("STUHFL_F_SetMultipleParams(paramCnt = %d, params:values =", paramCnt);

    // prepare send data for all params that need to be pulled from the board.
    // All other params that are not board related are processed here directly
    for (uint32_t i = 0; i < paramCnt; i++) {

        STUHFL_T_PARAM type = params[i] & STUHFL_PARAM_TYPE_MASK;
        STUHFL_T_PARAM param = params[i] & STUHFL_PARAM_KEY_MASK;

        switch (type) {
        case STUHFL_PARAM_TYPE_CONNECTION: {
            switch (param) {
            case STUHFL_PARAM_KEY_PORT:
                comPort = (STUHFL_T_ParamTypeConnectionPort)&(((uint8_t *)values)[valuesOffset]);
                valuesOffset = (uint16_t)(valuesOffset + (uint16_t)strlen(comPort));        // Warning removal
                ret = ERR_NONE;
                hostParam = true;
                TRACE_DL_LOG_APPEND(" Port:%s", comPort);
                break;
            case STUHFL_PARAM_KEY_BR:
                br = *((STUHFL_T_ParamTypeConnectionBR*)&(((uint8_t *)values)[valuesOffset]));
                valuesOffset = (uint16_t)(valuesOffset + (uint16_t)sizeof(STUHFL_T_ParamTypeConnectionBR));     // Warning removal
                ret = ERR_NONE;
                hostParam = true;
                TRACE_DL_LOG_APPEND(" BaudRate:%s", br);
                break;

            // forward to PL
            case STUHFL_PARAM_KEY_RD_TIMEOUT_MS:
            case STUHFL_PARAM_KEY_WR_TIMEOUT_MS:
                ret = STUHFL_F_SetParam_Dispatcher(deviceCtx, type | param, &(((uint8_t *)values)[valuesOffset]));
                if (param == STUHFL_PARAM_KEY_RD_TIMEOUT_MS) {
                    TRACE_DL_LOG_APPEND(" RdTimeout:%d ", *(uint32_t*)&(((uint8_t *)values)[valuesOffset]));
                } else {
                    TRACE_DL_LOG_APPEND(" WdTimeout:%d ", *(uint32_t*)&(((uint8_t *)values)[valuesOffset]));
                }
                valuesOffset = (uint16_t)(valuesOffset + (uint16_t)sizeof(uint32_t));   // Warning removal
                ret = ERR_NONE;
                hostParam = true;
                break;

            case STUHFL_PARAM_KEY_DTR:
            case STUHFL_PARAM_KEY_RTS:
                ret = STUHFL_F_SetParam_Dispatcher(deviceCtx, type | param, &(((uint8_t *)values)[valuesOffset]));
                if (param == STUHFL_PARAM_KEY_DTR) {
                    TRACE_DL_LOG_APPEND(" DTR:%d ", ((uint8_t *)values)[valuesOffset]);
                } else {
                    TRACE_DL_LOG_APPEND(" RTS:%d ", ((uint8_t *)values)[valuesOffset]);
                }
                valuesOffset = (uint16_t)(valuesOffset + (uint16_t)sizeof(uint8_t));    // Warning removal
                ret = ERR_NONE;
                hostParam = true;
                break;

            default:
                ret = ERR_PARAM;
                break;
            }
            break;
        }

        case STUHFL_PARAM_TYPE_ST25RU3993:
            if (hostParam) {
                ret = ERR_PARAM;
                break;
            }
            ret = SetParam_SndPayload_TYPE_ST25RU3993(param, values, &valuesOffset, sndPayload, &sndPayloadLen);
            break;

        default:
            ret = ERR_PARAM;
            break;
        }

        // in case of any error return ..
        if (ret != ERR_NONE) {
            TRACE_DL_LOG_APPEND(") = %d", ret);
            TRACE_DL_LOG_FLUSH();
            return ret;
        }
    }

    // Exchange all board relevant data..
    if (!hostParam && ((STUHFL_T_POINTER2UINT)deviceCtx != (STUHFL_T_POINTER2UINT)NULL) && ((STUHFL_T_POINTER2UINT)deviceCtx != (STUHFL_T_POINTER2UINT)INVALID_HANDLE_VALUE)) {
        ret = STUHFL_F_Snd_Dispatcher(deviceCtx, (STUHFL_CG_DL << 8) | STUHFL_CC_SET_PARAM, STATUS_DEFAULT_SND, sndPayload, sndPayloadLen);
        ret |= STUHFL_F_Rcv_Dispatcher(deviceCtx, rcvPayload, &rcvPayloadLen);
        ret |= STUHFL_F_Get_RcvStatus();

        // if succeeded get data
        if (ret == ERR_NONE) {

            uint16_t rcvPayloadPos = 0;
            valuesOffset = 0;
            for (uint32_t i = 0; i < paramCnt; i++) {

                STUHFL_T_PARAM type = params[i] & STUHFL_PARAM_TYPE_MASK;
                STUHFL_T_PARAM param = params[i] & STUHFL_PARAM_KEY_MASK;

                switch (type) {
                case STUHFL_PARAM_TYPE_CONNECTION: {
                    switch (param) {
                    case STUHFL_PARAM_KEY_PORT:
                    case STUHFL_PARAM_KEY_BR:
                    case STUHFL_PARAM_KEY_RD_TIMEOUT_MS:
                    case STUHFL_PARAM_KEY_WR_TIMEOUT_MS:
                    case STUHFL_PARAM_KEY_DTR:
                    case STUHFL_PARAM_KEY_RTS:
                        // already handled..
                        break;

                    default:
                        ret = ERR_PARAM;
                        break;
                    }
                    break;
                }

                case STUHFL_PARAM_TYPE_ST25RU3993:
                    ret = SetParam_RcvPayload_TYPE_ST25RU3993(param, values, &valuesOffset, rcvPayload, &rcvPayloadPos);
                    break;

                default:
                    ret = ERR_PARAM;
                    break;
                }

                // in case of any error return ..
                if (ret != ERR_NONE) {
                    TRACE_DL_LOG_APPEND(") = %d", ret);
                    TRACE_DL_LOG_FLUSH();
                    return ret;
                }
            }


        }
    }

    TRACE_DL_LOG_APPEND(") = %d", ret);
    TRACE_DL_LOG_FLUSH();
    return ret;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetMultipleParams(STUHFL_T_PARAM_CNT paramCnt, STUHFL_T_PARAM *params, STUHFL_T_PARAM_VALUE *values)
{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    uint8_t *sndPayload = STUHFL_F_Get_SndPayloadPtr();
    uint16_t sndPayloadLen = 0;
    uint8_t *rcvPayload = STUHFL_F_Get_RcvPayloadPtr();
    uint16_t rcvPayloadLen = 0;
    uint16_t valuesOffset = 0;
    bool hostParam = false;

    TRACE_DL_LOG_CLEAR();
    TRACE_DL_LOG_APPEND("STUHFL_F_GetMultipleParams(paramCnt = %d, params:values =", paramCnt);

    // prepare send data for all params that need to be pulled from the board.
    // All other params that are not board related are processed here directly
    for (uint32_t i = 0; i < paramCnt; i++) {

        STUHFL_T_PARAM type = params[i] & STUHFL_PARAM_TYPE_MASK;
        STUHFL_T_PARAM param = params[i] & STUHFL_PARAM_KEY_MASK;

        switch (type) {

        case STUHFL_PARAM_TYPE_CONNECTION: {
            switch (param) {
            case STUHFL_PARAM_KEY_PORT:
                *(STUHFL_T_ParamTypeConnectionPort*)&(((uint8_t *)values)[valuesOffset]) = comPort;
                valuesOffset = (uint16_t)(valuesOffset + (uint16_t)strlen(comPort));        // Warning removal
                ret = ERR_NONE;
                hostParam = true;
                TRACE_DL_LOG_APPEND(" Port:%s", comPort);
                break;
            case STUHFL_PARAM_KEY_BR:
                *(STUHFL_T_ParamTypeConnectionBR*)&(((uint8_t *)values)[valuesOffset]) = br;
                valuesOffset = (uint16_t)(valuesOffset + (uint16_t)sizeof(STUHFL_T_ParamTypeConnectionBR));     // Warning removal
                ret = ERR_NONE;
                hostParam = true;
                TRACE_DL_LOG_APPEND(" BaudRate:%d", br);
                break;

            // forward to PL
            case STUHFL_PARAM_KEY_RD_TIMEOUT_MS:
            case STUHFL_PARAM_KEY_WR_TIMEOUT_MS:
                ret = STUHFL_F_GetParam_Dispatcher(deviceCtx, type | param, &(((uint8_t *)values)[valuesOffset]));
                if (param == STUHFL_PARAM_KEY_RD_TIMEOUT_MS) {
                    TRACE_DL_LOG_APPEND(" RdTimeout:%d ", *(uint32_t*)&(((uint8_t *)values)[valuesOffset]));
                } else {
                    TRACE_DL_LOG_APPEND(" WdTimeout:%d ", *(uint32_t*)&(((uint8_t *)values)[valuesOffset]));
                }
                valuesOffset = (uint16_t)(valuesOffset + (uint16_t)sizeof(uint32_t));   // Warning removal
                ret = ERR_NONE;
                hostParam = true;
                break;

            // forward to PL
            case STUHFL_PARAM_KEY_DTR:
            case STUHFL_PARAM_KEY_RTS:
                ret = STUHFL_F_GetParam_Dispatcher(deviceCtx, type | param, &(((uint8_t *)values)[valuesOffset]));
                if (param == STUHFL_PARAM_KEY_DTR) {
                    TRACE_DL_LOG_APPEND(" DTR:%d ", ((uint8_t *)values)[valuesOffset]);
                } else {
                    TRACE_DL_LOG_APPEND(" RTS:%d ", ((uint8_t *)values)[valuesOffset]);
                }
                valuesOffset = (uint16_t)(valuesOffset + (uint16_t)sizeof(uint8_t));    // Warning removal
                ret = ERR_NONE;
                hostParam = true;
                break;

            default:
                ret = ERR_PARAM;
                break;
            }
            break;
        }

        case STUHFL_PARAM_TYPE_ST25RU3993:
            if (hostParam) {
                ret = ERR_PARAM;
                break;
            }
            ret = GetParam_SndPayload_TYPE_ST25RU3993(param, values, &valuesOffset, sndPayload, &sndPayloadLen);
            break;

        default:
            ret = ERR_PARAM;
            break;
        }

        // in case of any error return ..
        if (ret != ERR_NONE) {
            TRACE_DL_LOG_APPEND(") = %d", ret);
            TRACE_DL_LOG_FLUSH();
            return ret;
        }
    }

    // Exchange all board relevant data..
    if (!hostParam && ((STUHFL_T_POINTER2UINT)deviceCtx != (STUHFL_T_POINTER2UINT)NULL) && ((STUHFL_T_POINTER2UINT)deviceCtx != (STUHFL_T_POINTER2UINT)INVALID_HANDLE_VALUE)) {
        ret = STUHFL_F_Snd_Dispatcher(deviceCtx, (STUHFL_CG_DL << 8) | STUHFL_CC_GET_PARAM, STATUS_DEFAULT_SND, sndPayload, sndPayloadLen);
        ret |= STUHFL_F_Rcv_Dispatcher(deviceCtx, rcvPayload, &rcvPayloadLen);
        ret |= STUHFL_F_Get_RcvStatus();

        // if succeeded get data
        if (ret == ERR_NONE) {

            uint16_t rcvPayloadPos = 0;
            valuesOffset = 0;
            for (uint32_t i = 0; i < paramCnt; i++) {

                STUHFL_T_PARAM type = params[i] & STUHFL_PARAM_TYPE_MASK;
                STUHFL_T_PARAM param = params[i] & STUHFL_PARAM_KEY_MASK;

                switch (type) {
                case STUHFL_PARAM_TYPE_CONNECTION: {
                    switch (param) {
                    case STUHFL_PARAM_KEY_PORT:
                    case STUHFL_PARAM_KEY_BR:
                    case STUHFL_PARAM_KEY_RD_TIMEOUT_MS:
                    case STUHFL_PARAM_KEY_WR_TIMEOUT_MS:
                    case STUHFL_PARAM_KEY_DTR:
                    case STUHFL_PARAM_KEY_RTS:
                        // already handled..
                        break;

                    default:
                        ret = ERR_PARAM;
                        break;
                    }
                    break;
                }

                case STUHFL_PARAM_TYPE_ST25RU3993:
                    ret = GetParam_RcvPayload_TYPE_ST25RU3993(param, values, &valuesOffset, rcvPayload, (uint16_t)(rcvPayloadLen - rcvPayloadPos), &rcvPayloadPos);
                    break;

                default:
                    ret = ERR_PARAM;
                    break;
                }

                // in case of any error return ..
                if (ret != ERR_NONE) {
                    TRACE_DL_LOG_APPEND(") = %d", ret);
                    TRACE_DL_LOG_FLUSH();
                    return ret;
                }
            }
        }
    }

    TRACE_DL_LOG_APPEND(") = %d", ret);
    TRACE_DL_LOG_FLUSH();
    return ret;
}

// --------------------------------------------------------------------------
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_SendCmd(STUHFL_T_CMD cmd, STUHFL_T_CMD_SND_PARAMS sndParams)
{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    TRACE_DL_LOG_START();
    uint8_t *sndPayload = STUHFL_F_Get_SndPayloadPtr();
    uint16_t sndPayloadLen = 0;

    uint8_t cg = (uint8_t)(cmd >> 8);
    uint8_t c = (uint8_t)cmd;

    switch (cg) {
    case STUHFL_CG_DL:
        switch (c) {
        case STUHFL_CC_TUNE:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_TUNE, sizeof(STUHFL_T_ST25RU3993_Tune), (STUHFL_T_ST25RU3993_Tune *)sndParams);
            break;
        case STUHFL_CC_TUNE_CHANNEL:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_TUNE_CHANNEL, sizeof(STUHFL_T_ST25RU3993_TuneCfg), (STUHFL_T_ST25RU3993_Tune *)sndParams);
            break;

        default:
            break;
        }
        break;

    case STUHFL_CG_AL:
        switch (c) {
        case STUHFL_CC_INVENTORY_START:
#ifdef USE_INVENTORY_EXT
        case STUHFL_CC_INVENTORY_START_W_SLOT_STATISTICS:
#endif
            gIgnoreInventoryData = false;
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_INVENTORY_OPTION, sizeof(STUHFL_T_Inventory_Option), (STUHFL_T_Inventory_Option *)sndParams);
            break;

        case STUHFL_CC_INVENTORY_STOP:
            gIgnoreInventoryData = true;
            break;

        case STUHFL_CC_INVENTORY_DATA:
            break;

        default:
            break;
        }
        break;

    case STUHFL_CG_SL:
        switch (c) {
        // Gen2
        case STUHFL_CC_GEN2_INVENTORY:
            gIgnoreInventoryData = false;
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GEN2_INVENTORY_OPTION, sizeof(STUHFL_T_Inventory_Option), (STUHFL_T_Inventory_Option *)sndParams);
            break;
        case STUHFL_CC_GEN2_SELECT:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GEN2_SELECT, sizeof(STUHFL_T_Gen2_Select), (STUHFL_T_Gen2_Select *)sndParams);
            break;
        case STUHFL_CC_GEN2_READ:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GEN2_READ, sizeof(STUHFL_T_Read), (STUHFL_T_Read *)sndParams);
            break;
        case STUHFL_CC_GEN2_WRITE:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GEN2_WRITE, sizeof(STUHFL_T_Write), (STUHFL_T_Write *)sndParams);
            break;
        case STUHFL_CC_GEN2_BLOCKWRITE:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GEN2_BLOCKWRITE, sizeof(STUHFL_T_BlockWrite), (STUHFL_T_BlockWrite *)sndParams);
            break;
        case STUHFL_CC_GEN2_LOCK:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GEN2_LOCK, sizeof(STUHFL_T_Gen2_Lock), (STUHFL_T_Gen2_Lock *)sndParams);
            break;
        case STUHFL_CC_GEN2_KILL:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GEN2_KILL, sizeof(STUHFL_T_Kill), (STUHFL_T_Kill *)sndParams);
            break;
        case STUHFL_CC_GEN2_GENERIC_CMD:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GEN2_GENERIC, sizeof(STUHFL_T_Gen2_GenericCmdSnd), (STUHFL_T_Gen2_GenericCmdSnd *)sndParams);
            break;
        case STUHFL_CC_GEN2_QUERY_MEASURE_RSSI_CMD:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GEN2_QUERY_MEASURE_RSSI, sizeof(STUHFL_T_Gen2_QueryMeasureRssi), (STUHFL_T_Gen2_QueryMeasureRssi *)sndParams);
            break;

        // ISO18000-6B
        // GB29768
        case STUHFL_CC_GB29768_INVENTORY:
            gIgnoreInventoryData = false;
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GB29768_INVENTORY_OPTION, sizeof(STUHFL_T_Inventory_Option), (STUHFL_T_Inventory_Option *)sndParams);
            break;
        case STUHFL_CC_GB29768_SORT:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GB29768_SORT, sizeof(STUHFL_T_Gb29768_Sort), (STUHFL_T_Gb29768_Sort *)sndParams);
            break;
        case STUHFL_CC_GB29768_READ:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GB29768_READ, sizeof(STUHFL_T_Read), (STUHFL_T_Read *)sndParams);
            break;
        case STUHFL_CC_GB29768_WRITE:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GB29768_WRITE, sizeof(STUHFL_T_Write), (STUHFL_T_Write *)sndParams);
            break;
        case STUHFL_CC_GB29768_LOCK:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GB29768_LOCK, sizeof(STUHFL_T_Gb29768_Lock), (STUHFL_T_Gb29768_Lock *)sndParams);
            break;
        case STUHFL_CC_GB29768_KILL:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GB29768_KILL, sizeof(STUHFL_T_Kill), (STUHFL_T_Kill *)sndParams);
            break;
        case STUHFL_CC_GB29768_ERASE:
            sndPayloadLen = addTlvExt(sndPayload, STUHFL_TAG_GB29768_ERASE, sizeof(STUHFL_T_Gb29768_Erase), (STUHFL_T_Gb29768_Erase *)sndParams);
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    //
    ret = STUHFL_F_Snd_Dispatcher(deviceCtx, cmd, STATUS_DEFAULT_SND, sndPayload, sndPayloadLen);
    TRACE_DL_LOG("STUHFL_F_SendCmd(cmd = 0x%x, sndParams = 0x%x) = %d", cmd, sndParams, ret);
    return ret;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_ReceiveCmdData(STUHFL_T_CMD cmd, STUHFL_T_CMD_RCV_DATA rcvParams)
{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    TRACE_DL_LOG_START();
    uint8_t *rcvPayload = STUHFL_F_Get_RcvPayloadPtr();
    uint16_t rcvPayloadLen = 0;
    uint8_t tag;
    uint16_t len;

    uint8_t cg = (uint8_t)(cmd >> 8);
    uint8_t c = (uint8_t)cmd;
    bool waitInventoryEnd = (cmd == ((STUHFL_CG_SL << 8) | STUHFL_CC_GEN2_INVENTORY)) || (cmd == ((STUHFL_CG_SL << 8) | STUHFL_CC_GB29768_INVENTORY));

    do {
        //
        ret = STUHFL_F_Rcv_Dispatcher(deviceCtx, rcvPayload, &rcvPayloadLen);
        if ((cmd == ((STUHFL_CG_AL << 8) | STUHFL_CC_INVENTORY_DATA)) && gIgnoreInventoryData)  {
            ret =  ERR_IO;          // RUNNERSTOP is on going, no more INVENTORYDATA is expected: generate error
        } else {
            ret |= STUHFL_F_Get_RcvStatus();
        }
        if (ret != ERR_NONE) {
            // in case the received frame seams not ok we do not continue decoding
            break;
        }


        // in case we are waiting for a Gen2Inventory/Gb29768Inventory reply we get on our way a
        // possible many STUHFL_CC_INVENTORY_DATA replies.
        if(waitInventoryEnd) {
            if (STUHFL_F_Get_RcvCmd() == ((STUHFL_CG_AL << 8) | STUHFL_CC_INVENTORY_DATA)) {
                // divert cg and c because FW do not distinguish here
                // and reply always with STUHFL_CG_AL and STUHFL_CC_INVENTORY_DATA
                cg = STUHFL_CG_AL;
                c = STUHFL_CC_INVENTORY_DATA;
            } else {
                cg = (uint8_t)(cmd >> 8);
                c = (uint8_t)cmd;
            }

        } else {
            // in all other cases verify that received data matches to expected cmd
            if (STUHFL_F_Get_RcvCmd() != cmd) {
                ret = ERR_IO;
                break;
            }
        }

        switch (cg) {
        case STUHFL_CG_DL:
            switch (c) {
            case STUHFL_CC_TUNE: {
                STUHFL_T_ST25RU3993_Tune *tune = rcvParams;
                getTlvExt(rcvPayload, &tag, &len, tune);
                break;
            }
            case STUHFL_CC_TUNE_CHANNEL: {
                STUHFL_T_ST25RU3993_ChannelList *channelList = rcvParams;
                getTlvExt(rcvPayload, &tag, &len, channelList);
                break;
            }
            default:
                break;
            }
            break;
        case STUHFL_CG_AL:
            switch (c) {

            default:
            case STUHFL_CC_INVENTORY_STOP:
            case STUHFL_CC_INVENTORY_START:
#ifdef USE_INVENTORY_EXT
            case STUHFL_CC_INVENTORY_START_W_SLOT_STATISTICS:
#endif
                // no data replied here
                break;

            case STUHFL_CC_INVENTORY_DATA: {
                if (gIgnoreInventoryData) {
                    break;
                }

#ifdef USE_INVENTORY_EXT
                STUHFL_T_Inventory_Data *invData = &((STUHFL_T_Inventory_Data_Ext*)rcvParams)->invData;
                STUHFL_T_Inventory_Slot_Info_Data *slotData = &((STUHFL_T_Inventory_Data_Ext*)rcvParams)->invSlotInfoData;
#else
                STUHFL_T_Inventory_Data *invData = rcvParams;
#endif
    
                uint16_t rcvPayloadOffset = 0;
                while (rcvPayloadOffset < rcvPayloadLen) {
                    tag = getTlvTag(&rcvPayload[rcvPayloadOffset]);
                    len = 0;

                    uint16_t tagIdx = (invData->tagListSize < invData->tagListSizeMax) ? invData->tagListSize : (uint16_t)(invData->tagListSizeMax-1U);

                    switch (tag) {

                    case STUHFL_TAG_INVENTORY_STATISTICS:
                        rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, &invData->statistics));
                        break;

                    case STUHFL_TAG_INVENTORY_TAG_INFO_HEADER:
                        rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, &invData->tagList[tagIdx]));
                        break;

                    case STUHFL_TAG_INVENTORY_TAG_EPC:
                        rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, &invData->tagList[tagIdx].epc.data));
                        invData->tagList[tagIdx].epc.len = (uint8_t)len;
                        break;

                    case STUHFL_TAG_INVENTORY_TAG_TID:
                        rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, &invData->tagList[tagIdx].tid.data));
                        invData->tagList[tagIdx].tid.len = (uint8_t)len;
                        break;

                    case STUHFL_TAG_INVENTORY_TAG_XPC:
                        rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, &invData->tagList[tagIdx].xpc.data));
                        invData->tagList[tagIdx].xpc.len = (uint8_t)len;
                        break;

                    case STUHFL_TAG_INVENTORY_TAG_FINISHED:
                        rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, NULL));
                        if (invData->tagListSize < invData->tagListSizeMax) {
                            invData->tagListSize++;
                        }
                        break;

#ifdef USE_INVENTORY_EXT
                    //
                    case  STUHFL_TAG_INVENTORY_SLOT_INFO_SYNC:
                        rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, &slotData->slotSync));
                        slotData->slotInfoListSize = 0;
                        break;

                    case STUHFL_TAG_INVENTORY_SLOT_INFO:
                        rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, &slotData->slotInfoList[slotData->slotInfoListSize++]));
                        break;
#endif  // USE_INVENTORY_EXT

                    default:
                        break;
                    }
                }
                break;
              }
            }
            break;

        case STUHFL_CG_SL:
            switch (c) {
            case STUHFL_CC_GB29768_INVENTORY:   /* Pass through */
            case STUHFL_CC_GEN2_INVENTORY: {
                waitInventoryEnd = false;
                break;
            }

            case STUHFL_CC_GB29768_READ:    /* Pass through */
            case STUHFL_CC_GEN2_READ: {
                STUHFL_T_Read *readData = rcvParams;
                getTlvExt(rcvPayload, &tag, &len, readData);
                break;
            }
            case STUHFL_CC_GB29768_WRITE:    /* Pass through */
            case STUHFL_CC_GEN2_WRITE: {
                STUHFL_T_Write *writeData = rcvParams;
                getTlvExt(rcvPayload, &tag, &len, writeData);
                break;
            }
            case STUHFL_CC_GEN2_BLOCKWRITE: {
                STUHFL_T_BlockWrite *blockWrite = rcvParams;
                getTlvExt(rcvPayload, &tag, &len, blockWrite);
                break;
            }
            case STUHFL_CC_GEN2_GENERIC_CMD: {
                STUHFL_T_Gen2_GenericCmdRcv *rcvData = rcvParams;
                getTlvExt(rcvPayload, &tag, &len, rcvData);
                break;
            }
            case STUHFL_CC_GEN2_QUERY_MEASURE_RSSI_CMD: {
                STUHFL_T_Gen2_QueryMeasureRssi *rssiData = rcvParams;
                getTlvExt(rcvPayload, &tag, &len, rssiData);
                break;
            }

            case STUHFL_CC_GB29768_SORT:
            case STUHFL_CC_GEN2_SELECT:
            case STUHFL_CC_GB29768_LOCK:
            case STUHFL_CC_GEN2_LOCK:
            case STUHFL_CC_GB29768_KILL:
            case STUHFL_CC_GEN2_KILL:
            case STUHFL_CC_GB29768_ERASE:
            default:
                break;
            }
            break;

        default:
            break;
        }

    } while (waitInventoryEnd);

    TRACE_DL_LOG("STUHFL_F_ReceiveCmd(cmd = 0x%x, rcvParams = 0x%x) = %d", cmd, rcvParams, ret);
    return ret;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_ExecuteCmd(STUHFL_T_CMD cmd, STUHFL_T_CMD_SND_PARAMS sndParams, STUHFL_T_CMD_RCV_DATA rcvParams)
{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    if (ERR_NONE == (ret = STUHFL_F_SendCmd(cmd, sndParams))) {
        ret = STUHFL_F_ReceiveCmdData(cmd, rcvParams);
    }
    return ret;
}

// --------------------------------------------------------------------------
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetVersionOld(uint8_t *swVersion)
{

    STUHFL_T_RET_CODE ret = ERR_PARAM;
    TRACE_DL_LOG_START();
    uint8_t *sndPayload = STUHFL_F_Get_SndPayloadPtr();
    uint16_t sndPayloadLen = 0;
    uint8_t *rcvPayload = STUHFL_F_Get_RcvPayloadPtr();
    uint16_t rcvPayloadLen = 12;    // # of bytes we expect as a correct "old" GetVersion reply..

    const uint8_t oldGetVersion[] = { 0x02,0x00,0x00,0x05,0x67,0x00,0x00,0x00,0x03 };
    sndPayloadLen = sizeof(oldGetVersion);
    memcpy(sndPayload, oldGetVersion, sndPayloadLen = sizeof(oldGetVersion));

    ret = STUHFL_F_SndRaw_Dispatcher(deviceCtx, sndPayload, sndPayloadLen);
    ret |= STUHFL_F_RcvRaw_Dispatcher(deviceCtx, rcvPayload, &rcvPayloadLen);

    if (ERR_NONE == ret) {
        if ((rcvPayload[4] == 0x67) && (rcvPayload[5] == 0x00) && (rcvPayload[6] == 0x00) && (rcvPayload[7] == 0x00) && (rcvPayload[8] == 0x03)) {
            swVersion[0] = rcvPayload[9];
            swVersion[1] = rcvPayload[10];
            swVersion[2] = rcvPayload[11];
        } else {
            ret = ERR_PROTO;
        }
    }
    TRACE_DL_LOG("STUHFL_F_GetVersionOld(swVersion = %d.%d.%d) = %d", swVersion[0], swVersion[1], swVersion[2], ret);
    return ret;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetVersion(uint8_t *swVersionMajor, uint8_t *swVersionMinor, uint8_t *swVersionMicro, uint8_t *swVersionBuild,
        uint8_t *hwVersionMajor, uint8_t *hwVersionMinor, uint8_t *hwVersionMicro, uint8_t *hwVersionBuild)

{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    TRACE_DL_LOG_START();
    uint8_t *sndPayload = STUHFL_F_Get_SndPayloadPtr();
    uint16_t sndPayloadLen = 0;
    uint8_t *rcvPayload = STUHFL_F_Get_RcvPayloadPtr();
    uint16_t rcvPayloadLen = 0;
    uint8_t swVersion[256];
    uint8_t hwVersion[256];

    ret = STUHFL_F_Snd_Dispatcher(deviceCtx, (STUHFL_CG_GENERIC << 8) | STUHFL_CC_GET_VERSION, STATUS_DEFAULT_SND, sndPayload, sndPayloadLen);
    ret |= STUHFL_F_Rcv_Dispatcher(deviceCtx, rcvPayload, &rcvPayloadLen);

    if (ERR_NONE == ret) {
        uint8_t tag;
        uint16_t len;
        uint16_t rcvPayloadOffset = 0;

        if (rcvPayloadLen > 3) {
            rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, &swVersion));
            if (len == 4) {
                *swVersionMajor = swVersion[0];
                *swVersionMinor = swVersion[1];
                *swVersionMicro = swVersion[2];
                *swVersionBuild = swVersion[3];
            }

            if (rcvPayloadLen > rcvPayloadOffset) {
                rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, &hwVersion));
                if (len == 4) {
                    *hwVersionMajor = hwVersion[0];
                    *hwVersionMinor = hwVersion[1];
                    *hwVersionMicro = hwVersion[2];
                    *hwVersionBuild = hwVersion[3];
                }
            }
        }
    }
    ret |= STUHFL_F_Get_RcvStatus();
    TRACE_DL_LOG("STUHFL_F_GetVersion(swVersion = %d.%d.%d.%d, hwVersion = %d.%d.%d.%d) = %d", swVersion[0], swVersion[1], swVersion[2], swVersion[3], hwVersion[0], hwVersion[1], hwVersion[2], hwVersion[3], ret);
    return ret;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetInfo(char *szSwInfo, char *szHwInfo)
{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    TRACE_DL_LOG_START();
    uint8_t *sndPayload = STUHFL_F_Get_SndPayloadPtr();
    uint16_t sndPayloadLen = 0;
    uint8_t *rcvPayload = STUHFL_F_Get_RcvPayloadPtr();
    uint16_t rcvPayloadLen = 0;

    ret = STUHFL_F_Snd_Dispatcher(deviceCtx, (STUHFL_CG_GENERIC << 8) | STUHFL_CC_GET_INFO, STATUS_DEFAULT_SND, sndPayload, sndPayloadLen);
    ret |= STUHFL_F_Rcv_Dispatcher(deviceCtx, rcvPayload, &rcvPayloadLen);

    if (ERR_NONE == ret) {
        uint8_t tag;
        uint16_t len;
        uint16_t rcvPayloadOffset = 0;

        if (rcvPayloadLen > 3) {
            rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, szSwInfo));

            if (rcvPayloadLen > rcvPayloadOffset) {
                rcvPayloadOffset = (uint16_t)(rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[rcvPayloadOffset], &tag, &len, szHwInfo));
            }
        }
    }
    ret |= STUHFL_F_Get_RcvStatus();
    TRACE_DL_LOG("STUHFL_F_GetInfo(swInfo = %s, hwInfo = %s) = %d", szSwInfo, szHwInfo, ret);
    return ret;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Upgrade(uint8_t *fwData, uint32_t fwdDataLen)
{
    TRACE_DL_LOG_START();
    TRACE_DL_LOG("STUHFL_F_Upgrade(fwData = 0x%x, fwdDataLen = %d) .. NYI", fwData, fwdDataLen);
    return ERR_GENERIC;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_EnterBootloader()
{
    uint8_t *sndPayload = STUHFL_F_Get_SndPayloadPtr();
    uint16_t sndPayloadLen = 0;
    TRACE_DL_LOG_START();
    STUHFL_T_RET_CODE ret = STUHFL_F_Snd_Dispatcher(deviceCtx, (STUHFL_CG_GENERIC << 8) | STUHFL_CC_ENTER_BOOTLOADER, STATUS_DEFAULT_SND, sndPayload, sndPayloadLen);
    TRACE_DL_LOG("STUHFL_F_EnterBootloader() = %d", ret);
    return ret;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Reboot()
{
    uint8_t *sndPayload = STUHFL_F_Get_SndPayloadPtr();
    uint16_t sndPayloadLen = 0;
    TRACE_DL_LOG_START();
    STUHFL_T_RET_CODE ret = STUHFL_F_Snd_Dispatcher(deviceCtx, (STUHFL_CG_GENERIC << 8) | STUHFL_CC_REBOOT, STATUS_DEFAULT_SND, sndPayload, sndPayloadLen);
    TRACE_DL_LOG("STUHFL_F_Reboot() = %d", ret);
    return ret;
}



// --------------------------------------------------------------------------
STUHFL_T_RET_CODE SetParam_SndPayload_TYPE_ST25RU3993(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE *values, uint16_t *valuesOffset, uint8_t *sndPayload, uint16_t *sndPayloadOffset)
{
    switch (param) {

    case STUHFL_PARAM_KEY_RWD_REGISTER: {
        STUHFL_T_ST25RU3993_Register *reg = (STUHFL_T_ST25RU3993_Register*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Register));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_REGISTER, sizeof(STUHFL_T_ST25RU3993_Register), reg));
        TRACE_DL_LOG_APPEND(" Register(addr: 0x%02x, data: 0x%02x)", reg->addr, reg->data);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_CONFIG: {
        STUHFL_T_ST25RU3993_RwdConfig *cfg = (STUHFL_T_ST25RU3993_RwdConfig*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_RwdConfig));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_RWD_CONFIG, sizeof(STUHFL_T_ST25RU3993_RwdConfig), cfg));
        TRACE_DL_LOG_APPEND(" RwdCfg(id: 0x%02x, value: 0x%02x)", cfg->id, cfg->value);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_ANTENNA_POWER: {
        STUHFL_T_ST25RU3993_Antenna_Power *antPwr = (STUHFL_T_ST25RU3993_Antenna_Power*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Antenna_Power));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_ANTENNA_POWER, sizeof(STUHFL_T_ST25RU3993_Antenna_Power), antPwr));
        TRACE_DL_LOG_APPEND(" AntennaPower(mode: 0x%02x timeout: %d, frequency: %d)", antPwr->mode, antPwr->timeout, antPwr->frequency);
        break;
    }

    //case STUHFL_PARAM_KEY_RWD_FREQ_RSSI:
    //case STUHFL_PARAM_KEY_RWD_FREQ_REFLECTED:

    case STUHFL_PARAM_KEY_RWD_CHANNEL_LIST: {
        STUHFL_T_ST25RU3993_ChannelList *channelList = (STUHFL_T_ST25RU3993_ChannelList*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_ChannelList));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_CHANNEL_LIST, sizeof(STUHFL_T_ST25RU3993_ChannelList), channelList));
        TRACE_DL_LOG_APPEND(" ChannelList(antenna: %d, nFrequencies: %d, currentChannelListIdx: %d, persistent: %d, idx:{Freq, (cin,clen,cout)} = ", channelList->antenna, channelList->nFrequencies, channelList->currentChannelListIdx, channelList->persistent);
        for (int i = 0; i < channelList->nFrequencies; i++) {
            TRACE_DL_LOG_APPEND("%d:{%d, (%d,%d,%d)}, ", i, channelList->item[i].frequency, channelList->item[i].caps.cin, channelList->item[i].caps.clen, channelList->item[i].caps.cout);
        }
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_PROFILE: {
        STUHFL_T_ST25RU3993_Freq_Profile *freqProfile = (STUHFL_T_ST25RU3993_Freq_Profile*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_Profile));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_FREQ_PROFILE, sizeof(STUHFL_T_ST25RU3993_Freq_Profile), freqProfile));
        TRACE_DL_LOG_APPEND(" FreqProfile(profile: %d)", freqProfile->profile);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_PROFILE_ADD2CUSTOM: {
        STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom *freqProfileAdd2Custom = (STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_FREQ_PROFILE_ADD2CUSTOM, sizeof(STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom), freqProfileAdd2Custom));
        TRACE_DL_LOG_APPEND(" ProfileAdd2Custom(clearList: %d, frequency: %d)", freqProfileAdd2Custom->clearList, freqProfileAdd2Custom->frequency);
        break;
    }

    //case STUHFL_PARAM_KEY_RWD_FREQ_PROFILE_INFO:

    case STUHFL_PARAM_KEY_RWD_FREQ_HOP: {
        STUHFL_T_ST25RU3993_Freq_Hop *freqHop = (STUHFL_T_ST25RU3993_Freq_Hop*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_Hop));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_FREQ_HOP, sizeof(STUHFL_T_ST25RU3993_Freq_Hop), freqHop));
        TRACE_DL_LOG_APPEND(" FreqHop(maxSendingTime: %d)", freqHop->maxSendingTime);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_LBT: {
        STUHFL_T_ST25RU3993_Freq_LBT *freqLBT = (STUHFL_T_ST25RU3993_Freq_LBT*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_LBT));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_FREQ_LBT, sizeof(STUHFL_T_ST25RU3993_Freq_LBT), freqLBT));
        TRACE_DL_LOG_APPEND(" FreqLBT(listeningTime: %d, idleTime: %d, rssiLogThreshold: %d, skipLBTcheck: %d)", freqLBT->listeningTime, freqLBT->idleTime, freqLBT->rssiLogThreshold, freqLBT->skipLBTcheck);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_CONT_MOD: {
        STUHFL_T_ST25RU3993_Freq_ContMod *freqContMod = (STUHFL_T_ST25RU3993_Freq_ContMod*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_ContMod));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_FREQ_CONT_MOD, sizeof(STUHFL_T_ST25RU3993_Freq_ContMod), freqContMod));
        TRACE_DL_LOG_APPEND(" FreqContMod(frequency: %d, enableContMod : %d, maxSendingTime : %d, modulationMode : %d)", freqContMod->frequency, freqContMod->enableContMod, freqContMod->maxSendingTime, freqContMod->modulationMode);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_GEN2TIMINGS: {
        STUHFL_T_Gen2_Timings *gen2Timings = (STUHFL_T_Gen2_Timings*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_Gen2_Timings));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_GEN2TIMINGS, sizeof(STUHFL_T_Gen2_Timings), gen2Timings));
        TRACE_DL_LOG_APPEND(" SetGen2Timings(T4Min: %d)", gen2Timings->T4Min);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_GEN2PROTOCOL_CFG: {
        STUHFL_T_ST25RU3993_Gen2Protocol_Cfg *gen2ProtocolCfg = (STUHFL_T_ST25RU3993_Gen2Protocol_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_GEN2PROTOCOL_CFG, sizeof(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg), gen2ProtocolCfg));
        TRACE_DL_LOG_APPEND(" Gen2ProtocolCfg(tari: %d, blf: %d, coding: %d, trext: %d)",
                            gen2ProtocolCfg->tari, gen2ProtocolCfg->blf, gen2ProtocolCfg->coding, gen2ProtocolCfg->trext);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_GB29768PROTOCOL_CFG: {
        STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg *gb29768ProtocolCfg = (STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_GB29768PROTOCOL_CFG, sizeof(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg), gb29768ProtocolCfg));
        TRACE_DL_LOG_APPEND(" Gb29768ProtocolCfg(tc: %d, blf: %d, coding: %d, trext: %d)", gb29768ProtocolCfg->tc, gb29768ProtocolCfg->blf, gb29768ProtocolCfg->coding, gb29768ProtocolCfg->trext);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_TXRX_CFG: {
        STUHFL_T_ST25RU3993_TxRx_Cfg *txRxCfg = (STUHFL_T_ST25RU3993_TxRx_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TxRx_Cfg));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TXRX_CFG, sizeof(STUHFL_T_ST25RU3993_TxRx_Cfg), txRxCfg));
        TRACE_DL_LOG_APPEND(" TxRxCfg(txOutputLevel: %d, rxSensitivity: %d, usedAntenna: %d, alternateAntennaInterval: %d)", txRxCfg->txOutputLevel, txRxCfg->rxSensitivity, txRxCfg->usedAntenna, txRxCfg->alternateAntennaInterval);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_PA_CFG: {
        STUHFL_T_ST25RU3993_PA_Cfg *paCfg = (STUHFL_T_ST25RU3993_PA_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_PA_Cfg));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_PA_CFG, sizeof(STUHFL_T_ST25RU3993_PA_Cfg), paCfg));
        TRACE_DL_LOG_APPEND(" PA_Cfg(useExternal: %d)", paCfg->useExternal);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_GEN2INVENTORY_CFG: {
        STUHFL_T_ST25RU3993_Gen2Inventory_Cfg *invGen2Cfg = (STUHFL_T_ST25RU3993_Gen2Inventory_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_GEN2INVENTORY_CFG, sizeof(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg), invGen2Cfg));
        TRACE_DL_LOG_APPEND(" Gen2InventoryCfg(fastInv: %d, autoAck: %d, readTID: %d, startQ: %d, adaptiveQEnable: %d, minQ: %d, maxQ: %d, adjustOptions: %d, C1: (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d), C2: (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d), autoTuningInterval: %d, autoTuningLevel: %d, autoTuningAlgo: %d, autoTuningFalsePositiveDetection: %d, sel: %d, session: %d, target: %d, toggleTarget: %d, targetDepletionMode: %d, adaptiveSensitivityEnable: %d, adaptiveSensitivityInterval: %d)",
                            invGen2Cfg->fastInv, invGen2Cfg->autoAck, invGen2Cfg->readTID,
                            invGen2Cfg->startQ, invGen2Cfg->adaptiveQEnable, invGen2Cfg->minQ, invGen2Cfg->maxQ, invGen2Cfg->adjustOptions,
                            invGen2Cfg->C1[0], invGen2Cfg->C1[1], invGen2Cfg->C1[2], invGen2Cfg->C1[3], invGen2Cfg->C1[4], invGen2Cfg->C1[5], invGen2Cfg->C1[6], invGen2Cfg->C1[7],
                            invGen2Cfg->C1[8], invGen2Cfg->C1[9], invGen2Cfg->C1[10], invGen2Cfg->C1[11], invGen2Cfg->C1[12], invGen2Cfg->C1[13], invGen2Cfg->C1[14], invGen2Cfg->C1[15],
                            invGen2Cfg->C2[0], invGen2Cfg->C2[1], invGen2Cfg->C2[2], invGen2Cfg->C2[3], invGen2Cfg->C2[4], invGen2Cfg->C2[5], invGen2Cfg->C2[6], invGen2Cfg->C2[7],
                            invGen2Cfg->C2[8], invGen2Cfg->C2[9], invGen2Cfg->C2[10], invGen2Cfg->C2[11], invGen2Cfg->C2[12], invGen2Cfg->C2[13], invGen2Cfg->C2[14], invGen2Cfg->C2[15],
                            invGen2Cfg->autoTuningInterval, invGen2Cfg->autoTuningLevel, invGen2Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD, (invGen2Cfg->autoTuningAlgo & TUNING_ALGO_ENABLE_FPD) ? true : false,
                            invGen2Cfg->sel, invGen2Cfg->session, invGen2Cfg->target, invGen2Cfg->toggleTarget, invGen2Cfg->targetDepletionMode,
                            invGen2Cfg->adaptiveSensitivityEnable, invGen2Cfg->adaptiveSensitivityInterval);
        break;
    }

    case STUHFL_PARAM_KEY_RWD_GB29768INVENTORY_CFG: {
        STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg *invGb29768Cfg = (STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_GB29768INVENTORY_CFG, sizeof(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg), invGb29768Cfg));
        TRACE_DL_LOG_APPEND(" Gb29768InventoryCfg(readTID: %d, autoTuningInterval: %d, autoTuningLevel: %d, autoTuningAlgo: %d, autoTuningFalsePositiveDetection: %d, adaptiveSensitivityEnable: %d, adaptiveSensitivityInterval: %d, condition: %d, session: %d, target: %d, toggleTarget: %d, targetDepletionMode: %d, endThreshold:%d, ccnThreshold:%d, cinThreshold:%d)",
                            invGb29768Cfg->readTID,
                            invGb29768Cfg->autoTuningInterval, invGb29768Cfg->autoTuningLevel, invGb29768Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD, (invGb29768Cfg->autoTuningAlgo & TUNING_ALGO_ENABLE_FPD) ? true : false,
                            invGb29768Cfg->adaptiveSensitivityEnable, invGb29768Cfg->adaptiveSensitivityInterval,
                            invGb29768Cfg->condition, invGb29768Cfg->session, invGb29768Cfg->target, invGb29768Cfg->toggleTarget, invGb29768Cfg->targetDepletionMode,
                            invGb29768Cfg->endThreshold, invGb29768Cfg->ccnThreshold, invGb29768Cfg->cinThreshold);
        break;
    }

    case STUHFL_PARAM_KEY_TUNING_CAPS: {
        STUHFL_T_ST25RU3993_TuningCaps *tuning = (STUHFL_T_ST25RU3993_TuningCaps*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TuningCaps));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TUNING_CAPS, sizeof(STUHFL_T_ST25RU3993_TuningCaps), tuning));
        TRACE_DL_LOG_APPEND(" TuningCaps(antenna: %d, channelListIdx: %d, cin: %d, clen: %d, cout: %d)", tuning->antenna, tuning->channelListIdx, tuning->caps.cin, tuning->caps.clen, tuning->caps.cout);
        break;
    }
    case STUHFL_PARAM_KEY_TUNING: {
        STUHFL_T_ST25RU3993_Tuning *tuning = (STUHFL_T_ST25RU3993_Tuning*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Tuning));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TUNING, sizeof(STUHFL_T_ST25RU3993_Tuning), tuning));
        TRACE_DL_LOG_APPEND(" Tuning(antenna: %d, cin: %d, clen: %d, cout: %d)", tuning->antenna, tuning->cin, tuning->clen, tuning->cout);
        break;
    }
    case STUHFL_PARAM_KEY_TUNING_TABLE_ENTRY: {
#define TB_SIZE    128
        char tb[4][TB_SIZE];
        STUHFL_T_ST25RU3993_TuningTableEntry *tuningTableEntry = (STUHFL_T_ST25RU3993_TuningTableEntry*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TuningTableEntry));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TUNING_TABLE_ENTRY, sizeof(STUHFL_T_ST25RU3993_TuningTableEntry), tuningTableEntry));
        TRACE_DL_LOG_APPEND(" TuningTableEntry(entry: %d, freq: %d, cin: 0x%s, clen: 0x%s, cout: 0x%s, IQ: 0x%s)",
                            tuningTableEntry->entry, tuningTableEntry->freq, byteArray2HexString(tb[0], TB_SIZE, tuningTableEntry->cin, MAX_ANTENNA), byteArray2HexString(tb[1], TB_SIZE, tuningTableEntry->clen, MAX_ANTENNA), byteArray2HexString(tb[2], TB_SIZE, tuningTableEntry->cout, MAX_ANTENNA), byteArray2HexString(tb[3], TB_SIZE, (uint8_t *)tuningTableEntry->IQ, MAX_ANTENNA * sizeof(uint16_t)));
#undef TB_SIZE
        break;
    }
    case STUHFL_PARAM_KEY_TUNING_TABLE_DEFAULT: {
        STUHFL_T_ST25RU3993_TunerTableSet *tuningTableSet = (STUHFL_T_ST25RU3993_TunerTableSet*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TunerTableSet));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TUNING_TABLE_DEFAULT, sizeof(STUHFL_T_ST25RU3993_TunerTableSet), tuningTableSet));
        TRACE_DL_LOG_APPEND(" TuningTableDefault(profile: %d, freq: %d)", tuningTableSet->profile, tuningTableSet->freq);
        break;
    }
    case STUHFL_PARAM_KEY_TUNING_TABLE_SAVE:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TUNING_TABLE_SAVE, 0, NULL));
        TRACE_DL_LOG_APPEND(" TuningTableSave2Flash()");
        break;
    case STUHFL_PARAM_KEY_TUNING_TABLE_EMPTY:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TUNING_TABLE_EMPTY, 0, NULL));
        TRACE_DL_LOG_APPEND(" TuningTableClear()");
        break;

    default:
        return ERR_PARAM;
    }

    //
    return ERR_NONE;
}
STUHFL_T_RET_CODE SetParam_RcvPayload_TYPE_ST25RU3993(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE *values, uint16_t *valuesOffset, uint8_t *rcvPayload, uint16_t *rcvPayloadOffset)
{
    switch (param) {

    case STUHFL_PARAM_KEY_RWD_REGISTER:
    case STUHFL_PARAM_KEY_RWD_CONFIG:
    case STUHFL_PARAM_KEY_RWD_ANTENNA_POWER:
    //case STUHFL_PARAM_KEY_RWD_FREQ_RSSI:
    //case STUHFL_PARAM_KEY_RWD_FREQ_REFLECTED:
    case STUHFL_PARAM_KEY_RWD_CHANNEL_LIST:
    case STUHFL_PARAM_KEY_RWD_FREQ_PROFILE:
    case STUHFL_PARAM_KEY_RWD_FREQ_PROFILE_ADD2CUSTOM:
    //case STUHFL_PARAM_KEY_RWD_FREQ_PROFILE_INFO:
    case STUHFL_PARAM_KEY_RWD_FREQ_HOP:
    case STUHFL_PARAM_KEY_RWD_FREQ_LBT:
    case STUHFL_PARAM_KEY_RWD_FREQ_CONT_MOD:
    case STUHFL_PARAM_KEY_RWD_GEN2PROTOCOL_CFG:
    case STUHFL_PARAM_KEY_RWD_GB29768PROTOCOL_CFG:
    case STUHFL_PARAM_KEY_RWD_TXRX_CFG:
    case STUHFL_PARAM_KEY_RWD_PA_CFG:
    case STUHFL_PARAM_KEY_RWD_GEN2TIMINGS:
    case STUHFL_PARAM_KEY_RWD_GEN2INVENTORY_CFG:
    case STUHFL_PARAM_KEY_RWD_GB29768INVENTORY_CFG:
    case STUHFL_PARAM_KEY_TUNING_CAPS:
    case STUHFL_PARAM_KEY_TUNING:
    case STUHFL_PARAM_KEY_TUNING_TABLE_ENTRY:
    case STUHFL_PARAM_KEY_TUNING_TABLE_DEFAULT:
    case STUHFL_PARAM_KEY_TUNING_TABLE_SAVE:
    case STUHFL_PARAM_KEY_TUNING_TABLE_EMPTY:
        break;
    default:
        return ERR_PARAM;
    }

    //
    return ERR_NONE;
}

STUHFL_T_RET_CODE GetParam_SndPayload_TYPE_ST25RU3993(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE *values, uint16_t *valuesOffset, uint8_t *sndPayload, uint16_t *sndPayloadOffset)
{
    switch (param) {

    case STUHFL_PARAM_KEY_RWD_REGISTER: {
        STUHFL_T_ST25RU3993_Register *reg = (STUHFL_T_ST25RU3993_Register*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Register));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlv8(&sndPayload[*sndPayloadOffset], STUHFL_TAG_REGISTER, reg->addr));
        break;
    }

    case STUHFL_PARAM_KEY_RWD_CONFIG: {
        STUHFL_T_ST25RU3993_RwdConfig *cfg = (STUHFL_T_ST25RU3993_RwdConfig*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_RwdConfig));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlv8(&sndPayload[*sndPayloadOffset], STUHFL_TAG_RWD_CONFIG, cfg->id));
        break;
    }

    case STUHFL_PARAM_KEY_RWD_ANTENNA_POWER:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_ANTENNA_POWER, 0, NULL));
        break;

    case STUHFL_PARAM_KEY_RWD_FREQ_RSSI: {
        STUHFL_T_ST25RU3993_Freq_Rssi *freqRssi = (STUHFL_T_ST25RU3993_Freq_Rssi*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_Rssi));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlv32(&sndPayload[*sndPayloadOffset], STUHFL_TAG_FREQ_RSSI, freqRssi->frequency));
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_REFLECTED: {
        STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info *freqRefl = (STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_FREQ_REFLECTED, sizeof(freqRefl->frequency) + sizeof(freqRefl->applyTunerSetting), freqRefl));
        break;
    }

    case STUHFL_PARAM_KEY_RWD_CHANNEL_LIST: {
        STUHFL_T_ST25RU3993_ChannelList *channelList = (STUHFL_T_ST25RU3993_ChannelList*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_ChannelList));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_CHANNEL_LIST, sizeof(channelList->antenna) + sizeof(channelList->persistent), channelList));
        break;
    }

    //case STUHFL_PARAM_KEY_RWD_FREQ_ADD_2_PROFILE:

    case STUHFL_PARAM_KEY_RWD_FREQ_PROFILE_INFO: {
        STUHFL_T_ST25RU3993_Freq_Profile_Info *freqProfInfo = (STUHFL_T_ST25RU3993_Freq_Profile_Info*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_Profile_Info));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_FREQ_PROFILE_INFO, sizeof(freqProfInfo->profile), freqProfInfo));
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_HOP:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_FREQ_HOP, 0, NULL));
        break;
    case STUHFL_PARAM_KEY_RWD_FREQ_LBT:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_FREQ_LBT, 0, NULL));
        break;

    //case STUHFL_PARAM_KEY_RWD_FREQ_CONT_MOD:

    case STUHFL_PARAM_KEY_RWD_GEN2TIMINGS:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_GEN2TIMINGS, 0, NULL));
        break;
    case STUHFL_PARAM_KEY_RWD_GEN2PROTOCOL_CFG:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_GEN2PROTOCOL_CFG, 0, NULL));
        break;
    case STUHFL_PARAM_KEY_RWD_GB29768PROTOCOL_CFG:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_GB29768PROTOCOL_CFG, 0, NULL));
        break;
    case STUHFL_PARAM_KEY_RWD_TXRX_CFG:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TXRX_CFG, 0, NULL));
        break;
    case STUHFL_PARAM_KEY_RWD_PA_CFG:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_PA_CFG, 0, NULL));
        break;
    case STUHFL_PARAM_KEY_RWD_GEN2INVENTORY_CFG:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_GEN2INVENTORY_CFG, 0, NULL));
        break;
    case STUHFL_PARAM_KEY_RWD_GB29768INVENTORY_CFG:
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_GB29768INVENTORY_CFG, 0, NULL));
        break;

    case STUHFL_PARAM_KEY_TUNING_CAPS: {
        STUHFL_T_ST25RU3993_TuningCaps *tuning = (STUHFL_T_ST25RU3993_TuningCaps*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TuningCaps));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TUNING_CAPS, sizeof(tuning->antenna) + sizeof(tuning->channelListIdx), tuning));
        break;
    }
    case STUHFL_PARAM_KEY_TUNING: {
        STUHFL_T_ST25RU3993_Tuning *tuning = (STUHFL_T_ST25RU3993_Tuning*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Tuning));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TUNING, sizeof(tuning->antenna), tuning));
        break;
    }
    case STUHFL_PARAM_KEY_TUNING_TABLE_ENTRY: {
        STUHFL_T_ST25RU3993_TuningTableEntry *tuningTableEntry = (STUHFL_T_ST25RU3993_TuningTableEntry*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TuningTableEntry));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TUNING_TABLE_ENTRY, sizeof(tuningTableEntry->entry), tuningTableEntry));
        break;
    }
    case STUHFL_PARAM_KEY_TUNING_TABLE_INFO: {
        STUHFL_T_ST25RU3993_TuningTableInfo *tuningTableInfo = (STUHFL_T_ST25RU3993_TuningTableInfo*)&(((uint8_t *)values)[*valuesOffset]);
        *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TuningTableInfo));
        *sndPayloadOffset = (uint16_t)(*sndPayloadOffset + (uint16_t)addTlvExt(&sndPayload[*sndPayloadOffset], STUHFL_TAG_TUNING_TABLE_INFO, sizeof(tuningTableInfo->profile), tuningTableInfo));
        break;
    }
    default:
        return ERR_PARAM;
    }

    //
    return ERR_NONE;
}
STUHFL_T_RET_CODE GetParam_RcvPayload_TYPE_ST25RU3993(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE *values, uint16_t *valuesOffset, uint8_t *rcvPayload, uint16_t rcvPayloadAvailable, uint16_t *rcvPayloadOffset)
{
    uint8_t tag;
    uint16_t len;

    switch (param) {
    case STUHFL_PARAM_KEY_RWD_REGISTER: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Register)) {
            STUHFL_T_ST25RU3993_Register *reg = (STUHFL_T_ST25RU3993_Register*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Register));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, reg));
            TRACE_DL_LOG_APPEND(" Register(addr: 0x%02x, data: 0x%02x)", reg->addr, reg->data);
        }
        break;
    }

    case STUHFL_PARAM_KEY_RWD_CONFIG: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_RwdConfig)) {
            STUHFL_T_ST25RU3993_RwdConfig *cfg = (STUHFL_T_ST25RU3993_RwdConfig*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_RwdConfig));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, cfg));
            TRACE_DL_LOG_APPEND(" RwdCfg(id: 0x%02x, value: 0x%02x)", cfg->id, cfg->value);
        }
        break;
    }

    case STUHFL_PARAM_KEY_RWD_ANTENNA_POWER: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Antenna_Power)) {
            STUHFL_T_ST25RU3993_Antenna_Power *antPwr = (STUHFL_T_ST25RU3993_Antenna_Power *)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Antenna_Power));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, antPwr));
            TRACE_DL_LOG_APPEND(" AntennaPower(mode: 0x%02x timeout: %d, frequency: %d)", antPwr->mode, antPwr->timeout, antPwr->frequency);
        }
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_RSSI: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Freq_Rssi)) {
            STUHFL_T_ST25RU3993_Freq_Rssi *freqRssi = (STUHFL_T_ST25RU3993_Freq_Rssi*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_Rssi));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, freqRssi));
            TRACE_DL_LOG_APPEND(" FreqRSSI(frequency: %d, rssiLogI: %d, rssiLogQ: %d)", freqRssi->frequency, freqRssi->rssiLogI, freqRssi->rssiLogQ);
        }
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_REFLECTED: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info)) {
            STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info *freqReflectedPower = (STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, freqReflectedPower));
            TRACE_DL_LOG_APPEND(" FreqReflectedPower(frequency: %d, applyTunerSetting: %d, reflectedI: %d, reflectedQ: %d)", freqReflectedPower->frequency, freqReflectedPower->applyTunerSetting, freqReflectedPower->reflectedI, freqReflectedPower->reflectedQ);
        }
        break;
    }

    //case STUHFL_PARAM_KEY_RWD_FREQ_ADD_2_PROFILE:

    case STUHFL_PARAM_KEY_RWD_CHANNEL_LIST: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_ChannelList)) {
            STUHFL_T_ST25RU3993_ChannelList *channelList = (STUHFL_T_ST25RU3993_ChannelList*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_ChannelList));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, channelList));
            TRACE_DL_LOG_APPEND(" ChannelList(antenna: %d, nFrequencies: %d, currentChannelListIdx: %d, persistent: %d, idx:{Freq, (cin,clen,cout)} = ", channelList->antenna, channelList->nFrequencies, channelList->currentChannelListIdx, channelList->persistent);
            for (int i = 0; i < channelList->nFrequencies; i++) {
                TRACE_DL_LOG_APPEND("%d:{%d, (%d,%d,%d)}, ", i, channelList->item[i].frequency, channelList->item[i].caps.cin, channelList->item[i].caps.clen, channelList->item[i].caps.cout);
            }
        }
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_PROFILE_INFO: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Freq_Profile_Info)) {
            STUHFL_T_ST25RU3993_Freq_Profile_Info *freqProfileInfo = (STUHFL_T_ST25RU3993_Freq_Profile_Info*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_Profile_Info));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, freqProfileInfo));
            TRACE_DL_LOG_APPEND(" FreqProfileInfo(profile: %d, minFreq : %d, maxFreq: %d, numFrequencies : %d)", freqProfileInfo->profile, freqProfileInfo->minFrequency, freqProfileInfo->maxFrequency, freqProfileInfo->numFrequencies);
        }
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_HOP: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Freq_Hop)) {
            STUHFL_T_ST25RU3993_Freq_Hop *freqHop = (STUHFL_T_ST25RU3993_Freq_Hop*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_Hop));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, freqHop));
            TRACE_DL_LOG_APPEND(" FreqHop(maxSendingTime: %d)", freqHop->maxSendingTime);
        }
        break;
    }

    case STUHFL_PARAM_KEY_RWD_FREQ_LBT: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Freq_LBT)) {
            STUHFL_T_ST25RU3993_Freq_LBT *freqLBT = (STUHFL_T_ST25RU3993_Freq_LBT*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Freq_LBT));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, freqLBT));
            TRACE_DL_LOG_APPEND(" FreqLBT(listeningTime: %d, idleTime: %d, rssiLogThreshold: %d, skipLBTcheck: %d)", freqLBT->listeningTime, freqLBT->idleTime, freqLBT->rssiLogThreshold, freqLBT->skipLBTcheck);
        }
        break;
    }

    //case STUHFL_PARAM_KEY_RWD_FREQ_CONT_MOD:

    case STUHFL_PARAM_KEY_RWD_GEN2TIMINGS: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_Gen2_Timings)) {
            STUHFL_T_Gen2_Timings *gen2Timings = (STUHFL_T_Gen2_Timings*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_Gen2_Timings));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, gen2Timings));
            TRACE_DL_LOG_APPEND(" GetGen2Timings(T4Min: %d)", gen2Timings->T4Min);
        }
        break;
    }
    case STUHFL_PARAM_KEY_RWD_GEN2PROTOCOL_CFG: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg)) {
            STUHFL_T_ST25RU3993_Gen2Protocol_Cfg *gen2ProtocolCfg = (STUHFL_T_ST25RU3993_Gen2Protocol_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, gen2ProtocolCfg));
            TRACE_DL_LOG_APPEND(" Gen2ProtocolCfg(tari: %d, blf: %d, coding: %d, trext: %d)",
                                gen2ProtocolCfg->tari, gen2ProtocolCfg->blf, gen2ProtocolCfg->coding, gen2ProtocolCfg->trext);
        }
        break;
    }
    case STUHFL_PARAM_KEY_RWD_GB29768PROTOCOL_CFG: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg)) {
            STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg *gb29768ProtocolCfg = (STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, gb29768ProtocolCfg));
            TRACE_DL_LOG_APPEND(" Gb29768ProtocolCfg(tc: %d, blf: %d, coding: %d, trext: %d)", gb29768ProtocolCfg->tc, gb29768ProtocolCfg->blf, gb29768ProtocolCfg->coding, gb29768ProtocolCfg->trext);
        }
        break;
    }
    case STUHFL_PARAM_KEY_RWD_TXRX_CFG: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_TxRx_Cfg)) {
            STUHFL_T_ST25RU3993_TxRx_Cfg *txRxCfg = (STUHFL_T_ST25RU3993_TxRx_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TxRx_Cfg));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, txRxCfg));
            TRACE_DL_LOG_APPEND(" TxRxCfg(txOutputLevel: %d, rxSensitivity: %d, usedAntenna: %d, alternateAntennaInterval: %d)", txRxCfg->txOutputLevel, txRxCfg->rxSensitivity, txRxCfg->usedAntenna, txRxCfg->alternateAntennaInterval);
        }
        break;
    }
    case STUHFL_PARAM_KEY_RWD_PA_CFG: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_PA_Cfg)) {
            STUHFL_T_ST25RU3993_PA_Cfg *paCfg = (STUHFL_T_ST25RU3993_PA_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_PA_Cfg));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, paCfg));
            TRACE_DL_LOG_APPEND(" PA_Cfg(useExternal: %d)", paCfg->useExternal);
        }
        break;
    }
    case STUHFL_PARAM_KEY_RWD_GEN2INVENTORY_CFG: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg)) {
            STUHFL_T_ST25RU3993_Gen2Inventory_Cfg *invGen2Cfg = (STUHFL_T_ST25RU3993_Gen2Inventory_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, invGen2Cfg));
            TRACE_DL_LOG_APPEND(" Gen2InventoryCfg(fastInv: %d, autoAck: %d, readTID: %d, startQ: %d, adaptiveQEnable: %d, minQ: %d, maxQ: %d, adjustOptions: %d, C1: (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d), C2: (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d), autoTuningInterval: %d, autoTuningLevel: %d, autoTuningAlgo: %d, autoTuningFalsePositiveDetection: %d, sel: %d, session: %d, target: %d, toggleTarget: %d, targetDepletionMode: %d, adaptiveSensitivityEnable: %d, adaptiveSensitivityInterval: %d)",
                                invGen2Cfg->fastInv, invGen2Cfg->autoAck, invGen2Cfg->readTID,
                                invGen2Cfg->startQ, invGen2Cfg->adaptiveQEnable, invGen2Cfg->minQ, invGen2Cfg->maxQ, invGen2Cfg->adjustOptions,
                                invGen2Cfg->C1[0], invGen2Cfg->C1[1], invGen2Cfg->C1[2], invGen2Cfg->C1[3], invGen2Cfg->C1[4], invGen2Cfg->C1[5], invGen2Cfg->C1[6], invGen2Cfg->C1[7],
                                invGen2Cfg->C1[8], invGen2Cfg->C1[9], invGen2Cfg->C1[10], invGen2Cfg->C1[11], invGen2Cfg->C1[12], invGen2Cfg->C1[13], invGen2Cfg->C1[14], invGen2Cfg->C1[15],
                                invGen2Cfg->C2[0], invGen2Cfg->C2[1], invGen2Cfg->C2[2], invGen2Cfg->C2[3], invGen2Cfg->C2[4], invGen2Cfg->C2[5], invGen2Cfg->C2[6], invGen2Cfg->C2[7],
                                invGen2Cfg->C2[8], invGen2Cfg->C2[9], invGen2Cfg->C2[10], invGen2Cfg->C2[11], invGen2Cfg->C2[12], invGen2Cfg->C2[13], invGen2Cfg->C2[14], invGen2Cfg->C2[15],
                                invGen2Cfg->autoTuningInterval, invGen2Cfg->autoTuningLevel, invGen2Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD, (invGen2Cfg->autoTuningAlgo & TUNING_ALGO_ENABLE_FPD) ? true : false,
                                invGen2Cfg->sel, invGen2Cfg->session, invGen2Cfg->target, invGen2Cfg->toggleTarget, invGen2Cfg->targetDepletionMode,
                                invGen2Cfg->adaptiveSensitivityEnable, invGen2Cfg->adaptiveSensitivityInterval);
        }
        break;
    }
    case STUHFL_PARAM_KEY_RWD_GB29768INVENTORY_CFG: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg)) {
            STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg *invGb29768Cfg = (STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, invGb29768Cfg));
            TRACE_DL_LOG_APPEND(" Gb29768InventoryCfg(readTID: %d, autoTuningInterval: %d, autoTuningLevel: %d, autoTuningAlgo: %d, autoTuningFalsePositiveDetection: %d, adaptiveSensitivityEnable: %d, adaptiveSensitivityInterval: %d, condition: %d, session: %d, target: %d, toggleTarget: %d, targetDepletionMode: %d, endThreshold:%d, ccnThreshold:%d, cinThreshold:%d)",
                                invGb29768Cfg->readTID,
                                invGb29768Cfg->autoTuningInterval, invGb29768Cfg->autoTuningLevel, invGb29768Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD, (invGb29768Cfg->autoTuningAlgo & TUNING_ALGO_ENABLE_FPD) ? true : false,
                                invGb29768Cfg->adaptiveSensitivityEnable, invGb29768Cfg->adaptiveSensitivityInterval,
                                invGb29768Cfg->condition, invGb29768Cfg->session, invGb29768Cfg->target, invGb29768Cfg->toggleTarget, invGb29768Cfg->targetDepletionMode,
                                invGb29768Cfg->endThreshold, invGb29768Cfg->ccnThreshold, invGb29768Cfg->cinThreshold);
        }
        break;
    }

    case STUHFL_PARAM_KEY_TUNING_CAPS: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_TuningCaps)) {
            STUHFL_T_ST25RU3993_TuningCaps *tuning = (STUHFL_T_ST25RU3993_TuningCaps*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TuningCaps));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, tuning));
            TRACE_DL_LOG_APPEND(" Tuning(antenna: %d, channelListIdx: %d, cin: %d, clen: %d, cout: %d)", tuning->antenna, tuning->channelListIdx, tuning->caps.cin, tuning->caps.clen, tuning->caps.cout);
        }
        break;
    }
    case STUHFL_PARAM_KEY_TUNING: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_Tuning)) {
            STUHFL_T_ST25RU3993_Tuning *tuning = (STUHFL_T_ST25RU3993_Tuning*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_Tuning));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, tuning));
            TRACE_DL_LOG_APPEND(" Tuning(antenna: %d, cin: %d, clen: %d, cout: %d)", tuning->antenna, tuning->cin, tuning->clen, tuning->cout);
        }
        break;
    }
    case STUHFL_PARAM_KEY_TUNING_TABLE_ENTRY: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_TuningTableEntry)) {
#define TB_SIZE    128
            char tb[4][TB_SIZE];
            STUHFL_T_ST25RU3993_TuningTableEntry *tuningTableEntry = (STUHFL_T_ST25RU3993_TuningTableEntry*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TuningTableEntry));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, tuningTableEntry));
            TRACE_DL_LOG_APPEND(" TuningTableEntry(entry: %d, freq: %d, cin: 0x%s, clen: 0x%s, cout: 0x%s, IQ: 0x%s)",
                                tuningTableEntry->entry, tuningTableEntry->freq, byteArray2HexString(tb[0], TB_SIZE, tuningTableEntry->cin, MAX_ANTENNA), byteArray2HexString(tb[1], TB_SIZE, tuningTableEntry->clen, MAX_ANTENNA), byteArray2HexString(tb[2], TB_SIZE, tuningTableEntry->cout, MAX_ANTENNA), byteArray2HexString(tb[3], TB_SIZE, (uint8_t *)tuningTableEntry->IQ, MAX_ANTENNA * sizeof(uint16_t)));
#undef TB_SIZE
        }
        break;
    }
    case STUHFL_PARAM_KEY_TUNING_TABLE_INFO: {
        if (rcvPayloadAvailable >= sizeof(STUHFL_T_ST25RU3993_TuningTableInfo)) {
            STUHFL_T_ST25RU3993_TuningTableInfo *tuningTableInfo = (STUHFL_T_ST25RU3993_TuningTableInfo*)&(((uint8_t *)values)[*valuesOffset]);
            *valuesOffset = (uint16_t)(*valuesOffset + (uint16_t)sizeof(STUHFL_T_ST25RU3993_TuningTableInfo));
            *rcvPayloadOffset = (uint16_t)(*rcvPayloadOffset + (uint16_t)getTlvExt(&rcvPayload[*rcvPayloadOffset], &tag, &len, tuningTableInfo));
            TRACE_DL_LOG_APPEND(" TuningTableInfo(profile: %d, numEntries: %d)", tuningTableInfo->profile, tuningTableInfo->numEntries);
        }
        break;
    }
    default:
        return ERR_PARAM;
    }

    //
    return ERR_NONE;
}

/**
  * @}
  */
/**
  * @}
  */
