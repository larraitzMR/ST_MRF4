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
#if !defined __STUHFL_SL_H
#define __STUHFL_SL_H

#include "stuhfl.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#pragma pack(push, 1)

//
#define MAX_TAGS_PER_ROUND         4096U
#define MAX_TID_LENGTH             12U
#define MAX_PC_LENGTH              2U
#define MAX_EPC_LENGTH             32U
#define MAX_XPC_LENGTH             4U            /**< XPC_W1 + XPC_W2.*/

//
#define RSSI_MODE_REALTIME              0x00
#define RSSI_MODE_PILOT                 0x04
#define RSSI_MODE_2NDBYTE               0x06
#define RSSI_MODE_PEAK                  0x08

#define INVENTORYREPORT_EACHROUND       0x01        /**< This bitfield is IGNORED (backward compatibility) and is always considered as set: sends statistics after every ROUND (or within round if inventoried tags within same round exceed 64 tags). */
#define INVENTORYREPORT_HEART_BEAT_DURATION_MS      400
#define INVENTORYREPORT_HEARTBEAT       0x02        /**< Bitfield enabling the sending of inventory heart beat (400ms pace) containing generic statistics only. */

typedef struct {
    uint8_t                             rssiMode;                       /**< I Param: RSSI mode. This defines what RSSI value is sent to the host along with the tag data.*/
    uint32_t                            roundCnt;                       /**< I Param: Inventory scan duration (in rounds). Set to 0 for infinity scanning. */
    uint16_t                            inventoryDelay;                 /**< I/O Param: additional delay that can be added for the each inventory round. */
    uint8_t                             reportOptions;                  /**< I/O Param: Defines Inventory data report scheme (Round/Slot, HeartBeat). */
} STUHFL_T_Inventory_Option;
#define STUHFL_O_INVENTORY_OPTION_INIT(...) ((STUHFL_T_Inventory_Option) { .rssiMode = RSSI_MODE_2NDBYTE, .roundCnt = 0, .inventoryDelay = 0, .reportOptions = 0x00, ##__VA_ARGS__ })

//
typedef struct {
    uint8_t                             len;                            /**< O Param: length of EPC data. */
    uint8_t                             data[MAX_EPC_LENGTH];           /**< O Param: EPC data. */
} STUHFL_T_Inventory_Tag_EPC;
#define STUHFL_O_INVENTORY_TAG_EPC_INIT(...) ((STUHFL_T_Inventory_Tag_EPC) { .len = 0, .data = {0}, ##__VA_ARGS__ })

typedef struct {
    uint8_t                             len;                            /**< O Param: length of XPC data. */
    uint8_t                             data[MAX_XPC_LENGTH];           /**< O Param: XPC data. */
} STUHFL_T_Inventory_Tag_XPC;
#define STUHFL_O_INVENTORY_TAG_XPC_INIT(...) ((STUHFL_T_Inventory_Tag_XPC) { .len = 0, .data = {0}, ##__VA_ARGS__ })

typedef struct {
    uint8_t                             len;                            /**< O Param: length of TID data. */
    uint8_t                             data[MAX_TID_LENGTH];           /**< O Param: TID data. */
} STUHFL_T_Inventory_Tag_TID;
#define STUHFL_O_INVENTORY_TAG_TID_INIT(...) ((STUHFL_T_Inventory_Tag_TID) { .len = 0, .data = {0}, ##__VA_ARGS__ })

// --------------------------------------------------------------------------
typedef struct {
    uint32_t                            timestamp;                      /**< O Param: Tag detection time stamp. */
    uint8_t                             antenna;                        /**< O Param: Antenna at which Tag was detected. */
    uint8_t                             agc;                            /**< O Param: Tag AGC. */
    uint8_t                             rssiLogI;                       /**< O Param: I part of Tag RSSI log. */
    uint8_t                             rssiLogQ;                       /**< O Param: Q part of Tag RSSI log. */
    int8_t                              rssiLinI;                       /**< O Param: Tag RSSI I level. */
    int8_t                              rssiLinQ;                       /**< O Param: Tag RSSI Q level. */
    uint8_t                             pc[MAX_PC_LENGTH];              /**< O Param: Tag PC. */
    STUHFL_T_Inventory_Tag_XPC          xpc;                            /**< O Param: Tag XPC. */
    STUHFL_T_Inventory_Tag_EPC          epc;                            /**< O Param: Tag EPC. */
    STUHFL_T_Inventory_Tag_TID          tid;                            /**< O Param: Tag TID. */
} STUHFL_T_Inventory_Tag;
#define STUHFL_O_INVENTORY_TAG_INIT(...) ((STUHFL_T_Inventory_Tag) { .timestamp = 0, \
                                                                .antenna = ANTENNA_1, .agc = 0, .rssiLogI = 0, .rssiLogQ = 0, .rssiLinI = 0, .rssiLinQ = 0, \
                                                                .pc = {0}, \
                                                                .xpc = STUHFL_O_INVENTORY_TAG_XPC_INIT(), \
                                                                .epc = STUHFL_O_INVENTORY_TAG_EPC_INIT(), \
                                                                .tid = STUHFL_O_INVENTORY_TAG_TID_INIT(), \
                                                                ##__VA_ARGS__ })

#define TUNING_STATUS_UNTUNED           0
#define TUNING_STATUS_TUNING            1
#define TUNING_STATUS_TUNED             2

typedef struct {
    uint32_t                            timestamp;                      /**< O Param: timestamp of last statistics update. */
    uint32_t                            roundCnt;                       /**< O Param: Inventory rounds already done. */
    uint8_t                             tuningStatus;                   /**< O Param: Reader status. */
    uint8_t                             rssiLogMean;                    /**< O Param: RSSI log mean value. Measurement is updated with every found TAG */
    int8_t                              sensitivity;                    /**< O Param: Reader sensitivity. */
    uint8_t                             Q;                              /**< O Param: Current Q, may vary if adaptiveQEnable is set. */
    uint32_t                            frequency;                      /**< O Param: Current Frequency. */
    uint16_t                            adc;                            /**< O Param: ADC value. Measured after each inventory round. */
    uint32_t                            tagCnt;                         /**< O Param: Number of detected tags. */
    uint32_t                            emptySlotCnt;                   /**< O Param: Number of empty slots. */
    uint32_t                            collisionCnt;                   /**< O Param: Number of collisions. */
    uint32_t                            skipCnt;                        /**< O Param: Number of skipped tags due to failed follow command (eg.: subsequent ReadTID after Query). */
    uint32_t                            preambleErrCnt;                 /**< O Param: Number of Preamble errors. */
    uint32_t                            crcErrCnt;                      /**< O Param: Number of CRC errors. */
    uint32_t                            headerErrCnt;                   /**< O Param: Number of Header errors. */
    uint32_t                            rxCountErrCnt;                  /**< O Param: Number of RX count errors. */
} STUHFL_T_Inventory_Statistics;
#define STUHFL_O_INVENTORY_STATISTICS_INIT(...) ((STUHFL_T_Inventory_Statistics) { .timestamp = 0, \
                                                                              .roundCnt = 0, .tuningStatus = TUNING_STATUS_UNTUNED, .rssiLogMean = 0, .sensitivity = 0, .Q = 0, .frequency = 0, \
                                                                              .adc = 0, .tagCnt = 0, \
                                                                              .emptySlotCnt = 0, .collisionCnt = 0, .skipCnt = 0, .preambleErrCnt = 0, .crcErrCnt = 0, .headerErrCnt = 0, .rxCountErrCnt = 0 \
                                                                              ##__VA_ARGS__ })

typedef struct {
    STUHFL_T_Inventory_Statistics       statistics;         /**< O Param: Inventory statistics. */
    STUHFL_T_Inventory_Tag              *tagList;           /**< O Param: Detected tags list. */
    uint16_t                            tagListSize;        /**< O Param: Detected tags number. */
    uint16_t                            tagListSizeMax;     /**< I Param: tagList size. Max number of tags to be stored during inventory. If more tags found, exceeding ones will overwrite last list entry. */
} STUHFL_T_Inventory_Data;
#define STUHFL_O_INVENTORY_DATA_INIT(...) ((STUHFL_T_Inventory_Data) { .statistics = STUHFL_O_INVENTORY_STATISTICS_INIT(), .tagList = NULL, .tagListSize = 0, .tagListSizeMax = 0, ##__VA_ARGS__ })

// --------------------------------------------------------------------------
// Tags events
#define EVENT_TAG_FOUND         0x0001
#define EVENT_EMPTY_SLOT        0x0002
#define EVENT_COLLISION         0x0004
#define EVENT_PREAMBLE_ERR      0x0008
#define EVENT_CRC_ERR           0x0010
#define EVENT_HEADER_ERR        0x0020
#define EVENT_RX_COUNT_ERR      0x0040
#define EVENT_STOPBIT_ERR       0x0080
#define EVENT_SKIP_FOLLOW_CMD   0x0100
#define EVENT_RESEND_ACK        0x0200
#define EVENT_QUERY_REP         0x0400

#ifdef USE_INVENTORY_EXT
typedef struct {
    uint32_t                            timeStampBase;                  /**< O Param: base timestamp slotInfoList members. */
    uint32_t                            slotIdBase;                     /**< O Param: base ID for slotInfoList members. Starting slot ID slotInfoList members. */
} STUHFL_T_Inventory_Slot_Info_Sync;

//
typedef struct {
    uint8_t                             deltaT;                         /**< O Param: time delta to previous slot. absolute time = timeStampBase + sum(smaller slotInfoList list member index) */
    uint8_t                             Q;                              /**< O Param: used Q in this slot */
    int8_t                              sensitivity;                    /**< O Param: used sensitivity in this slot */
    uint16_t                            eventMask;                      /**< O Param: eventMask of slot. Describes occured events */
} STUHFL_T_Inventory_Slot_Info;

#define INVENTORYREPORT_SLOT_INFO_LIST_SIZE         256
typedef struct {
    STUHFL_T_Inventory_Slot_Info_Sync   slotSync;                                           /**< O Param: */
    STUHFL_T_Inventory_Slot_Info        slotInfoList[INVENTORYREPORT_SLOT_INFO_LIST_SIZE];  /**< O Param: */
    uint16_t                            slotInfoListSize;                                   /**< O Param: */
} STUHFL_T_Inventory_Slot_Info_Data;

typedef struct {
    STUHFL_T_Inventory_Data             invData;            /**< O Param:  */
    STUHFL_T_Inventory_Slot_Info_Data   invSlotInfoData;    /**< O Param:  */
} STUHFL_T_Inventory_Data_Ext;
#endif  // USE_INVENTORY_EXT

// --------------------------------------------------------------------------
#define PASSWORD_LEN                4U
#define MAX_READ_DATA_LEN           16U
#define MAX_WRITE_DATA_LEN          2U
#define MAX_BLOCKWRITE_DATA_LEN     16U
typedef struct {
    uint32_t                            wordPtr;                    /**< I Param: Word address to which read data. */
    uint8_t                             memBank;                    /**< I Param: Bank (File, EPC, TID, USER) to which read data. */
    uint8_t                             bytes2Read;                 /**< I/O Param: Number of bytes to read. Updated with number of effectively read bytes. */
    uint8_t                             pwd[PASSWORD_LEN];          /**< I Param: Password. */
    uint8_t                             data[MAX_READ_DATA_LEN];    /**< O Param: Read data. */
} STUHFL_T_Read;
#define STUHFL_O_READ_INIT(...) ((STUHFL_T_Read) { .wordPtr = 0, .memBank = 0, .bytes2Read = 0, .pwd = {0}, .data = {0}, ##__VA_ARGS__ })

typedef struct {
    uint32_t                            wordPtr;                    /**< I Param: Word address to which write data. */
    uint8_t                             memBank;                    /**< I Param: Bank (File, EPC, TID, USER) to which write data. */
    uint8_t                             pwd[PASSWORD_LEN];          /**< I Param: Password. */
    uint8_t                             data[MAX_WRITE_DATA_LEN];   /**< I Param: Data to be written. */
    uint8_t                             tagReply;                   /**< O Param: Tag reply. */
} STUHFL_T_Write;
#define STUHFL_O_WRITE_INIT(...) ((STUHFL_T_Write) { .wordPtr = 0, .memBank = 0, .pwd = {0}, .data = {0}, .tagReply = 0, ##__VA_ARGS__ })

typedef struct {
    uint32_t                            wordPtr;                    /**< I Param: Word address to which write data. */
    uint8_t                             pwd[PASSWORD_LEN];          /**< I Param: Password. */
    uint8_t                             memBank;                    /**< I Param: Bank (File, EPC, TID, USER) to which write data. */
    uint8_t                             nbWords;                    /**< I/O Param: Number of words to write. */
    uint8_t                             data[MAX_BLOCKWRITE_DATA_LEN];   /**< O Param: Data to be written. */
    uint8_t                             tagReply;                   /**< O Param: Tag reply. */
} STUHFL_T_BlockWrite;
#define STUHFL_O_BLOCKWRITE_INIT(...) ((STUHFL_T_BlockWrite) { .wordPtr = 0, .pwd = {0}, .memBank = 0, .nbWords = 0, .data = {0}, .tagReply = 0, ##__VA_ARGS__ })

typedef struct {
    uint8_t                             pwd[PASSWORD_LEN];          /**< I Param: Password. */
    uint8_t                             rfu;                        /**< I Param: RFU. */
    uint8_t                             recom;                      /**< I Param: RFU. */
    uint8_t                             tagReply;                   /**< O Param: Tag reply. */
} STUHFL_T_Kill;
#define STUHFL_O_KILL_INIT(...) ((STUHFL_T_Kill) { .pwd = {0}, .rfu = 0, .recom = 0, .tagReply = 0, ##__VA_ARGS__ })

#pragma pack(pop)

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __STUHFL_SL_H
