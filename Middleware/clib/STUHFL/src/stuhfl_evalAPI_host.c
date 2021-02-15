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
 *  @brief Main dispatcher for streaming protocol uses a stream_driver for the Host
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup PC_Communication
  * @{
  */

#include "stuhfl.h"
#include "stuhfl_sl.h"
#include "stuhfl_sl_gen2.h"
#include "stuhfl_sl_gb29768.h"
#include "stuhfl_al.h"
#include "stuhfl_pl.h"
#include "stuhfl_dl.h"
#include "stuhfl_dl_ST25RU3993.h"
#include "stuhfl_evalAPI.h"
#include "stuhfl_err.h"
#include "stuhfl_helpers.h"
#include "stuhfl_log.h"

//
#define TRACE_EVAL_API_CLEAR()    { STUHFL_F_LogClear(LOG_LEVEL_TRACE_EVAL_API); }
#define TRACE_EVAL_API_APPEND(...){ STUHFL_F_LogAppend(LOG_LEVEL_TRACE_EVAL_API, __VA_ARGS__); }
#define TRACE_EVAL_API_FLUSH()    { STUHFL_F_LogFlush(LOG_LEVEL_TRACE_EVAL_API); }
//
#define TRACE_EVAL_API_START()      { STUHFL_F_LogClear(LOG_LEVEL_TRACE_EVAL_API); }
#define TRACE_EVAL_API(...)         { STUHFL_F_LogAppend(LOG_LEVEL_TRACE_EVAL_API, __VA_ARGS__); STUHFL_F_LogFlush(LOG_LEVEL_TRACE_EVAL_API); }

//
STUHFL_T_DEVICE_CTX device = 0;
#define SND_BUFFER_SIZE         (UART_RX_BUFFER_SIZE)   /* HOST SND buffer based on FW RCV buffer */
#define RCV_BUFFER_SIZE         (UART_TX_BUFFER_SIZE)   /* HOST RCV buffer based on FW SND buffer */
uint8_t sndData[SND_BUFFER_SIZE];
uint16_t sndDataLen = SND_BUFFER_SIZE;
uint8_t rcvData[RCV_BUFFER_SIZE];
uint16_t rcvDataLen = RCV_BUFFER_SIZE;

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Connect(char *szComPort)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_PORT, (STUHFL_T_PARAM_VALUE)szComPort);
    retCode |= STUHFL_F_Connect(&device, sndData, SND_BUFFER_SIZE, rcvData, RCV_BUFFER_SIZE);
    // enable data line
    uint8_t on = TRUE;
    retCode |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_DTR, (STUHFL_T_PARAM_VALUE)&on);
    // toogle reset line
    on = TRUE;
    retCode |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RTS, (STUHFL_T_PARAM_VALUE)&on);
    on = FALSE;
    retCode |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RTS, (STUHFL_T_PARAM_VALUE)&on);
    TRACE_EVAL_API("Connect(Port: %s) = %d", szComPort, retCode);
    return retCode;
}

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Disconnect()
{
    return STUHFL_F_Disconnect();
}

// ---- Getter Generic ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetBoardVersion(STUHFL_T_Version *swVersion, STUHFL_T_Version *hwVersion)
{
    memset(swVersion, 0, sizeof(STUHFL_T_Version));
    memset(hwVersion, 0, sizeof(STUHFL_T_Version));
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetVersion(&swVersion->major, &swVersion->minor, &swVersion->micro, &swVersion->build,
                                &hwVersion->major, &hwVersion->minor, &hwVersion->micro, &hwVersion->build);

    TRACE_EVAL_API("GetBoardVersion(swVersion: %d.%d.%d.%d, HW_Version: %d.%d.%d.%d) = %d", swVersion->major, swVersion->minor, swVersion->micro, swVersion->build,
                   hwVersion->major, hwVersion->minor, hwVersion->micro, hwVersion->build, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetBoardInfo(STUHFL_T_Version_Info *swInfo, STUHFL_T_Version_Info *hwInfo)
{
    memset(swInfo, 0, sizeof(STUHFL_T_Version_Info));
    memset(hwInfo, 0, sizeof(STUHFL_T_Version_Info));
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetInfo((char*)swInfo, (char*)hwInfo);
    swInfo->info[swInfo->infoLength] = 0;
    hwInfo->info[hwInfo->infoLength] = 0;
    TRACE_EVAL_API("GetBoardInfo(SW Info: %s, HW Info: %s) = %d", swInfo->info, hwInfo->info, retCode);
    return retCode;
}

STUHFL_DLL_API void CALL_CONV Reboot()
{
    TRACE_EVAL_API_START();
    STUHFL_F_Reboot();
    TRACE_EVAL_API("Reboot()");
}
STUHFL_DLL_API void CALL_CONV EnterBootloader()
{
    TRACE_EVAL_API_START();
    STUHFL_F_EnterBootloader();
    TRACE_EVAL_API("EnterBootloader()");
}

// ---- Setter Register ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetRegister(STUHFL_T_ST25RU3993_Register *reg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_REGISTER, (STUHFL_T_PARAM_VALUE)reg);
    TRACE_EVAL_API("SetRegister(addr: 0x%02x, data: 0x%02x) = %d", reg->addr, reg->data, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetRegisterMultiple(STUHFL_T_ST25RU3993_Register **reg, uint8_t numReg)
{
    STUHFL_T_PARAM params[256];
    for (int i = 0; i < numReg; i++) {
        params[i] = STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_REGISTER;
    }
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetMultipleParams(numReg, params, (STUHFL_T_PARAM_VALUE *)*reg);
#define TB_SIZE    1024U
    char tb[TB_SIZE];
    int j = 0;
    for (int i = 0; i < numReg; i++) {
        j += snprintf(tb + j, (uint32_t)(TB_SIZE - (uint32_t)j), "[%02X, %02X] ", (*reg)[i].addr, (*reg)[i].data);
    }
    TRACE_EVAL_API("SetRegister([addr, data]: %s) = %d", tb, retCode);
#undef TB_SIZE
    return retCode;
}

// ---- Getter Register ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetRegister(STUHFL_T_ST25RU3993_Register *reg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_REGISTER, (STUHFL_T_PARAM_VALUE)reg);
    TRACE_EVAL_API("GetRegister(addr: 0x%02x, data: 0x%02x) = %d", reg->addr, reg->data, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetRegisterMultiple(uint8_t numReg, STUHFL_T_ST25RU3993_Register **reg)
{
    STUHFL_T_PARAM params[256];
    for (int i = 0; i < numReg; i++) {
        params[i] = STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_REGISTER;
    }
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetMultipleParams(numReg, params, (STUHFL_T_PARAM_VALUE *)*reg);
#define TB_SIZE    1024U
    char tb[TB_SIZE];
    int j = 0;
    for (int i = 0; i < numReg; i++) {
        j += snprintf(tb + j, (uint32_t)(TB_SIZE - (uint32_t)j), "[%02X, %02X] ", (*reg)[i].addr, (*reg)[i].data);
    }
    TRACE_EVAL_API("GetRegister([addr, data]: %s) = %d", tb, retCode);
#undef TB_SIZE
    return retCode;
}



// ---- Setter RwdCfg ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetRwdCfg(STUHFL_T_ST25RU3993_RwdConfig *rwdCfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CONFIG, (STUHFL_T_PARAM_VALUE)rwdCfg);
    TRACE_EVAL_API("SetRwdCfg(id: 0x%02x, value: 0x%02x) = %d", rwdCfg->id, rwdCfg->value, retCode);
    return retCode;
}

// ---- Getter RwdCfg ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetRwdCfg(STUHFL_T_ST25RU3993_RwdConfig *rwdCfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CONFIG, (STUHFL_T_PARAM_VALUE)rwdCfg);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (   (rwdCfg->id > STUHFL_RWD_CFG_ID_HARDWARE_ID_NUM)
                   || ((rwdCfg->id == STUHFL_RWD_CFG_ID_EXTVCO) && (rwdCfg->value > 1))
                   || ((rwdCfg->id == STUHFL_RWD_CFG_ID_INP) && (rwdCfg->value > 1))
                   || ((rwdCfg->id == STUHFL_RWD_CFG_ID_ANTENNA_SWITCH) && (rwdCfg->value > 1))
                   || ((rwdCfg->id == STUHFL_RWD_CFG_ID_TUNER) && (rwdCfg->value & ~(TUNER_CIN | TUNER_CLEN | TUNER_COUT)))
                   || ((rwdCfg->id == STUHFL_RWD_CFG_ID_HARDWARE_ID_NUM) && (rwdCfg->value > ELANCE_HW_ID))
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetRwdCfg(id: 0x%02x, value: 0x%02x) = %d", rwdCfg->id, rwdCfg->value, retCode);
    return retCode;
}



// ---- Setter Antenna Power ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetAntennaPower(STUHFL_T_ST25RU3993_Antenna_Power *antPwr)
{
    // depending on the timeout it might be longer as the communication timeout..
    uint32_t rdTimeOut = 4000;
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&rdTimeOut);
    uint32_t tmpRdTimeOut = antPwr->timeout + 4000U;
    if (ERR_NONE == STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&tmpRdTimeOut)) {
        // set power..
        retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_ANTENNA_POWER, (STUHFL_T_PARAM_VALUE)antPwr);
        // revert max timeout
        retCode |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&rdTimeOut);
    }
    TRACE_EVAL_API("SetAntennaPower(mode: 0x%02x timeout: %d, frequency: %d) = %d", antPwr->mode, antPwr->timeout, antPwr->frequency, retCode);
    return retCode;
}

// ---- Getter Antenna Power ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetAntennaPower(STUHFL_T_ST25RU3993_Antenna_Power *antPwr)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_ANTENNA_POWER, (STUHFL_T_PARAM_VALUE)antPwr);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (   ((antPwr->mode != ANTENNA_POWER_MODE_OFF) && (antPwr->mode != ANTENNA_POWER_MODE_ON))
                   || (antPwr->frequency > FREQUENCY_MAX_VALUE)
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetAntennaPower(mode: 0x%02x timeout: %d, frequency: %d) = %d", antPwr->mode, antPwr->timeout, antPwr->frequency, retCode);
    return retCode;
}




// ---- Setter Frequency ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetChannelList(STUHFL_T_ST25RU3993_ChannelList *channelList)
{
    TRACE_EVAL_API_CLEAR();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CHANNEL_LIST, (STUHFL_T_PARAM_VALUE)channelList);
    TRACE_EVAL_API_APPEND("SetChannelList(antenna: %d, nFrequencies: %d, currentChannelListIdx: %d, persistent: %d, idx:{Freq, (cin,clen,cout)} = ", channelList->antenna, channelList->nFrequencies, channelList->currentChannelListIdx, channelList->persistent);
    for (int i = 0; i < channelList->nFrequencies; i++) {
        TRACE_EVAL_API_APPEND("%d:{%d, (%d,%d,%d)}, ", i, channelList->item[i].frequency, channelList->item[i].caps.cin, channelList->item[i].caps.clen, channelList->item[i].caps.cout);
    }
    TRACE_EVAL_API_APPEND(") = %d", retCode);
    TRACE_EVAL_API_FLUSH();
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetFreqProfile(STUHFL_T_ST25RU3993_Freq_Profile *freqProfile)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_FREQ_PROFILE, (STUHFL_T_PARAM_VALUE)freqProfile);
    TRACE_EVAL_API("SetFreqProfile(profile: %d) = %d", freqProfile->profile, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetFreqProfileAdd2Custom(STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom *freqProfileAdd)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_FREQ_PROFILE_ADD2CUSTOM, (STUHFL_T_PARAM_VALUE)freqProfileAdd);
    TRACE_EVAL_API("SetFreqProfileAdd2Custom(clearList: %d, frequency: %d) = %d", freqProfileAdd->clearList, freqProfileAdd->frequency, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetFreqHop(STUHFL_T_ST25RU3993_Freq_Hop *freqHop)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_FREQ_HOP, (STUHFL_T_PARAM_VALUE)freqHop);
    TRACE_EVAL_API("SetFreqHop(maxSendingTime: %d) = %d", freqHop->maxSendingTime, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetFreqLBT(STUHFL_T_ST25RU3993_Freq_LBT *freqLBT)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_FREQ_LBT, (STUHFL_T_PARAM_VALUE)freqLBT);
    TRACE_EVAL_API("SetFreqLBT(listeningTime: %d, idleTime: %d, rssiLogThreshold: %d, skipLBTcheck: %d) = %d", freqLBT->listeningTime, freqLBT->idleTime, freqLBT->rssiLogThreshold, freqLBT->skipLBTcheck, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetFreqContMod(STUHFL_T_ST25RU3993_Freq_ContMod *freqContMod)
{
    uint32_t rdTimeOut = 4000;
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&rdTimeOut);
    uint32_t tmpRdTimeOut = freqContMod->maxSendingTime + 4000U;
    retCode |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&tmpRdTimeOut);
    //
    retCode |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_FREQ_CONT_MOD, (STUHFL_T_PARAM_VALUE)freqContMod);
    // revert max timeout
    retCode |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&rdTimeOut);
    TRACE_EVAL_API("SetFreqContMod(frequency: %d, enableContMod : %d, maxSendingTime : %d, modulationMode : %d) = %d", freqContMod->frequency, freqContMod->enableContMod, freqContMod->maxSendingTime, freqContMod->modulationMode, retCode);
    return retCode;
}

// ---- Getter Frequency ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetChannelList(STUHFL_T_ST25RU3993_ChannelList *channelList)
{
    TRACE_EVAL_API_CLEAR();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CHANNEL_LIST, (STUHFL_T_PARAM_VALUE)channelList);
#ifdef CHECK_API_INTEGRITY
    if (retCode == ERR_NONE) {
        bool isOk = (channelList->nFrequencies <= MAX_FREQUENCY) ? true : false;
        for (uint32_t i=0 ; (i<channelList->nFrequencies) && (isOk) ; i++) {
            isOk = (channelList->item[i].frequency <= FREQUENCY_MAX_VALUE) ? true : false;
        }
        if (  (!isOk)
                || (channelList->currentChannelListIdx > MAX_FREQUENCY)
#if ELANCE
                || ((channelList->antenna != ANTENNA_ALT) && (channelList->antenna > ANTENNA_4))
#else
                || ((channelList->antenna != ANTENNA_ALT) && (channelList->antenna > ANTENNA_2))
#endif
           ) {
            return (STUHFL_T_RET_CODE)ERR_PROTO;
        }
    }
#endif
    TRACE_EVAL_API_APPEND("GetChannelList(antenna: %d, nFrequencies: %d, currentChannelListIdx: %d, persistent: %d, idx:{Freq, (cin,clen,cout)} = ", channelList->antenna, channelList->nFrequencies, channelList->currentChannelListIdx, channelList->persistent);
    for (int i = 0; i < channelList->nFrequencies; i++) {
        TRACE_EVAL_API_APPEND("%d:{%d, (%d,%d,%d)}, ", i, channelList->item[i].frequency, channelList->item[i].caps.cin, channelList->item[i].caps.clen, channelList->item[i].caps.cout);
    }
    TRACE_EVAL_API_APPEND(") = %d", retCode);
    TRACE_EVAL_API_FLUSH();
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetFreqRSSI(STUHFL_T_ST25RU3993_Freq_Rssi *freqRSSI)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_FREQ_RSSI, (STUHFL_T_PARAM_VALUE)freqRSSI);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (   (freqRSSI->frequency > FREQUENCY_MAX_VALUE)
                   || (freqRSSI->rssiLogI > 15)
                   || (freqRSSI->rssiLogQ > 15)
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetFreqRSSI(frequency: %d, rssiLogI: %d, rssiLogQ: %d) = %d", freqRSSI->frequency, freqRSSI->rssiLogI, freqRSSI->rssiLogQ, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetFreqReflectedPower(STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info *freqReflectedPower)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_FREQ_REFLECTED, (STUHFL_T_PARAM_VALUE)freqReflectedPower);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && ((freqReflectedPower->frequency > FREQUENCY_MAX_VALUE)
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetFreqReflectedPower(frequency: %d, applyTunerSetting: %d, reflectedI: %d, reflectedQ: %d) = %d", freqReflectedPower->frequency, freqReflectedPower->applyTunerSetting, freqReflectedPower->reflectedI, freqReflectedPower->reflectedQ, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetFreqProfileInfo(STUHFL_T_ST25RU3993_Freq_Profile_Info *freqProfileInfo)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_FREQ_PROFILE_INFO, (STUHFL_T_PARAM_VALUE)freqProfileInfo);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (   (freqProfileInfo->profile >= NUM_SAVED_PROFILES)
                   || (freqProfileInfo->minFrequency > FREQUENCY_MAX_VALUE)
                   || (freqProfileInfo->maxFrequency > FREQUENCY_MAX_VALUE)
                   || (freqProfileInfo->numFrequencies > MAX_FREQUENCY)
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetFreqProfileInfo(profile: %d, minFreq : %d, maxFreq: %d, numFrequencies : %d) = %d", freqProfileInfo->profile, freqProfileInfo->minFrequency, freqProfileInfo->maxFrequency, freqProfileInfo->numFrequencies, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetFreqHop(STUHFL_T_ST25RU3993_Freq_Hop *freqHop)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_FREQ_HOP, (STUHFL_T_PARAM_VALUE)freqHop);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && ((freqHop->maxSendingTime < 40)
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetFreqHop(maxSendingTime: %d) = %d", freqHop->maxSendingTime, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetFreqLBT(STUHFL_T_ST25RU3993_Freq_LBT *freqLBT)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_FREQ_LBT, (STUHFL_T_PARAM_VALUE)freqLBT);
    TRACE_EVAL_API("GetFreqLBT(listeningTime: %d, idleTime: %d, rssiLogThreshold: %d, skipLBTcheck: %d) = %d", freqLBT->listeningTime, freqLBT->idleTime, freqLBT->rssiLogThreshold, freqLBT->skipLBTcheck, retCode);
    return retCode;
}



// ---- Setter SW Cfg ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGen2Timings(STUHFL_T_Gen2_Timings *gen2Timings)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_GEN2TIMINGS, (STUHFL_T_PARAM_VALUE)gen2Timings);
    TRACE_EVAL_API("SetGen2Timings(T4Min: %d) = %d", gen2Timings->T4Min, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGen2ProtocolCfg(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg *gen2ProtocolCfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_GEN2PROTOCOL_CFG, (STUHFL_T_PARAM_VALUE)gen2ProtocolCfg);
    TRACE_EVAL_API("SetGen2ProtocolCfg(tari: %d, blf: %d, coding: %d, trext: %d) = %d",
                   gen2ProtocolCfg->tari, gen2ProtocolCfg->blf, gen2ProtocolCfg->coding, gen2ProtocolCfg->trext, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGb29768ProtocolCfg(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg *gb29768ProtocolCfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_GB29768PROTOCOL_CFG, (STUHFL_T_PARAM_VALUE)gb29768ProtocolCfg);
    TRACE_EVAL_API("SetGb29768ProtocolCfg(tc: %d, blf: %d, coding: %d, trext: %d) = %d",
                   gb29768ProtocolCfg->tc, gb29768ProtocolCfg->blf, gb29768ProtocolCfg->coding, gb29768ProtocolCfg->trext, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTxRxCfg(STUHFL_T_ST25RU3993_TxRx_Cfg *txRxCfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_TXRX_CFG, (STUHFL_T_PARAM_VALUE)txRxCfg);
    TRACE_EVAL_API("SetTxRxCfg(txOutputLevel: %d, rxSensitivity: %d, usedAntenna: %d, alternateAntennaInterval: %d) = %d", txRxCfg->txOutputLevel, txRxCfg->rxSensitivity, txRxCfg->usedAntenna, txRxCfg->alternateAntennaInterval, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetPA_Cfg(STUHFL_T_ST25RU3993_PA_Cfg *paCfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_PA_CFG, (STUHFL_T_PARAM_VALUE)paCfg);
    TRACE_EVAL_API("SetPA_Cfg(useExternal: %d) = %d", paCfg->useExternal, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGen2InventoryCfg(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg *invGen2Cfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_GEN2INVENTORY_CFG, (STUHFL_T_PARAM_VALUE)invGen2Cfg);
    TRACE_EVAL_API("SetGen2InventoryCfg(fastInv: %d, autoAck: %d, readTID: %d, startQ: %d, adaptiveQEnable: %d, minQ: %d, maxQ: %d, adjustOptions: %d, C1: (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d), C2: (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d), autoTuningInterval: %d, autoTuningLevel: %d, autoTuningAlgo: %d, autoTuningFalsePositiveDetection: %d, sel: %d, session: %d, target: %d, toggleTarget: %d, targetDepletionMode: %d, adaptiveSensitivityEnable: %d, adaptiveSensitivityInterval: %d) = %d",
                   invGen2Cfg->fastInv, invGen2Cfg->autoAck, invGen2Cfg->readTID,
                   invGen2Cfg->startQ, invGen2Cfg->adaptiveQEnable, invGen2Cfg->minQ, invGen2Cfg->maxQ, invGen2Cfg->adjustOptions,
                   invGen2Cfg->C1[0], invGen2Cfg->C1[1], invGen2Cfg->C1[2], invGen2Cfg->C1[3], invGen2Cfg->C1[4], invGen2Cfg->C1[5], invGen2Cfg->C1[6], invGen2Cfg->C1[7],
                   invGen2Cfg->C1[8], invGen2Cfg->C1[9], invGen2Cfg->C1[10], invGen2Cfg->C1[11], invGen2Cfg->C1[12], invGen2Cfg->C1[13], invGen2Cfg->C1[14], invGen2Cfg->C1[15],
                   invGen2Cfg->C2[0], invGen2Cfg->C2[1], invGen2Cfg->C2[2], invGen2Cfg->C2[3], invGen2Cfg->C2[4], invGen2Cfg->C2[5], invGen2Cfg->C2[6], invGen2Cfg->C2[7],
                   invGen2Cfg->C2[8], invGen2Cfg->C2[9], invGen2Cfg->C2[10], invGen2Cfg->C2[11], invGen2Cfg->C2[12], invGen2Cfg->C2[13], invGen2Cfg->C2[14], invGen2Cfg->C2[15],
                   invGen2Cfg->autoTuningInterval, invGen2Cfg->autoTuningLevel, invGen2Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD, (invGen2Cfg->autoTuningAlgo & TUNING_ALGO_ENABLE_FPD) ? true : false,
                   invGen2Cfg->sel, invGen2Cfg->session, invGen2Cfg->target, invGen2Cfg->toggleTarget, invGen2Cfg->targetDepletionMode,
                   invGen2Cfg->adaptiveSensitivityEnable, invGen2Cfg->adaptiveSensitivityInterval, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGb29768InventoryCfg(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg *invGb29768Cfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_GB29768INVENTORY_CFG, (STUHFL_T_PARAM_VALUE)invGb29768Cfg);
    TRACE_EVAL_API("SetGb29768InventoryCfg(readTID: %d, autoTuningInterval: %d, autoTuningLevel: %d, autoTuningAlgo: %d, autoTuningFalsePositiveDetection: %d, adaptiveSensitivityEnable: %d, adaptiveSensitivityInterval: %d, condition: %d, session: %d, target: %d, toggleTarget: %d, targetDepletionMode: %d, endThreshold:%d, ccnThreshold:%d, cinThreshold:%d) = %d",
                   invGb29768Cfg->readTID,
                   invGb29768Cfg->autoTuningInterval, invGb29768Cfg->autoTuningLevel, invGb29768Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD, (invGb29768Cfg->autoTuningAlgo & TUNING_ALGO_ENABLE_FPD) ? true : false,
                   invGb29768Cfg->adaptiveSensitivityEnable, invGb29768Cfg->adaptiveSensitivityInterval,
                   invGb29768Cfg->condition, invGb29768Cfg->session, invGb29768Cfg->target, invGb29768Cfg->toggleTarget, invGb29768Cfg->targetDepletionMode,
                   invGb29768Cfg->endThreshold, invGb29768Cfg->ccnThreshold, invGb29768Cfg->cinThreshold,
                   retCode);
    return retCode;
}

// ---- Getter SW Cfg ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGen2Timings(STUHFL_T_Gen2_Timings *gen2Timings)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_GEN2TIMINGS, (STUHFL_T_PARAM_VALUE)gen2Timings);
    TRACE_EVAL_API("GetGen2Timings(T4Min: %d) = %d", gen2Timings->T4Min, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGen2ProtocolCfg(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg *gen2ProtocolCfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_GEN2PROTOCOL_CFG, (STUHFL_T_PARAM_VALUE)gen2ProtocolCfg);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (   (gen2ProtocolCfg->tari > GEN2_TARI_25_00)
                   || (gen2ProtocolCfg->coding > GEN2_COD_MILLER8)
                   || (gen2ProtocolCfg->blf > GEN2_LF_640)
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetGen2ProtocolCfg(tari: %d, blf: %d, coding: %d, trext: %d) = %d",
                   gen2ProtocolCfg->tari, gen2ProtocolCfg->blf, gen2ProtocolCfg->coding, gen2ProtocolCfg->trext, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGb29768ProtocolCfg(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg *gb29768ProtocolCfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_GB29768PROTOCOL_CFG, (STUHFL_T_PARAM_VALUE)gb29768ProtocolCfg);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (   (gb29768ProtocolCfg->blf > GB29768_BLF_640)
                   || (gb29768ProtocolCfg->coding > GB29768_COD_MILLER8)
                   || (gb29768ProtocolCfg->tc > GB29768_TC_12_5)
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetGb29768ProtocolCfg(tc: %d, blf: %d, coding: %d, trext: %d) = %d",
                   gb29768ProtocolCfg->tc, gb29768ProtocolCfg->blf, gb29768ProtocolCfg->coding, gb29768ProtocolCfg->trext, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTxRxCfg(STUHFL_T_ST25RU3993_TxRx_Cfg *txRxCfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_TXRX_CFG, (STUHFL_T_PARAM_VALUE)txRxCfg);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (
#if ELANCE
                ((txRxCfg->usedAntenna != ANTENNA_ALT) && (txRxCfg->usedAntenna > ANTENNA_4))
#else
                ((txRxCfg->usedAntenna != ANTENNA_ALT) && (txRxCfg->usedAntenna > ANTENNA_2))
#endif
            )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetTxRxCfg(txOutputLevel: %d, rxSensitivity: %d, usedAntenna: %d, alternateAntennaInterval: %d) = %d", txRxCfg->txOutputLevel, txRxCfg->rxSensitivity, txRxCfg->usedAntenna, txRxCfg->alternateAntennaInterval, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetPA_Cfg(STUHFL_T_ST25RU3993_PA_Cfg *paCfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_PA_CFG, (STUHFL_T_PARAM_VALUE)paCfg);
    TRACE_EVAL_API("GetPA_Cfg(useExternal: %d) = %d", paCfg->useExternal, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGen2InventoryCfg(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg *invGen2Cfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_GEN2INVENTORY_CFG, (STUHFL_T_PARAM_VALUE)invGen2Cfg);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (   (invGen2Cfg->session > GEN2_SESSION_S3)
                   || (invGen2Cfg->target > GEN2_TARGET_B)
                   || ((invGen2Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD) > TUNING_ALGO_MEDIUM)       // remove FPD bit
                   || (invGen2Cfg->startQ > MAXGEN2Q)
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetGen2InventoryCfg(fastInv: %d, autoAck: %d, readTID: %d, startQ: %d, adaptiveQEnable: %d, minQ: %d, maxQ: %d, adjustOptions: %d, C1: (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d), C2: (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d), autoTuningInterval: %d, autoTuningLevel: %d, autoTuningAlgo: %d, autoTuningFalsePositiveDetection: %d, sel: %d, session: %d, target: %d, toggleTarget: %d, targetDepletionMode: %d, adaptiveSensitivityEnable: %d, adaptiveSensitivityInterval: %d) = %d",
                   invGen2Cfg->fastInv, invGen2Cfg->autoAck, invGen2Cfg->readTID,
                   invGen2Cfg->startQ, invGen2Cfg->adaptiveQEnable, invGen2Cfg->minQ, invGen2Cfg->maxQ, invGen2Cfg->adjustOptions,
                   invGen2Cfg->C1[0], invGen2Cfg->C1[1], invGen2Cfg->C1[2], invGen2Cfg->C1[3], invGen2Cfg->C1[4], invGen2Cfg->C1[5], invGen2Cfg->C1[6], invGen2Cfg->C1[7],
                   invGen2Cfg->C1[8], invGen2Cfg->C1[9], invGen2Cfg->C1[10], invGen2Cfg->C1[11], invGen2Cfg->C1[12], invGen2Cfg->C1[13], invGen2Cfg->C1[14], invGen2Cfg->C1[15],
                   invGen2Cfg->C2[0], invGen2Cfg->C2[1], invGen2Cfg->C2[2], invGen2Cfg->C2[3], invGen2Cfg->C2[4], invGen2Cfg->C2[5], invGen2Cfg->C2[6], invGen2Cfg->C2[7],
                   invGen2Cfg->C2[8], invGen2Cfg->C2[9], invGen2Cfg->C2[10], invGen2Cfg->C2[11], invGen2Cfg->C2[12], invGen2Cfg->C2[13], invGen2Cfg->C2[14], invGen2Cfg->C2[15],
                   invGen2Cfg->autoTuningInterval, invGen2Cfg->autoTuningLevel, invGen2Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD, (invGen2Cfg->autoTuningAlgo & TUNING_ALGO_ENABLE_FPD) ? true : false,
                   invGen2Cfg->sel, invGen2Cfg->session, invGen2Cfg->target, invGen2Cfg->toggleTarget, invGen2Cfg->targetDepletionMode,
                   invGen2Cfg->adaptiveSensitivityEnable, invGen2Cfg->adaptiveSensitivityInterval, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGb29768InventoryCfg(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg *invGb29768Cfg)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_GB29768INVENTORY_CFG, (STUHFL_T_PARAM_VALUE)invGb29768Cfg);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (   (invGb29768Cfg->condition > GB29768_CONDITION_FLAG0)
                   || (invGb29768Cfg->session > GB29768_SESSION_S3)
                   || (invGb29768Cfg->target > GB29768_TARGET_1)
                   || ((invGb29768Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD) > TUNING_ALGO_MEDIUM)         // remove FPD bit
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetGb29768InventoryCfg(readTID: %d, autoTuningInterval: %d, autoTuningLevel: %d, autoTuningAlgo: %d, autoTuningFalsePositiveDetection: %d, adaptiveSensitivityEnable: %d, adaptiveSensitivityInterval: %d, condition: %d, session: %d, target: %d, toggleTarget: %d, targetDepletionMode: %d, endThreshold:%d, ccnThreshold:%d, cinThreshold:%d) = %d",
                   invGb29768Cfg->readTID,
                   invGb29768Cfg->autoTuningInterval, invGb29768Cfg->autoTuningLevel, invGb29768Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD, (invGb29768Cfg->autoTuningAlgo & TUNING_ALGO_ENABLE_FPD) ? true : false,
                   invGb29768Cfg->adaptiveSensitivityEnable, invGb29768Cfg->adaptiveSensitivityInterval,
                   invGb29768Cfg->condition, invGb29768Cfg->session, invGb29768Cfg->target, invGb29768Cfg->toggleTarget, invGb29768Cfg->targetDepletionMode,
                   invGb29768Cfg->endThreshold, invGb29768Cfg->ccnThreshold, invGb29768Cfg->cinThreshold,
                   retCode);
    return retCode;
}



// ---- Setter Tuning ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuning(STUHFL_T_ST25RU3993_Tuning *tuning)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_TUNING, (STUHFL_T_PARAM_VALUE)tuning);
    TRACE_EVAL_API("SetTuning(antenna: %d, cin: %d, clen: %d, cout: %d) = %d", tuning->antenna, tuning->cin, tuning->clen, tuning->cout, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningTableEntry(STUHFL_T_ST25RU3993_TuningTableEntry *tuningTableEntry)
{
#define TB_SIZE    256U
    char tb[5][TB_SIZE];
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_TUNING_TABLE_ENTRY, (STUHFL_T_PARAM_VALUE)tuningTableEntry);
    TRACE_EVAL_API("SetTuningTableEntry(entry: %d, freq: %d, applyCapValues: 0x%s, cin: 0x%s, clen: 0x%s, cout: 0x%s, IQ: 0x%s) = %d",
                   tuningTableEntry->entry, tuningTableEntry->freq, byteArray2HexString(tb[0], TB_SIZE, tuningTableEntry->applyCapValues, MAX_ANTENNA), byteArray2HexString(tb[1], TB_SIZE, tuningTableEntry->cin, MAX_ANTENNA), byteArray2HexString(tb[2], TB_SIZE, tuningTableEntry->clen, MAX_ANTENNA), byteArray2HexString(tb[3], TB_SIZE, tuningTableEntry->cout, MAX_ANTENNA), byteArray2HexString(tb[4], TB_SIZE, (uint8_t *)tuningTableEntry->IQ, MAX_ANTENNA * sizeof(uint16_t)), retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningTableDefault(STUHFL_T_ST25RU3993_TunerTableSet *set)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_TUNING_TABLE_DEFAULT, (STUHFL_T_PARAM_VALUE)set);
    TRACE_EVAL_API("SetTuningTableDefault(profile: %d, freq: %d) = %d", set->profile, set->freq, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningTableSave2Flash(void)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_TUNING_TABLE_SAVE, (STUHFL_T_PARAM_VALUE)NULL);
    TRACE_EVAL_API("SetTuningTableSave2Flash() = %d", retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningTableEmpty(void)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_TUNING_TABLE_EMPTY, (STUHFL_T_PARAM_VALUE)NULL);
    TRACE_EVAL_API("SetTuningTableEmpty() = %d", retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningCaps(STUHFL_T_ST25RU3993_TuningCaps *tuning)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_TUNING_CAPS, (STUHFL_T_PARAM_VALUE)tuning);
    TRACE_EVAL_API("SetTuningCaps(antenna: %d, channelListIdx: %d, cin: %d, clen: %d, cout: %d) = %d", tuning->antenna, tuning->channelListIdx, tuning->caps.cin, tuning->caps.clen, tuning->caps.cout, retCode);
    return retCode;
}

// ---- Getter Tuning ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTuning(STUHFL_T_ST25RU3993_Tuning *tuning)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_TUNING, (STUHFL_T_PARAM_VALUE)tuning);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (
#if ELANCE
                ((tuning->antenna != ANTENNA_ALT) && (tuning->antenna > ANTENNA_4))
#else
                ((tuning->antenna != ANTENNA_ALT) && (tuning->antenna > ANTENNA_2))
#endif
            )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetTuning(antenna: %d, cin: %d, clen: %d, cout: %d) = %d", tuning->antenna, tuning->cin, tuning->clen, tuning->cout, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTuningTableEntry(STUHFL_T_ST25RU3993_TuningTableEntry *tuningTableEntry)
{
#define TB_SIZE    256U
    char tb[5][TB_SIZE];
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_TUNING_TABLE_ENTRY, (STUHFL_T_PARAM_VALUE)tuningTableEntry);
#ifdef CHECK_API_INTEGRITY
    uint8_t disabledCapValues[MAX_ANTENNA] = {0};
    if ((retCode == ERR_NONE)
            && (   (tuningTableEntry->entry > MAX_FREQUENCY)
                   || (tuningTableEntry->freq > FREQUENCY_MAX_VALUE)
                   || (memcmp(tuningTableEntry->applyCapValues, disabledCapValues, MAX_ANTENNA) != 0)
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetTuningTableEntry(entry: %d, freq: %d, applyCapValues: 0x%s,  cin: 0x%s, clen: 0x%s, cout: 0x%s, IQ: 0x%s) = %d",
                   tuningTableEntry->entry, tuningTableEntry->freq, byteArray2HexString(tb[0], TB_SIZE, tuningTableEntry->applyCapValues, MAX_ANTENNA), byteArray2HexString(tb[1], TB_SIZE, tuningTableEntry->cin, MAX_ANTENNA), byteArray2HexString(tb[2], TB_SIZE, tuningTableEntry->clen, MAX_ANTENNA), byteArray2HexString(tb[3], TB_SIZE, tuningTableEntry->cout, MAX_ANTENNA), byteArray2HexString(tb[4], TB_SIZE, (uint8_t *)tuningTableEntry->IQ, MAX_ANTENNA * sizeof(uint16_t)), retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTuningTableInfo(STUHFL_T_ST25RU3993_TuningTableInfo *tuningTableInfo)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_TUNING_TABLE_INFO, (STUHFL_T_PARAM_VALUE)tuningTableInfo);
#ifdef CHECK_API_INTEGRITY
    if ((retCode == ERR_NONE)
            && (   (tuningTableInfo->profile >= NUM_SAVED_PROFILES)
                   || (tuningTableInfo->numEntries > MAX_FREQUENCY)
               )) {
        retCode = ERR_PROTO;
    }
#endif
    TRACE_EVAL_API("GetTuningTableInfo(profile: %d, numEntries: %d) = %d", tuningTableInfo->profile, tuningTableInfo->numEntries, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTuningCaps(STUHFL_T_ST25RU3993_TuningCaps *tuning)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_TUNING_CAPS, (STUHFL_T_PARAM_VALUE)tuning);
#ifdef CHECK_API_INTEGRITY
    if (retCode == ERR_NONE) {
        if (   (tuning->channelListIdx > MAX_FREQUENCY)
#if ELANCE
                || ((tuning->antenna != ANTENNA_ALT) && (tuning->antenna > ANTENNA_4))
#else
                || ((tuning->antenna != ANTENNA_ALT) && (tuning->antenna > ANTENNA_2))
#endif
           ) {
            retCode = ERR_PROTO;
        }
    }
#endif
    TRACE_EVAL_API("GetTuningCaps(antenna: %d, channelListIdx: %d, cin: %d, clen: %d, cout: %d) = %d", tuning->antenna, tuning->channelListIdx, tuning->caps.cin, tuning->caps.clen, tuning->caps.cout, retCode);
    return retCode;
}

// ---- Tuning ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Tune(STUHFL_T_ST25RU3993_Tune *tune)
{
    // As tuning may take a while we increase the communication timeout to be
    uint32_t rdTimeOut = 4000;
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&rdTimeOut);
    uint32_t tmpRdTimeOut = 60000;   // to be on the safe side use 60 sec
    if (ERR_NONE == STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&tmpRdTimeOut)) {
        // start tuning..
        retCode |= STUHFL_F_ExecuteCmd((STUHFL_CG_DL << 8) | STUHFL_CC_TUNE, (STUHFL_T_PARAM_VALUE)tune, (STUHFL_T_PARAM_VALUE)tune);
        // revert max timeout
        retCode |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&rdTimeOut);
    }
    TRACE_EVAL_API("Tune(algo: %d, doFalsePositiveDetection: %d) = %d", tune->algo & ~TUNING_ALGO_ENABLE_FPD, (tune->algo & TUNING_ALGO_ENABLE_FPD) ? true : false, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV TuneChannel(STUHFL_T_ST25RU3993_TuneCfg *tuneCfg)
{
    // As tuning may take a while we increase the communication timeout to be
    uint32_t rdTimeOut = 4000;
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_GetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&rdTimeOut);
    uint32_t tmpRdTimeOut = 60000;   // to be on the safe side use 60 sec
    if (ERR_NONE == STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&tmpRdTimeOut)) {
        // start tuning..
        retCode |= STUHFL_F_ExecuteCmd((STUHFL_CG_DL << 8) | STUHFL_CC_TUNE_CHANNEL, (STUHFL_T_PARAM_VALUE)tuneCfg, (STUHFL_T_PARAM_VALUE)tuneCfg);
        // revert max timeout
        retCode |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RD_TIMEOUT_MS, (STUHFL_T_PARAM_VALUE)&rdTimeOut);
    }
    TRACE_EVAL_API("TuneChannel(enableFPD: %d, save2Flash: %d, channelListIdx: %d, antenna: %d, algo: %d) = %d", tuneCfg->enableFPD, tuneCfg->save2Flash, tuneCfg->channelListIdx, tuneCfg->antenna, tuneCfg->algorithm, retCode);
    return retCode;
}

// ---- Gen2 ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Inventory(STUHFL_T_Inventory_Option *invOption, STUHFL_T_Inventory_Data *invData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gen2_Inventory(invOption, invData);
    TRACE_EVAL_API("Gen2_Inventory(rssiMode: %d, roundCnt: %d, inventoryDelay: %d, reportOptions: %d, tagListSizeMax: %d, tagListSize: %d, STATISTICS: tuningStatus: %d, roundCnt: %d, sensitivity: %d, Q: %d, adc: %d, frequency: %d, tagCnt: %d, emptySlotCnt: %d, collisionCnt: %d, skipCnt: %d, preambleErrCnt: %d, crcErrCnt: %d, TAGLIST: ..) = %d",
                   invOption->rssiMode, invOption->roundCnt, invOption->inventoryDelay, invOption->reportOptions,
                   invData->tagListSizeMax, invData->tagListSize,
                   invData->statistics.tuningStatus, invData->statistics.roundCnt, invData->statistics.sensitivity, invData->statistics.Q, invData->statistics.adc, invData->statistics.frequency, invData->statistics.tagCnt, invData->statistics.emptySlotCnt, invData->statistics.collisionCnt, invData->statistics.skipCnt, invData->statistics.preambleErrCnt, invData->statistics.crcErrCnt, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Select(STUHFL_T_Gen2_Select *selData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gen2_Select(selData);
    TRACE_EVAL_API("Gen2_Select(mode: %d, target: %d, action: %d, memBank: %d, mask[32]: 0x%02x.., maskAddress: %d, maskLen: %d, truncation: %d) = %d",
                   selData->mode, selData->target, selData->action, selData->memBank, selData->mask[0], selData->maskAddress, selData->maskLen, selData->truncation, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Read(STUHFL_T_Read *readData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gen2_Read(readData);
#define TB_SIZE    256U
    char tb[2][TB_SIZE];
    TRACE_EVAL_API("Gen2_Read(memBank: %d, wordPtr: %d, bytes2Read: %d, pwd: 0x%s, data: 0x%s) = %d", readData->memBank, readData->wordPtr, readData->bytes2Read, byteArray2HexString(tb[0], TB_SIZE, readData->pwd, PASSWORD_LEN), byteArray2HexString(tb[1], TB_SIZE, readData->data, MAX_READ_DATA_LEN), retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Write(STUHFL_T_Write *writeData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gen2_Write(writeData);
#define TB_SIZE    256U
    char tb[TB_SIZE];
    TRACE_EVAL_API("Gen2_Write(memBank: %d, wordPtr: %d, pwd: 0x%s, data: 0x%02x%02x, tagReply: 0x%02x) = %d", writeData->memBank, writeData->wordPtr, byteArray2HexString(tb, TB_SIZE, writeData->pwd, PASSWORD_LEN), writeData->data[0], writeData->data[1], writeData->tagReply, retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_BlockWrite(STUHFL_T_BlockWrite *blockWrite)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gen2_BlockWrite(blockWrite);
#define TB_SIZE    256U
    char tb[2][TB_SIZE];
    TRACE_EVAL_API("Gen2_BlockWrite(memBank: %d, wordPtr: %d, pwd: 0x%s, nbWords: %d, data: 0x%s, tagReply: 0x%02x) = %d", blockWrite->memBank, blockWrite->wordPtr, byteArray2HexString(tb[0], TB_SIZE, blockWrite->pwd, PASSWORD_LEN), blockWrite->nbWords, byteArray2HexString(tb[1], TB_SIZE, blockWrite->data, MAX_BLOCKWRITE_DATA_LEN), blockWrite->tagReply, retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Lock(STUHFL_T_Gen2_Lock *lockData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gen2_Lock(lockData);
#define TB_SIZE    256U
    char tb[2][TB_SIZE];
    TRACE_EVAL_API("Gen2_Lock(mask: 0x%s, pwd: 0x%s, tagReply: 0x%02x) = %d", byteArray2HexString(tb[0], TB_SIZE, lockData->mask, 3), byteArray2HexString(tb[1], TB_SIZE, lockData->pwd, PASSWORD_LEN), lockData->tagReply, retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Kill(STUHFL_T_Kill *killData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gen2_Kill(killData);
#define TB_SIZE    256U
    char tb[TB_SIZE];
    TRACE_EVAL_API("Gen2_Kill(pwd: 0x%s, rfu: %d, recom: %d, tagReply: 0x%02x) = %d", byteArray2HexString(tb, TB_SIZE, killData->pwd, PASSWORD_LEN), killData->rfu, killData->recom, killData->tagReply, retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_GenericCmd(STUHFL_T_Gen2_GenericCmdSnd *genericCmdSnd, STUHFL_T_Gen2_GenericCmdRcv *genericCmdRcv)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gen2_GenericCmd(genericCmdSnd, genericCmdRcv);
#define TB_SIZE    256U
    char tb[3][TB_SIZE];
    TRACE_EVAL_API("Gen2_GenericCmd(pwd: 0x%s, cmd: 0x%02x, noResponseTime: %d, sndDataBitCnt: %d, sndData: 0x%s.., expectedRcvDataBitCnt: %d, rcvDataByteCnt: %d, rcvData: 0x%s..) = %d",
                   byteArray2HexString(tb[0], TB_SIZE, genericCmdSnd->pwd, PASSWORD_LEN), genericCmdSnd->cmd, genericCmdSnd->noResponseTime, genericCmdSnd->sndDataBitCnt, byteArray2HexString(tb[1], TB_SIZE, genericCmdSnd->sndData, 4), genericCmdSnd->expectedRcvDataBitCnt, genericCmdRcv->rcvDataByteCnt, byteArray2HexString(tb[2], TB_SIZE, genericCmdRcv->rcvData, 4), retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_QueryMeasureRssi(STUHFL_T_Gen2_QueryMeasureRssi *queryMeasureRssi)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gen2_QueryMeasureRssi(queryMeasureRssi);
#define TB_SIZE    256U
    char tb[5][TB_SIZE];
    TRACE_EVAL_API("STUHFL_F_Gen2_QueryMeasureRssi(frequency: %d, measureCnt: %d, agc: 0x%s.., rssiLogI: 0x%s.., rssiLogQ: 0x%s.., rssiLinI: 0x%s.., rssiLinQ: 0x%s..) = %d", queryMeasureRssi->frequency, queryMeasureRssi->measureCnt, byteArray2HexString(tb[0], TB_SIZE, queryMeasureRssi->agc, 4), byteArray2HexString(tb[1], TB_SIZE, queryMeasureRssi->rssiLogI, 4), byteArray2HexString(tb[2], TB_SIZE, queryMeasureRssi->rssiLogQ, 4), byteArray2HexString(tb[3], TB_SIZE, queryMeasureRssi->rssiLinI, 4), byteArray2HexString(tb[4], TB_SIZE, queryMeasureRssi->rssiLinQ, 4), retCode);
#undef TB_SIZE
    return retCode;
}



// ---- Gb29768 ----
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Inventory(STUHFL_T_Inventory_Option *invOption, STUHFL_T_Inventory_Data *invData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gb29768_Inventory(invOption, invData);
    TRACE_EVAL_API("Gb29768_Inventory(rssiMode: %d, roundCnt: %d, inventoryDelay: %d, reportOptions: %d, tagListSizeMax: %d, tagListSize: %d, STATISTICS: tuningStatus: %d, roundCnt: %d, sensitivity: %d, adc: %d, frequency: %d, tagCnt: %d, emptySlotCnt: %d, collisionCnt: %d, skipCnt: %d, preambleErrCnt: %d, crcErrCnt: %d, TAGLIST: ..) = %d",
                   invOption->rssiMode, invOption->roundCnt, invOption->inventoryDelay, invOption->reportOptions,
                   invData->tagListSizeMax, invData->tagListSize,
                   invData->statistics.tuningStatus, invData->statistics.roundCnt, invData->statistics.sensitivity, invData->statistics.adc, invData->statistics.frequency, invData->statistics.tagCnt, invData->statistics.emptySlotCnt, invData->statistics.collisionCnt, invData->statistics.skipCnt, invData->statistics.preambleErrCnt, invData->statistics.crcErrCnt, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Sort(STUHFL_T_Gb29768_Sort *sortData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gb29768_Sort(sortData);
    TRACE_EVAL_API("Gb29768_Sort(mode: %d, target: %d, rule: %d, storageArea: %d, mask[32]: 0x%02x.., bitPointer: %d, bitLength: %d) = %d",
                   sortData->mode, sortData->target, sortData->rule, sortData->storageArea, sortData->mask[0], sortData->bitPointer, sortData->bitLength, retCode);
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Read(STUHFL_T_Read *readData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gb29768_Read(readData);
#define TB_SIZE    256U
    char tb[2][TB_SIZE];
    TRACE_EVAL_API("Gb29768_Read(memBank: %d, wordPtr: %d, bytes2Read: %d, pwd: 0x%s, data: 0x%s) = %d", readData->memBank, readData->wordPtr, readData->bytes2Read, byteArray2HexString(tb[0], TB_SIZE, readData->pwd, PASSWORD_LEN), byteArray2HexString(tb[1], TB_SIZE, readData->data, MAX_READ_DATA_LEN), retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Write(STUHFL_T_Write *writeData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gb29768_Write(writeData);
#define TB_SIZE    256U
    char tb[TB_SIZE];
    TRACE_EVAL_API("Gb29768_Write(memBank: %d, wordPtr: %d, pwd: 0x%s, data: 0x%02x%02x) = %d", writeData->memBank, writeData->wordPtr, byteArray2HexString(tb, TB_SIZE, writeData->pwd, PASSWORD_LEN), writeData->data[0], writeData->data[1], retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Lock(STUHFL_T_Gb29768_Lock *lockData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gb29768_Lock(lockData);
#define TB_SIZE    256U
    char tb[TB_SIZE];
    TRACE_EVAL_API("Gb29768_Lock(storageArea: 0x%02x, configuration: 0x%02x, action: 0x%02x, pwd: 0x%s) = %d", lockData->storageArea, lockData->configuration, lockData->action, byteArray2HexString(tb, TB_SIZE, lockData->pwd, PASSWORD_LEN), retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Kill(STUHFL_T_Kill *killData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gb29768_Kill(killData);
#define TB_SIZE    256U
    char tb[TB_SIZE];
    TRACE_EVAL_API("Gb29768_Kill(pwd: 0x%s) = %d", byteArray2HexString(tb, TB_SIZE, killData->pwd, PASSWORD_LEN), retCode);
#undef TB_SIZE
    return retCode;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Erase(STUHFL_T_Gb29768_Erase *eraseData)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Gb29768_Erase(eraseData);
#define TB_SIZE    256U
    char tb[TB_SIZE];
    TRACE_EVAL_API("Gb29768_Erase(storageArea: %d, bytePtr: %d, bytes2Erase: %d, pwd: 0x%s) = %d", eraseData->storageArea, eraseData->bytePtr, eraseData->bytes2Erase, byteArray2HexString(tb, TB_SIZE, eraseData->pwd, PASSWORD_LEN), retCode);
#undef TB_SIZE
    return retCode;
}



// ---- Inventory Runner ----
STUHFL_T_ACTION_ID invRunnerId = 0;
typedef STUHFL_T_RET_CODE(*STUHFL_T_InventoryFinished)(STUHFL_T_Inventory_Data *data); // Finished Callback definition
STUHFL_T_RET_CODE _finishedCallback(STUHFL_T_Inventory_Data *data);
STUHFL_T_InventoryFinished callerFinishedCallback = NULL;

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV InventoryRunnerStart(STUHFL_T_Inventory_Option *option, STUHFL_T_InventoryCycle cycleCallback, STUHFL_T_InventoryFinished finishedCallback, STUHFL_T_Inventory_Data *data)
{
    // allow only one instance
    if (invRunnerId) {
        return ERR_BUSY;
    }

    // remember the finish callback
    callerFinishedCallback = finishedCallback;

    // start the runner
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Start(STUHFL_ACTION_INVENTORY, option, (STUHFL_T_ActionCycle)cycleCallback, data, (STUHFL_T_ActionFinished)_finishedCallback, &invRunnerId);

    TRACE_EVAL_API("InventoryRunnerStart(rssiMode: %d, roundCnt: %d, inventoryDelay: %d, reportOptions: %d, tagListSizeMax: %d, tagListSize: %d) = %d",
                   option->rssiMode, option->roundCnt, option->inventoryDelay, option->reportOptions,
                   data->tagListSizeMax, data->tagListSize,
                   retCode);

    //
    if (retCode == ERR_NONE) {
        // block execution until thread has finished
#if defined(WIN32) || defined(WIN64)
        WaitForSingleObject((HANDLE)invRunnerId, INFINITE);
#elif defined(POSIX)
        pthread_join((pthread_t)invRunnerId, NULL);
#endif
    }
    return retCode;
}

#ifdef USE_INVENTORY_EXT
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV InventoryRunnerStartExt(STUHFL_T_Inventory_Option *option, STUHFL_T_InventoryCycle cycleCallback, STUHFL_T_InventoryFinished finishedCallback, STUHFL_T_Inventory_Data_Ext *data)
{
    // allow only one instance
    if (invRunnerId) {
        return ERR_BUSY;
    }

    // remember the finish callback
    callerFinishedCallback = finishedCallback;

    // start the runner
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Start(STUHFL_ACTION_INVENTORY_W_SLOT_STATISTICS, option, (STUHFL_T_ActionCycle)cycleCallback, data, (STUHFL_T_ActionFinished)_finishedCallback, &invRunnerId);

    TRACE_EVAL_API("InventoryRunnerStartExt(rssiMode: %d, roundCnt: %d, inventoryDelay: %d, reportOptions: %d, tagListSizeMax: %d, tagListSize: %d, slotInfoList..) = %d",
                   option->rssiMode, option->roundCnt, option->inventoryDelay, option->reportOptions,
                   data->invData.tagListSizeMax, data->invData.tagListSize,
                   retCode);

    //
    if (retCode == ERR_NONE) {
        // block execution until thread has finished
#if defined(WIN32) || defined(WIN64)
        WaitForSingleObject((HANDLE)invRunnerId, INFINITE);
#elif defined(POSIX)
        pthread_join((pthread_t)invRunnerId, NULL);
#endif
    }
    return retCode;
}
#endif  // USE_INVENTORY_EXT

STUHFL_T_RET_CODE _finishedCallback(STUHFL_T_Inventory_Data *data)
{
    // callback caller if callback is provided
    if (callerFinishedCallback) {
        callerFinishedCallback(data);
    }
    // Reset ID to allow new runner to start
    invRunnerId = 0;
    return ERR_NONE;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV InventoryRunnerStop(void)
{
    TRACE_EVAL_API_START();
    STUHFL_T_RET_CODE retCode = STUHFL_F_Stop(invRunnerId);
    TRACE_EVAL_API("InventoryRunnerStop() = %d", retCode);
    return retCode;
}

/**
  * @}
  */
/**
  * @}
  */
