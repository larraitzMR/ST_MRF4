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
#if !defined __STUHFL_EVALAPI_H
#define __STUHFL_EVALAPI_H

#include "stuhfl.h"
#include "stuhfl_al.h"
#include "stuhfl_sl.h"
#include "stuhfl_sl_gen2.h"
#include "stuhfl_sl_gb29768.h"
#include "stuhfl_dl_ST25RU3993.h"

//
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

// --------------------------------------------------------------------------
// General
// --------------------------------------------------------------------------
/**
    * Connect to ST25RU3993 based EVAL board.
    * @param szComPort: The port name were the ST25RU3993 board is connected.
    + The string must be null terminated.
    *
    * @return error code
    */
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Connect(char *szComPort);
/**
    * Disconnect from a current connected board
    * @param none
    *
    * @return error code
    */
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Disconnect(void);

/**
    * Read the board software and hardware information\n
    * @param swVersion: See STUHFL_T_Version struct for further info
    * @param swVersion: See STUHFL_T_Version struct for further info
    *
    * @return error code
    */
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetBoardVersion(STUHFL_T_Version *swVersion, STUHFL_T_Version *hwVersion);
/**
    * Get human readable software and hardware device information\n
    * @param swInfo: See STUHFL_T_Version_Info struct for further info
    * @param hwInfo: See STUHFL_T_Version_Info struct for further info
    *
    * @return error code
    */
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetBoardInfo(STUHFL_T_Version_Info *swInfo, STUHFL_T_Version_Info *hwInfo);

/**
    * Reboot FW\n
    * NOTE: This function will never return, as the reboot is triggered immediately
    *
    * @return: never returns
    */
STUHFL_DLL_API void CALL_CONV Reboot(void);

/**
    * Shutdown FW and enter STM32 ROM bootloader of EVAL board.\n
    *
    * @return: never returns
    */
STUHFL_DLL_API void CALL_CONV EnterBootloader(void);

// --------------------------------------------------------------------------
// Configurations
// --------------------------------------------------------------------------
/**
 * Writes to ST25RU3993 register at address addr the value and prepares tx answer to host.\n
 * or Runs Direct commands if given address addr fits a Direct command code.\n
 * @param reg: See STUHFL_T_ST25RU3993_Register struct for further info
 *
 * @return error code
 */
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetRegister(STUHFL_T_ST25RU3993_Register *reg);
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetRegisterMultiple(STUHFL_T_ST25RU3993_Register **reg, uint8_t numReg);

/**
 * Reads one ST25RU3993 register at address and puts the value into the reply to the host.\n
 * @param reg: See STUHFL_T_ST25RU3993_Register struct for further info \n
 *
 * @return error code
 */
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetRegister(STUHFL_T_ST25RU3993_Register *reg);
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetRegisterMultiple(uint8_t numReg, STUHFL_T_ST25RU3993_Register **reg);

/**
 * Set reader configuration
 * @param rwdCfg: See STUHFL_T_ST25RU3993_RwdConfig struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetRwdCfg(STUHFL_T_ST25RU3993_RwdConfig *rwdCfg);
/**
 * Get reader configuration
 * @param rwdCfg: See STUHFL_T_ST25RU3993_RwdConfig struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetRwdCfg(STUHFL_T_ST25RU3993_RwdConfig *rwdCfg);



/**
  * Set antenna power
  * @param antPwr: See STUHFL_T_ST25RU3993_Antenna_Power struct for further info
  *
  * @return error code
 */
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetAntennaPower(STUHFL_T_ST25RU3993_Antenna_Power *antPwr);
/**
 * Get antenna power
 * @param antPwr: See STUHFL_T_ST25RU3993_Antenna_Power struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetAntennaPower(STUHFL_T_ST25RU3993_Antenna_Power *antPwr);



/**
 * Set frequency profile
 * @param freqProfile: See STUHFL_T_ST25RU3993_Freq_Profile struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV SetFreqProfile(STUHFL_T_ST25RU3993_Freq_Profile *freqProfile);
/**
 * Add frequency to custom profile
 * @param freqProfileAdd2Custom: See STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV SetFreqProfileAdd2Custom(STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom *freqProfileAdd2Custom);
/**
 * Set frequency channel list
 * @param channelList: See STUHFL_T_ST25RU3993_ChannelList struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetChannelList(STUHFL_T_ST25RU3993_ChannelList *channelList);
/**
 * Set frequency hopping time
 * @param freqHop: See STUHFL_T_ST25RU3993_Freq_Hop struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetFreqHop(STUHFL_T_ST25RU3993_Freq_Hop *freqHop);
/**
 * Set listen before talk configuration
 * @param freqLBT: See STUHFL_T_ST25RU3993_Freq_LBT struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetFreqLBT(STUHFL_T_ST25RU3993_Freq_LBT *freqLBT);
/**
 * Set continuous modulation configuration
 * @param freqContMod: See STUHFL_T_ST25RU3993_Freq_ContMod struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetFreqContMod(STUHFL_T_ST25RU3993_Freq_ContMod *freqContMod);

/**
 * Get frequency RSSI
 * @param freqRSSI: See STUHFL_T_ST25RU3993_Freq_Rssi struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetFreqRSSI(STUHFL_T_ST25RU3993_Freq_Rssi *freqRSSI);
/**
 * Get reflected power
 * @param freqProfileCustomAppend: See STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetFreqReflectedPower(STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info *freqReflectedPower);

/**
 * Get info of current used profile
 * @param freqContMod: See STUHFL_T_ST25RU3993_Freq_Profile_Info struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV GetFreqProfileInfo(STUHFL_T_ST25RU3993_Freq_Profile_Info *freqProfileInfo);
/**
 * Get frequency channel list
 * @param channelList: See STUHFL_T_ST25RU3993_ChannelList struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetChannelList(STUHFL_T_ST25RU3993_ChannelList *channelList);
/**
 * Get frequency hopping time
 * @param freqHop: See STUHFL_T_ST25RU3993_Freq_Hop struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetFreqHop(STUHFL_T_ST25RU3993_Freq_Hop *freqHop);
/**
 * Get listen before talk configuration
 * @param freqLBT: See STUHFL_T_ST25RU3993_Freq_LBT struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetFreqLBT(STUHFL_T_ST25RU3993_Freq_LBT *freqLBT);

/**
 * Set protocols timings
 * @param gen2Timings: See STUHFL_T_Gen2_Timings struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGen2Timings(STUHFL_T_Gen2_Timings *gen2Timings);
/**
 * Set Gen2 configuration
 * @param gen2ProtocolCfg: See STUHFL_T_ST25RU3993_Gen2Protocol_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGen2ProtocolCfg(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg *gen2ProtocolCfg);
/**
 * Set Gb29768 configuration
 * @param gb29768ProtocolCfg: See STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGb29768ProtocolCfg(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg *gb29768ProtocolCfg);
/**
 * Set TxRx configuration
 * @param txRxCfg: See STUHFL_T_ST25RU3993_TxRx_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTxRxCfg(STUHFL_T_ST25RU3993_TxRx_Cfg *txRxCfg);
/**
 * Set power amplifier configuration
 * @param paCfg: See STUHFL_T_ST25RU3993_PA_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetPA_Cfg(STUHFL_T_ST25RU3993_PA_Cfg *paCfg);
/**
 * Set general Gen2 inventory configuration
 * @param invGen2Cfg: See STUHFL_T_ST25RU3993_Gen2Inventory_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGen2InventoryCfg(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg *invGen2Cfg);
/**
 * Set general Gb29768 inventory configuration
 * @param invGb29768Cfg: See STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGb29768InventoryCfg(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg *invGb29768Cfg);

/**
 * Get protocols timings
 * @param gen2Timings: See STUHFL_T_Gen2_Timings struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGen2Timings(STUHFL_T_Gen2_Timings *gen2Timings);
/**
 * Get Gen2 configuration
 * @param gen2ProtocolCfg: See STUHFL_T_ST25RU3993_Gen2Protocol_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGen2ProtocolCfg(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg *gen2ProtocolCfg);
/**
 * Get Gb29768 configuration
 * @param gb29768ProtocolCfg: See STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGb29768ProtocolCfg(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg *gb29768ProtocolCfg);
/**
 * Get TxRx configuration
 * @param txRxCfg: See STUHFL_T_ST25RU3993_TxRx_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTxRxCfg(STUHFL_T_ST25RU3993_TxRx_Cfg *txRxCfg);
/**
 * Get power amplifier configuration
 * @param paCfg: See STUHFL_T_ST25RU3993_PA_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetPA_Cfg(STUHFL_T_ST25RU3993_PA_Cfg *paCfg);
/**
 * Get general Gen2 inventory configuration
 * @param invGen2Cfg: See STUHFL_T_ST25RU3993_Gen2Inventory_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGen2InventoryCfg(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg *invGen2Cfg);
/**
 * Get general Gb29768 inventory configuration
 * @param invGb29768Cfg: See STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGb29768InventoryCfg(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg *invGb29768Cfg);




// --------------------------------------------------------------------------
// Tuning
// --------------------------------------------------------------------------
/**
 * Set Cin, Clen and Cout
 * @param tuning: See STUHFL_T_ST25RU3993_Tuning struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV SetTuning(STUHFL_T_ST25RU3993_Tuning *tuning);
/**
 * Set Cin, Clen and Cout at given antenna
 * @param tuning: See STUHFL_T_ST25RU3993_TuningCaps struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningCaps(STUHFL_T_ST25RU3993_TuningCaps *tuning);
/**
 * Set tuning table entry
 * @param tuningTableEntry: See STUHFL_T_ST25RU3993_TuningTableEntry struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV SetTuningTableEntry(STUHFL_T_ST25RU3993_TuningTableEntry *tuningTableEntry);
/**
 * Revert to default tuning
 * @param set: See STUHFL_T_ST25RU3993_TunerTableSet struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV SetTuningTableDefault(STUHFL_T_ST25RU3993_TunerTableSet *set);
/**
 * Save current configured tuning table to flash
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV SetTuningTableSave2Flash(void);
/**
 * Delete tuning table entries
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV SetTuningTableEmpty(void);


/**
 * Get Cin, Clen and Cout configuration
 * @param tuning: See STUHFL_T_ST25RU3993_Tuning struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV GetTuning(STUHFL_T_ST25RU3993_Tuning *tuning);
/**
 * Get Cin, Clen and Cout from given antenna
 * @param tuning: See STUHFL_T_ST25RU3993_TuningCaps struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTuningCaps(STUHFL_T_ST25RU3993_TuningCaps *tuning);
/**
 * Get tuning table entry
 * @param tuningTableEntry: See STUHFL_T_ST25RU3993_TuningTableEntry struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV GetTuningTableEntry(STUHFL_T_ST25RU3993_TuningTableEntry *tuningTableEntry);
/**
 * Set current tuning table info
 * @param tuningTableInfo: See STUHFL_T_ST25RU3993_TuningTableInfo struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV GetTuningTableInfo(STUHFL_T_ST25RU3993_TuningTableInfo *tuningTableInfo);

/**
 * Start tuning
 * @param tune: See STUHFL_T_ST25RU3993_Tune struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_DEPRECATED STUHFL_T_RET_CODE CALL_CONV Tune(STUHFL_T_ST25RU3993_Tune *tune);
/**
 * Start tuning
 * @param tuneCfg: See STUHFL_T_ST25RU3993_Tune struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV TuneChannel(STUHFL_T_ST25RU3993_TuneCfg *tuneCfg);



// --------------------------------------------------------------------------
// Gen2
// --------------------------------------------------------------------------
/**
 * Perform Gen2 Inventory depending on the current inventory and gen2 configuration
 * @param invOption: See STUHFL_T_Inventory_Option struct for further info
 * @param invData: See STUHFL_T_Inventory_Data struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Inventory(STUHFL_T_Inventory_Option *invOption, STUHFL_T_Inventory_Data *invData);
/**
 * Perform Gen2 Select command to select or filter transponders
 * @param selData: See STUHFL_T_Gen2_Select struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Select(STUHFL_T_Gen2_Select *selData);
/**
 * Perform Gen2 Read command to read from the Gen2 transponders
 * @param readData: See STUHFL_T_Read struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Read(STUHFL_T_Read *readData);
/**
 * Perform Gen2 Write command to write data to Gen2 transponders
 * @param writeData: See STUHFL_T_Write struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Write(STUHFL_T_Write *writeData);
/**
 * Perform Gen2 Block Write command to write block data to Gen2 transponders
 * @param writeData: See STUHFL_T_BlockWrite struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_BlockWrite(STUHFL_T_BlockWrite *blockWrite);
/**
 * Perform Gen2 Lock command to lock transponders
 * @param lockData: See STUHFL_T_Gen2_Lock struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Lock(STUHFL_T_Gen2_Lock *lockData);
/**
 * Perform Gen2 Kill command to kill transponders.
 * NOTE: After this command your transponders will not be functional anymore
 * @param killData: See STUHFL_T_Kill struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_Kill(STUHFL_T_Kill *killData);
/**
 * Perform generic Gen2 bit exchange
 * @param genericCmdSnd: See STUHFL_T_Gen2_GenericCmdSnd struct for further info
 * @param genericCmdRcv: See STUHFL_T_Gen2_GenericCmdRcv struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_GenericCmd(STUHFL_T_Gen2_GenericCmdSnd *genericCmdSnd, STUHFL_T_Gen2_GenericCmdRcv *genericCmdRcv);
/**
 * Perform RSSI measurement during Gen2 Query command
 * @param queryMeasureRssi: See STUHFL_T_Gen2_QueryMeasureRssi struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gen2_QueryMeasureRssi(STUHFL_T_Gen2_QueryMeasureRssi *queryMeasureRssi);


// --------------------------------------------------------------------------
// GB29768
// --------------------------------------------------------------------------
/**
 * Perform GB29768 Inventory depending on the current inventory and GB29768 configuration
 * @param invOption: See STUHFL_T_Inventory_Option struct for further info
 * @param invData: See STUHFL_T_Inventory_Data struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Inventory(STUHFL_T_Inventory_Option *invOption, STUHFL_T_Inventory_Data *invData);
/**
 * Perform Gb29768 Sort command to select or filter transponders
 * Nota: If Sort defined on Matching flag, inventories and tag accesses are based on the assumption
 * the matching flags are only '1'
 * as a consequence the rule GB29768_RULE_MATCH0_ELSE_1 (0x03) is to be used to invert selection
 *
 * @param sortData: See STUHFL_T_Gb29768_Sort struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Sort(STUHFL_T_Gb29768_Sort *sortData);
/**
 * Perform Gb29768 Read command to read from the Gb29768 transponders
 * @param readData: See STUHFL_T_Read struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Read(STUHFL_T_Read *readData);
/**
 * Perform Gb29768 Write command to write data to Gb29768 transponders
 * @param writeData: See STUHFL_T_Write struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Write(STUHFL_T_Write *writeData);
/**
 * Perform Gb29768 Lock command to lock transponders
 * @param lockData: See STUHFL_T_Gb29768_Lock struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Lock(STUHFL_T_Gb29768_Lock *lockData);
/**
 * Perform Gb29768 Kill command to kill transponders.
 * NOTE: After this command your transponders will not be functional anymore
 * @param killData: See STUHFL_T_Kill struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Kill(STUHFL_T_Kill *killData);
/**
 * Perform Gb29768 Erase command to erase transponders
 * @param lockData: See STUHFL_T_Gb29768_Erase struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Gb29768_Erase(STUHFL_T_Gb29768_Erase *eraseData);


// --------------------------------------------------------------------------
// Inventory Runner
// --------------------------------------------------------------------------
/**
 * Function definition for callback when a transponder is found during inventory.
 * This callback is executed whenever a transponder is detected during the inventory
 * round.
 *
 * @param *data: inventory data of transponder that is given back in the callback with
 *               information about the transponder and the current reader status.
 *               See STUHFL_T_Inventory_Data struct for further info.
 *
 * @return error code
*/
typedef STUHFL_T_RET_CODE(*STUHFL_T_InventoryCycle)(STUHFL_T_Inventory_Data *data);

/**
 * Function definition for callback when inventory is finished.
 * This callback is executed after all inventory ounds are executed.
 *
 * @param *data: inventory data of transponder that is given back in the callback with
 *               information about the transponder and the current reader status.
 *               See STUHFL_T_Inventory_Data struct for further info.
 *
 * @return error code
*/
typedef STUHFL_T_RET_CODE(*STUHFL_T_InventoryFinished)(STUHFL_T_Inventory_Data *data);
/**
 * Start inventory runner
 * @param *option: set runner options. NOTE:
 *                 See STUHFL_T_Inventory_Option struct for further info
 * @param cycleCallback: callback when a transponder is found during inventory
 * @param finishedCallback: callback when the inventory is finished
 * @param *data: inventory data of transponder that is given back in the callback with
 *               information about the transponder and the current reader status.
 *               See STUHFL_T_Inventory_Data struct for further info.
 *
 * NOTE: Starting the inventory runner is a blocking call. Whenever
 * transponders found the STUHFL_T_InventoryCycle callback is called.
 * Within this callback processing of the data should take place.
 *
 * To stop the runner and return from the InventoryRunnerStart function
 * there are 2 possibilities.
 *   1. A call to InventoryRunnerStop. Can be called from the callback
 *   2. When the number of requested rounds are executed
 *
 * @return error code
 */
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV InventoryRunnerStart(STUHFL_T_Inventory_Option *option, STUHFL_T_InventoryCycle cycleCallback, STUHFL_T_InventoryFinished finishedCallback, STUHFL_T_Inventory_Data *data);

#ifdef USE_INVENTORY_EXT
/**
 * Start Extended inventory runner (Inventory runner data + slot info)
 * @param *option: set runner options. NOTE:
 *                 See STUHFL_T_Inventory_Option struct for further info
 * @param cycleCallback: callback when a transponder is found during inventory
 * @param finishedCallback: callback when the inventory is finished
 * @param *data: inventory data of transponder that is given back in the callback with
 *               information about the transponder and the current reader status plus the slot info.
 *               See STUHFL_T_Inventory_Data_Ext struct for further info.
 *
 * NOTE: Starting the inventory runner is a blocking call. Whenever
 * transponders found the STUHFL_T_InventoryCycle callback is called.
 * Within this callback processing of the data should take place.
 *
 * To stop the runner and return from the InventoryRunnerStartExt function
 * there are 2 possibilities.
 *   1. A call to InventoryRunnerStop. Can be called from the callback
 *   2. When the number of requested rounds are executed
 *
 * @return error code
 */
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV InventoryRunnerStartExt(STUHFL_T_Inventory_Option *option, STUHFL_T_InventoryCycle cycleCallback, STUHFL_T_InventoryFinished finishedCallback, STUHFL_T_Inventory_Data_Ext *data);
#endif  // USE_INVENTORY_EXT

/**
 * Stop current inventory runner
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV InventoryRunnerStop(void);



#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __STUHFL_EVALAPI_H
