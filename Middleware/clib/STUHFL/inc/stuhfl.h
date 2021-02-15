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
#if !defined __STUHFL_H
#define __STUHFL_H

//
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
//
#include "stuhfl_platform.h"

// --------------------------------------------------------------------------
// Prefix explanation
// --------------------------------------------------------------------------
// STUHFL_T_                            .. STUHLF typedef
// STUHFL_D_                            .. STUHLF define
// STUHFL_E_                            .. STUHLF enum
// STUHFL_F_                            .. STUHLF function
// STUHFL_O_                            .. STUHLF object
// STUHFL_P_                            .. STUHLF pointer





//
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus



// --------------------------------------------------------------------------
#ifdef WIN64
typedef uint64_t                                STUHFL_T_POINTER2UINT;
#else
typedef uint32_t                                STUHFL_T_POINTER2UINT;
#endif

typedef uint32_t                                STUHFL_T_RET_CODE;
typedef uint32_t                                STUHFL_T_RESET;
typedef uint32_t                                STUHFL_T_VERSION;
typedef void*                                   STUHFL_T_DEVICE_CTX;
typedef void*                                   STUHFL_T_PARAM_VALUE;
typedef void*                                   STUHFL_T_PARAM_INFO;
typedef uint32_t                                STUHFL_T_PARAM;
typedef uint32_t                                STUHFL_T_PARAM_CNT;
typedef uint32_t                                STUHFL_T_TYPE;
typedef uint16_t                                STUHFL_T_CMD;
typedef void*                                   STUHFL_T_CMD_SND_PARAMS;
typedef void*                                   STUHFL_T_CMD_RCV_DATA;
typedef uint8_t                                 STUHFL_T_TAG;
typedef uint8_t                                 STUHFL_T_LEN8;
typedef uint16_t                                STUHFL_T_LEN16;
#define STUHFL_T_LEN_LENGTH(l)                  (uint32_t)((l & 0x80) ? sizeof(STUHFL_T_LEN16) : sizeof(STUHFL_T_LEN8))

// - Parameter Types & Keys -----------------------------------------------
#define STUHFL_PARAM_TYPE_MASK                  0xFF000000
#define STUHFL_PARAM_KEY_MASK                   0x00FFFFFF

// TYPES
#define STUHFL_PARAM_TYPE_CONNECTION            0x01000000
#define STUHFL_PARAM_TYPE_ST25RU3993            0x02000000

// KEYS CONNECTION
#define STUHFL_PARAM_KEY_PORT                   0x00000001
#define STUHFL_PARAM_KEY_BR                     0x00000002
#define STUHFL_PARAM_KEY_RD_TIMEOUT_MS          0x00000003
#define STUHFL_PARAM_KEY_WR_TIMEOUT_MS          0x00000004
#define STUHFL_PARAM_KEY_DTR                    0x00000005
#define STUHFL_PARAM_KEY_RTS                    0x00000006


// KEYS ST25RU3993
#define STUHFL_PARAM_KEY_VERSION_FW             0x00000001
#define STUHFL_PARAM_KEY_VERSION_HW             0x00000002
// - HW Cfg
#define STUHFL_PARAM_KEY_RWD_REGISTER           0x00000010
#define STUHFL_PARAM_KEY_RWD_CONFIG             0x00000011
// - Power
#define STUHFL_PARAM_KEY_RWD_ANTENNA_POWER      0x00000012
// - Freq
#define STUHFL_PARAM_KEY_RWD_FREQ_RSSI          0x00000013          /* DEPRECATED */
#define STUHFL_PARAM_KEY_RWD_FREQ_REFLECTED     0x00000014          /* DEPRECATED */
#define STUHFL_PARAM_KEY_RWD_FREQ_PROFILE       0x00000015          /* DEPRECATED */
#define STUHFL_PARAM_KEY_RWD_FREQ_PROFILE_ADD2CUSTOM    0x00000016  /* DEPRECATED */
#define STUHFL_PARAM_KEY_RWD_FREQ_PROFILE_INFO  0x00000017          /* DEPRECATED */
#define STUHFL_PARAM_KEY_RWD_FREQ_HOP           0x00000018
#define STUHFL_PARAM_KEY_RWD_FREQ_LBT           0x00000019
#define STUHFL_PARAM_KEY_RWD_FREQ_CONT_MOD      0x0000001A
#define STUHFL_PARAM_KEY_RWD_TXRX_CFG           0x0000001B
#define STUHFL_PARAM_KEY_RWD_PA_CFG             0x0000001C
#define STUHFL_PARAM_KEY_RWD_CHANNEL_LIST       0x0000001D
//- Gen2 SW Cfg
#define STUHFL_PARAM_KEY_RWD_GEN2PROTOCOL_CFG   0x00000020
#define STUHFL_PARAM_KEY_RWD_GEN2INVENTORY_CFG  0x00000021
#define STUHFL_PARAM_KEY_RWD_GEN2TIMINGS        0x00000022
//- Gb29768 SW Cfg
#define STUHFL_PARAM_KEY_RWD_GB29768PROTOCOL_CFG        0x00000028
#define STUHFL_PARAM_KEY_RWD_GB29768INVENTORY_CFG       0x00000029

// - Tuner
#define STUHFL_PARAM_KEY_TUNING                 0x00000030          /* DEPRECATED */
#define STUHFL_PARAM_KEY_TUNING_TABLE_ENTRY     0x00000031          /* DEPRECATED */
#define STUHFL_PARAM_KEY_TUNING_TABLE_DEFAULT   0x00000032          /* DEPRECATED */
#define STUHFL_PARAM_KEY_TUNING_TABLE_SAVE      0x00000033          /* DEPRECATED */
#define STUHFL_PARAM_KEY_TUNING_TABLE_EMPTY     0x00000034          /* DEPRECATED */
#define STUHFL_PARAM_KEY_TUNING_TABLE_INFO      0x00000035          /* DEPRECATED */
//#define STUHFL_PARAM_KEY_TUNE                   0x00000036
#define STUHFL_PARAM_KEY_TUNING_CAPS            0x00000037

// - Gen2
// - Iso6B
// - GB29768

// Command Group
#define STUHFL_CG_GENERIC                                   0x00
#define STUHFL_CG_DL                                        0x01
#define STUHFL_CG_SL                                        0x02
#define STUHFL_CG_AL                                        0x03
#define STUHFL_CG_TS                                        0x04
// --------------------------------------------------------------------------
// Command Code - Generic
#define STUHFL_CC_GET_VERSION                               0x00
#define STUHFL_CC_GET_INFO                                  0x01
#define STUHFL_CC_UPGRADE                                   0x02
#define STUHFL_CC_ENTER_BOOTLOADER                          0x03
#define STUHFL_CC_REBOOT                                    0x04
// Command Code - Unit Layer
#define STUHFL_CC_GET_PARAM                                 0x00
#define STUHFL_CC_SET_PARAM                                 0x01
#define STUHFL_CC_TUNE                                      0x02
#define STUHFL_CC_TUNE_CHANNEL                              0x03
// Command Code - Session Layer
// Command Code - Activity Layer
#define STUHFL_CC_INVENTORY_START                           0x00
#define STUHFL_CC_INVENTORY_STOP                            0x01
#define STUHFL_CC_INVENTORY_DATA                            0x02
#ifdef USE_INVENTORY_EXT
#define STUHFL_CC_INVENTORY_START_W_SLOT_STATISTICS         0x03
#endif

#define STUHFL_CC_GEN2_INVENTORY                            0x03
#define STUHFL_CC_GEN2_SELECT                               0x04
#define STUHFL_CC_GEN2_READ                                 0x05
#define STUHFL_CC_GEN2_WRITE                                0x06
#define STUHFL_CC_GEN2_LOCK                                 0x07
#define STUHFL_CC_GEN2_KILL                                 0x08
#define STUHFL_CC_GEN2_GENERIC_CMD                          0x09
#define STUHFL_CC_GEN2_QUERY_MEASURE_RSSI_CMD               0x0A
#define STUHFL_CC_GEN2_BLOCKWRITE                           0x0B

#define STUHFL_CC_GB29768_INVENTORY                         0x13
#define STUHFL_CC_GB29768_SORT                              0x14
#define STUHFL_CC_GB29768_READ                              0x15
#define STUHFL_CC_GB29768_WRITE                             0x16
#define STUHFL_CC_GB29768_LOCK                              0x17
#define STUHFL_CC_GB29768_KILL                              0x18
#define STUHFL_CC_GB29768_GENERIC_CMD                       0x19
#define STUHFL_CC_GB29768_ERASE                             0x1A


// TLV TAGs - Generic
#define STUHFL_TAG_VERSION_FW                               0x01
#define STUHFL_TAG_VERSION_HW                               0x02
#define STUHFL_TAG_INFO_FW                                  0x03
#define STUHFL_TAG_INFO_HW                                  0x04
// TLV TAGs - Unit Layer
#define STUHFL_TAG_REGISTER                                 0x01
#define STUHFL_TAG_RWD_CONFIG                               0x02
#define STUHFL_TAG_ANTENNA_POWER                            0x03
#define STUHFL_TAG_FREQ_RSSI                                0x04
#define STUHFL_TAG_FREQ_REFLECTED                           0x05
#define STUHFL_TAG_FREQ_PROFILE                             0x06
#define STUHFL_TAG_FREQ_PROFILE_ADD2CUSTOM                  0x07
#define STUHFL_TAG_FREQ_PROFILE_INFO                        0x08
#define STUHFL_TAG_FREQ_HOP                                 0x09
#define STUHFL_TAG_FREQ_LBT                                 0x0A
#define STUHFL_TAG_FREQ_CONT_MOD                            0x0B
#define STUHFL_TAG_GEN2PROTOCOL_CFG                         0x0C
#define STUHFL_TAG_GB29768PROTOCOL_CFG                      0x0D
#define STUHFL_TAG_TXRX_CFG                                 0x0E
#define STUHFL_TAG_PA_CFG                                   0x0F
#define STUHFL_TAG_GEN2INVENTORY_CFG                        0x10
#define STUHFL_TAG_GB29768INVENTORY_CFG                     0x11
#define STUHFL_TAG_TUNING                                   0x12
#define STUHFL_TAG_TUNING_TABLE_DEFAULT                     0x13
#define STUHFL_TAG_TUNING_TABLE_ENTRY                       0x14
#define STUHFL_TAG_TUNING_TABLE_INFO                        0x15
#define STUHFL_TAG_TUNING_TABLE_SAVE                        0x16
#define STUHFL_TAG_TUNING_TABLE_EMPTY                       0x17
#define STUHFL_TAG_TUNE                                     0x18
#define STUHFL_TAG_GEN2TIMINGS                              0x19
#define STUHFL_TAG_TUNE_CHANNEL                             0x20
#define STUHFL_TAG_TUNING_CAPS                              0x21
#define STUHFL_TAG_CHANNEL_LIST                             0x22
// TLV TAGs - Session Layer
#define STUHFL_TAG_GEN2_INVENTORY_OPTION                    0x01
#define STUHFL_TAG_GEN2_INVENTORY                           0x02
#define STUHFL_TAG_GEN2_INVENTORY_TAG                       0x03
#define STUHFL_TAG_GEN2_SELECT                              0x04
#define STUHFL_TAG_GEN2_READ                                0x05
#define STUHFL_TAG_GEN2_WRITE                               0x06
#define STUHFL_TAG_GEN2_LOCK                                0x07
#define STUHFL_TAG_GEN2_KILL                                0x08
#define STUHFL_TAG_GEN2_GENERIC                             0x09
#define STUHFL_TAG_GEN2_QUERY_MEASURE_RSSI                  0x0A
#define STUHFL_TAG_GEN2_BLOCKWRITE                          0x0B

#define STUHFL_TAG_GB29768_INVENTORY_OPTION                 STUHFL_TAG_GEN2_INVENTORY_OPTION
#define STUHFL_TAG_GB29768_SORT                             0x14
#define STUHFL_TAG_GB29768_READ                             0x15
#define STUHFL_TAG_GB29768_WRITE                            0x16
#define STUHFL_TAG_GB29768_LOCK                             0x17
#define STUHFL_TAG_GB29768_KILL                             0x18
#define STUHFL_TAG_GB29768_GENERIC                          0x19
#define STUHFL_TAG_GB29768_ERASE                            0x1A

// TLV TAGs - Activity Layer
#define STUHFL_TAG_INVENTORY_OPTION                         0x01
#define STUHFL_TAG_INVENTORY_STATISTICS                     0x02
#define STUHFL_TAG_INVENTORY_TAG_INFO_HEADER                0x03
#define STUHFL_TAG_INVENTORY_TAG_EPC                        0x04
#define STUHFL_TAG_INVENTORY_TAG_TID                        0x05
#define STUHFL_TAG_INVENTORY_TAG_XPC                        0x06
#define STUHFL_TAG_INVENTORY_TAG_FINISHED                   0xFF

#define STUHFL_TAG_INVENTORY_SLOT_INFO_SYNC                 0x07
#define STUHFL_TAG_INVENTORY_SLOT_INFO                      0x08

//
#define STUHFL_TYPE_UINT8                                   0
#define STUHFL_TYPE_UINT16                                  1
#define STUHFL_TYPE_UINT32                                  2
#define STUHFL_TYPE_INT8                                    3
#define STUHFL_TYPE_INT16                                   4
#define STUHFL_TYPE_INT32                                   5
#define STUHFL_TYPE_BOOL                                    6
#define STUHFL_TYPE_ST25RU3993_ACCESS_REGISTER              7
//..

#pragma pack(push, 1)

typedef struct {
    uint8_t                             major;      /**< O Param: Major version information */
    uint8_t                             minor;      /**< O Param: Minor version information */
    uint8_t                             micro;      /**< O Param: Micro version information */
    uint8_t                             build;      /**< O Param: Build version information */
} STUHFL_T_Version;
#define STUHFL_O_VERSION_INIT(...) ((STUHFL_T_Version) { .major = 0, .minor = 0, .micro = 0, .build = 0, ##__VA_ARGS__ })

#define MAX_VERSION_INFO_LENGTH                             64U
typedef struct {
    char                                info[MAX_VERSION_INFO_LENGTH];  /**< O Param: Readable version information string. Zero terminated */
    uint8_t                             infoLength;                     /**< O Param: Readable version information string. Zero terminated */
} STUHFL_T_Version_Info;
#define STUHFL_O_VERSION_INFO_INIT(...) ((STUHFL_T_Version_Info) { .info = {0}, .infoLength = 0, ##__VA_ARGS__ })

typedef struct {
    STUHFL_T_TYPE                       type;       /**< O Param: Type of the data (UINT8, 32, BOOL, REG, ...) */
    uint32_t                            size_of;    /**< O Param: Size in bytes of the data */
    bool                                canRd;      /**< O Param: Data can be read */
    bool                                canWr;      /**< O Param: Data can be written */
} STUHFL_T_Param_Info;
#define STUHFL_O_PARAM_INFO_INIT(...) ((STUHFL_T_Param_Info) { .type = (STUHFL_T_TYPE)STUHFL_TYPE_UINT32, .size_of = sizeof(STUHFL_T_ParamTypeUINT32), .canRd = true, .canWr = true, ##__VA_ARGS__ })

#pragma pack(pop)


// --------------------------------------------------------------------------
typedef uint8_t                         STUHFL_T_ParamTypeUINT8;
typedef uint16_t                        STUHFL_T_ParamTypeUINT16;
typedef uint32_t                        STUHFL_T_ParamTypeUINT32;
typedef char*                           STUHFL_T_ParamTypeConnectionPort;
typedef uint32_t                        STUHFL_T_ParamTypeConnectionBR;
typedef uint32_t                        STUHFL_T_ParamTypeConnectionRdTimeout;
typedef uint32_t                        STUHFL_T_ParamTypeConnectionWrTimeout;

// --------------------------------------------------------------------------
#define DISCOVERY_HW_ID                 1
#define EVAL_HW_ID                      2
#define JIGEN_HW_ID                     3
#define ELANCE_HW_ID                    4


// --------------------------------------------------------------------------
/**
 * Reply version information of STUHFL
 *
 * @return version number
*/
STUHFL_DLL_API STUHFL_T_VERSION CALL_CONV STUHFL_F_GetLibVersion(void);



#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __STUHFL_H
