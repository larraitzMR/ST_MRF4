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
 *  @brief GB-29768 protocol header file
 *
 * Before calling any of the functions herein the ST25RU399X chip needs to
 * be initialized using st25RU3993Initialize(). Thereafter the function
 * gb29768Open() needs to be called for opening a session.
 *
 * The following graph shows several states of an GB-29768 tag as well as their
 * transitions based on GB-29768 commands.
 *
 * \dot
 * digraph gb29768_states{
 * Power_off -> Ready [ label="st25RU3993AntennaPower(1)" ];
 * Ready -> Data_Exchange [ label="gb29768Read()" ];
 * Ready -> Data_Exchange [ label="gb29768Write()" ];
 * Ready -> ID [ label="gb29768InventoryRound(),\nafter select command has been emitted" ];
 * ID -> Data_Exchange [ label="gb29768InventoryRound(),\nafter collision arbitration" ];
 * Data_Exchange -> Power_off [ label="st25RU3993AntennaPower(0)" ];
 * }
 * \enddot
 *
 * It can be seen that gb29768Read() and gb29768Write() can be called without a prior
 * inventory round. Both commands, however, do need the ID of the tag which can
 * only be determined by calling gb29768InventoryRound().
 */

/** @addtogroup Protocol
  * @{
  */
/** @addtogroup GB-29768
  * @{
  */

#ifndef GB29768_H
#define GB29768_H

/* DEBUG options */
//#define GB29768FRAMES_DEBUG

#define MAX_DEBUG_FRAMESSIZE    1024

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include <stdbool.h>
#include "st25RU3993_public.h"
#include "errno_application.h"
#include "stuhfl_sl.h"
#include "stuhfl_sl_gb29768.h"
#include "stuhfl_dl_ST25RU3993.h"
#include "st25RU3993_config.h"

/*
******************************************************************************
* STRUCTS
******************************************************************************
*/
typedef struct
{
    uint8_t condition;   /*< Condition setting */
    uint8_t session;     /*< GB29768_SESSION_S0, ... */
    uint8_t target;      /*< For QUERY Target field */
    uint8_t trext;       /*< 1 if the lead code is sent, 0 otherwise */
    uint8_t blf;         /*< backscatter link frequency factor */
    uint8_t coding;      /*< GB29768_COD_FM0, GB29768_COD_MILLER2, ... */
    uint8_t tc;          /*< GB29768_TC_12_5, GB29768_TC_6_25 */
    uint8_t toggleTarget; /**< I/O Param: Toggle between Target A and B */
    uint8_t targetDepletionMode; /**< I/O Param: If set to 1 and the target shall be toggled in inventory an additional inventory round before the target is toggled will be executed. This gives "weak" transponders an additional chance to reply. */
} gb29768Config_t;

typedef struct
{
    uint16_t bitPointer;
    uint8_t storageArea;
    uint8_t target;
    uint8_t rule;
    uint8_t bitLength;
    uint8_t mask[32];
} gb29768SortParams_t;

#define GB29768_OKAY                        ERR_NONE
#define GB29768_PARAM_ERROR                 ERR_PARAM
#define GB29768_PROTO_ERROR                 ERR_PROTO
#define GB29768_NO_RESPONSE                 ERR_CHIP_NORESP
#define GB29768_CRC_ERROR                   ERR_CHIP_CRCERROR
#define GB29768_STOPBIT_ERROR               ERR_CHIP_HEADER
#define GB29768_PREAMBLE_ERROR              ERR_CHIP_PREAMBLE
#define GB29768_COLLISION                   ERR_CHIP_COLL
#define GB29768_TAG_POWER_SHORTAGE          ERR_GB29768_POWER_SHORTAGE      
#define GB29768_TAG_PERMISSION_ERROR        ERR_GB29768_PERMISSION_ERROR    
#define GB29768_TAG_STORAGE_OVERFLOW        ERR_GB29768_STORAGE_OVERFLOW    
#define GB29768_TAG_STORAGE_LOCKED          ERR_GB29768_STORAGE_LOCKED      
#define GB29768_TAG_PASSWORD_ERROR          ERR_GB29768_PASSWORD_ERROR      
#define GB29768_TAG_AUTH_ERROR              ERR_GB29768_AUTH_ERROR          
#define GB29768_TAG_ACCESS_ERROR            ERR_GB29768_ACCESS_ERROR        
#define GB29768_TAG_ACCESS_TIMEOUT_ERROR    ERR_GB29768_ACCESS_TIMEOUT_ERROR
#define GB29768_TAG_UNKNOWN_ERROR           ERR_GB29768_OTHER               
typedef int8_t gb29768_error_t;

typedef struct {
    uint16_t                            endThreshold;         /**< I Param: GBT anticollision end threshold parameter.*/
    uint16_t                            ccnThreshold;         /**< I Param: GBT anticollision CCN threshold parameter.*/
    uint16_t                            cinThreshold;         /**< I Param: GBT anticollision CIN threshold parameter.*/
} gb29768_Anticollision_t;

typedef struct
{
    tag_t *tag;
    gb29768_Anticollision_t         antiCollision;
    STUHFL_T_Inventory_Statistics  *statistics;
    
    bool singulate;
    bool toggleSession;
    
    bool (*cbTagFound)(tag_t *tag);
    void (*cbSlotFinished)(uint32_t slotTime, uint16_t eventMask, uint8_t Q);
    bool (*cbContinueScanning)(void);
    int8_t (*cbFollowTagCommand)(tag_t *tag);
} gb29768_SearchForTagsParams_t;

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
/* GB29768 Timings */
#define GB29768_TIMING_Tpri_multiplier_NS   (25*1000)   // TPri=25000/8= 3125ns (@ BLF=320kHz)
#define GB29768_TIMING_Tpri_divider         8
#define GB29768_TIMING_Tc_multiplier_NS     (25*1000)   // Tc=25000/2= 12500ns or 6250ns
#define GB29768_TIMING_Tc_divider           2
#define GB29768_TIMING_SEPARATOR_NS         12500       // Separator is independent from TC value: 12.5us

#define GB29768_TIMING_T1min_NS(blf)            ((8 * GB29768_Blf2NS(blf)) - 2000)      // T1min= 23000 ns (@ BLF=320kHz)
#define GB29768_TIMING_T1_NS(blf)               (10 * GB29768_Blf2NS(blf))              // T1= 31250 ns    (@ BLF=320kHz)
#define GB29768_TIMING_T1max_NS(blf)            (12 * GB29768_Blf2NS(blf) + 2000)       // T1max= 39500 ns (@ BLF=320kHz)
#define GB29768_TIMING_T2min_NS(blf)            (3  * GB29768_Blf2NS(blf))              // T2min= 9375 ns  (@ BLF=320kHz)   
#define GB29768_TIMING_T2max_NS(blf)            (20 * GB29768_Blf2NS(blf))              // T2max= 62500 ns (@ BLF=320kHz)   
#define GB29768_TIMING_T3min_NS(blf)            0 
#define GB29768_TIMING_T4min_NS(tc)             GB29768_Tc2NS(3, tc)                    // T4 = 3Tc (37500 ns @ tc=12.5)

#define NB_GB29768_BLF                      (GB29768_BLF_640+1)
extern const uint32_t TPriBLF[NB_GB29768_BLF];
#define GB29768_Blf2NS(blf)                     (((blf) < NB_GB29768_BLF) ? TPriBLF[blf] : 3125)
#define GB29768_Tc2NS(nb,tc)                    (((nb)*GB29768_TIMING_Tc_multiplier_NS) / (GB29768_TIMING_Tc_divider * (((tc) == GB29768_TC_6_25) ? 2 : 1)))

/* Command Codes, Required Commands */
#define GB29768_CMD_SORT            0xAA
#define GB29768_CMD_QUERY           0xA4
#define GB29768_CMD_QUERYREP        0x00
#define GB29768_CMD_DIVIDE          0x03
#define GB29768_CMD_DISPERSE        0x08
#define GB29768_CMD_SHRINK          0x09
#define GB29768_CMD_ACK             0x01
#define GB29768_CMD_NAK             0xAF
#define GB29768_CMD_REFRESHHANDLE   0xB4
#define GB29768_CMD_GETRN           0xB2
#define GB29768_CMD_ACCESS          0xA3
#define GB29768_CMD_READ            0xA5
#define GB29768_CMD_WRITE           0xA6
#define GB29768_CMD_ERASE           0xA7
#define GB29768_CMD_LOCK            0xA8
#define GB29768_CMD_KILL            0xA9
#define GB29768_CMD_SECURITY        0xAE

#define GB29768_CMD_DIVIDE0         0xFE
#define GB29768_CMD_DIVIDE1         0xFF

#define OPERATING_STATE_SUCCESS             0x00
#define OPERATING_STATE_POWER_SHORTAGE      0x83
#define OPERATING_STATE_PERMISSION_ERROR    0x81
#define OPERATING_STATE_STORAGE_OVERFLOW    0x82
#define OPERATING_STATE_STORAGE_LOCKED      0x85
#define OPERATING_STATE_PASSWORD_ERROR      0x86
#define OPERATING_STATE_AUTH_ERROR          0x87
#define OPERATING_STATE_UNKNOWN_ERROR       0x88

#define PW_CATEGORY_KILL_L  0x00
#define PW_CATEGORY_KILL_H  0x01
#define PW_CATEGORY_LOCK_L  0x02
#define PW_CATEGORY_LOCK_H  0x03
#define PW_CATEGORY_READ_L  0x06
#define PW_CATEGORY_READ_H  0x07
#define PW_CATEGORY_WRITE_L 0x08
#define PW_CATEGORY_WRITE_H 0x09

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
/** Search for tags (aka do an inventory round). Before calling any other
  * GB-29768 functions this routine has to be called. It first selects using the
  * given mask a set of tags and then does an inventory round issuing query
  * commands. All tags are stored in then tags array for examination by the
  * user.
  *
  * @param *tags an array for the found tags to be stored to
  * @param maxtags the size of the tags array
  * @return the number of tags found
  */
uint16_t gb29768SearchForTags(gb29768_SearchForTagsParams_t *p);

/** ACCESS command send to the Tag.
  * This function is used to bring a tag with set access password from the Open
  * state to the Secured state.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gb29768SearchForTags().
  *
  * @param *tag Pointer to the tag_t structure.
  * @param *password Pointer to first byte of the access password
  *
  * @return The function returns an errorcode.
                  0x00 means no Error occurred.
                  Any other value is the backscattered error code from the tag.
  */
gb29768_error_t gb29768AccessTag(tag_t *tag, const uint8_t storageArea, const uint8_t category, const uint8_t *password);

/** LOCK command send to the Tag.
  * This function is used to lock some data region in the tag.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gb29768SearchForTags().
  *
  * @param *tag Pointer to the tag_t structure.
  * @param *maskAction Pointer to the first byte of the mask and
                                    action array.
  * @param *tagReply In case ERR_HEADER is returned this variable will
  *                   contain the 8-bit error code from the tag.
  *
  * @return The function returns an errorcode.
                  0x00 means no Error occurred.
                  Any other value is the backscattered error code from the tag.
  */
gb29768_error_t gb29768LockTag(tag_t *tag, const uint8_t storageArea, const uint8_t configuration, const uint8_t action);

/** KILL command send to the Tag.
  * This function is used to permanently kill a tag. After that the
  * tag will never ever respond again.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gb29768SearchForTags().
  *
  * @param *tag Pointer to the tag_t structure.
  * @param *password Pointer to first byte of the kill password
  * @param rfu 3 bits used as rfu content for first half of kill, should be zero
  * @param recom 3 bits used as recom content for second half of kill, zero
  *        for real kill, !=0 for recommissioning
  * @param *tagError: in case header bit is set this will be the return code from the tag
  *
  * @return The function returns an errorcode.
                  0x00 means no Error occurred.
                  Any other value is the backscattered error code from the tag.
  */
gb29768_error_t gb29768KillTag(tag_t *tag);

/** WRITE command send to the Tag.
  * This function writes one word (16 bit) to the tag.
  * It first requests a new handle. The handle is then exored with the data.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gb29768SearchForTags().
  *
  * @param *tag Pointer to the tag_t structure.
  * @param memBank Memory Bank to which the data should be written.
  * @param wordPtr Word Pointer Address to which the data should be written.
  * @param *databuf Pointer to the first byte of the data array. The data buffer
                             has to be 2 bytes long.
  * @param *tagError In case tag returns an error (header bit set), this
                      functions returns ERR_HEADER and inside tagError the actual code
  *
  * @return The function returns an errorcode.
                  0x00 means no error occurred.
                  0xFF unknown error occurred.
                  Any other value is the backscattered error code from the tag.
  */
gb29768_error_t gb29768WriteToTag(tag_t *tag, const uint8_t storageArea, const uint16_t pointer, const uint16_t length, const uint8_t *data);

/** WRITE command send to the Tag.
  * This function writes one word (16 bit) to the tag.
  * It first requests a new handle. The handle is then exored with the data.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gb29768SearchForTags().
  *
  * @param *tag Pointer to the tag_t structure.
  * @param memBank Memory Bank to which the data should be written.
  * @param wordPtr Word Pointer Address to which the data should be written.
  * @param *databuf Pointer to the first byte of the data array. The data buffer
                             has to be 2 bytes long.
  * @param *tagError In case tag returns an error (header bit set), this
                      functions returns ERR_HEADER and inside tagError the actual code
  *
  * @return The function returns an errorcode.
                  0x00 means no error occurred.
                  0xFF unknown error occurred.
                  Any other value is the backscattered error code from the tag.
  */
gb29768_error_t gb29768EraseTag(tag_t *tag, const uint8_t storageArea, const uint16_t pointer, const uint16_t length);

/** READ command send to the Tag.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gb29768SearchForTags().
  *
  * @param *tag Pointer to the tag_t structure.
  * @param storageArea Memory Bank to which the data should be written.
  * @param pointer Word Pointer Address to which the data should be written.
  * @param length Number of bytes to read from the tag (if 0, reads full area content).
  * @param *destBuffer Pointer to the first byte of the data array.
  *
  * @return The function returns an errorcode.
                  0x00 means no error occurred.
                  0xFF unknown error occurred.
                  Any other value is the backscattered error code from the tag.
  */
gb29768_error_t gb29768ReadFromTag(tag_t *tag, const uint8_t storageArea, const uint16_t pointer, uint16_t *length, uint8_t *destBuffer);

gb29768_error_t gb29768Sort(gb29768SortParams_t *p);

/** Close the session for gb29768 protocol. */
gb29768_error_t gb29768Close(void);

/** Configure session for gb29768 protocol. */
gb29768_error_t gb29768Configure(const gb29768Config_t *config);

/** Open a session for gb29768 protocol. */
gb29768_error_t gb29768Open(const gb29768Config_t *config);

#endif /* GB29768_H */

/**
  * @}
  */
/**
  * @}
  */
