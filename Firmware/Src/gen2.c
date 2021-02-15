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
 *  @brief This file includes functions providing an implementation of the 
 *  ISO6c aka GEN2 RFID EPC protocol.
 *
 *  Detailed documentation of the provided functionality can be found in gen2.h.
 */

/** @addtogroup Protocol
  * @{
  */
/** @addtogroup Gen2
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include <string.h>
#include <math.h> 

#include "st25RU3993_config.h"
#include "platform.h"
#include "st25RU3993.h"
#include "logger.h"
#include "timer.h"
#include "gen2.h"
#include "global.h"
#include "crc.h"
#include "stream_dispatcher.h"
#include "leds.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
/** Definition for the CRC length */
#define CRCLENGTH         2

/* EPC Commands */
/** Definition for queryrep EPC command */
#define EPC_QUERYREP      0
/** Definition for acknowledge EPC command */
#define EPC_ACK           1

/** Definition for query EPC command */
#define EPC_QUERY         0x08
/** Definition for query adjust EPC command */
#define EPC_QUERYADJUST   0x09
/** Definition for select EPC command */
#define EPC_SELECT        0x0A

/** Definition for not acknowledge EPC command */
#define EPC_NAK           0xC0
/** Definition for request new RN16 or handle */
#define EPC_REQRN         0xC1
/** Definition for read EPC command */
#define EPC_READ          0xC2
/** Definition for write EPC command */
#define EPC_WRITE         0xC3
/** Definition for kill EPC command */
#define EPC_KILL          0xC4
/** Definition for lock EPC command */
#define EPC_LOCK          0xC5
/** Definition for access EPC command */
#define EPC_ACCESS        0xC6
/** Definition for blockwrite EPC command */
#define EPC_BLOCKWRITE    0xC7
/** Definition for blockerase EPC command */
#define EPC_BLOCKERASE    0xC8
/** Definition for block permalock EPC command */
#define EPC_BLOCKPERMALOCK 0xC9

#define GEN2_RESET_TIMEOUT  10
#define MAX_Q               15


/* STOREDPC bits */
#define STOREDPC_L      0xF8
#define STOREDPC_UMI    0x04
#define STOREDPC_XI     0x02
#define STOREDPC_T      0x01


/* XPC bits */
#define XPC_W1B0_XEB    0x80
#define XPC_W1B1_B      0x80
#define XPC_W1B1_C      0x40
#define XPC_W1B1_SLI    0x20
#define XPC_W1B1_TN     0x10
#define XPC_W1B1_U      0x08
#define XPC_W1B1_K      0x04
#define XPC_W1B1_NR     0x02
#define XPC_W1B1_H      0x01

/* Tari defines */
#define TARI_6_25         0 /**< set tari to  6.25 us */
#define TARI_12_50        1 /**< set tari to 12.5  us */
#define TARI_25_00        2 /**< set tari to 25    us */

/*
******************************************************************************
* STRUCTS
******************************************************************************
*/
/**< Gen2 configuration parameters + additional parameters used internally */
typedef struct
{
    gen2Config_t config;
    uint8_t DR;             /**< Division ratio */
    uint8_t noRespTime;     /**< value for ST25RU3993_REG_RXNORESPONSEWAITTIME */
} gen2InternalConfig_t;

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/

/*
******************************************************************************
* EXTERNAL
******************************************************************************
*/
extern volatile uint16_t st25RU3993Response;

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
/** Global buffer for generating data, sending to the Tag.\n
  * Caution: In case of a global variable pay attention to the sequence of generating
  * and executing epc commands. */
static uint8_t buf_[8 + MAX_EPC_LENGTH + MAX_PC_LENGTH + CRCLENGTH]; /*8->tx_length+EPCcommand+wordptr+wordcount+handle+broken byte */

static gen2InternalConfig_t gen2IntConfig;

/** Variable holds RX_crc_n bit value of Register ST25RU3993_REG_PROTOCOLCTRL.
  * Needed to adjust expected receive length.*/
static uint8_t gRxWithoutCRC = 0;

static int8_t lastErr = 0;

static uint32_t gGen2RssiLogIandQSum;

static bool gGen2Truncate = false;

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
/** EPC REQRN command send to the Tag. This function is used to
  * request a new RN16 or a handle from the tag.
  *
  * @param *handle Pointer to the first byte of the handle.
  * @param *destHandle Pointer to the first byte of the backscattered handle.
  *
  * @return The function returns an errorcode.
                  0x00 means no Error occurred.
                  0xFF means Error occurred.
  */
static int8_t gen2ReqRNHandleChar(uint8_t const *handle, uint8_t *destHandle);

/**
 * Sends a Query, QueryRep or QueryAdjust command and checks for replies from tags.
 * If a tag is found, the Ack command is sent. If fast mode is not enabled the Reg_RN
 * command will be sent after receiving the Ack reply.\n
 * All of the data which is retrieved from the tag is written into parameter tag.
 * @param tag The reply data of the tag is written into this data structure
 * @param manualAck Does not use autoACK feature of reader
 * @param qCommand The command which should be sent. Should be Query, QueryRep or QueryAdjust
 * @param q Q as specified in the Gen2 spec
 * @param fast If fast mode is activated no Req_RN command will be sent after receiving the
 * Ack reply. If no QueryRep, QueryAdjust or Reg_RN is sent to the tag within T2 the tag will
 * go back to arbitrate state.
 * @param followCommand The command which will be sent after a slot has been processed
 * successfully (valid tag response). Usually this will be used to send a QueryRep
 * immediately after receiving a tag response to trigger a change of the current
 * session flag.
 * @return 1 if one tag has been successfully read, 0 if no response in slot, -1 if error occurred (collision)
 */
static int8_t gen2Slot(tag_t *tag, bool manualAck, uint8_t qCommand, uint8_t q, bool fast, uint8_t followCommand, uint16_t *eventMask);

static uint8_t gen2InsertEBV(uint32_t value, uint8_t *p, uint8_t bitpos);
static void gen2GetAgcRssiLog(uint8_t *agc, uint8_t *rssiLogI, uint8_t *rssiLogQ);
static void gen2GetRssiLin(int8_t *rssiLinI, int8_t *rssiLinQ);
static void gen2PrepareQueryCmd(uint8_t *buf, uint8_t q);

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
static uint8_t gen2InsertEBV(uint32_t value, uint8_t *p, uint8_t bitpos)
{
    uint8_t ebv[5];
    uint8_t ebvlen;
    uint32ToEbv(value, ebv, &ebvlen);
    insertBitStream(p, ebv, ebvlen, bitpos);
    return ebvlen;
}

/*------------------------------------------------------------------------- */
static void gen2GetAgcRssiLog(uint8_t *agc, uint8_t *rssiLogI, uint8_t *rssiLogQ)
{
    uint8_t buf[2];
    st25RU3993ContinuousRead(ST25RU3993_REG_AGCANDSTATUS, 2, buf);
    *agc = buf[0];
    *rssiLogI = (uint8_t)(buf[1] & 0x0F);
    *rssiLogQ = (uint8_t)(buf[1] >> 4);
}

/*------------------------------------------------------------------------- */
static void gen2GetRssiLin(int8_t *rssiLinI, int8_t *rssiLinQ)
{
    uint8_t regMeas = st25RU3993SingleRead(ST25RU3993_REG_MEASUREMENTCONTROL);

    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, (regMeas & 0xF0) | 0x0B);  // set msel bits -> RSSI I level
    *rssiLinI = st25RU3993GetADC();
    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, (regMeas & 0xF0) | 0x0C);  // set msel bits -> RSSI Q level
    *rssiLinQ = st25RU3993GetADC();

    // restore register value
    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, regMeas);
}

/*------------------------------------------------------------------------- */
static void gen2PrepareQueryCmd(uint8_t *buf, uint8_t q)
{
    buf[0] = ((gen2IntConfig.DR<<5)&0x20)/*DR*/ |
              ((gen2IntConfig.config.coding<<3)&0x18)/*M*/ |
              (((gen2IntConfig.config.trext ? 1 : 0)<<2)&0x04)/*TREXT*/ |
              ((gen2IntConfig.config.sel<<0)&0x03)/*SEL*/;
    buf[1] = ((gen2IntConfig.config.session<<6)&0xC0)/*SESSION*/ |
              ((gen2IntConfig.config.target<<5)&0x20)/*TARGET*/ |
              ((q<<1)&0x1E)/*Q*/;
}

/*------------------------------------------------------------------------- */
static int8_t gen2Slot(tag_t *tag, bool manualAck, uint8_t qCommand, uint8_t q, bool fast, uint8_t followCommand, uint16_t *eventMask)
{
    uint16_t rxlen;
    uint8_t  *rxBuf;
    uint8_t  nextCmd;
    bool     waitTxIrq;
    lastErr = 0;

    /*********************************************************************************/
    /* 1. Send proper query command */
    /*********************************************************************************/
    if (manualAck) {
        rxlen = 16;
        rxBuf = tag->rn16;
        nextCmd = ST25RU3993_CMD_ACK;
        waitTxIrq = true;
    }
    else {
        rxlen = sizeof(buf_)*8;  //receive all data, length auto-calculated by ST25RU3993
        rxBuf = buf_;
        nextCmd = fast ? followCommand : 0;
        waitTxIrq = false;
    }
    SET_CRC_LED_OFF();
    switch(qCommand)
    {
        case ST25RU3993_CMD_QUERY:
        {
            gen2PrepareQueryCmd(buf_, q);
            lastErr = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_QUERY, buf_, 16, rxBuf, &rxlen, gen2IntConfig.noRespTime, nextCmd, waitTxIrq);
            break;
        }
        case 0:
        {
            //query rep has already been sent
            lastErr = st25RU3993TxRxGen2Bytes(0, NULL, 0, rxBuf, &rxlen, gen2IntConfig.noRespTime, nextCmd, waitTxIrq);
            break;
        }
        default:
        {
            lastErr = st25RU3993TxRxGen2Bytes(qCommand, NULL, 0, rxBuf, &rxlen, gen2IntConfig.noRespTime, nextCmd, waitTxIrq);
            break;
        }
    }
    if(lastErr == ST25RU3993_ERR_NORES) {
        return 0;
    }
    if(lastErr < 0) {
        return -1;
    }
    gen2GetRssiLin(&tag->rssiLinI, &tag->rssiLinQ);

    if (manualAck) {
        /**************************************************************************************/
        /* 2. After sending ACK, Receive pc, xpc, epc, send REQRN immediately after FIFO has the data  */
        /**************************************************************************************/
        rxlen = sizeof(buf_)*8;  //receive all data, length auto-calculated by ST25RU3993
#ifdef NO_ACK_RETRY
        lastErr = st25RU3993TxRxGen2Bytes(0, buf_, 0, buf_, &rxlen, gen2IntConfig.noRespTime, fast ? followCommand : ST25RU3993_CMD_REQRN, 0);
#else
        #define MAX_ACK_RETRY   2
        uint8_t maxAckRetry = MAX_ACK_RETRY;
        lastErr = st25RU3993RxGen2EPC(buf_, &rxlen, gen2IntConfig.noRespTime, fast ? followCommand : ST25RU3993_CMD_REQRN, 0, &maxAckRetry);
        if(maxAckRetry != MAX_ACK_RETRY){
            (*eventMask) |= EVENT_RESEND_ACK;            
        }
#endif

        if(lastErr == ST25RU3993_ERR_NORES) {
            lastErr = ST25RU3993_ERR_COLL; // rename to collision error because we did not detect any problem with the RN16 but did not get anything for the EPC
            return -1;
        }
        else if(lastErr == ST25RU3993_ERR_CRC) {
            SET_CRC_LED_ON();
            return -1;
        }
    }

    bool truncating = (gGen2Truncate) && (gen2IntConfig.config.sel & 0x02); // Truncated and (SL or ~SL)
    if (truncating) {
        if(lastErr < 0 || rxlen < 5) {   // Not even Truncated PC received
            SET_TAG_LED_OFF();
            return -1;
        }

        tag->pc[0] = buf_[0] & 0xF8;
        tag->pc[1] = 0x00;

        tag->xpclen = 0;
        tag->epclen = 0;

        if ((tag->pc[0] & 0xF8) != 0x00) {      // Gen2V2 spec p45: "When sending a truncated EPC a tag substitute 00000 for its PC field"
            lastErr = ST25RU3993_ERR_COLL;          // Issue with PC
            return -1;
        }

        uint16_t effectiveBits = rxlen;
        if(gRxWithoutCRC) {
            bool crcIsOk = false;
            if (effectiveBits >= 16) {
                effectiveBits -= 16;        // Remove CRC

                uint16_t crc = crc16Bitwise(buf_, effectiveBits);
                uint16_t crc_recv;
                extractBitStream((uint8_t *)&crc_recv, buf_, 16, effectiveBits);
                crcIsOk = (crc == crc_recv);
            }
            if (crcIsOk)
            {
                lastErr = ST25RU3993_ERR_CRC;
                SET_CRC_LED_ON();
                return -1;
            }
        }
        SET_CRC_LED_OFF();

        if(effectiveBits < 5) {     // Did not receive enough bytes (at least 00000+CRC)
            SET_TAG_LED_OFF();
            return -1;
        }

        effectiveBits -= 5;         // Remove truncatedPC
        extractBitStream(tag->epc, buf_, effectiveBits, 5);
        tag->epclen = (effectiveBits+7)/8;
    }
    else {
        if(lastErr < 0 || rxlen < 16) {   // Not even PC received
            SET_TAG_LED_OFF();
            return -1;
        }

        tag->pc[0] = buf_[0];
        tag->pc[1] = buf_[1];

        tag->xpclen = 0;
        if ((tag->pc[0] & STOREDPC_XI) == STOREDPC_XI) {
            // Tag has at least XPC_W1 ...
            tag->xpc[0] = buf_[2];
            tag->xpc[1] = buf_[3];
            tag->xpclen += 2;
            
            if ((tag->xpc[0] & XPC_W1B0_XEB) == XPC_W1B0_XEB) {
                // ... and tag has XPC_W2
                tag->xpc[2] = buf_[4];
                tag->xpc[3] = buf_[5];
                tag->xpclen += 2;
            }
        }
        
        tag->epclen = (rxlen+7)/8;
        if(gRxWithoutCRC) {
            tag->epclen -= 2;   // Remove CRC from computation

            /** CRC Check done here */
            uint16_t crc = crc16Bytewise(buf_, tag->epclen);
            uint16_t crc_recv = (buf_[tag->epclen]<<8) | buf_[tag->epclen+1];
            if(crc != crc_recv)
            {
                lastErr = ST25RU3993_ERR_CRC;
                SET_CRC_LED_ON();
                return -1;
            }
        }
        SET_CRC_LED_OFF();

        if(tag->epclen < 2 + tag->xpclen) {   // Did not receive enough bytes
            SET_TAG_LED_OFF();
            return -1;
        }

        tag->epclen -= 2 + tag->xpclen;     // Remove PC + XPC length
        if(tag->epclen > MAX_EPC_LENGTH) {
            tag->epclen = MAX_EPC_LENGTH;
        }
        memcpy(tag->epc, buf_ + 2 + tag->xpclen, tag->epclen);
    }
    
    /*********************************************************************************/
    /* 3. Receive the two bytes handle */
    /*********************************************************************************/
    if(! fast)
    {
        rxlen = 32;
        lastErr = st25RU3993TxRxGen2Bytes(0,NULL,0,tag->handle,&rxlen,gen2IntConfig.noRespTime,followCommand,0);
    }

    // get Agc & RssiLog
    gen2GetAgcRssiLog(&tag->agc, &tag->rssiLogI, &tag->rssiLogQ);
    if (lastErr < 0) {
        return -1;
    }
    if ((!truncating) && ((tag->pc[0] & STOREDPC_L) >> 2) != (tag->epclen+tag->xpclen)) {    // Gen2V2 spec p45: "A packet PC differs from a StoredPC in its L bits, which a Tag adjust to match the length of the data that follow the PC word (ie XPC_Wx)"
        lastErr = ST25RU3993_ERR_COLL;      // rename to collision error because we did not detect any problem with the handle but did not get the correct StoredPC L
        return -1;
    }

    return 1;
}

/*------------------------------------------------------------------------- */
static int8_t gen2ReqRNHandleChar(uint8_t const *handle, uint8_t *destHandle)
{
    uint16_t rxbits = 32;

    buf_[0] = EPC_REQRN;                 /*Command REQRN */
    buf_[1] = handle[0];
    buf_[2] = handle[1];

    lastErr = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_TRANSMCRC,buf_,24,destHandle,&rxbits,gen2IntConfig.noRespTime,0,1);

    if(lastErr < 0) {
        return GEN2_ERR_REQRN;
    }

    return GEN2_OK;
}

/*------------------------------------------------------------------------- */
static int8_t gen2ProcessErrorCode(uint8_t code)
{
    /* Translates error codes defined in Gen2V2 spec Annex I into FW specific */
    switch (code) {
        case 0x00: return ERR_GEN2_ERRORCODE_OTHER          ;
        case 0x01: return ERR_GEN2_ERRORCODE_NOTSUPPORTED   ;
        case 0x02: return ERR_GEN2_ERRORCODE_PRIVILEGES     ;
        case 0x03: return ERR_NOMEM                         ;        // Use generic error code to keep FW backward compatibility
        case 0x04: return ERR_GEN2_ERRORCODE_MEMLOCKED      ;
        case 0x05: return ERR_GEN2_ERRORCODE_CRYPTO         ;
        case 0x06: return ERR_GEN2_ERRORCODE_ENCAPSULATION  ;
        case 0x07: return ERR_GEN2_ERRORCODE_RESPBUFOVERFLOW;
        case 0x08: return ERR_GEN2_ERRORCODE_SECURITYTIMEOUT;
        case 0x0B: return ERR_GEN2_ERRORCODE_POWER_SHORTAGE ;
        case 0x0F: return ERR_GEN2_ERRORCODE_NONSPECIFIC    ;
        default:   return ERR_GEN2_ERRORCODE_NONSPECIFIC    ;
    }
}

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void gen2Select(STUHFL_T_Gen2_Select *select)
{
    uint16_t len = select->maskLen;
    uint8_t ebvLen;
    uint16_t rxbits = 1;
    uint8_t *mask = select->mask;
    uint16_t j,i;
    uint8_t resp_null;
    uint8_t jLeftBits;
    uint8_t posTruncbit;
    st25RU3993ClrResponse();
    memset(buf_,0,sizeof(buf_));

    buf_[0] = ((EPC_SELECT<<4)&0xF0) | ((select->target<<1)&0x0E)   | ((select->action>>2)&0x01);
    buf_[1] = ((select->action<<6)&0xC0)  | ((select->memBank<<4)&0x30);

    ebvLen = gen2InsertEBV(select->maskAddress, buf_+1, 4);
    buf_[1+ebvLen] |= ((select->maskLen>>4)&0x0F);
    buf_[2+ebvLen] = ((select->maskLen<<4)&0xF0);

    i = 2+ebvLen; /* the counter set to last buffer location */
    for(j=len ; j>=8 ; j-=8,mask++)
    {
        buf_[i] |= ((*mask>>4)&0x0F);
        i++;
        buf_[i] = ((*mask<<4)&0xF0);
    }

    if(j==0)
    {
        buf_[i] |= ((select->truncation<<3)&0x08);   /* Truncate */
    }
    else    // if length is not dividable by 8
    {
        jLeftBits = 0xFF << (8-j);

        buf_[i]   |= (jLeftBits >> 4) & (*mask >> 4);
        buf_[i+1]  = (jLeftBits << 4) & (*mask << 4);
        // add truncation bit
        posTruncbit = (select->truncation & 0x01) <<(7-j);
        buf_[i]   |= (posTruncbit >> 4) ;
        buf_[i+1] |= (posTruncbit << 4) ;
    }

    len += 21 + ebvLen * 8; /* Add the bits for EPC command before continuing */

    // Handle truncation for next ACK
    gGen2Truncate = ((select->truncation) && (select->target == GEN2_TARGET_SL) && (select->memBank == GEN2_MEMORY_BANK_EPC));

    /* Pseudo 1-bit receival with small timeout to have ST25RU3993 state machine
       finished and avoiding spurious interrupts (no response) */
    lastErr = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_TRANSMCRC, buf_, len, &resp_null, &rxbits, 1, 0, 1);

    if (gen2IntConfig.config.T4Min != TIMING_USE_DEFAULT_T4MIN) {
        delay_us(gen2IntConfig.config.T4Min);
    }
    else {
        switch (gen2IntConfig.config.tari) {
            case TARI_25_00: delay_us(150); break;  // T4: 150us: 2.0*3*Tari @ Tx_one_length = 2
            case TARI_12_50: delay_us(75);  break;  // T4: 75us: 2.0*3*Tari @ Tx_one_length = 2
            default:         delay_us(38);  break;  // T4: 37.5us: 2.0*3*Tari @ Tx_one_length = 2
        }
    }
}

/*------------------------------------------------------------------------- */
int8_t gen2QueryMeasureRSSI(uint8_t Q, uint8_t *agc, uint8_t *rssiLogI, uint8_t *rssiLogQ, int8_t *rssiLinI, int8_t *rssiLinQ)
{
    st25RU3993AntennaPower(1);
    st25RU3993ContinuousRead(ST25RU3993_REG_IRQSTATUS1, 2, &buf_[0]);   // ensure that IRQ bits are reset
    st25RU3993ClrResponse();

    /* Special Settings for M4 short preamble */ // Errata
    if((gen2IntConfig.config.coding == GEN2_COD_MILLER4) && (! gen2IntConfig.config.trext)){
        st25RU3993SingleWrite(ST25RU3993_REG_ICD, 0xF0);
    }
    
    uint8_t rn16[2];
    uint16_t rxlen;
    lastErr = 0;

    rxlen = 16;
    SET_CRC_LED_OFF();

    // prepare QUERY and send
    gen2PrepareQueryCmd(buf_, Q);
    lastErr = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_QUERY, buf_, 16, rn16, &rxlen, gen2IntConfig.noRespTime, 0, 1);
    // measure AGC, RssiLog, RssiLin
    if(lastErr == ERR_NONE){
        gen2GetRssiLin(rssiLinI, rssiLinQ);
        gen2GetAgcRssiLog(agc, rssiLogI, rssiLogQ);
    }
    else {
        *agc = 0;
        *rssiLinI = 0;
        *rssiLinQ = 0;
        *rssiLogI = 0;
        *rssiLogQ = 0;
    }
    SET_CRC_LED_OFF();

    /* Special Settings for M4 short preamble */  //Errata
    if((gen2IntConfig.config.coding == GEN2_COD_MILLER4) && (! gen2IntConfig.config.trext)){
            st25RU3993SingleWrite(ST25RU3993_REG_ICD, 0x00);
    }
    delay_us(150);

    //
    return lastErr;
}

/*------------------------------------------------------------------------- */
int8_t gen2AccessTag(tag_t const *tag, uint8_t const *password)
{
    int8_t error;
    uint8_t count;
    uint16_t rxcount;
    uint8_t tagResponse[5];
    uint8_t temp_rn16[2];

    if((password[0] == 0x00) && (password[1] == 0x00) && (password[2] == 0x00) && (password[3] == 0x00)){
        // No need to access tag
        return GEN2_OK;
    }

    for(count=0 ; count<2 ; count++)
    {
        error = gen2ReqRNHandleChar(tag->handle, temp_rn16);
        if(error)
        {
            return(error);
        }

        buf_[0] = EPC_ACCESS;
        buf_[1] = password[0] ^ temp_rn16[0];
        buf_[2] = password[1] ^ temp_rn16[1];
        buf_[3] = tag->handle[0];
        buf_[4] = tag->handle[1];

        rxcount = 32;
        lastErr = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_TRANSMCRC,buf_,40,tagResponse,&rxcount,gen2IntConfig.noRespTime,0,1);
        if(lastErr < 0) {
            return ((lastErr == ERR_CHIP_HEADER) && rxcount) ? gen2ProcessErrorCode(tagResponse[0]) : GEN2_ERR_ACCESS;
        }
        password += 2;  /* Increase pointer by two to fetch the next bytes;- */

        if((tagResponse[0] != tag->handle[0]) || (tagResponse[1] != tag->handle[1])) {
            return GEN2_ERR_ACCESS;
        }
    }
    return lastErr;
}

/*------------------------------------------------------------------------- */
int8_t gen2LockTag(tag_t *tag, const uint8_t *maskAction, uint8_t *tagReply)
{
    uint16_t rxbits = 32+1;

    *tagReply = 0xA5;

    buf_[0] = EPC_LOCK;                 /*Command EPC_LOCK */

    buf_[1] = maskAction[0];
    buf_[2] = maskAction[1];

    buf_[3] = ((maskAction[2]) & 0xF0);
    insertBitStream(&buf_[3], tag->handle, 2, 4);

    // as the lock command is a "Delayed Tag reply" command, save rx no wait time as st25RU3993TxRxGen2Bytes() will set it to max (26ms)
    uint8_t tmpRxNoRespWaitTime = st25RU3993SingleRead(ST25RU3993_REG_RXNORESPONSEWAITTIME);
    
    lastErr = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_TRANSMCRCEHEAD, buf_, 44, buf_, &rxbits, 0xFF, 0, 1);

    // reset the rx no wait time for normal operation
    st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, tmpRxNoRespWaitTime);

    if((ERR_CHIP_HEADER == lastErr) && rxbits) {
        *tagReply = buf_[0];
        lastErr = gen2ProcessErrorCode(buf_[0]);
    }

    return lastErr;
}

/*------------------------------------------------------------------------- */
int8_t gen2KillTag(tag_t const *tag, uint8_t const *password, uint8_t rfu, uint8_t recom, uint8_t *tagError)
{
    uint8_t count;
    uint8_t temp_rn16[2];
    uint16_t rxbits = 32;
    uint8_t cmd = ST25RU3993_CMD_TRANSMCRC;/* first command has no header Bit */
    uint8_t noRespTime = gen2IntConfig.noRespTime;

    *tagError = 0xa5;
    for(count=0 ; count<2 ; count++)
    {
        lastErr = gen2ReqRNHandleChar(tag->handle, temp_rn16);
        if(lastErr)
        {
            break;
        }

        if(count==1)
        { /* Values for second part of kill */
            cmd = ST25RU3993_CMD_TRANSMCRCEHEAD; /* expect header bit */
            rxbits = 32 + 1; /* add header bit */
            rfu = recom; /* different data for rfu/recom */
            noRespTime = 0xFF; /* waiting time up to 20ms */
        }

        buf_[0] = EPC_KILL;

        buf_[1] = password[0] ^ temp_rn16[0];
        buf_[2] = password[1] ^ temp_rn16[1];

        buf_[3] = ((rfu << 5) & 0xE0);
        insertBitStream(&buf_[3], tag->handle, 2, 5);


        // as the kill command is a "Delayed Tag reply" command, save rx no wait time as st25RU3993TxRxGen2Bytes() will set it to max (26ms)
        uint8_t tmpRxNoRespWaitTime = st25RU3993SingleRead(ST25RU3993_REG_RXNORESPONSEWAITTIME);

        lastErr = st25RU3993TxRxGen2Bytes(cmd, buf_, 43, buf_, &rxbits, noRespTime, 0, 1);
        
        // reset the rx no wait time for normal operation
        st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, tmpRxNoRespWaitTime);
    
        if(lastErr) {
            break;
        }
        password += 2;
    }
    if ((ERR_CHIP_HEADER == lastErr) && rxbits) {
        *tagError = buf_[0];
        lastErr = gen2ProcessErrorCode(buf_[0]);
    }
    return lastErr;
}

/*------------------------------------------------------------------------- */
int8_t gen2WriteWordToTag(tag_t const *tag, uint8_t memBank, uint32_t wordPtr,
                                  uint8_t const *databuf, uint8_t *tagError)
{
    uint8_t datab;
    int8_t ret;
    uint8_t ebvlen;
    uint8_t temp_rn16[2];
    uint16_t rxbits = 32+1;
    *tagError = 0xA5;

    lastErr = gen2ReqRNHandleChar(tag->handle, temp_rn16);
    if(lastErr)
    {
        return(lastErr);
    }

    buf_[0]  = EPC_WRITE;                 /*Command EPC_WRITE */

    buf_[1]  = (memBank << 6) & 0xC0;
    ebvlen = gen2InsertEBV(wordPtr, &buf_[1], 6);

    datab = databuf[0] ^ temp_rn16[0];
    buf_[1+ebvlen] |= ((datab >> 2) & 0x3F);
    buf_[2+ebvlen]  = (datab << 6) & 0xC0;

    datab = databuf[1] ^ temp_rn16[1];
    buf_[2+ebvlen] |= ((datab >> 2) & 0x3F);
    buf_[3+ebvlen]  = (datab << 6) & 0xC0;

    insertBitStream(&buf_[3+ebvlen], tag->handle, 2, 6);
    
    // as the write command is a "Delayed Tag reply" command, save rx no wait time as st25RU3993TxRxGen2Bytes() will set it to max (26ms)
    uint8_t tmpRxNoRespWaitTime = st25RU3993SingleRead(ST25RU3993_REG_RXNORESPONSEWAITTIME);

    // transceive
    ret = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_TRANSMCRCEHEAD, buf_, 42+8*ebvlen, buf_, &rxbits, 0xFF, 0, 1);

    // reset the rx no wait time for normal operation
    st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, tmpRxNoRespWaitTime);
    
    if((ERR_CHIP_HEADER == ret) && rxbits) {
        *tagError = buf_[0];
        ret = gen2ProcessErrorCode(buf_[0]);
    }
    return ret;
}

/*------------------------------------------------------------------------- */
int8_t gen2ReadFromTag(tag_t *tag, uint8_t memBank, uint32_t wordPtr,
                          uint8_t *wordCount, uint8_t *destbuf)
{
    if ((*wordCount) > MAX_READ_DATA_LEN/2) {
        lastErr = (int8_t)ERR_PARAM;
        return lastErr;
    }

    uint8_t readTmpBuf[MAX_READ_DATA_LEN+5];   // +5 for rn16+crc+header
    uint8_t CRCTmpBuf[MAX_READ_DATA_LEN+5];    // +5 for rn16+crc+header
    uint16_t bit_count = ((*wordCount) != 0) ? (*wordCount)*2*8 : MAX_READ_DATA_LEN*8;  /* Requestedwords or MAX_READ_DATA_LEN */
    uint8_t ebvlen;

    bit_count += 2*2*8 + 1;     /* 2 bytes rn16 + 2 bytes crc + 1 header bit */

    buf_[0]  = EPC_READ;                 /*Command EPC_READ */
    buf_[1]  = (memBank << 6) & 0xC0;
    ebvlen = gen2InsertEBV(wordPtr, &buf_[1], 6);
    buf_[1+ebvlen] |= ((*wordCount) >> 2) & 0x3F;
    buf_[2+ebvlen]  = ((*wordCount) << 6) & 0xC0;
    insertBitStream(&buf_[2+ebvlen], tag->handle, 2, 6);

    lastErr = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_TRANSMCRCEHEAD, buf_, 34+8*ebvlen, readTmpBuf, &bit_count, gen2IntConfig.noRespTime, 0, 1);

    if (lastErr == ERR_CHIP_HEADER) {
        lastErr = gen2ProcessErrorCode(readTmpBuf[0]);
    }

    if (((*wordCount) == 0) && (lastErr == ST25RU3993_ERR_RXCOUNT)) {
        while ((*wordCount) < MAX_READ_DATA_LEN/2) {
            // Seek for handle
            if (memcmp(&readTmpBuf[(*wordCount)*2], tag->handle, 2) == 0) {
                // Compares with CRC
                CRCTmpBuf[0] = 0x00;
                insertBitStream(CRCTmpBuf, readTmpBuf, (*wordCount)*2+2, 7);

                uint16_t calculatedCrc = crc16Bitwise(CRCTmpBuf, (*wordCount)*16+16+1); // CRC over 0-bit, memory words and handle
                uint16_t crc = (readTmpBuf[(*wordCount)*2+2] << 8) | readTmpBuf[(*wordCount)*2+2+1];

                if (crc == calculatedCrc) {
                    lastErr = ERR_NONE;
                    break;
                }
            }
            (*wordCount)++;
        }
    }

    if (*wordCount) {
        memcpy(destbuf, readTmpBuf, (*wordCount)*2);       // Copy data whatever error code
    }
    return lastErr;
}

/*------------------------------------------------------------------------- */
int8_t gen2WriteBlockToTag(tag_t const *tag, uint8_t memBank, uint32_t wordPtr,
                                  uint8_t const *databuf, uint8_t nbWords, uint8_t *tagError)
{
    int8_t ret;
    uint8_t ebvlen;
    uint16_t rxbits = 32+1;     /* 2 bytes rn16 + 2bytes crc + 1 header bit */
    *tagError = 0xA5;

    // No Req_RN
    lastErr = 0;

    buf_[0]  = EPC_BLOCKWRITE;              /*Command EPC_BLOCKWRITE */ 
    buf_[1]  = (memBank << 6) & 0xC0;
    ebvlen = gen2InsertEBV(wordPtr, &buf_[1], 6);
    insertBitStream(&buf_[1+ebvlen], (const uint8_t *)&nbWords, 1, 6);
    insertBitStream(&buf_[2+ebvlen], databuf, nbWords*2, 6);
    insertBitStream(&buf_[2+ebvlen+nbWords*2], tag->handle, 2, 6);
    
    // as the Block write command is a "Delayed Tag reply" command, save rx no wait time as st25RU3993TxRxGen2Bytes() will set it to max (26ms)
    uint8_t tmpRxNoRespWaitTime = st25RU3993SingleRead(ST25RU3993_REG_RXNORESPONSEWAITTIME);

    // transceive
    ret = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_TRANSMCRCEHEAD, buf_, 2+8*(4+ebvlen+nbWords*2), buf_, &rxbits, 0xFF, 0, 1);

    // reset the rx no wait time for normal operation
    st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, tmpRxNoRespWaitTime);
    
    if((ERR_CHIP_HEADER==ret) && rxbits) {
        *tagError = buf_[0];
        ret = gen2ProcessErrorCode(buf_[0]);
    }
    return ret;
}

/*------------------------------------------------------------------------- */
uint16_t gen2SearchForTags(bool manualAck, gen2SearchForTagsParams_t *p)
{ 
    uint16_t num_of_tags = 0;
    uint8_t cmd = ST25RU3993_CMD_QUERY;
    uint8_t followCmd = 0;
    uint8_t adjOpt =  p->adaptiveQ->options;
    
    uint8_t regProtCtrl;
    uint8_t autoAck;
    int8_t slotStatus;
    bool goOn, ready4MoreTags = true;
    bool singulate = p->singulate;
    uint16_t slot_count = 1UL << p->statistics->Q;
    uint32_t tmpQ = p->statistics->Q;
    uint8_t initialQ = p->statistics->Q;
    uint16_t eventMask = 0;
    uint32_t slotTime = 0;
    
    bool resetQAfterRound = adjOpt & 0x80;
    bool useCeilFloor = adjOpt & 0x40;
    bool singleAdj = adjOpt & 0x20;
    bool useQueryAdjNIC = adjOpt & 0x10;
    
    const uint32_t oneBase = 100000; 
    uint32_t maxQ  = (p->adaptiveQ->maxQ * oneBase);
    uint32_t minQ  = (p->adaptiveQ->minQ * oneBase);

    uint32_t adjCnt = singleAdj ? 1 : 0xFFFFFFFF;
    uint32_t qfp = (p->adaptiveQ->qfp * oneBase);
    uint32_t c1[MAX_Q + 1] = {0};
    uint32_t c2[MAX_Q + 1] = {0};
    if(p->adaptiveQ->isEnabled){
        for(int i = 0; i <= MAX_Q; i++){
            c1[i] = (p->adaptiveQ->c1[i] * oneBase)/100;
            c2[i] = (p->adaptiveQ->c2[i] * oneBase)/100;
        }
    }


    
    if(p->toggleSession){
        followCmd = ST25RU3993_CMD_QUERYREP;
    }

    if(p->cbFollowTagCommand != NULL){
        followCmd = 0;  //no follow Command
        singulate = true;  // not fast
    }
    st25RU3993AntennaPower(1);
    st25RU3993ContinuousRead(ST25RU3993_REG_IRQSTATUS1, 2, &buf_[0]);   // ensure that IRQ bits are reset
    st25RU3993ClrResponse();

    regProtCtrl = st25RU3993SingleRead(ST25RU3993_REG_PROTOCOLCTRL);
    gRxWithoutCRC = (regProtCtrl & 0x80) >> 7; // if crc is received fifo    
    
    if(!manualAck){
        //configure autoACK mode
        autoAck = regProtCtrl & ~0x30;
        autoAck |= (singulate) ? 0x20 : 0x10;
        st25RU3993SingleWrite(ST25RU3993_REG_PROTOCOLCTRL, autoAck);    
    }
    
    /* Special Settings for M4 short preamble */ // Errata
    if((gen2IntConfig.config.coding == GEN2_COD_MILLER4) && (! gen2IntConfig.config.trext)){
        st25RU3993SingleWrite(ST25RU3993_REG_ICD, 0xF0);
    }
    
    do{
        slot_count--;

        // T4 Time
        if((cmd == ST25RU3993_CMD_QUERYADJUSTUP) || (cmd == ST25RU3993_CMD_QUERYADJUSTNIC) ||(cmd == ST25RU3993_CMD_QUERYADJUSTDOWN)){
            switch (gen2IntConfig.config.tari) {
                case TARI_25_00: delay_us(150); break;  // T4: 150us: 2.0*3*Tari @ Tx_one_length = 2
                case TARI_12_50: delay_us(75);  break;  // T4: 75us: 2.0*3*Tari @ Tx_one_length = 2
                default:         delay_us(38);  break;  // T4: 37.5us: 2.0*3*Tari @ Tx_one_length = 2
            }
            st25RU3993ClrResponse(); // clear also TX IRQ
        }

        // execute slot ..
        eventMask = 0;
        slotTime = HAL_GetTick();
        slotStatus = gen2Slot(p->tag, manualAck, cmd, p->statistics->Q, singulate ? false : true, followCmd, &eventMask);
        eventMask |= (cmd == ST25RU3993_CMD_QUERYREP) ? EVENT_QUERY_REP : 0;
        switch(slotStatus){
            case -1:{ // any communication problem is seen as collision
                if((lastErr == ST25RU3993_ERR_PREAMBLE) || (lastErr == ST25RU3993_ERR_COLL)){
                    // treat only these errors as collision
                    p->statistics->collisionCnt++;
                    eventMask |= EVENT_COLLISION;
                    if(p->adaptiveQ->isEnabled){
                        if((qfp + c2[p->statistics->Q]) < maxQ){
                            qfp += c2[p->statistics->Q];
                        }
                            tmpQ = useCeilFloor ? floor((float)qfp/(float)oneBase) : round((float)qfp/(float)oneBase);
                    }
                }                
                break;
            }

            case 0: { // empty slot
                p->statistics->emptySlotCnt++;
                eventMask |= EVENT_EMPTY_SLOT;
                if(p->adaptiveQ->isEnabled){
                    if(qfp >= minQ){
                        if((qfp - minQ) >= c1[p->statistics->Q]){
                            qfp -= c1[p->statistics->Q];
                        }
                    }
                    tmpQ = useCeilFloor ? ceil((float)qfp/(float)oneBase) : round((float)qfp/(float)oneBase);
                }
                break;
            }   
            
            case 1:{ // TAG found  
                cmd = followCmd ? 0 : ST25RU3993_CMD_QUERYREP;                    
                if(p->cbFollowTagCommand != NULL){
                    cmd = ST25RU3993_CMD_QUERYREP;
                    if (p->cbFollowTagCommand(p->tag) != ERR_NONE) {
                        // FollowTagCommand encountered issues, skip this tag
                        p->statistics->skipCnt++;
                        eventMask |= EVENT_SKIP_FOLLOW_CMD;                        
                        break;
                    }
                }
                // calc rssi log mean
                if (! p->statistics->tagCnt) {
                    // No tag inventoried yet, init value
                    gGen2RssiLogIandQSum = 0;
                }
                gGen2RssiLogIandQSum += p->tag->rssiLogI + p->tag->rssiLogQ;     // 0-30
                p->statistics->rssiLogMean = (uint8_t)((gGen2RssiLogIandQSum + p->statistics->tagCnt) / (((uint32_t)p->statistics->tagCnt + 1UL) * 2UL));   // 0 - 15, tagCnt is added to LogIandQSUm to handle upper/lower integer rounding
                
                // get timestamp
                p->tag->timeStamp = slotTime;
                // increase tag count
                p->statistics->tagCnt++;
                num_of_tags++;   
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
            case ST25RU3993_ERR_PREAMBLE: p->statistics->preambleErrCnt++;  eventMask |= EVENT_PREAMBLE_ERR; break;
            case ST25RU3993_ERR_CRC: p->statistics->crcErrCnt++;            eventMask |= EVENT_CRC_ERR;      break;
            case ST25RU3993_ERR_HEADER: p->statistics->headerErrCnt++;      eventMask |= EVENT_HEADER_ERR;   break;
            case ST25RU3993_ERR_RXCOUNT: p->statistics->rxCountErrCnt++;    eventMask |= EVENT_RX_COUNT_ERR; break;
            default: 
                break;
        }

        // notify caller about slot has finished
        if(p->cbSlotFinished){
            p->cbSlotFinished(slotTime, eventMask, p->statistics->Q);
        }      

        // 
        if(p->adaptiveQ->isEnabled){
            // decide how to continue with Q adoption
            if(tmpQ == p->statistics->Q){                
                switch(slotStatus){
                    case -1: // COL
                    case 0:  // EMP
                        cmd = useQueryAdjNIC ? ST25RU3993_CMD_QUERYADJUSTNIC : ST25RU3993_CMD_QUERYREP;
                        break;
                    case 1:  // TAG
                    default:
                        cmd = ST25RU3993_CMD_QUERYREP;
                        break;
                }
            }else // tmpQ != p->statistics->Q
            { 
                if(adjCnt > 0){ 
                    if(tmpQ > p->statistics->Q){
                        p->statistics->Q++;                            
                        cmd = ST25RU3993_CMD_QUERYADJUSTUP;
                        adjCnt--;
                    }
                    else { //(tmpQ < p->statistics->Q)
                        p->statistics->Q--;
                        cmd = ST25RU3993_CMD_QUERYADJUSTDOWN;
                        adjCnt--;
                    }
                    slot_count = (1 << p->statistics->Q);
                    tmpQ = p->statistics->Q;
                    qfp = (p->statistics->Q*oneBase);
                }
                else{
                    cmd = ST25RU3993_CMD_QUERYREP;
                }
            }
        }
        else{
            cmd = ST25RU3993_CMD_QUERYREP;
        }
        
        //
        goOn = (p->cbContinueScanning == NULL) ? true : p->cbContinueScanning();   
        
    } while(slot_count && goOn && ready4MoreTags);

    // reset Q
    if(p->adaptiveQ->isEnabled){
        if(resetQAfterRound){
            p->statistics->Q = initialQ;
            p->adaptiveQ->qfp = (float)initialQ;
        }
        else{
            p->adaptiveQ->qfp = ((float)qfp / (float)oneBase);
        }
    }
    /* Special Settings for M4 short preamble */  //Errata
    if((gen2IntConfig.config.coding == GEN2_COD_MILLER4) && (! gen2IntConfig.config.trext)){
            st25RU3993SingleWrite(ST25RU3993_REG_ICD, 0x00);
    }

    //
    if(!manualAck){
        // unset autoACK mode again
        autoAck = st25RU3993SingleRead(ST25RU3993_REG_PROTOCOLCTRL);
        autoAck &= ~0x30;
        st25RU3993SingleWrite(ST25RU3993_REG_PROTOCOLCTRL, autoAck);
    }

    // Reset truncate for Next ACK (until a Select re-enables it)
    gGen2Truncate = false;

    // If a tag is found in the last slot of an inventory round, follow Command QueryRep will be executed.
    // Add some time to send this command to the tag over the field.
    // If function will be left to early and field will be switched off the tag will not invert the inventoried flag.
    // Problem especially  for S2 and S3.
    // If there was a follow Tag Command a Query Rep. has to be sent to invert the session flag.
    if(p->cbFollowTagCommand == NULL){
        delay_us(150);
    }else{
        st25RU3993SingleCommand(ST25RU3993_CMD_QUERYREP);
        st25RU3993WaitForResponse(RESP_TXIRQ);
        st25RU3993ClrResponse();
    }
    return num_of_tags;
}

/*------------------------------------------------------------------------- */
int8_t gen2ContinueCommand(uint8_t *tagError)
{
    int8_t ret;
    uint16_t rxbits = 32+1;
    *tagError = 0xA5;

    // transceive
    ret = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_ENABLERX, NULL, 0, buf_, &rxbits, 0xFF, 0, 0);

    if((ret == ERR_CHIP_HEADER) && rxbits) {
        *tagError = buf_[0];
        ret = gen2ProcessErrorCode(buf_[0]);
    }
    return ret;
}

/*------------------------------------------------------------------------- */
void gen2Configure(const gen2Config_t *config)
{
    gen2IntConfig.config = *config;
    if(gen2IntConfig.config.session > GEN2_IINV_S3){
        gen2IntConfig.config.session = GEN2_IINV_S0; /* limit SL and invalid settings */
    }
    if(gen2IntConfig.config.coding == GEN2_COD_FM0 || gen2IntConfig.config.coding == GEN2_COD_MILLER2){
        gen2IntConfig.config.trext = TREXT_LEAD_ON;
    }
}

void gen2Open(const gen2Config_t *config)
{
    /* depending on link frequency setting adjust */
    /* registers 01, 02, 03, 04, 05, 06, 07, 08 and 09 */
    uint8_t reg[9];
    gen2IntConfig.DR = 1;

    gen2Configure(config);

    switch(config->blf)
    {
        case GEN2_LF_640:
        {
            /* 640kHz */
            reg[0] = 0x20; /* ST25RU3993_REG_TXOPTIONS              */
            reg[1] = 0xF0; /* ST25RU3993_REG_RXOPTIONS              */
            reg[2] = 0x01; /* ST25RU3993_REG_TRCALHIGH              */
            reg[3] = 0x4D; /* ST25RU3993_REG_TRCALLOW               */
            reg[4] = 0x03; /* ST25RU3993_REG_AUTOACKTIMER           */
            reg[5] = 0x02; /* ST25RU3993_REG_RXNORESPONSEWAITTIME   */
            reg[6] = 0x01; /* ST25RU3993_REG_RXWAITTIME             */
            reg[7] = 0x02; /* ST25RU3993_REG_RXFILTER               */
            break;
        }
        case GEN2_LF_320:
        {
            /* 320kHz */
            reg[0] = 0x20;
            reg[1] = 0xC0;
            if(gen2IntConfig.config.tari == TARI_6_25){   
                /* TRcal = 25us */
                gen2IntConfig.DR = 0;
                reg[2] = 0x00;
                reg[3] = 0xFA;
            }
            else{
                /* TRcal = 66.7us */
                reg[2] = 0x02;
                reg[3] = 0x9B;
            }
            reg[4] = 0x04;      /* ST25RU3993_REG_AUTOACKTIMER */
            reg[5] = 0x02;      /* ST25RU3993_REG_RXNORESPONSEWAITTIME */ 
            if(gen2IntConfig.config.tari == TARI_25_00){
                reg[6] = 0x05;  /* ST25RU3993_REG_RXWAITTIME */
            }
            else{
                reg[6] = 0x04;  /* ST25RU3993_REG_RXWAITTIME */
            }
            //reg[6] = 0x04;      
 
            if(gen2IntConfig.config.coding > GEN2_COD_MILLER2){
                reg[7] = 0x24; /* ST25RU3993_REG_RXFILTER */
            }
            else{
                reg[7] = 0x27; /* ST25RU3993_REG_RXFILTER */
            }
            break;
        }
        case GEN2_LF_256:
        {
            /* 256kHz */
            reg[0] = 0x20;
            reg[1] = 0x90;
            if(gen2IntConfig.config.tari == TARI_6_25){   
                /* TRcal = 31.3us */
                gen2IntConfig.DR = 0;
                reg[2] = 0x01;
                reg[3] = 0x39;
            }
            else{
                /* TRcal = 83.3us */
                reg[2] = 0x03;
                reg[3] = 0x41;
            }
            reg[4] = 0x05;
            reg[5] = 0x05;
            if(gen2IntConfig.config.tari == TARI_25_00){
                reg[6] = 0x0B;
            }
            else{
                reg[6] = 0x05;
            }

            if(gen2IntConfig.config.coding > GEN2_COD_MILLER2){
                reg[7] = 0x34; /* ST25RU3993_REG_RXFILTER         */
            }
            else if((gen2IntConfig.config.coding == GEN2_COD_MILLER2) && (gen2IntConfig.config.trext)){
                reg[7] = 0x27; /* ST25RU3993_REG_RXFILTER         */
            }
            else{
                reg[7] = 0x37; /* ST25RU3993_REG_RXFILTER         */
            }
            break;
        }
        case GEN2_LF_213:
        {
            /* 213.3kHz */
            reg[0] = 0x20;
            reg[1] = 0x80;
            if(gen2IntConfig.config.tari == TARI_6_25){
                /* TRcal = 37.5us */
                gen2IntConfig.DR = 0;
                reg[2] = 0x01;
                reg[3] = 0x77;
            }
            else{  
                /* TRcal = 100us */
                reg[2] = 0x03;
                reg[3] = 0xE8;
            }
            reg[4] = 0x06;
            reg[5] = 0x05;
            if(gen2IntConfig.config.tari == TARI_25_00){
                reg[6] = 0x0B;
            }
            else{
                reg[6] = 0x06;
            }

            if(gen2IntConfig.config.coding > GEN2_COD_MILLER2){
                reg[7] = 0x34; /* ST25RU3993_REG_RXFILTER         */
            }
            else{
                reg[7] = 0x37; /* ST25RU3993_REG_RXFILTER         */
            }
            break;
        }
        case GEN2_LF_160:
        {
            /* 160kHz */
            reg[0] = 0x20;
            reg[1] = 0x60;
            if(gen2IntConfig.config.tari == TARI_12_50){   
                /* TRcal = 50us */
                gen2IntConfig.DR = 0;
                reg[2] = 0x01;
                reg[3] = 0xF4;
            }
            else{ 
                /* TRcal = 1333.3us */
                reg[2] = 0x05;
                reg[3] = 0x35;
            }
            reg[4] = 0x0A;
            reg[5] = 0x05;
            if(gen2IntConfig.config.tari == TARI_25_00){
                reg[6] = 0x0B;
            }
            if(gen2IntConfig.config.tari == TARI_12_50){
                reg[6] = 0x09;
            }
            else{
                reg[6] = 0x08;
            }

//            if(gen2IntConfig.config.coding == GEN2_COD_FM0)
//            {
//                reg[7] = 0x90; /* ST25RU3993_REG_RXFILTER         */
//            }
//            else if((gen2IntConfig.config.coding == GEN2_COD_MILLER2) && (gen2IntConfig.config.trext))
//            {
//                reg[7] = 0x27; /* ST25RU3993_REG_RXFILTER         */
//            }
//            else
//            {
//                reg[7] = 0x3F; /* ST25RU3993_REG_RXFILTER         */
//            }
            if(gen2IntConfig.config.coding == GEN2_COD_FM0)
            {
                reg[7] = 0xBF; /* ST25RU3993_REG_RXFILTER         */
            }
            else
            {
                reg[7] = 0x3F; /* ST25RU3993_REG_RXFILTER         */
            }
            break;
        }
        case GEN2_LF_40:
        {
            /* 40kHz */
            reg[0] = 0x30; /* ST25RU3993_REG_TXOPTIONS            */
            reg[1] = 0x00; /* ST25RU3993_REG_RXOPTIONS            */
            reg[2] = 0x07; /* ST25RU3993_REG_TRCALHIGH            */
            reg[3] = 0xD0; /* ST25RU3993_REG_TRCALLOW             */
            reg[4] = 0x3F; /* ST25RU3993_REG_AUTOACKTIMER         */
            reg[5] = 0x0C; /* ST25RU3993_REG_RXNORESPONSEWAITTIME */
            reg[6] = 0x24; /* ST25RU3993_REG_RXWAITTIME           */
            reg[7] = 0xFF; /* ST25RU3993_REG_RXFILTER             */
            gen2IntConfig.DR = 0;
            break;
        }
        default:
        {
            return; /* use preset settings */
        }
    }
    
    reg[0] |= gen2IntConfig.config.tari;        /* ST25RU3993_REG_TXOPTIONS */
    reg[1] = (reg[1] & 0xF0) | 
                gen2IntConfig.config.coding | 
                ((gen2IntConfig.config.trext ? 1 : 0)<<3); /* ST25RU3993_REG_RXOPTIONS */

    gen2IntConfig.noRespTime = reg[5];
    /* Modify only the gen2 relevant settings */
    st25RU3993ContinuousWrite(ST25RU3993_REG_TXOPTIONS, reg+0, 8);

    /* set session */
    reg[0] = st25RU3993SingleRead(ST25RU3993_REG_TXSETTING);
    reg[0] = (reg[0] & 0xFC) | gen2IntConfig.config.session;
    st25RU3993SingleWrite(ST25RU3993_REG_TXSETTING, reg[0]);
    
    /* set Gen2 + Normal operation  */
    reg[0] = st25RU3993SingleRead(ST25RU3993_REG_PROTOCOLCTRL);
    reg[0] = reg[0] & 0xB8;
    st25RU3993SingleWrite(ST25RU3993_REG_PROTOCOLCTRL, reg[0]);
    
    /* Modulation Ctrl 2, 4 for PR-ASK or ASK */
    reg[0] = st25RU3993SingleRead(ST25RU3993_REG_MODULATORCONTROL2);
    if(reg[0] & 0x40){
        /* PR-ASK */
        if(gen2IntConfig.config.tari == TARI_25_00){
            st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL2, 0xEF);
        } else {
            st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL2, 0xE3);
        }
        /* Modulation Ctrl2 ASK/PR-ASK */
        st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL4, 0x89);    
    } else {
        /* ASK */
        st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL2, 0x9D);
        st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL4, 0x7E);
    }
}

/*------------------------------------------------------------------------- */
void gen2Close(void)
{
}

/*------------------------------------------------------------------------- */
gen2Config_t *getGen2IntConfig(void)
{
    return &(gen2IntConfig.config);
}

/**
  * @}
  */
/**
  * @}
  */
