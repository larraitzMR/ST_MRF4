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

#include "stuhfl_al.h"
#include "stuhfl_log.h"
#include "stuhfl_err.h"
#include "stuhfl_platform.h"

static STUHFL_T_CallerCtx gCallerCtxPointer = NULL;
static STUHFL_T_Log gLogCallBack = NULL;
static STUHFL_T_LogOOP gLogCallBackOOP = NULL;
static STUHFL_T_Log_Option gLogOptions = { 0 };
static STUHFL_T_Log_Data gLogData[LOG_LEVEL_COUNT] = { 0 };
static uint32_t gLogId = 0;

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_EnableLog(STUHFL_T_Log_Option option, STUHFL_T_Log logCallback)
{
    gLogCallBack = logCallback;
    return STUHFL_F_EnableLog_OOP(option, NULL, NULL);
}

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_EnableLog_OOP(STUHFL_T_Log_Option option, STUHFL_T_CallerCtx callerCtx, STUHFL_T_LogOOP logCallback)
{
    memcpy(&gLogOptions, &option, sizeof(STUHFL_T_Log_Option));
    // assign buffers
    for (int i = 0; i < LOG_LEVEL_COUNT; i++) {
        gLogData[i].logBuf = option.logBuf[i];
        gLogData[i].logBufSize = 0;
    }
    gCallerCtxPointer = callerCtx;
    gLogCallBackOOP = logCallback;
    return ERR_NONE;
}

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_DisableLog(void)
{
    gLogCallBack = NULL;
    gLogCallBackOOP = NULL;
    return ERR_NONE;
}

STUHFL_DLL_API bool CALL_CONV STUHFL_F_IsLogEnabled(void)
{
    return (gLogCallBack != NULL) || (gLogCallBackOOP != NULL);
}

STUHFL_DLL_API bool CALL_CONV STUHFL_F_IsLogLevelSupported(uint32_t level)
{
    if ((gLogOptions.logLevels & level) == level) {
        return true;
    }
    return false;
}

STUHFL_DLL_API char* CALL_CONV STUHFL_F_LogLevel2Txt(uint32_t level)
{
    if      (level & LOG_LEVEL_INFO)                 { return "I"; }
    else if (level & LOG_LEVEL_WARNING)              { return "W"; }
    else if (level & LOG_LEVEL_DEBUG)                { return "D"; }
    else if (level & LOG_LEVEL_ERROR)                { return "E"; }
    else if (level & LOG_LEVEL_TRACE_AL)             { return "AL"; }
    else if (level & LOG_LEVEL_TRACE_SL)             { return "SL"; }
    else if (level & LOG_LEVEL_TRACE_DL)             { return "DL"; }
    else if (level & LOG_LEVEL_TRACE_PL)             { return "PL"; }
    else if (level & LOG_LEVEL_TRACE_EVAL_API)       { return "EA"; }
    else { return ""; }
}

STUHFL_DLL_API uint8_t CALL_CONV STUHFL_F_LogLevel2Idx(uint32_t level)
{
    if      (level & LOG_LEVEL_INFO)            { return 0; }
    else if (level & LOG_LEVEL_WARNING)         { return 1; }
    else if (level & LOG_LEVEL_DEBUG)           { return 2; }
    else if (level & LOG_LEVEL_ERROR)           { return 3; }
    else if (level & LOG_LEVEL_TRACE_AL)        { return 4; }
    else if (level & LOG_LEVEL_TRACE_SL)        { return 5; }
    else if (level & LOG_LEVEL_TRACE_DL)        { return 6; }
    else if (level & LOG_LEVEL_TRACE_PL)        { return 7; }
    else if (level & LOG_LEVEL_TRACE_EVAL_API)  { return 8; }
    else { return 0; }
}

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_LogClear(uint32_t level)
{
    // logging enabled ?
    if ((gLogCallBack == NULL) && (gLogCallBackOOP == NULL)) {
        return ERR_PARAM;
    }
    gLogData[STUHFL_F_LogLevel2Idx(level)].logTickCountMs[0] = getMilliCount();
    gLogData[STUHFL_F_LogLevel2Idx(level)].logBufSize = 0;
    return ERR_NONE;
}

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_LogAppend(uint32_t level, const char* format, ...)
{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    // logging enabled ?
    if ((gLogCallBack == NULL) && (gLogCallBackOOP == NULL)) {
        return ret;
    }

    ret = ERR_NOMEM;

    // build info
    va_list args;
    va_start(args, format);
    uint8_t idx = STUHFL_F_LogLevel2Idx(level);
    if ((gLogOptions.logBufSize[idx] - gLogData[idx].logBufSize) > 0) {
        int writtenLen = vsnprintf(&gLogData[idx].logBuf[gLogData[idx].logBufSize], (uint32_t)(gLogOptions.logBufSize[idx] - gLogData[idx].logBufSize), format, args);
        if ((writtenLen > 0) && (writtenLen < (gLogOptions.logBufSize[idx] - gLogData[idx].logBufSize))) {
            gLogData[idx].logBufSize = (uint16_t)(gLogData[idx].logBufSize + (uint16_t)writtenLen);
            if (gLogData[idx].logBufSize > gLogOptions.logBufSize[idx]) {
                gLogData[idx].logBufSize = gLogOptions.logBufSize[idx];
                gLogData[idx].logBuf[gLogData[idx].logBufSize - 1] = 0;
            }
            ret = ERR_NONE;
        }
    }
    va_end(args);
    return ret;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_LogFlush(uint32_t level)
{
    STUHFL_T_RET_CODE ret = ERR_PARAM;
    // logging enabled ?
    if ((gLogCallBack == NULL) && (gLogCallBackOOP == NULL)) {
        return ret;
    }

    STUHFL_F_LogAppend(level, "\n");

    uint8_t idx = STUHFL_F_LogLevel2Idx(level);

    // logLevel supported ?
    if ((gLogOptions.logLevels & level) == level) {
        gLogData[idx].logLevel = level;
    } else {
        return ret;
    }

    // increase idx and set timestamp
    gLogId++;
    gLogData[idx].logIdx = gLogId;
    if (gLogOptions.generateLogTimestamp) {
        gLogData[idx].logTickCountMs[1] = getMilliCount();
    }

    // call callBack
    ret = gLogCallBack != NULL ? gLogCallBack(&gLogData[idx]) : gLogCallBackOOP(gCallerCtxPointer, &gLogData[idx]);
    gLogData[idx].logBufSize = 0;
    return ret;
}

/**
  * @}
  */
/**
  * @}
  */
