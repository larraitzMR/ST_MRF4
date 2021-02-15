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
#if !defined __STUHFL_SL_GEN2_H
#define __STUHFL_SL_GEN2_H

#include "stuhfl.h"
#include "stuhfl_sl.h"


#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

// - GEN2 -------------------------------------------------------------------

#pragma pack(push, 1)
#define GEN2_SELECT_MODE_CLEAR_LIST             0
#define GEN2_SELECT_MODE_ADD2LIST               1
#define GEN2_SELECT_MODE_CLEAR_AND_ADD          2

#define GEN2_TARGET_S0                  0
#define GEN2_TARGET_S1                  1
#define GEN2_TARGET_S2                  2
#define GEN2_TARGET_S3                  3
#define GEN2_TARGET_SL                  4

#define GEN2_MEMORY_BANK_RESERVED       0
#define GEN2_MEMORY_BANK_EPC            1
#define GEN2_MEMORY_BANK_TID            2
#define GEN2_MEMORY_BANK_USER           3

typedef struct {
    uint8_t                             mode;           /**< I Param: Select mode to be applied (CLEAR_LIST, ADD2LIST, CLEAR_AND_ADD). */
    uint8_t                             target;         /**< I Param: indicates whether the select modifies a tag's SL flag or its inventoried flag. */
    uint8_t                             action;         /**< I Param: Elicit the tag behavior according to Gen2 Select specification. */
    uint8_t                             memBank;        /**< I Param: Bank (File, EPC, TID, USER) on which apply the select. */
    uint8_t                             mask[32];       /**< I Param: Selection mask. */
    uint32_t                            maskAddress;    /**< I Param: Starting address to which mask is applied. */
    uint8_t                             maskLen;        /**< I Param: Mask length. */
    uint8_t                             truncation;     /**< I Param: Truncate enabling. */
} STUHFL_T_Gen2_Select;
#define STUHFL_O_GEN2_SELECT_INIT(...) ((STUHFL_T_Gen2_Select) { .mode = GEN2_SELECT_MODE_CLEAR_LIST, .target = GEN2_TARGET_S0, .action = 0, .memBank = GEN2_MEMORY_BANK_EPC, .mask = {0}, .maskAddress = 0, .maskLen = 0, .truncation = false, ##__VA_ARGS__ })

typedef STUHFL_T_Read   STUHFL_T_Gen2_Read;
typedef STUHFL_T_Write  STUHFL_T_Gen2_Write;

typedef struct {
    uint8_t                             mask[3];                    /**< I Param: Mask and actions field. */
    uint8_t                             pwd[PASSWORD_LEN];          /**< I Param: Password. */
    uint8_t                             tagReply;                   /**< O Param: Tag reply. */
} STUHFL_T_Gen2_Lock;
#define STUHFL_O_GEN2_LOCK_INIT(...) ((STUHFL_T_Gen2_Lock) { .mask = {0}, .pwd = {0}, .tagReply = 0, ##__VA_ARGS__ })

typedef STUHFL_T_Kill  STUHFL_T_Gen2_Kill;

#define GENERIC_CMD_TRANSMCRC           0x90
#define GENERIC_CMD_TRANSMCRCEHEAD      0x91
#define GENERIC_CMD_TRANSM              0x92

#define GEN2_GENERIC_CMD_MAX_SND_DATA_BYTES     (512/8)
typedef struct {
    uint8_t                             pwd[PASSWORD_LEN];          /**< I Param: Password. */
    uint8_t                             cmd;                        /**< I Param: Generic command. */
    uint8_t                             noResponseTime;             /**< I Param: Tag response timeout. */
    uint16_t                            expectedRcvDataBitCnt;      /**< I Param: Size in bits of expected received data. NOTE: For the direct commands 0x90 and 0x91 (Tranmission with CRC) CRC is handled by HW and need not to be included into the expected bit count. The received CRC will also not replied to the host. When using command 0x92 handling of any data integrity checking must be done manually.*/
    uint16_t                            sndDataBitCnt;              /**< I Param: Size in bits of data sent to Tag. */
    bool                                appendRN16;                 /**< I Param: Append tag handle to generic command. */
    uint8_t                             sndData[GEN2_GENERIC_CMD_MAX_SND_DATA_BYTES];   /**< I Param: Data being sent to Tag. */
} STUHFL_T_Gen2_GenericCmdSnd;
#define STUHFL_O_GEN2_GENERICCMDSND_INIT(...) ((STUHFL_T_Gen2_GenericCmdSnd) { .pwd = {0}, .cmd = 0, .noResponseTime = 0, .expectedRcvDataBitCnt = 0, .sndDataBitCnt = 0, .appendRN16 = true, .sndData = {0}, ##__VA_ARGS__ })

#define GEN2_GENERIC_CMD_MAX_RCV_DATA_BYTES     (64)
typedef struct {
    uint16_t                            rcvDataByteCnt;              /**< O Param: Size in bytes of received data from Tag. */
    uint8_t                             rcvData[GEN2_GENERIC_CMD_MAX_RCV_DATA_BYTES];   /**< O Param: Data received from Tag. */
} STUHFL_T_Gen2_GenericCmdRcv;
#define STUHFL_O_GEN2_GENERICCMDRCV_INIT(...) ((STUHFL_T_Gen2_GenericCmdRcv) { .rcvDataByteCnt = 0, .rcvData = {0}, ##__VA_ARGS__ })

typedef struct {
    uint32_t                            frequency;                  /**< I Param: Frequency. */
    uint8_t                             measureCnt;                 /**< I Param: Number of measures. */
    uint8_t                             agc[256];                   /**< O Param: AGC. */
    uint8_t                             rssiLogI[256];              /**< O Param: RSSI log. */
    uint8_t                             rssiLogQ[256];              /**< O Param: RSSI log. */
    int8_t                              rssiLinI[256];              /**< O Param: RSSI I Level. */
    int8_t                              rssiLinQ[256];              /**< O Param: RSSI Q Level. */
} STUHFL_T_Gen2_QueryMeasureRssi;
#define STUHFL_O_GEN2_QUERYMEASURERSSI_INIT(...) ((STUHFL_T_Gen2_QueryMeasureRssi) { .frequency = 0, .measureCnt = 0, .agc = {0}, .rssiLogI = {0}, .rssiLogQ = {0}, .rssiLinI = {0}, .rssiLinQ = {0}, ##__VA_ARGS__ })

#define TIMING_USE_DEFAULT_T4MIN        0       /**< T4 Min default value will be used (Gen2: 2*3*Tari: 75us @TARI_12_50). */
typedef struct {
    uint16_t                            T4Min;              /**< I/O Param: T4 minimum value in us. */
} STUHFL_T_Gen2_Timings;
#define STUHFL_O_GEN2_TIMINGS_INIT(...) ((STUHFL_T_Gen2_Timings) { .T4Min = TIMING_USE_DEFAULT_T4MIN, ##__VA_ARGS__ })

#pragma pack(pop)

// --------------------------------------------------------------------------
/**
 * Perform Gen2 Inventory depending on the current inventory and gen2 configuration
 * @param invOption: See STUHFL_T_Inventory_Option struct for further info
 * @param invData: See STUHFL_T_Inventory_Data struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gen2_Inventory(STUHFL_T_Inventory_Option *invOption, STUHFL_T_Inventory_Data *invData);
/**
 * Perform Gen2 Select command to select or filter transponders
 * @param selData: See STUHFL_T_Gen2_Select struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gen2_Select(STUHFL_T_Gen2_Select *selData);
/**
 * Perform Gen2 Read command to read from the Gen2 transponders
 * @param readData: See STUHFL_T_Read struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gen2_Read(STUHFL_T_Read *readData);
/**
 * Perform Gen2 Write command to write data to Gen2 transponders
 * @param writeData: See STUHFL_T_Write struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gen2_Write(STUHFL_T_Write *writeData);
/**
 * Perform Gen2 Block Write command to write block data to Gen2 transponders
 * @param blockWrite: See STUHFL_T_BlockWrite struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gen2_BlockWrite(STUHFL_T_BlockWrite *blockWrite);
/**
 * Perform Gen2 Lock command to lock transponders
 * @param lockData: See STUHFL_T_Gen2_Lock struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gen2_Lock(STUHFL_T_Gen2_Lock *lockData);
/**
 * Perform Gen2 Kill command to kill transponders.
 * NOTE: After this command your transponders will not be functional anymore
 * @param killData: See STUHFL_T_Kill struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gen2_Kill(STUHFL_T_Kill *killData);
/**
 * Perform generic Gen2 bit exchange
 * @param genericCmdDataSnd: See STUHFL_T_Gen2_GenericCmdSnd struct for further info
 * @param genericCmdDataRcv: See STUHFL_T_Gen2_GenericCmdRcv struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gen2_GenericCmd(STUHFL_T_Gen2_GenericCmdSnd *genericCmdSnd, STUHFL_T_Gen2_GenericCmdRcv *genericCmdRcv);
/**
 * Perform RSSI measurement during Gen2 Query commmand
 * @param queryMeasureRssi: See STUHFL_T_Gen2_QueryMeasureRssi struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gen2_QueryMeasureRssi(STUHFL_T_Gen2_QueryMeasureRssi *queryMeasureRssi);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __STUHFL_SL_GEN2_H
