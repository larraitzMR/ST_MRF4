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
 *  @brief Implementation of GB-29768 protocol
 *
 *  Implementation of GB-29768 protocol
 */

/** @addtogroup Protocol
  * @{
  */
/** @addtogroup GB-29768
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <string.h>
#include <stdbool.h>
#include <math.h> 

#include "st25RU3993_config.h"
#include "timer.h"
#include "st25RU3993.h"
#include "gb29768.h"
#include "bitbang.h"
#include "crc.h"
#include "logger.h"
#include "platform.h"
#include "leds.h"

#if GB29768
/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#define CRC_CHECK_OFF       0
#define CRC16_CHECK_ON      1
#define CRC5_CHECK_ON       2

#ifdef BTC
#define ENABLE_BTC_BOOT()       SET_GPIO(GPIO_BTC_BOOT_PORT,GPIO_BTC_BOOT_PIN)
#define DISABLE_BTC_BOOT()      RESET_GPIO(GPIO_BTC_BOOT_PORT,GPIO_BTC_BOOT_PIN)
#else
#define ENABLE_BTC_BOOT()
#define DISABLE_BTC_BOOT()
#endif

#define SET_PULSE_HIGH()        {DM_TX_SET;     RESET_DWT();}
#define SET_PULSE_LOW()         {DM_TX_RESET;   RESET_DWT();}

/*
******************************************************************************
* STRUCTS
******************************************************************************
*/
typedef struct
{
    uint32_t    numBytes;       // Nb of full bytes
    uint32_t    numBits;        // Nb of remaining bits
    uint8_t     data[MAX_RESPONSE_BYTES];
} gb29768_RxTxData_t;

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
static gb29768Config_t gb29768IntConfig;

static uint8_t gLastCommand;

const uint32_t TPriBLF[NB_GB29768_BLF] = {  (5*GB29768_TIMING_Tpri_multiplier_NS)/GB29768_TIMING_Tpri_divider,          /*  64 kHz :     TPri=TPri320*5= 15625 ns  */ 
                                            (7*GB29768_TIMING_Tpri_multiplier_NS)/(3*GB29768_TIMING_Tpri_divider),      /* 137 kHz :     TPri=TPri320*7/3= 7292 ns   */
                                            (11*GB29768_TIMING_Tpri_multiplier_NS)/(6*GB29768_TIMING_Tpri_divider),     /* 174 kHz :     TPri=TPri320*11/6= 5729 ns   */
                                            GB29768_TIMING_Tpri_multiplier_NS/GB29768_TIMING_Tpri_divider    ,          /* 320 kHz : TPri=TPri320=25000/8= 3125ns                 */
                                            (5*GB29768_TIMING_Tpri_multiplier_NS)/(2*GB29768_TIMING_Tpri_divider),      /* 128 kHz :     TPri=TPri320*5/2= 7812 ns   */
                                            (7*GB29768_TIMING_Tpri_multiplier_NS)/(6*GB29768_TIMING_Tpri_divider),      /* 274 kHz :     TPri=TPri320*7/6= 3646 ns   */
                                            (11*GB29768_TIMING_Tpri_multiplier_NS)/(12*GB29768_TIMING_Tpri_divider),    /* 349 kHz :     TPri=TPri320*11/12= 2865 ns   */
                                            (GB29768_TIMING_Tpri_multiplier_NS)/(2*GB29768_TIMING_Tpri_divider)         /* 640 kHz :     TPri=TPri320/2= 1562 ns   */
                                          }; 

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
static gb29768_RxTxData_t gGb29768RxTxData;

static uint8_t  gTagReplyBits[MAX_RESPONSE_BITS];
#ifdef GB29768PULSES_DEBUG
static uint32_t gTagReplyNbBits = 0;

extern uint8_t  gDebugPulses[MAX_RESPONSE_PULSES_DEBUG];
extern uint32_t gDebugPulsesNb;
#endif

#ifdef GB29768FRAMES_DEBUG
#define GB29768FRAMES_SND       0xAA
#define GB29768FRAMES_RCV       0xBB
uint8_t     gDebugFrames[MAX_DEBUG_FRAMESSIZE];
uint32_t    gDebugFramesSize = 0;

#define DEBUG_STORE_FRAME(direction,error,full)                                                                                                         \
    if (gDebugFramesSize < MAX_DEBUG_FRAMESSIZE-20) {                                                                                                   \
        uint8_t *debugFrames_p = &gDebugFrames[gDebugFramesSize];                                                                                       \
        uint16_t debugFramesBitsNb = (uint16_t)(gGb29768RxTxData.numBytes << 3) + gGb29768RxTxData.numBits;     /* Store length on only 16 bits */      \
        uint16_t lg;                                                                                                                                    \
                                                                                                                                                        \
        debugFrames_p[0] = (direction);                                                                                                                 \
        debugFrames_p[1] = (uint8_t)(error);                      /* Error code LSB */                                                                  \
        debugFrames_p[2] = (uint8_t)(debugFramesBitsNb >> 8);                                                                                           \
        debugFrames_p[3] = (uint8_t)(debugFramesBitsNb);                                                                                                \
        if ((full) && (gDebugFramesSize + gGb29768RxTxData.numBytes < MAX_DEBUG_FRAMESSIZE-(1+4+4))) {   /*1+4+4: extra bits+header+end of frame */     \
            debugFrames_p[2] |= 0x80;        /* Mark as full buffer */                                                                                  \
            lg = (gGb29768RxTxData.numBits) ? gGb29768RxTxData.numBytes+1 : gGb29768RxTxData.numBytes;                                                  \
            memcpy(&debugFrames_p[4], gGb29768RxTxData.data, lg);                                                                                       \
            lg += 4;                        /* Frame header */                                                                                          \
        }                                                                                                                                               \
        else {                                                                                                                                          \
            *(uint32_t *)(&debugFrames_p[4]) = *(uint32_t *)(&gGb29768RxTxData.data[0]);                                                                \
            lg = 8;                             /* Frame header + first data lg */                                                                      \
            if (debugFramesBitsNb > 32) {                                                                                                               \
                *(uint32_t *)(&debugFrames_p[8]) = *(uint32_t *)(&gGb29768RxTxData.data[4]);                                                            \
                lg += 4;                                                                                                                                \
            }                                                                                                                                           \
            if (debugFramesBitsNb > 64) {                                                                                                               \
                *(uint32_t *)(&debugFrames_p[12]) = *(uint32_t *)(&gGb29768RxTxData.data[8]);                                                           \
                lg += 4;                                                                                                                                \
            }                                                                                                                                           \
        }                                                                                                                                               \
        *(uint32_t *)(&debugFrames_p[lg]) = 0xFFFFFFFF;      /* End of frame buffer */                                                                  \
        gDebugFramesSize += lg;                                                                                                                         \
    }

#else
#define DEBUG_STORE_FRAME(direction,error,full)    
#endif

static uint32_t gGb29768RssiLogIandQSum;

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/

/** All raw gb29768 commands
*/
static gb29768_error_t gb29768SendCmdSort(const uint8_t storageArea, const uint8_t target, const uint8_t rule, const uint16_t pointer, const uint8_t length, const uint8_t *mask);
static gb29768_error_t gb29768SendCmdQuery(const uint8_t condition, const uint8_t session, const uint8_t target, const uint8_t trext, const uint8_t blf, const uint8_t coding);
static gb29768_error_t gb29768SendCmdQueryRep(const uint8_t session);
static gb29768_error_t gb29768SendCmdDivide(const uint8_t position, const uint8_t session);
static gb29768_error_t gb29768SendCmdDisperse(const uint8_t session);
static gb29768_error_t gb29768SendCmdShrink(const uint8_t session);
static gb29768_error_t gb29768SendCmdAck(const uint8_t *handle);
static gb29768_error_t gb29768SendCmdNak(void);
static gb29768_error_t gb29768SendCmdRefreshHandle(const uint8_t *handle);
static gb29768_error_t gb29768SendCmdGetRn(const uint8_t *handle);
static gb29768_error_t gb29768SendCmdAccess(const uint8_t storageArea, const uint8_t category, const uint8_t *password, const uint8_t *handle);  
static gb29768_error_t gb29768SendCmdRead(const uint8_t storageArea, const uint16_t pointer, const uint16_t length, const uint8_t *handle);  
static gb29768_error_t gb29768SendCmdWrite(const uint8_t storageArea, const uint16_t pointer, const uint16_t length, const uint8_t *data, const uint8_t *handle);  
static gb29768_error_t gb29768SendCmdErase(const uint8_t storageArea, const uint16_t pointer, const uint16_t length, const uint8_t *handle);  
static gb29768_error_t gb29768SendCmdLock(const uint8_t storageArea, const uint8_t configuration, const uint8_t action, const uint8_t *handle);
static gb29768_error_t gb29768SendCmdKill(const uint8_t *handle);
//static gb29768_error_t gb29768SendCmdSecurity(const uint8_t *handle);

/** adds bits to the output buffer to be send
@param data: value to add
@param bits: number of bits to add (1-8)
*/
static void gb29768AddByteToBitStream(const uint8_t data, const uint8_t bits);

/** adds bits to the output buffer to be send
@param data: value to add
@param bits: number of bits to add (9-16)
*/
static void gb29768AddWordToBitStream(const uint16_t data, const uint8_t bits);

/** adds bits to the output buffer to be send
@param data: value buffer to add
@param bits: number of bits to add (varying)
*/
static void gb29768AddArrayToBitStream(const uint8_t *data, uint16_t bits);

/** enters direct mode and initializes command structure */
static void gb29768InitializeCmd(void);

/** adds CRC over all output buffer bytes to the output buffer */
static void gb29768AddCrc(void);

/** converts the output buffer to TPP coding and send it, exit direct mode */
static void gb29768FinalizeCmdAndSend(void);

/** wait for IRQ and readout response from tag */
static gb29768_error_t gb29768WaitForResponse(const uint8_t do_crc_check);

/** decode tag operating state and print log output */
static gb29768_error_t decodeOperatingState(const uint8_t operatingState);

static gb29768_error_t gb29768Decode(gb29768_RxTxData_t *rxData, gb29768Config_t *gbCfg, uint8_t lastCommand);

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768WaitForResponse(const uint8_t do_crc_check)
{
    uint16_t crc = 0x0000;
    uint16_t calculatedCrc;

    gb29768_error_t err = gb29768Decode(&gGb29768RxTxData, &gb29768IntConfig, gLastCommand);
    if(err == GB29768_OKAY)
    {
        if(do_crc_check == CRC16_CHECK_ON)
        {
            // do CRC-16 check
            gGb29768RxTxData.numBits &= 0xFE;       // Force even number of bits
            if (gGb29768RxTxData.numBytes >= 2)         // At least CRC16
            {
                if(gGb29768RxTxData.numBits == 0)
                {
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes-2 ] << 8) & 0xFF00;
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes-1 ]     ) & 0x00FF;
                }
                else if(gGb29768RxTxData.numBits == 2)
                {
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes-2 ] << 10) & 0xFC00;
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes-1 ] <<  2) & 0x03FC;
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes ]   >>  6) & 0x0003;
                }
                else if(gGb29768RxTxData.numBits == 4)
                {
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes-2 ] << 12) & 0xF000;
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes-1 ] <<  4) & 0x0FF0;
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes ]   >>  4) & 0x000F;
                }
                else if(gGb29768RxTxData.numBits == 6)
                {
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes-2 ] << 14) & 0xC000;
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes-1 ] <<  6) & 0x3FC0;
                    crc |= (gGb29768RxTxData.data[ gGb29768RxTxData.numBytes ]   >>  2) & 0x003F;
                }

                calculatedCrc = crc16Bitwise(gGb29768RxTxData.data, (gGb29768RxTxData.numBytes*8 + gGb29768RxTxData.numBits) - 16); // 16 = CRC
                if(calculatedCrc == crc) 
                {
                    gGb29768RxTxData.numBytes -= 2;      // Remove CRC from genuine data
                }
                else {
                    err = GB29768_CRC_ERROR;
                }
            }
            else
            {
                err = GB29768_CRC_ERROR;
            }
        }

        if(do_crc_check == CRC5_CHECK_ON)
        {
            // do CRC-5 check
            // random number | CRC-5
            // xxxx xxxx xxx | C CCCC
            crc = (uint16_t)(gGb29768RxTxData.data[1] & 0x1F);
            calculatedCrc = (uint16_t)crc5Bitwise(gGb29768RxTxData.data, 11);
            
            if(calculatedCrc == crc) {
                gGb29768RxTxData.numBytes = 1;
                gGb29768RxTxData.numBits = 3;
            }
            else {
                err = GB29768_CRC_ERROR;
            }
        }
    }

    DEBUG_STORE_FRAME(GB29768FRAMES_RCV, err, (gLastCommand == GB29768_CMD_READ));

    if (err != GB29768_OKAY) {
        // Debug code
        __NOP();
    }

    return err;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768Decode(gb29768_RxTxData_t *rxData, gb29768Config_t *gbCfg, uint8_t lastCommand)
{
    uint32_t nbBits = 0;
    uint8_t  expectedPreamble = 0;
    uint8_t  preamble;
    
    ENABLE_BTC_BOOT();

    // Waits minimal time for tag reply since last sent pulse: T1
    nsdelay(GB29768_TIMING_T1_NS(gbCfg->blf));

    gb29768_error_t err;
    if (gb29768IntConfig.coding == GB29768_COD_FM0) {
        err = gb29768GetFM0Bits(gTagReplyBits, &nbBits, gbCfg, lastCommand);
        expectedPreamble = 0xE1;
    }
    else {
        err = gb29768GetMillerBits(gTagReplyBits, &nbBits, gbCfg, lastCommand);
        expectedPreamble = 0x3D;
    }

    DISABLE_BTC_BOOT();

    // Check data integrity
    if (err == GB29768_OKAY) {
        if (nbBits < 9) {                  // At least preamble + stop bit received
            err = GB29768_NO_RESPONSE;
        }
    }
    if (err == GB29768_OKAY) {
        if (!gTagReplyBits[ nbBits-1 ]) {    // At least stop bit is 1
            err = GB29768_STOPBIT_ERROR;
        }
    }

    // Process preamble
    if (err == GB29768_OKAY) {
        preamble = (gTagReplyBits[0] << 7) | (gTagReplyBits[1] << 6) | (gTagReplyBits[2] << 5) | (gTagReplyBits[3] << 4) | (gTagReplyBits[4] << 3) | (gTagReplyBits[5] << 2) | (gTagReplyBits[6] << 1) | (gTagReplyBits[7]);
        
        //LOG("Preamble %02x\n",preamble);
        if(preamble != expectedPreamble) {       // preamble okay
            //LOG("M2 Preamble Error\n");
            err = GB29768_PREAMBLE_ERROR;
        }
    }

#ifdef GB29768PULSES_DEBUG
    gTagReplyNbBits = nbBits;
#endif

    if (err == GB29768_OKAY) {
        nbBits -= 9;                            // -9: Genuine data do not contain neither preamble nor stop bit
        switch (gLastCommand) {
            case GB29768_CMD_QUERY:
            case GB29768_CMD_QUERYREP:
            case GB29768_CMD_DIVIDE:
            case GB29768_CMD_DISPERSE:
            case GB29768_CMD_SHRINK:
            case GB29768_CMD_REFRESHHANDLE:
                if (nbBits > 16) {    // Random + CRC5 = 16 bits
                    nbBits = 16;
                }                    
                break;
            case GB29768_CMD_GETRN:
                if (nbBits > 48) {    // Random + handle + CRC16 = 48 bits
                    nbBits = 48;
                }                    
                break;
            case GB29768_CMD_ACCESS:
            case GB29768_CMD_WRITE:
            case GB29768_CMD_ERASE:
            case GB29768_CMD_LOCK:
            case GB29768_CMD_KILL:
            case GB29768_CMD_SECURITY:
                if (nbBits > 40) {    // State + handle + CRC16 = 40 bits
                    nbBits = 40;
                }                    
                break;
            case GB29768_CMD_ACK:
                if (nbBits > 18) {   // Security mode + Coding + CRC16 = 18 bits + Coding data
                    nbBits = ((nbBits - 2) & 0xFFFFFFF8) + 2;       // x bytes + 2 bits
                }                    
                break;
            case GB29768_CMD_READ:
                if (nbBits > 32) {          // Read data + Handle + CRC16
                    nbBits &= 0xFFFFFFF8;       // x bytes + 0 bits
                }                    
                break;
//            case GB29768_CMD_SORT:
//            case GB29768_CMD_NAK:
            default:
                break;
        }
        rxData->numBytes = nbBits / 8;
        rxData->numBits = nbBits % 8;

        uint8_t *dataBits = &gTagReplyBits[8];   // Skip preamble and point on genuine bits
        uint32_t nbBytes = rxData->numBytes;
        if (rxData->numBits) {
            // Handle extra bits
            memset(&(dataBits[nbBits]), 0, 8 - rxData->numBits);    // Fill remaining bits of extra byte with 0
            nbBytes++;                                              // Add extra byte
        }

        // Process all genuine bytes (preamble skipped and stop bit ignored)
        for (uint32_t i=0 ; i < nbBytes ; ++i) {
            rxData->data[i] = (dataBits[0] << 7) | (dataBits[1] << 6) | (dataBits[2] << 5) | (dataBits[3] << 4) | (dataBits[4] << 3) | (dataBits[5] << 2) | (dataBits[6] << 1) | (dataBits[7]);
            dataBits = &dataBits[8];        // Go to next 8 bits
        }

        //LOG("M2 Preamble Okay\n");
        err = GB29768_OKAY;
    }
    else {
        // Debug code
        __NOP();
    }

    return err;    
}

/*------------------------------------------------------------------------- */
static void highLowPulse(uint32_t nbTC)
{
    // High pulse
    nsdelay( GB29768_Tc2NS(nbTC, gb29768IntConfig.tc) );   // nbTC * TC (12500ns or 6250ns)

    // Low Pulse
    SET_PULSE_LOW();
    nsdelay( GB29768_Tc2NS(1, gb29768IntConfig.tc) );      // 1 TC: 12500ns or 6250ns

    SET_PULSE_HIGH();
}

/*------------------------------------------------------------------------- */
static void separatorPulse(void)
{
    SET_PULSE_LOW();
    nsdelay(GB29768_TIMING_SEPARATOR_NS);

    SET_PULSE_HIGH();
}

/*------------------------------------------------------------------------- */
static int8_t gb29768Slot(tag_t *tag, uint8_t qCommand, uint32_t *checkEnding, bool fast, uint16_t *eventMask, gb29768_error_t *lastErr)
{
    *lastErr = GB29768_OKAY;

    /*********************************************************************************/
    /* 1. Send proper query command */
    /*********************************************************************************/
    SET_CRC_LED_OFF();
    switch(qCommand)
    {
        case ST25RU3993_CMD_QUERY: {
            *lastErr = gb29768SendCmdQuery(gb29768IntConfig.condition, gb29768IntConfig.session, gb29768IntConfig.target, gb29768IntConfig.trext, gb29768IntConfig.blf, gb29768IntConfig.coding);
            break;
        }
        case ST25RU3993_CMD_QUERYREP: {
            //query rep has already been sent
            *lastErr = gb29768SendCmdQueryRep(gb29768IntConfig.session);
            if (*checkEnding > 0) {
                (*checkEnding)--;
            }
            break;
        }
        case GB29768_CMD_DIVIDE0: {
            uint8_t position = 0;
            *lastErr = gb29768SendCmdDivide(position, gb29768IntConfig.session);
            (*checkEnding)++;
            break;
        }
        case GB29768_CMD_DIVIDE1: {
            uint8_t position = 1;
            *lastErr = gb29768SendCmdDivide(position, gb29768IntConfig.session);
            break;
        }
        case GB29768_CMD_SHRINK: {
            *lastErr = gb29768SendCmdShrink(gb29768IntConfig.session);
            *checkEnding = (*checkEnding)/2;
            break;
        }
        case GB29768_CMD_DISPERSE: {
            *lastErr = gb29768SendCmdDisperse(gb29768IntConfig.session);
            *checkEnding = (*checkEnding)*2 + 1;
            break;
        }
        default: {
            break;
        }
    }

    if(*lastErr != GB29768_OKAY)
    {
        gb29768RxFlush(gb29768IntConfig.blf);
        gb29768SendCmdNak();
        return (*lastErr == GB29768_NO_RESPONSE) ? 0 : -1;
    }
#if 0
    gen2GetRssiLin(&tag->rssiLinI, &tag->rssiLinQ);
#endif

    tag->handle[0] = gGb29768RxTxData.data[0];
    tag->handle[1] = gGb29768RxTxData.data[1];
    
    tag->tidlen = 0;
    tag->xpclen = 0;
    tag->epclen = 0;

    /*********************************************************************************/
    /* 2. Send ACK to receive epc                                                      */
    /*********************************************************************************/
    *lastErr = gb29768SendCmdAck(tag->handle);

#if !defined(NO_ACK_RETRY)
    uint32_t maxAckRetry = 2;
    while (((*lastErr == GB29768_CRC_ERROR) || (*lastErr == GB29768_PREAMBLE_ERROR) || (*lastErr == GB29768_STOPBIT_ERROR)) && maxAckRetry) {
        *lastErr = gb29768SendCmdAck(tag->handle);
        (*eventMask) |= EVENT_RESEND_ACK;
        maxAckRetry--;
    }
#endif

    if(*lastErr == GB29768_OKAY)
    {                
        uint32_t totalBytes = (gGb29768RxTxData.numBits) ? gGb29768RxTxData.numBytes + 1 : gGb29768RxTxData.numBytes;
        if (totalBytes >= 3) {
            uint16_t lenInBytes;
            uint16_t tmp = gGb29768RxTxData.data[0];           // Use u16 to handle U8 bit shifting 

            // page 31:
            // |     byte 0                          |     byte 1          |                 byte 2                   |
            // [2-bit security mode][8 bits coding length][8 bits coding header][coding length bytes coding, leading 0]
            tmp = (tmp << 8) | gGb29768RxTxData.data[1];    // Prepare U16 for bit shifting
            tag->pc[0] = (uint8_t)(tmp >> 6);
            tmp = (tmp << 8) | gGb29768RxTxData.data[2];             
            tag->pc[1] = (uint8_t)(tmp >> 6);

            lenInBytes = tag->pc[0] * 2;                    // Convert length to byte
            if(lenInBytes > MAX_EPC_LENGTH+2) {
                lenInBytes = MAX_EPC_LENGTH+2;
            }
            lenInBytes += 3;    // Take header into account in length

            for(uint32_t i=3 ; (i < lenInBytes) && (i < totalBytes) ; i++)
            {
                tmp = (tmp << 8) | gGb29768RxTxData.data[i];
                tag->epc[ (tag->epclen)++ ] = (uint8_t)(tmp >> 6);   // Only get bit6 to bit14
            }

            //LOG("Response okay: %d, %02x %02x\n",tags[numTags].epclen, tags[numTags].epc[0],tags[numTags].epc[1]);
        }
        else {
            *lastErr = GB29768_NO_RESPONSE;
        }
    }

    if (*lastErr == GB29768_NO_RESPONSE) {
        *lastErr = GB29768_COLLISION; // rename to collision error because we did not detect any problem with the RN16 but do not get anything for the EPC
    }
    if (*lastErr == GB29768_CRC_ERROR) {
        SET_CRC_LED_ON();
//LOG("ERR_CRC\n");
    }
    if (*lastErr != GB29768_OKAY) {
        SET_TAG_LED_OFF();

        gb29768RxFlush(gb29768IntConfig.blf);
        gb29768SendCmdNak();
        
        return -1;
    }

    SET_CRC_LED_OFF();
#if 0
    if (fast)
    {
        gen2GetAgcRssiLog(&tag->agc, &tag->rssiLogI, &tag->rssiLogQ);
        if((tag->pc[0] << 1) != tag->epclen)
        {
            return -1;
        }
    }
#else
    // REMINDER: update rssiLogI + rssiLogQ
    tag->rssiLogI = 8;
    tag->rssiLogQ = 7;
#endif

    return 1;
}

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
uint16_t gb29768SearchForTags(gb29768_SearchForTagsParams_t *p)
{
    gb29768_error_t  lastErr;
    uint16_t numTags = 0;
    int8_t  slotStatus;
    uint16_t eventMask = 0;
    uint32_t slotTime = 0;
    
    uint8_t continuousCollisionTime = 0;
    uint8_t continuousIdleTime = 0;

    uint32_t checkEndingThreshold = p->antiCollision.endThreshold;
    uint32_t ccn = p->antiCollision.ccnThreshold;     // continuous collision threshold: 3
    uint32_t cin = p->antiCollision.cinThreshold;     // continuous idle threshold: 4

    uint8_t cmd = ST25RU3993_CMD_QUERY;
    
    bool goOn, ready4MoreTags = true;
    bool singulate = p->singulate;
    
    if(p->cbFollowTagCommand != NULL){
        singulate = true;  // not fast
    }
#if 0
    uint8_t regProtCtrl;
    st25RU3993AntennaPower(1);
    st25RU3993ContinuousRead(ST25RU3993_REG_IRQSTATUS1, 2, &buf_[0]);   // ensure that IRQ bits are reset
    st25RU3993ClrResponse();

    regProtCtrl = st25RU3993SingleRead(ST25RU3993_REG_PROTOCOLCTRL);
    rxWithoutCRC = (regProtCtrl & 0x80) >> 7; // if crc is received fifo    
#endif

    uint32_t count = 1000;
    do{
        // execute slot ..
        eventMask = 0;
        slotTime = HAL_GetTick();
        slotStatus = gb29768Slot(p->tag, cmd, &checkEndingThreshold, singulate ? false : true, &eventMask, &lastErr);
        switch(slotStatus){
            case -1:{ // COLLISION: any communication problem is seen as collision
                continuousIdleTime=0;

                continuousCollisionTime++;
                cmd = (continuousCollisionTime < ccn) ? GB29768_CMD_DIVIDE0 : GB29768_CMD_DISPERSE;

                if((lastErr == GB29768_PREAMBLE_ERROR) || (lastErr == GB29768_COLLISION)) {
                    // treat only these errors as collision
                    p->statistics->collisionCnt++;
                    eventMask |= EVENT_COLLISION;
                }
                break;
            }

            case 0: { // EMPTY: empty slot
                //LOG("No response\n");
                continuousCollisionTime=0;

                continuousIdleTime++;
                if(continuousIdleTime < cin) {
                    cmd = (gLastCommand==GB29768_CMD_DIVIDE) ? GB29768_CMD_DIVIDE1 : ST25RU3993_CMD_QUERYREP;
                }
                else {
                    cmd = GB29768_CMD_SHRINK;
                }

                p->statistics->emptySlotCnt++;
                eventMask |= EVENT_EMPTY_SLOT;
                break;
            }   
            
            case 1:{ // TAG found  
                continuousIdleTime=0;
                continuousCollisionTime=0;

                cmd = ST25RU3993_CMD_QUERYREP;                    

                if(p->cbFollowTagCommand != NULL){
                    if (p->cbFollowTagCommand(p->tag) != ERR_NONE) {
                        // FollowTagCommand encountered issues, skip this tag
                        p->statistics->skipCnt++;
                        eventMask |= EVENT_SKIP_FOLLOW_CMD;
                        break;
                    }
                }

                // calc rssi log mean
                if (p->statistics->tagCnt == 0) {
                    // No tag inventoried yet, init value
                    gGb29768RssiLogIandQSum = 0;
                }
                gGb29768RssiLogIandQSum += p->tag->rssiLogI + p->tag->rssiLogQ;     // 0-30
                p->statistics->rssiLogMean = (uint8_t)((gGb29768RssiLogIandQSum + p->statistics->tagCnt) / (((uint32_t)p->statistics->tagCnt + 1UL) * 2UL));   // 0 - 15, tagCnt is added to LogIandQSUm to handle upper/lower integer rounding

                // get timestamp
                p->tag->timeStamp = slotTime;
                // increase tag count
                p->statistics->tagCnt++;
                numTags++;   
                eventMask |= EVENT_TAG_FOUND;
                
                // notify caller about tag 
                if(p->cbTagFound){
                    ready4MoreTags = p->cbTagFound(p->tag);
                }
                break;
            }

            default:{
                break;
            }
        }
        // update statistics
        switch(lastErr){
            case GB29768_PREAMBLE_ERROR:    p->statistics->preambleErrCnt++;    eventMask |= EVENT_PREAMBLE_ERR;    break;
            case GB29768_CRC_ERROR:         p->statistics->crcErrCnt++;         eventMask |= EVENT_CRC_ERR;         break;
            case GB29768_STOPBIT_ERROR:     p->statistics->headerErrCnt++;      eventMask |= EVENT_STOPBIT_ERR;     break;      // StopBit error count reported with headerErrCnt
            default: break;
        }

        // notify that slot has finished
        if(p->cbSlotFinished){
            p->cbSlotFinished(slotTime, eventMask, 0);
        }

        goOn = (p->cbContinueScanning == NULL) ? true : p->cbContinueScanning();
        count --;
    } while(checkEndingThreshold && goOn && ready4MoreTags && count);

    return numTags;
}

/*------------------------------------------------------------------------- */
gb29768_error_t gb29768AccessTag(tag_t *tag, const uint8_t storageArea, const uint8_t category, const uint8_t *password)
{
    gb29768_error_t resp;
    uint8_t         tmpPassword[2];
    
    resp = gb29768SendCmdGetRn(tag->handle);
    if(resp == GB29768_OKAY)
    {
        tag->handle[0] = gGb29768RxTxData.data[2];
        tag->handle[1] = gGb29768RxTxData.data[3];
        
        //gGb29768RxTxData.data[0] = 16-bit random number
        //gGb29768RxTxData.data[1] = 16-bit random number
        tmpPassword[0] = gGb29768RxTxData.data[0];
        tmpPassword[1] = gGb29768RxTxData.data[1];
        if ((storageArea != GB29768_AREA_TAGINFO) && (storageArea != GB29768_AREA_CODING) && (password != NULL)) {
            tmpPassword[0] ^= password[0];
            tmpPassword[1] ^= password[1];
        }

        resp = gb29768SendCmdAccess(storageArea, category, tmpPassword, tag->handle);
        if(resp==GB29768_OKAY)
        {
            tag->handle[0] = gGb29768RxTxData.data[1];
            tag->handle[1] = gGb29768RxTxData.data[2];
            resp = decodeOperatingState(gGb29768RxTxData.data[0]);
        }
    }
    return resp;
}

/*------------------------------------------------------------------------- */
gb29768_error_t gb29768LockTag(tag_t *tag, const uint8_t storageArea, const uint8_t configuration, const uint8_t action)
{
    gb29768_error_t resp;
    
    resp = gb29768SendCmdLock(storageArea, configuration, action, tag->handle);
    if(resp == GB29768_OKAY)
    {
        tag->handle[0] = gGb29768RxTxData.data[1];
        tag->handle[1] = gGb29768RxTxData.data[2];
        resp = decodeOperatingState(gGb29768RxTxData.data[0]);
    }
    return resp;
}

/*------------------------------------------------------------------------- */
gb29768_error_t gb29768KillTag(tag_t *tag)
{
    gb29768_error_t resp;
    
    resp = gb29768SendCmdKill(tag->handle);
    if(resp == GB29768_OKAY)
    {
        tag->handle[0] = gGb29768RxTxData.data[1];
        tag->handle[1] = gGb29768RxTxData.data[2];
        resp = decodeOperatingState(gGb29768RxTxData.data[0]);
    }
    return resp;
}

/*------------------------------------------------------------------------- */
gb29768_error_t gb29768EraseTag(tag_t *tag, const uint8_t storageArea, const uint16_t pointer, const uint16_t length)
{
    gb29768_error_t resp;
    
    resp = gb29768SendCmdRefreshHandle(tag->handle);
    if(resp == GB29768_OKAY)
    {
        tag->handle[0] = gGb29768RxTxData.data[0];
        tag->handle[1] = gGb29768RxTxData.data[1];
        
        resp = gb29768SendCmdErase(storageArea, (pointer+1)/2, (length+1)/2, tag->handle);  // Pointer & length are words
        if(resp == GB29768_OKAY)
        {
            tag->handle[0] = gGb29768RxTxData.data[1];
            tag->handle[1] = gGb29768RxTxData.data[2];
            resp = decodeOperatingState(gGb29768RxTxData.data[0]);
        }
    }
    return resp;
}

/*------------------------------------------------------------------------- */
gb29768_error_t gb29768WriteToTag(tag_t *tag, const uint8_t storageArea, const uint16_t pointer, const uint16_t length, const uint8_t *data)
{
    gb29768_error_t resp;
    
    resp = gb29768SendCmdRefreshHandle(tag->handle);
    if(resp == GB29768_OKAY)
    {
        tag->handle[0] = gGb29768RxTxData.data[0];
        tag->handle[1] = gGb29768RxTxData.data[1];
        
        resp = gb29768SendCmdWrite(storageArea, pointer, length, data, tag->handle);
        if(resp == GB29768_OKAY)
        {
            tag->handle[0] = gGb29768RxTxData.data[1];
            tag->handle[1] = gGb29768RxTxData.data[2];
            resp = decodeOperatingState(gGb29768RxTxData.data[0]);
        }
    }
    return resp;
}

/*------------------------------------------------------------------------- */
gb29768_error_t gb29768ReadFromTag(tag_t *tag, const uint8_t storageArea, const uint16_t pointer, uint16_t *length, uint8_t *destBuffer)
{
    gb29768_error_t resp;
    
    resp = gb29768SendCmdRead(storageArea, pointer, ((*length) + 1)/2, tag->handle);    // Length in words
    if(resp == GB29768_OKAY)
    {
        tag->handle[0] = gGb29768RxTxData.data[gGb29768RxTxData.numBytes-2];
        tag->handle[1] = gGb29768RxTxData.data[gGb29768RxTxData.numBytes-1];
        uint32_t  genuineLg = (gGb29768RxTxData.numBytes >= 3) ? gGb29768RxTxData.numBytes - 3 : 0;     // Removes tag reply + handle
        if ((*length == 0) || (genuineLg < *length)){
            // If size is 0, tag return all data from pointer to end of area
            *length = genuineLg;
        }
        if (*length > MAX_READ_DATA_LEN) {
            *length = MAX_READ_DATA_LEN;
        }
        memcpy(destBuffer, &gGb29768RxTxData.data[1], *length);
        resp = decodeOperatingState(gGb29768RxTxData.data[0]);
    }
    return resp;
}

/*------------------------------------------------------------------------- */
gb29768_error_t gb29768Sort(gb29768SortParams_t *p)
{
    return gb29768SendCmdSort((uint8_t)p->storageArea, p->target, p->rule, p->bitPointer, p->bitLength, p->mask);
}

/*------------------------------------------------------------------------- */
gb29768_error_t gb29768Configure(const gb29768Config_t *config)
{
    if (   (config->condition > GB29768_CONDITION_FLAG0)
        || (config->session > GB29768_SESSION_S3)
        || (config->target > GB29768_TARGET_1)
        || (config->blf > GB29768_BLF_640)
        || (config->coding > GB29768_COD_MILLER8)
        || (config->tc > GB29768_TC_12_5))
    {
        return GB29768_PARAM_ERROR;
    }

    memcpy(&gb29768IntConfig, config, sizeof(gb29768Config_t));
    return GB29768_OKAY;
}

/*------------------------------------------------------------------------- */
gb29768_error_t gb29768Open(const gb29768Config_t *config)
{    
    gb29768_error_t ret;

    ret = gb29768Configure(config);
    if (ret != GB29768_OKAY) {
        return ret;
    }

    st25RU3993SingleWrite(ST25RU3993_REG_TRCALHIGH,0x02);
    st25RU3993SingleWrite(ST25RU3993_REG_TRCALLOW,0x9B);
    st25RU3993SingleWrite(ST25RU3993_REG_AUTOACKTIMER,0x04);
    
    //st25RU3993SingleWrite(ST25RU3993_REG_RXMIXERGAIN, 0xF2);
    
    /* dir_mode: Direct */
    st25RU3993SingleWrite(ST25RU3993_REG_PROTOCOLCTRL, 0x40);
    
    /* 320 / 640 kHz BLF */
    uint8_t rxOptionsReg = 0x00;    
    st25RU3993SingleWrite(ST25RU3993_REG_RXOPTIONS, rxOptionsReg);      // Not used for GBT as Rx is in direct mode
    
    /* Tari 12.5us */
    uint8_t txOptionsReg = 0x00;
    st25RU3993SingleWrite(ST25RU3993_REG_TXOPTIONS, txOptionsReg);      // Not used for GBT as Tx is in direct mode

    /* No Response 690us*/
    st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, 0x0B);
    /* No Response 26.2ms*/
    //st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, 0xFF);

    /* Set Rx Wait Time to None */
    st25RU3993SingleWrite(ST25RU3993_REG_RXWAITTIME, 0x00);
    
    /* RX Filter Setting proposed in Datasheet */
    uint8_t rxFilterReg = 0x00;
    switch(gb29768IntConfig.blf)
    {
        case GB29768_BLF_64  /*0*/: rxFilterReg |= 0x9C; break;
        case GB29768_BLF_128 /*4*/: rxFilterReg |= 0x82; break;
        case GB29768_BLF_137 /*1*/: rxFilterReg |= 0x82; break;
        case GB29768_BLF_174 /*2*/: rxFilterReg |= 0x3E; break;
        case GB29768_BLF_274 /*5*/: rxFilterReg |= 0x3E; break;
        case GB29768_BLF_320 /*3*/: rxFilterReg |= (gb29768IntConfig.coding <= GB29768_COD_MILLER2) ? 0x2D : 0x2B; break;
        case GB29768_BLF_349 /*6*/: rxFilterReg |= (gb29768IntConfig.coding <= GB29768_COD_MILLER2) ? 0x2C : 0x20; break;
        case GB29768_BLF_640 /*7*/: rxFilterReg |= 0x02; break;
    }
    st25RU3993SingleWrite(ST25RU3993_REG_RXFILTER, rxFilterReg);

    /* ASK, Delimiter length 11.1us */
    st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL2, 0x8F);
    /* ASK, Delimiter length 12.5us */
    //st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL2, 0x9D);

    return GB29768_OKAY;
}


/*------------------------------------------------------------------------- */
gb29768_error_t gb29768Close(void)
{
    return GB29768_OKAY;
}




/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdSort(const uint8_t storageArea, const uint8_t target, const uint8_t rule, const uint16_t bitPointer, const uint8_t bitLength, const uint8_t *mask)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_SORT;
    gb29768AddByteToBitStream(gLastCommand,8);
    gb29768AddByteToBitStream(storageArea,6);
    gb29768AddByteToBitStream(target,4);
    gb29768AddByteToBitStream(rule,2);
    gb29768AddWordToBitStream(bitPointer,16);       // Points on bits
    gb29768AddByteToBitStream(bitLength,8);         // Length in bits
    gb29768AddArrayToBitStream(mask,bitLength);
    gb29768AddCrc();
    
    gb29768FinalizeCmdAndSend();

    // Waits minimal time for commands with no expected reply: T4
    nsdelay(GB29768_TIMING_T4min_NS(gb29768IntConfig.tc));      // Waits T4

    // No Tag Reply
    return GB29768_OKAY;
}

/*------------------------------------------------------------------------- */
//static gb29768_error_t gb29768SendCmdSecurity(const uint8_t *handle)
//{
//    gb29768InitializeCmd();
//
//    gLastCommand = GB29768_CMD_SECURITY;
//    gb29768AddByteToBitStream(gLastCommand,8);
//    gb29768AddByteToBitStream(handle[0],8);
//    gb29768AddByteToBitStream(handle[1],8);
//    gb29768AddCrc();
//    
//    gb29768FinalizeCmdAndSend();
//
//    gb29768_error_t resp = gb29768WaitForResponse(CRC16_CHECK_ON);
//    return resp;
//}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdQuery(const uint8_t condition, const uint8_t session, const uint8_t target, const uint8_t trext, const uint8_t blf, const uint8_t coding)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_QUERY;
    gb29768AddByteToBitStream(gLastCommand,8);
    gb29768AddByteToBitStream(condition,2);
    gb29768AddByteToBitStream(session,2);
    gb29768AddByteToBitStream(target,1);
    gb29768AddByteToBitStream(trext ? 1 : 0,1);
    gb29768AddByteToBitStream(blf,4);
    gb29768AddByteToBitStream(coding,2);
    gb29768AddCrc();
    
    gb29768FinalizeCmdAndSend();

    gb29768_error_t resp = gb29768WaitForResponse(CRC5_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdQueryRep(const uint8_t session)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_QUERYREP;
    gb29768AddByteToBitStream(gLastCommand,2);
    gb29768AddByteToBitStream(session,2);
    
    gb29768FinalizeCmdAndSend();
    //LOG("QUERY_REP: session=%01X\n",session);

    gb29768_error_t resp = gb29768WaitForResponse(CRC5_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdDivide(const uint8_t position, const uint8_t session)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_DIVIDE;
    gb29768AddByteToBitStream(gLastCommand,2);
    gb29768AddByteToBitStream(position,2);
    gb29768AddByteToBitStream(session,2);
    
    gb29768FinalizeCmdAndSend();
    //LOG("DIVIDE: pos=%01X session=%01X\n",position,session);
    
    gb29768_error_t resp = gb29768WaitForResponse(CRC5_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdDisperse(const uint8_t session)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_DISPERSE;
    gb29768AddByteToBitStream(gLastCommand,4);
    gb29768AddByteToBitStream(session,2);
    
    gb29768FinalizeCmdAndSend();
    //LOG("DISPERSE: session=%01X\n",session);

    gb29768_error_t resp = gb29768WaitForResponse(CRC5_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdShrink(const uint8_t session)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_SHRINK;
    gb29768AddByteToBitStream(gLastCommand,4);
    gb29768AddByteToBitStream(session,2);
    
    gb29768FinalizeCmdAndSend();
    //LOG("SHRINK: session=%01X\n",session);

    gb29768_error_t resp = gb29768WaitForResponse(CRC5_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdAck(const uint8_t *handle)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_ACK;
    gb29768AddByteToBitStream(gLastCommand,2);
    gb29768AddByteToBitStream(handle[0],8);
    gb29768AddByteToBitStream(handle[1],8);
    
    gb29768FinalizeCmdAndSend();
    //LOG("ACK: handle=%04X\n",handle);

    gb29768_error_t resp = gb29768WaitForResponse(CRC16_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdNak(void)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_NAK;
    gb29768AddByteToBitStream(gLastCommand,8);

    gb29768FinalizeCmdAndSend();
    //LOG("NAK\n");

    // Waits minimal time for commands with no expected reply: T4
    nsdelay(GB29768_TIMING_T4min_NS(gb29768IntConfig.tc));      // Waits T4

    // No Tag Reply
    return GB29768_OKAY;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdRefreshHandle(const uint8_t *handle)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_REFRESHHANDLE;
    gb29768AddByteToBitStream(gLastCommand,8);
    gb29768AddByteToBitStream(handle[0],8);
    gb29768AddByteToBitStream(handle[1],8);
    gb29768AddCrc();
    
    gb29768FinalizeCmdAndSend();

    gb29768_error_t resp = gb29768WaitForResponse(CRC5_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdGetRn(const uint8_t *handle)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_GETRN;
    gb29768AddByteToBitStream(gLastCommand,8);
    gb29768AddByteToBitStream(handle[0],8);
    gb29768AddByteToBitStream(handle[1],8);
    gb29768AddCrc();

    gb29768FinalizeCmdAndSend();

    gb29768_error_t resp = gb29768WaitForResponse(CRC16_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdAccess(const uint8_t storageArea, const uint8_t category, const uint8_t * password, const uint8_t *handle) 
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_ACCESS;
    gb29768AddByteToBitStream(gLastCommand,8);
    gb29768AddByteToBitStream(storageArea,6);
    gb29768AddByteToBitStream(category,4);
    gb29768AddByteToBitStream(password[0],8);
    gb29768AddByteToBitStream(password[1],8);
    gb29768AddByteToBitStream(handle[0],8);
    gb29768AddByteToBitStream(handle[1],8);
    gb29768AddCrc();
    
    gb29768FinalizeCmdAndSend();

    gb29768_error_t resp = gb29768WaitForResponse(CRC16_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdRead(const uint8_t storageArea, const uint16_t pointer, const uint16_t length, const uint8_t *handle)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_READ;
    gb29768AddByteToBitStream(gLastCommand,8);
    gb29768AddByteToBitStream(storageArea,6);
    gb29768AddWordToBitStream(pointer,16);
    gb29768AddWordToBitStream(length,16);
    gb29768AddByteToBitStream(handle[0],8);
    gb29768AddByteToBitStream(handle[1],8);
    gb29768AddCrc();
    
    gb29768FinalizeCmdAndSend();

    gb29768_error_t resp = gb29768WaitForResponse(CRC16_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdWrite(const uint8_t storageArea, const uint16_t pointer, const uint16_t length, const uint8_t *data, const uint8_t *handle)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_WRITE;
    gb29768AddByteToBitStream(gLastCommand,8);
    gb29768AddByteToBitStream(storageArea,6);
    gb29768AddWordToBitStream(pointer,16);
    gb29768AddWordToBitStream(length,16);
    gb29768AddArrayToBitStream(data,length*16);
    gb29768AddByteToBitStream(handle[0],8);
    gb29768AddByteToBitStream(handle[1],8);
    gb29768AddCrc();
    
    gb29768FinalizeCmdAndSend();

    gb29768_error_t resp = gb29768WaitForResponse(CRC16_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdErase(const uint8_t storageArea, const uint16_t pointer, const uint16_t length, const uint8_t *handle)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_ERASE;
    gb29768AddByteToBitStream(gLastCommand,8);
    gb29768AddByteToBitStream(storageArea,6);
    gb29768AddWordToBitStream(pointer,16);
    gb29768AddWordToBitStream(length,16);
    gb29768AddByteToBitStream(handle[0],8);
    gb29768AddByteToBitStream(handle[1],8);
    gb29768AddCrc();
    
    gb29768FinalizeCmdAndSend();

    gb29768_error_t resp = gb29768WaitForResponse(CRC16_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */ 
static gb29768_error_t gb29768SendCmdLock(const uint8_t storageArea, const uint8_t configuration, const uint8_t action, const uint8_t *handle)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_LOCK;
    gb29768AddByteToBitStream(gLastCommand,8);
    gb29768AddByteToBitStream(storageArea,6);
    gb29768AddByteToBitStream(configuration,2);
    gb29768AddByteToBitStream(action,2);
    gb29768AddByteToBitStream(handle[0],8);
    gb29768AddByteToBitStream(handle[1],8);
    gb29768AddCrc();
    
    gb29768FinalizeCmdAndSend();
    
    gb29768_error_t resp = gb29768WaitForResponse(CRC16_CHECK_ON);
    return resp;
}

/*------------------------------------------------------------------------- */
static gb29768_error_t gb29768SendCmdKill(const uint8_t *handle)
{
    gb29768InitializeCmd();

    gLastCommand = GB29768_CMD_KILL;
    gb29768AddByteToBitStream(gLastCommand,8);
    gb29768AddByteToBitStream(handle[0],8);
    gb29768AddByteToBitStream(handle[1],8);
    gb29768AddCrc();
    
    gb29768FinalizeCmdAndSend();

    gb29768_error_t resp = gb29768WaitForResponse(CRC16_CHECK_ON);
    return resp;
}


/*------------------------------------------------------------------------- */
static void gb29768InitializeCmd(void)
{
    gGb29768RxTxData.data[0] = 0x00;
    gGb29768RxTxData.numBytes = 0;
    gGb29768RxTxData.numBits = 0;
}

/*------------------------------------------------------------------------- */
static void gb29768AddCrc(void)
{
    uint16_t num = gGb29768RxTxData.numBytes*8 + gGb29768RxTxData.numBits;
    if (num%2 != 0)
    {
        gb29768AddByteToBitStream(0x00,1);
        num++;
    }
    uint16_t crc = crc16Bitwise(gGb29768RxTxData.data, num);
    gb29768AddWordToBitStream(crc,16);
}

/*------------------------------------------------------------------------- */
static void output2Bits(uint8_t bits)
{
    switch(bits & 0x03)
    {
        case 0x00: highLowPulse(1);break;       // 1 High + 1 Low=2 TC (25us   @ tc=12.5)
        case 0x01: highLowPulse(2);break;       // 2 High + 1 Low=3 TC (37.5us @ tc=12.5)
        case 0x03: highLowPulse(3);break;       // 3 High + 1 Low=4 TC (50us   @ tc=12.5)
        case 0x02: highLowPulse(4);break;       // 4 High + 1 Low=5 TC (62.5us @ tc=12.5)
    }
}

/*------------------------------------------------------------------------- */
static void gb29768FinalizeCmdAndSend(void)
{
    DEBUG_STORE_FRAME(GB29768FRAMES_SND, GB29768_OKAY, false);

    DIRECT_MODE_ENABLE_SENDER();

    INIT_DWT();
    // Ensure pulse state is HIGH
    SET_PULSE_HIGH();
    
    // Lead Code
    separatorPulse();   // 12.5us
    highLowPulse(7);    // 7 High + 1 Low=8 TC (100us @ tc=12.5)
    highLowPulse(1);    // 1 High + 1 Low=2 TC (25us  @ tc=12.5)
    
    // Send all bytes
    uint32_t i;
    uint8_t data;
    for(i=0 ; i<gGb29768RxTxData.numBytes ; ++i)
    {
        data = gGb29768RxTxData.data[i];
        output2Bits((data&0xC0)>>6);
        output2Bits((data&0x30)>>4);
        output2Bits((data&0x0C)>>2);
        output2Bits(data&0x03);
    }

    // Send remaining bits
    if (gGb29768RxTxData.numBits) {
        data = gGb29768RxTxData.data[i];  // Get last byte partial content

        if(gGb29768RxTxData.numBits >= 1) { output2Bits((data&0xC0)>>6); }
        if(gGb29768RxTxData.numBits >= 3) { output2Bits((data&0x30)>>4); }
        if(gGb29768RxTxData.numBits >= 5) { output2Bits((data&0x0C)>>2); }
        if(gGb29768RxTxData.numBits >= 7) { output2Bits((data&0x03)>>0); }
    }

    DIRECT_MODE_ENABLE_RECEIVER();

    // Reinit Rx/Tx data structure before Receive state
    gb29768InitializeCmd();
}

/*------------------------------------------------------------------------- */
static void gb29768AddByteToBitStream(const uint8_t data, const uint8_t bits)
{
    if(bits > 0)
    {
        if(gGb29768RxTxData.numBits == 8)
        {
            gGb29768RxTxData.numBytes++;
            gGb29768RxTxData.data[ gGb29768RxTxData.numBytes ] = 0x00;
            gGb29768RxTxData.numBits = 0;
        }

        uint32_t cumulatedBits = gGb29768RxTxData.numBits + bits;
        if (cumulatedBits >= 8)
        {
            // 111x xxxx | xxxx xxxx   gGb29768RxTxData.numBits = 3, bits = 6
            // 1110 0000 | xxxx xxxx
            gGb29768RxTxData.data[ gGb29768RxTxData.numBytes ] |= (uint8_t)(data >> (cumulatedBits-8));
            gGb29768RxTxData.numBytes++;
            
            // 1110 0000 | 0xxx xxxx
            gGb29768RxTxData.data[ gGb29768RxTxData.numBytes ] = (uint8_t)(data << (16-cumulatedBits));
            gGb29768RxTxData.numBits = cumulatedBits - 8;
        }
        else
        {
            gGb29768RxTxData.data[ gGb29768RxTxData.numBytes ] |= (uint8_t)(data << (8-cumulatedBits));
            gGb29768RxTxData.numBits = cumulatedBits;
        }
    }
}

/*------------------------------------------------------------------------- */
static void gb29768AddWordToBitStream(const uint16_t data, const uint8_t bits)
{
    if(bits>8) {
        gb29768AddByteToBitStream((uint8_t)(data>>8), 8);
        gb29768AddByteToBitStream((uint8_t)(data), bits-8);
    }
    else {
        gb29768AddByteToBitStream((uint8_t)(data>>8), bits);
    }
}

/*------------------------------------------------------------------------- */
static void gb29768AddArrayToBitStream(const uint8_t *data, uint16_t bits)
{
    uint32_t indexMax = ((bits+7)/8) - 1;
    for(uint32_t index=0 ; index<indexMax ; ++index) {
        gb29768AddByteToBitStream(data[index], 8);
        bits -= 8;
    }
    gb29768AddByteToBitStream(data[indexMax], bits);
}

/*------------------------------------------------------------------------- */
static gb29768_error_t decodeOperatingState(const uint8_t operatingState)
{
    LOG("OS: %02X\n",operatingState);
    // see OPERATING_STATE_* defines
    
    switch(operatingState)
    {
        case OPERATING_STATE_SUCCESS:
        {
            LOG("Tag: Success\n");
            return GB29768_OKAY;
        }
        case OPERATING_STATE_POWER_SHORTAGE:
        {
            LOG("Tag: Power Shortage\n");
            return GB29768_TAG_POWER_SHORTAGE;
        }
        case OPERATING_STATE_PERMISSION_ERROR:
        {
            LOG("Tag: Insufficient Permissions\n");
            return GB29768_TAG_PERMISSION_ERROR;
        }
        case OPERATING_STATE_STORAGE_OVERFLOW:
        {
            LOG("Tag: Storage Area Overflow\n");
            return GB29768_TAG_STORAGE_OVERFLOW;
        }
        case OPERATING_STATE_STORAGE_LOCKED:
        {
            LOG("Tag: Storage Area Locked\n");
            return GB29768_TAG_STORAGE_LOCKED;
        }
        case OPERATING_STATE_PASSWORD_ERROR:
        {
            LOG("Tag: False Password\n");
            return GB29768_TAG_PASSWORD_ERROR;
        }
        case OPERATING_STATE_AUTH_ERROR:
        {
            LOG("Tag: Authentication Failure\n");
            return GB29768_TAG_AUTH_ERROR;
        }
        case OPERATING_STATE_UNKNOWN_ERROR:
        {
            LOG("Tag: Unknown Error\n");
            return GB29768_TAG_UNKNOWN_ERROR;
        }
        default:
        {
            return GB29768_OKAY;
        }
    }
}
#endif

/**
  * @}
  */
/**
  * @}
  */
