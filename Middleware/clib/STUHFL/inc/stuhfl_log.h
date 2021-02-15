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

#if !defined __STUHFL_LOG_H
#define __STUHFL_LOG_H

//
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#include "stuhfl.h"

// --------------------------------------------------------------------------
#define LOG_LEVEL_INFO                  0x00000001
#define LOG_LEVEL_WARNING               0x00000002
#define LOG_LEVEL_DEBUG                 0x00000004
#define LOG_LEVEL_ERROR                 0x00000008
#define LOG_LEVEL_TRACE_AL              0x00000010
#define LOG_LEVEL_TRACE_SL              0x00000020
#define LOG_LEVEL_TRACE_DL              0x00000040
#define LOG_LEVEL_TRACE_PL              0x00000080
#define LOG_LEVEL_TRACE_EVAL_API        0x00000100
#define LOG_LEVEL_TRACE_BL              0x00000200
#define LOG_LEVEL_ALL                   0xFFFFFFFF

#define LOG_LEVEL_COUNT                 10

#pragma pack(push, 1)

typedef struct {
    bool                                generateLogTimestamp;
    uint32_t                            logLevels;
    char*                               logBuf[LOG_LEVEL_COUNT];        /* log buffer used to store log information for each log level */
    uint16_t                            logBufSize[LOG_LEVEL_COUNT];    /* size of logBuf for each log level*/
} STUHFL_T_Log_Option;
#define STUHFL_O_LOG_OPTION_INIT(logStorage, logStorageSize, ...) ((STUHFL_T_Log_Option) {  .generateLogTimestamp = true, \
                                                                                            .logLevels = LOG_LEVEL_ALL,     \
                                                                                            .logBuf = { &(logStorage)[0 * ((logStorageSize) / LOG_LEVEL_COUNT)], &(logStorage)[1 * ((logStorageSize) / LOG_LEVEL_COUNT)], \
                                                                                                        &(logStorage)[2 * ((logStorageSize) / LOG_LEVEL_COUNT)], &(logStorage)[3 * ((logStorageSize) / LOG_LEVEL_COUNT)], \
                                                                                                        &(logStorage)[4 * ((logStorageSize) / LOG_LEVEL_COUNT)], &(logStorage)[5 * ((logStorageSize) / LOG_LEVEL_COUNT)], \
                                                                                                        &(logStorage)[6 * ((logStorageSize) / LOG_LEVEL_COUNT)], &(logStorage)[7 * ((logStorageSize) / LOG_LEVEL_COUNT)], \
                                                                                                        &(logStorage)[8 * ((logStorageSize) / LOG_LEVEL_COUNT)], &(logStorage)[9 * ((logStorageSize) / LOG_LEVEL_COUNT)]},\
                                                                                            .logBufSize = { (logStorageSize) / LOG_LEVEL_COUNT, (logStorageSize) / LOG_LEVEL_COUNT, \
                                                                                                            (logStorageSize) / LOG_LEVEL_COUNT, (logStorageSize) / LOG_LEVEL_COUNT, \
                                                                                                            (logStorageSize) / LOG_LEVEL_COUNT, (logStorageSize) / LOG_LEVEL_COUNT, \
                                                                                                            (logStorageSize) / LOG_LEVEL_COUNT, (logStorageSize) / LOG_LEVEL_COUNT, \
                                                                                                            (logStorageSize) / LOG_LEVEL_COUNT, (logStorageSize) / LOG_LEVEL_COUNT},\
                                                                                            ##__VA_ARGS__ })

typedef struct {
    uint32_t                            logIdx;
    uint32_t                            logTickCountMs[2];              /* [0] = TickCount time of STUHFL_F_LogClear() call and [1] = TickCount time of STUHFL_F_LogFlush() */
    uint32_t                            logLevel;
    char*                               logBuf;
    uint16_t                            logBufSize;
} STUHFL_T_Log_Data;
#define STUHFL_O_LOG_DATA_INIT(...) ((STUHFL_T_Log_Data) { .logIdx = 0, .logTickCountMs = {0,0}, .logLevel = 0, .logBuf = NULL, .logBufSize = 0, ##__VA_ARGS__ })

#pragma pack(pop)

typedef void* STUHFL_T_CallerCtx;
typedef void* STUHFL_T_LOG_DATA_TYPE;
typedef STUHFL_T_RET_CODE(*STUHFL_T_Log)(STUHFL_T_LOG_DATA_TYPE data);
typedef STUHFL_T_RET_CODE(*STUHFL_T_LogOOP)(STUHFL_T_CallerCtx obj, STUHFL_T_LOG_DATA_TYPE data);

// --------------------------------------------------------------------------
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_EnableLog(STUHFL_T_Log_Option option, STUHFL_T_Log logCallback);
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_EnableLog_OOP(STUHFL_T_Log_Option option, STUHFL_T_CallerCtx callerCtx, STUHFL_T_LogOOP logCallback);
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_DisableLog(void);

STUHFL_DLL_API bool CALL_CONV STUHFL_F_IsLogEnabled(void);
STUHFL_DLL_API bool CALL_CONV STUHFL_F_IsLogLevelSupported(uint32_t level);
STUHFL_DLL_API char* CALL_CONV STUHFL_F_LogLevel2Txt(uint32_t level);
STUHFL_DLL_API uint8_t CALL_CONV STUHFL_F_LogLevel2Idx(uint32_t level);

/* Logging: Call order: 1: Clear, 2..n:Append, n+1:Flush */
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_LogClear(uint32_t level);
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_LogAppend(uint32_t level, const char* format, ...);
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_LogFlush(uint32_t level);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__STUHFL_LOG_H
