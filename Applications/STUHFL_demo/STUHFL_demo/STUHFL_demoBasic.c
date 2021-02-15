/**
  ******************************************************************************
  * @file           STUHFL_demoBasic.c
  * @brief          Low level functions (regs+power) related demos + utilities
  ******************************************************************************
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

#include "stuhfl.h"
#include "stuhfl_sl.h"
#include "stuhfl_sl_gen2.h"
#include "stuhfl_sl_gb29768.h"
#include "stuhfl_al.h"
#include "stuhfl_dl.h"
#include "stuhfl_evalAPI.h"
#include "stuhfl_err.h"
#include "stuhfl_log.h"
#include "stuhfl_platform.h"

#include "STUHFL_demo.h"

//
STUHFL_T_RET_CODE LogCycle(STUHFL_T_LOG_DATA_TYPE data);

/**
  * @brief      Outputs HW and SW versions
  *
  * @param      None
  *
  * @retval     None
  */
void demo_GetVersion()
{
    // get board version information

    STUHFL_T_Version        swVer =  STUHFL_O_VERSION_INIT();
    STUHFL_T_Version        hwVer =  STUHFL_O_VERSION_INIT();
    STUHFL_T_Version_Info   swInfo = STUHFL_O_VERSION_INFO_INIT();
    STUHFL_T_Version_Info   hwInfo = STUHFL_O_VERSION_INFO_INIT();

    STUHFL_T_RET_CODE ret = GetBoardVersion(&swVer, &hwVer);
    ret |= GetBoardInfo(&swInfo, &hwInfo);

    log2Screen(false, true, "\n-------------------------------------------------------\nSW: V%d.%d.%d.%d, %s\nHW: V%d.%d.%d.%d, %s\n-------------------------------------------------------\n\n",
               swVer.major, swVer.minor, swVer.micro, swVer.build, swInfo.info,
               hwVer.major, hwVer.minor, hwVer.micro, hwVer.build, hwInfo.info);
}

/**
  * @brief      Get Rwd registers demo
  *
  * @param      None
  *
  * @retval     None
  */
void demo_DumpRegisters()
{
    // read register (one by one)
    STUHFL_T_ST25RU3993_Register reg = STUHFL_O_ST25RU3993_REGISTER_INIT();

    for (int i = 0; i < 0x40; i++) {
        reg.addr = (uint8_t)i;
        reg.data = (uint8_t)0;
        GetRegister(&reg);
        log2Screen(false, false, "RegAddr: 0x%02x RegData: 0x%02x\n", reg.addr, reg.data);
    }
    log2Screen(false, true, "");
}

/**
  * @brief      Set registers demo
  *
  * @param[in]  addr: Register address
  * @param[in]  data: data to be written
  *
  * @retval     None
  */
void demo_WriteRegister(uint8_t addr, uint8_t data)
{
    STUHFL_T_ST25RU3993_Register reg = STUHFL_O_ST25RU3993_REGISTER_INIT();
    // write register
    reg.addr = addr & 0x3F;
    reg.data = data;
    SetRegister(&reg);
}


/**
  * @brief      Get reader config values demo
  *
  * @param      None
  *
  * @retval     None
  */
void demo_DumpRwdCfg()
{
    // read reader config (multiple)
#define PARAM_CNT   7
    STUHFL_T_PARAM params[PARAM_CNT];
    STUHFL_T_PARAM_VALUE *values;
    STUHFL_T_ST25RU3993_RwdConfig cfg[PARAM_CNT];
    params[0] = STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CONFIG; cfg[0].id = STUHFL_RWD_CFG_ID_PWR_DOWN_MODE;
    params[1] = STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CONFIG; cfg[1].id = STUHFL_RWD_CFG_ID_EXTVCO;
    params[2] = STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CONFIG; cfg[2].id = STUHFL_RWD_CFG_ID_PA;
    params[3] = STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CONFIG; cfg[3].id = STUHFL_RWD_CFG_ID_INP;
    params[4] = STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CONFIG; cfg[4].id = STUHFL_RWD_CFG_ID_ANTENNA_SWITCH;
    params[5] = STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CONFIG; cfg[5].id = STUHFL_RWD_CFG_ID_TUNER;
    params[6] = STUHFL_PARAM_TYPE_ST25RU3993 | STUHFL_PARAM_KEY_RWD_CONFIG; cfg[6].id = STUHFL_RWD_CFG_ID_HARDWARE_ID_NUM;
    values = (STUHFL_T_PARAM_VALUE *)cfg;
    //
    STUHFL_F_GetMultipleParams(PARAM_CNT, params, values);

    for (int i = 0; i < PARAM_CNT; i++) {
        log2Screen(false, false, "Cfg ID: %d Cfg Value: %d\n", cfg[i].id, cfg[i].value);
    }
    log2Screen(false, true, "");
#undef PARAM_CNT

}

/**
  * @brief      Set power demo
  *
  * @param[in]  on: enable/disable antenna power
  *
  * @retval     None
  */
void demo_Power(bool on)
{

    STUHFL_T_ST25RU3993_Register reg = STUHFL_O_ST25RU3993_REGISTER_INIT();
    reg.addr = 1;
    GetRegister(&reg);

    STUHFL_T_ST25RU3993_Antenna_Power pwr = STUHFL_O_ST25RU3993_ANTENNA_POWER_INIT();

    // get current freq
    GetAntennaPower(&pwr);

    // set power
    pwr.mode = on ? ANTENNA_POWER_MODE_ON : ANTENNA_POWER_MODE_OFF;
    SetAntennaPower(&pwr);
}

// --------------------------------------------------------------------------
/**
  * @brief      Logs enabling/disabling utility
  *
  * @param[in]  enable: enable/disable low level traces
  *
  * @retval     None
  */
#define LOG_STORAGE_SIZE        0xFFFF
char logStorage[LOG_STORAGE_SIZE];
void demo_LogLowLevel(bool enable)
{
    if (enable) {
        STUHFL_T_Log_Option option = STUHFL_O_LOG_OPTION_INIT(logStorage, LOG_STORAGE_SIZE);
        STUHFL_F_EnableLog(option, LogCycle);
#if defined(POSIX)
        openlog("STUHFL", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
#endif
    } else {
        STUHFL_F_DisableLog();
#if defined(POSIX)
        closelog();
#endif
    }
}
/**
  * @brief      Logs loop
  *
  * @param[in]  data: data to be processed
  *
  * @retval     None
  */
STUHFL_T_RET_CODE  LogCycle(STUHFL_T_LOG_DATA_TYPE data)
{
    STUHFL_T_Log_Data *d = (STUHFL_T_Log_Data *)data;

    d->logBuf[(d->logBufSize < (LOG_STORAGE_SIZE/LOG_LEVEL_COUNT)) ? d->logBufSize : (LOG_STORAGE_SIZE/LOG_LEVEL_COUNT)-1] = 0;

    static char tmp[36 + (LOG_STORAGE_SIZE/LOG_LEVEL_COUNT)];
    snprintf(tmp, 36 + (LOG_STORAGE_SIZE/LOG_LEVEL_COUNT), "%010d, %010u, %06u, %s: %s", d->logIdx, d->logTickCountMs[1], d->logTickCountMs[1] - d->logTickCountMs[0], STUHFL_F_LogLevel2Txt(d->logLevel), d->logBuf);

#if defined(WIN32) || defined(WIN64)
    OutputDebugStringA(tmp);
#elif defined(POSIX)
    syslog(LOG_DEBUG, tmp);
#else
#endif

    return ERR_NONE;
}
