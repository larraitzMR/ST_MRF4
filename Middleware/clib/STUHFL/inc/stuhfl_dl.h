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

//
#if !defined __STUHFL_DL_H
#define __STUHFL_DL_H

#include "stuhfl.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#define STATUS_DEFAULT_SND              ERR_NONE
#define UART_RX_BUFFER_SIZE             (2*1024)
#define UART_TX_BUFFER_SIZE             (4*1024)


// --------------------------------------------------------------------------
/**
 * Connect to a device via STUHFL
 * @param *device: replied pointer to the device
 * @param *sndData: pointer to data array that shall be used for all send frames
 * @param sndDataLen: length of send data array
 * @param *rcvData: pointer to data array that shall be used for all received frames
 * @param rcvDataLen: length of receive data array
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Connect(STUHFL_T_DEVICE_CTX *device, uint8_t *sndData, uint16_t sndDataLen, uint8_t *rcvData, uint16_t rcvDataLen);
/**
 * Disconnect from current connected device via STUHFL
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Disconnect(void);
/**
 * Get device context of current attached device
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_DEVICE_CTX CALL_CONV STUHFL_F_GetCtx(void);

// --------------------------------------------------------------------------
#define STUHFL_RESET_TYPE_SOFT              0x00 // clear all internal states and pending operations
#define STUHFL_RESET_TYPE_HARD              0x01 // reboot device and clear all states
#define STUHFL_RESET_TYPE_CLEAR_COMM        0x02 // clear outstanding communications  
/**
 * Reset current attached device
 * @param resetType: type of reset that shall be applied
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Reset(STUHFL_T_RESET resetType);

// --------------------------------------------------------------------------
/**
 * Generic set param function to set value of configuration
 * @param param: Parameter to be set. For a list of the available parameters see STUHFL_PARAM_xyz defeines stuhfl.c
 * @param value: the value to applied
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_SetParam(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE value);
/**
 * Generic get param function to get value of configuration
 * @param param: Parameter to be get. For a list of the available parameters see STUHFL_PARAM_xyz defeines stuhfl.c
 * @param value: the value to applied
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetParam(STUHFL_T_PARAM param, STUHFL_T_PARAM_VALUE value);
/**
 * Get param information
 * @param param: Parameter where further information shall be received
 * @param info: info of parameter. For further info see STUHFL_T_Info struct
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetParamInfo(STUHFL_T_PARAM param, STUHFL_T_Param_Info info);
/**
 * Set multiple param
 * @param paramCnt: Number of parameter that is passed in.
 * @param *params: List of Parameter to be set. For a list of the available parameters see STUHFL_PARAM_xyz defines in stuhfl.c
 * @param *values: the values to be applied
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_SetMultipleParams(STUHFL_T_PARAM_CNT paramCnt, STUHFL_T_PARAM *params, STUHFL_T_PARAM_VALUE *values);
/**
 * Get multiple param
 * @param paramCnt: Number of parameter that is passed in.
 * @param *params: List of Parameter to be get. For a list of the available parameters see STUHFL_PARAM_xyz defines in stuhfl.c
 * @param *values: the values to be applied
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetMultipleParams(STUHFL_T_PARAM_CNT paramCnt, STUHFL_T_PARAM *params, STUHFL_T_PARAM_VALUE *values);

/**
 * Send command to device
 * @param cmd: command to be send
 * @param sndParams: command parameters. The format of the params must be a correct TLV coded payload matching to the cmd
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_SendCmd(STUHFL_T_CMD cmd, STUHFL_T_CMD_SND_PARAMS sndParams);
/**
 * Receive command to device
 * @param cmd: command to be received
 * @param rcvParams: command parameters. The received params of the command
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_ReceiveCmdData(STUHFL_T_CMD cmd, STUHFL_T_CMD_RCV_DATA rcvParams);
/**
 * Send and receive combination in one call
 * @param cmd: command to be received
 * @param sndParams: command parameters. The format of the params must be a correct TLV coded payload matching to the cmd
 * @param rcvParams: command parameters. The received params of the command
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_ExecuteCmd(STUHFL_T_CMD cmd, STUHFL_T_CMD_SND_PARAMS sndParams, STUHFL_T_CMD_RCV_DATA rcvParams);

// --------------------------------------------------------------------------
/**
 * Try to read out FW version by using the old stream protocol.
 * @param *swVersion: firmware version number retrived using old protocol
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetVersionOld(uint8_t *swVersion);

// --------------------------------------------------------------------------
/**
 * Get device version information
 * @param *swVersionMajor: Major firmware version number information
 * @param *swVersionMinor: Minor firmware version number information
 * @param *swVersionMicro: Micro firmware version number information
 * @param *swVersionBuild: Build firmware version number information
 * @param *hwVersionMajor: Major hardware version number information
 * @param *hwVersionMinor: Minor hardware version number information
 * @param *hwVersionMicro: Micro hardware version number information
 * @param *hwVersionBuild: Build hardware version number information
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetVersion(uint8_t *swVersionMajor, uint8_t *swVersionMinor, uint8_t *swVersionMicro, uint8_t *swVersionBuild,
        uint8_t *hwVersionMajor, uint8_t *hwVersionMinor, uint8_t *hwVersionMicro, uint8_t *hwVersionBuild);

/**
 * Get device info information
 * @param *szSwInfo: zero terminated firmware information string
 * @param *szHwInfo: zero terminated hardware information string
 *
 * @return error code
*/

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetInfo(char *szSwInfo, char *szHwInfo);
/**
 * Firmware upgrade
 * @param *fwData: firmware data to be ugraded
 * @param fwdDataLen: firmware data length
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Upgrade(uint8_t *fwData, uint32_t fwdDataLen);

/**
 * Reboot and enter bootloader
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_EnterBootloader(void);

/**
 * Reboot FW
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Reboot(void);


/**
 * Receive Raw data from device
 * @param *data: data In pointer
 * @param *dataLen: received data length
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_GetRawData(uint8_t *data, uint16_t *dataLen);



#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __STUHFL_DL_H
