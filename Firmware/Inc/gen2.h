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
 *  @brief This file provides declarations for functions for the GEN2 aka ISO6c protocol.
 *
 *  Detailed documentation of the provided functionality can be found in gen2.h.
 *  Before calling any of the functions herein the ST25RU3993 chip needs to
 *  be initialized using st25RU3993Initialize(). Thereafter the function
 *  gen2Open() needs to be called for opening a session.
 *  gen2SearchForTags() should be called to identify the tags in
 *  reach. Usually the the user then wants to select one of the found
 *  tags. For doing this gen2Select() can be used. If gen2Config and gen2SelectParams
 *  provide a matching configuration the next call to gen2SearchForTags() will only
 *  return 1 tag. If singulate parameter of gen2SearchForTags() is set to true
 *  the found tag will be in the Open state and
 *  can then be accessed using the other functions ,
 *  gen2WriteWordToTag(), gen2ReadFromTag()...
 *  When finished with gen2 operations this session should be closed using
 *  gen2Close().
 *
 *  The tag state diagram looks as follows using the
 *  provided functions. For exact details please refer to
 *  the standard document provided by EPCglobal under
 *  <a href="http://www.epcglobalinc.org/standards/uhfc1g2/uhfc1g2_1_2_0-standard-20080511.pdf">uhfc1g2_1_2_0-standard-20080511.pdf</a>
 *
 *  If the tag in question does have a password set:
 *  \dot
 *  digraph gen2_statesp{
 *  Ready -> Open [ label="gen2SearchForTags()" ];
 *  Ready->Ready;
 *  Open -> Secured [ label="gen2AccessTag() + correct PW" ];
 *  Open -> Open [ label="gen2ReadFromTag()\n..." ];
 *  Secured -> Secured [ label="gen2WriteWordToTag()\ngen2ReadFromTag()\n..." ];
 *  }
 *  \enddot
 *
 *  If the tag in question does <b>not</b> have a password set:
 *  \dot
 *  digraph gen2_statesn{
 *  Ready -> Secured [ label="gen2SearchForTags()" ];
 *  Secured -> Secured [ label="gen2WriteWordToTag()\ngen2ReadFromTag()\n..." ];
 *  }
 *  \enddot
 *
 *  So a typical use case may look like this:
 *  \code
 *  tag_t tags[16];
 *  uint8_t tagError;
 *  gen2Config_t config = = {GEN2_TARI_12_50, GEN2_LF_256, GEN2_COD_MILLER4, TREXT_OFF, 0, GEN2_SESSION_S0, 0, false, TIMING_USE_DEFAULT_T4MIN};
 *  // setup gen2SelectParams
 *  unsigned n;
 *  uint8_t buf[4];
 *  ...
 *  st25RU3993Initialize();
 *
 *  gen2Open(&config);
 *
 *  performSelects();
 *  n = gen2SearchForTags(tags+1, 1, 0, continueAlways, 1);
 *  if(n == 0) return;
 *
 *  //Pick one of the tags based on the contents of tags. Here we use the very first tag
 *
 *  if(gen2ReadFromTag(tags+0, MEM_USER, 0, 2, buf))
 *     return;
 *
 *  buf[0] ^= 0xFF; buf[1]^= 0x55; // change the data
 *
 *  if(gen2WriteWordToTag(tags+0, MEM_USER, 0, buf, &tagError))
 *  { // wrote back one of the two read words
 *     gen2Close();
 *     return;
 *  }
 *
 *  //...
 *
 *  gen2Close();
 *
 *  \endcode
 *
 */

/** @addtogroup Protocol
  * @{
  */
/** @addtogroup Gen2
  * @{
  */

#ifndef __GEN2_H__
#define __GEN2_H__

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
#include "stuhfl_sl_gen2.h"
#include "stuhfl_dl_ST25RU3993.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/

/* Gen2 specific error codes */
#define ST25RU3993_ERR_NORES      ERR_CHIP_NORESP   // 0xDF
#define ST25RU3993_ERR_HEADER     ERR_CHIP_HEADER   // 0xDE
#define ST25RU3993_ERR_PREAMBLE   ERR_CHIP_PREAMBLE // 0xDD
#define ST25RU3993_ERR_RXCOUNT    ERR_CHIP_RXCOUNT  // 0xDC
#define ST25RU3993_ERR_CRC        ERR_CHIP_CRCERROR // 0xDB
#define ST25RU3993_ERR_FIFO_OVER  ERR_CHIP_FIFO     // 0xDA
#define ST25RU3993_ERR_COLL       ERR_CHIP_COLL     // 0xD9

/* Protocol configuration settings */
#define GEN2_LF_40  0   /** <link frequency  40 kHz */
#define GEN2_LF_160 6   /** <link frequency 160 kHz */
#define GEN2_LF_213 8   /** <link frequency 213 kHz */
#define GEN2_LF_256 9   /** <link frequency 256 kHz */
#define GEN2_LF_320 12  /** <link frequency 320 kHz */
#define GEN2_LF_640 15  /** <link frequency 640 kHz */

/* Rx coding values */
#define GEN2_COD_FM0      0 /** <FM coding for rx      */
#define GEN2_COD_MILLER2  1 /** <MILLER2 coding for rx */
#define GEN2_COD_MILLER4  2 /** <MILLER4 coding for rx */
#define GEN2_COD_MILLER8  3 /** <MILLER8 coding for rx */

/* TrExt defines */
#define TREXT_OFF         0 /**< use short preamble */
#define TREXT_ON          1 /**< use long preamble  */

/* EPC Mem Banks */
/** Definition for EPC tag memory bank: reserved */
#define MEM_RES           0x00
/** Definition for EPC tag memory bank: EPC memory */
#define MEM_EPC           0x01
/** Definition for EPC tag memory bank: TID */
#define MEM_TID           0x02
/** Definition for EPC tag memory bank: user */
#define MEM_USER          0x03

/* EPC Wordpointer Addresses */
/** Definition for EPC wordpointer: Address for CRC value */
#define MEMADR_CRC        0x00
/** Definition for EPC wordpointer: Address for PC value Word position*/
#define MEMADR_PC         0x01
/** Definition for EPC wordpointer: Address for EPC value */
#define MEMADR_EPC        0x02

/** Definition for EPC wordpointer: Address for kill password value */
#define MEMADR_KILLPWD    0x00
/** Definition for EPC wordpointer: Address for access password value */
#define MEMADR_ACCESSPWD  0x02

/** Definition for EPC wordpointer: Address for TID value */
#define MEMADR_TID        0x00

/* EPC SELECT TARGET */
/** Definition for inventory: Inventoried (S0) */
#define GEN2_IINV_S0           0x00
/** Definition for inventory: 001: Inventoried (S1) */
#define GEN2_IINV_S1           0x01
/** Definition for inventory: 010: Inventoried (S2) */
#define GEN2_IINV_S2           0x02
/** Definition for inventory: 011: Inventoried (S3) */
#define GEN2_IINV_S3           0x03
/** Definition for inventory: 100: SL */
#define GEN2_IINV_SL           0x04

#define GEN2_SESSION_S0           0x00
#define GEN2_SESSION_S1           0x01
#define GEN2_SESSION_S2           0x02
#define GEN2_SESSION_S3           0x03

/* Defines for various return codes */
#define GEN2_OK                  ERR_NONE                   /**< No error */
#define GEN2_ERR_REQRN           ERR_GEN2_REQRN             /**< Error sending ReqRN */
#define GEN2_ERR_ACCESS          ERR_GEN2_ACCESS            /**< Error sending Access password */
#define GEN2_ERR_SELECT          ERR_GEN2_SELECT            /**< Error when selecting tag */
#define GEN2_ERR_CHANNEL_TIMEOUT ERR_GEN2_CHANNEL_TIMEOUT   /**< Error RF channel timed out */

/*
******************************************************************************
* STRUCTS
******************************************************************************
*/
/**< Gen2 configuration parameters */
typedef struct
{
    uint8_t tari;           /*< Tari setting */
    uint8_t blf;            /*< GEN2_LF_40, ... */
    uint8_t coding;         /*< GEN2_COD_FM0, ... */
    uint8_t trext;          /*< 1 if the preamble is long, i.e. with pilot tone */
    uint8_t sel;            /*< For QUERY Sel field */
    uint8_t session;        /*< GEN2_SESSION_S0, ... */
    uint8_t target;         /*< For QUERY Target field */
    uint8_t toggleTarget;   /*< toogle target during session when no reply */
    uint16_t T4Min;         /*< T4 Minimum value (Minimum timing before 2 commands) */
} gen2Config_t;

/**< Gen2 adaptive Q parameters */
typedef struct
{
    bool isEnabled;
    uint8_t options;
    uint8_t c1[NUM_C_VALUES];
    uint8_t c2[NUM_C_VALUES];
    float qfp;
    uint8_t minQ;
    uint8_t maxQ;
} gen2AdaptiveQ_Params_t;

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
typedef struct
{
    tag_t *tag;
    gen2AdaptiveQ_Params_t *adaptiveQ;    
    STUHFL_T_Inventory_Statistics *statistics;
    
    bool singulate;
    bool toggleSession;
    
    bool (*cbTagFound)(tag_t *tag);
    void (*cbSlotFinished)(uint32_t slotTime, uint16_t eventMask, uint8_t Q);
    bool (*cbContinueScanning)(void);
    int8_t (*cbFollowTagCommand)(tag_t *tag);
} gen2SearchForTagsParams_t;

/** Search for tags (aka do an inventory round). Before calling any other
  * gen2 functions this routine has to be called. It first selects using the
  * given mask a set of tags and then does an inventory round issuing query
  * commands. All tags are stored in then tags array for examination by the
  * user.
  *
  * @param *tags an array for the found tags to be stored to
  * @param maxtags the size of the tags array
  * @param q 2^q slots will be done first, additional 2 round with increased
  * or decreased q may be performed
  * @param addRounds additional Rounds with Query Adjust
  * @param queryAdjustUpTh   Threshold for the additional round with Query Adjust to increase Q; slots * (queryAdjustUpTh/100)
  * @param queryAdjustDownTh Threshold for the additional round with Query Adjust to decrease Q; slots * (queryAdjustDownTh/100)
  * @param cbContinueScanning callback is called after each slot to inquire if we should
  * continue scanning (e.g. for allowing a timeout)
  * @param singulate If set to true Req_RN command will be sent to get tag into Open state
  *                  otherwise it will end up in arbitrate
  * @param toggleSession If set to true, QueryRep commands will be sent immediately
  *                  after receiving tag reply to toggle session flag on tag.
  * @param followTagCommand callback function is called after a tag is inventoried.
  *        If function will be called, no fast mode is possible.
  * @return the number of tags found
  */
uint16_t gen2SearchForTags(bool manualAck, gen2SearchForTagsParams_t *p);

/** EPC ACCESS command send to the Tag.
  * This function is used to bring a tag with set access password from the Open
  * state to the Secured state.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the tag_t structure.
  * @param *password Pointer to first byte of the access password
  *
  * @return The function returns an errorcode.
                  0x00 means no Error occurred.
                  Any other value is the backscattered error code from the tag.
  */
int8_t gen2AccessTag(tag_t const *tag, uint8_t const *password);

/** EPC LOCK command send to the Tag.
  * This function is used to lock some data region in the tag.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gen2SearchForTags().
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
int8_t gen2LockTag(tag_t *tag, const uint8_t *maskAction, uint8_t *tagReply);

/** EPC KILL command send to the Tag.
  * This function is used to permanently kill a tag. After that the
  * tag will never ever respond again.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gen2SearchForTags().
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
int8_t gen2KillTag(tag_t const *tag, uint8_t const *password, uint8_t rfu, uint8_t recom, uint8_t *tagError);

/** EPC WRITE command send to the Tag.
  * This function writes one word (16 bit) to the tag.
  * It first requests a new handle. The handle is then exored with the data.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gen2SearchForTags().
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
int8_t gen2WriteWordToTag(tag_t const *tag, uint8_t memBank, uint32_t wordPtr, uint8_t const *databuf, uint8_t *tagError);

/** EPC READ command send to the Tag.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the tag_t structure.
  * @param memBank Memory Bank from which the data should be read.
  * @param wordPtr Word Pointer Address from which the data should be read.
  * @param *wordCount Number of words to read from the tag. If 0, updated with number of words effectively read.
  * @param *destbuf Pointer to the first byte of the data array.
  *
  * @return The function returns an errorcode.
                  0x00 means no error occurred.
                  0xFF unknown error occurred.
                  Any other value is the backscattered error code from the tag.
  */
int8_t gen2ReadFromTag(tag_t *tag, uint8_t memBank, uint32_t wordPtr,
                          uint8_t *wordCount, uint8_t *destbuf);

/** EPC EPC_BLOCKWRITE command send to the Tag.
  * This function writes multiple words (16 bit) to the tag.
  * On contrary from gen2WriteWordToTag, data are not cover coded before being sent.
  *
  * @attention This command works on the one tag which is currently in the open
  *            state, i.e. on the last tag found by gen2SearchForTags().
  *
  * @param *tag Pointer to the tag_t structure.
  * @param memBank Memory Bank to which the data should be written.
  * @param wordPtr Word Pointer Address to which the data should be written.
  * @param *databuf Pointer to the first byte of the data array.
  * @param nbWords Number of words to be written.
  * @param *tagError In case tag returns an error (header bit set), this
                      functions returns ERR_HEADER and inside tagError the actual code
  *
  * @return The function returns an errorcode.
                  0x00 means no error occurred.
                  0xFF unknown error occurred.
                  Any other value is the backscattered error code from the tag.
  */
int8_t gen2WriteBlockToTag(tag_t const *tag, uint8_t memBank, uint32_t wordPtr, uint8_t const *databuf, uint8_t nbWords, uint8_t *tagError);

/** ENABLERX command send to the Tag.
  * This function re-enables data reception over the air from current command.
  *
  * @param *tagError In case tag returns an error (header bit set), this
                      functions returns ERR_HEADER and inside tagError the actual code
  *
  * @return The function returns an errorcode.
                  0x00 means no error occurred.
                  0xFF unknown error occurred.
                  Any other value is the backscattered error code from the tag.
  */
int8_t gen2ContinueCommand(uint8_t *tagError);

/** EPC SELECT command. send to the tag.
  * This function does not take or return a parameter
  */
void gen2Select(STUHFL_T_Gen2_Select *p);

/** @brief Configure Gen session
  * Set gen2 specific parameters.
  */
void gen2Configure(const gen2Config_t *config);

/** @brief Open a session
  * Open a session for gen2 protocol
  * @param config: configuration to use
  */
void gen2Open(const gen2Config_t *config);

/** @brief Close a session
  * Close the session for gen2 protocol
  */
void gen2Close(void);

/** Perform a gen2 QUERY command and measure received signal strength
  */
int8_t gen2QueryMeasureRSSI(uint8_t Q, uint8_t *agc, uint8_t *rssiLogI, uint8_t *rssiLogQ, int8_t *rssiLinI, int8_t *rssiLinQ);

/** Returns the Gen2 Configuration
  * @return Gen2 Configuration
  */
gen2Config_t *getGen2IntConfig(void);

#endif

/**
  * @}
  */
/**
  * @}
  */
