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
 *  @brief Main dispatcher for streaming protocol uses a stream_driver for the io
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup PC_Communication
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include <string.h>
#include "usart.h"
#include "stream_driver.h"
#include "spi_driver.h"
#include "timer.h"
#include "st25RU3993_config.h"
#include "platform.h"
#include "uart_driver.h"
#include "errno_application.h"
#include "st25RU3993_public.h"
#include "st25RU3993.h"
#include "bootloader.h"
#include "tuner.h"
#include "logger.h"

//#include "st_stream.h"
#include "evalAPI_commands.h"
#include "stuhfl_helpers.h"
#include "stuhfl_evalAPI.h"
#include "stuhfl_al.h"
#include "stuhfl_pl.h"
#include "stream_dispatcher.h"


/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/

/* RX/TX communication buffers */
static uint8_t rxBuffer[UART_RX_BUFFER_SIZE];      /** buffer to store protocol packets received from the Host */
static uint8_t txBuffer[UART_TX_BUFFER_SIZE];    /** buffer to store protocol packets which are transmitted to the Host */

uint8_t *gTxPayload = NULL;
uint16_t gTxPayloadLen = 0;

static uint8_t lastError;   /** flag indicating different types of errors that cannot be reported in the protocol status field */

static uint16_t lastCmd;
uint8_t process_CG_GENERIC(uint8_t cmd, uint8_t *rxPayload, uint16_t rxPayloadLen, uint8_t *txPayload, uint16_t *txPayloadLen);
uint8_t process_CG_UL(uint8_t cmd, uint8_t *rxPayload, uint16_t rxPayloadLen, uint8_t *txPayload, uint16_t *txPayloadLen);
uint8_t process_CG_SL(uint8_t cmd, uint8_t *rxPayload, uint16_t rxPayloadLen, uint8_t *txPayload, uint16_t *txPayloadLen);
uint8_t process_CG_AL(uint8_t cmd, uint8_t *rxPayload, uint16_t rxPayloadLen, uint8_t *txPayload, uint16_t *txPayloadLen);

#ifdef EMBEDDED_TESTS
uint8_t process_CG_TS(uint16_t lastcmd, uint8_t *rxPayload, uint16_t rxPayloadLen, uint8_t *txPayload, uint16_t *txPayloadLen);
#else
#define process_CG_TS(a,b,c,d,e)    (uint8_t)ERR_PROTO
#endif  /* EMBEDDED_TESTS */


/*
******************************************************************************
* LOCAL FUNCTION PROTOYPES
******************************************************************************
*/
static uint16_t processReceivedPackets(void);

/*
******************************************************************************
* LOCAL FUNCTIONS, APPLICATION SPECIFIC COMMANDS
******************************************************************************
*/

/*------------------------------------------------------------------------- */

uint16_t decodeRxFrame(uint8_t *sndData, uint16_t *mode, uint16_t *id, uint16_t *status, uint16_t *cmd, uint8_t *payloadData, uint16_t *payloadDataLen)
{
	// 1. decode header info
	*mode = COMM_GET_PREAMBLE_MODE(sndData);
	*id = COMM_GET_PREAMBLE_ID(sndData);
	*status = COMM_GET_STATUS(sndData);
	*cmd = COMM_GET_CMD(sndData);
	*payloadDataLen = COMM_GET_PAYLOAD_LENGTH(sndData);

	// 2. payload (copy if needed)
	if (&sndData[COMM_PAYLOAD_POS] != payloadData) {
		memcpy(payloadData, &sndData[COMM_PAYLOAD_POS], *payloadDataLen);
	}

//	// 3. verify checksum (if existing)
//	if (*mode & SIGNATURE_XOR_BCC) {
//		// ..
//	}
    
    return ERR_NONE;
} 

static uint16_t processReceivedPackets(void)
{
    /* every time we enter this function, the last txBuffer was already sent.
       So we fill a new transfer buffer */    
    uint16_t txSize = 0;

    while(StreamHasAnotherPacket())
    {
    
        uint16_t mode;
        uint16_t id;
        uint16_t status;
        uint8_t *rxPayload = COMM_GET_PAYLOAD_PTR(rxBuffer);
        uint16_t rxPayloadLen = 0;
       
        decodeRxFrame(rxBuffer, &mode, &id, &status, &lastCmd, rxPayload, &rxPayloadLen);
    
        lastError = (uint8_t)ERR_PROTO;
        uint8_t cmdGrp = (lastCmd >> 8);
        uint8_t cmd = (lastCmd & 0x00FF);

        //
        gTxPayloadLen = 0;
        gTxPayload  = COMM_GET_PAYLOAD_PTR(txBuffer);
        switch(cmdGrp)
        {   
            case STUHFL_CG_GENERIC:
                lastError = process_CG_GENERIC(cmd, rxPayload, rxPayloadLen, gTxPayload, &gTxPayloadLen);
                break;
                
            case STUHFL_CG_DL:
                lastError = process_CG_UL(cmd, rxPayload, rxPayloadLen, gTxPayload, &gTxPayloadLen);
                break;

            case STUHFL_CG_SL:
                lastError = process_CG_SL(cmd, rxPayload, rxPayloadLen, gTxPayload, &gTxPayloadLen);
                break;

            case STUHFL_CG_AL:
                lastError = process_CG_AL(cmd, rxPayload, rxPayloadLen, gTxPayload, &gTxPayloadLen);
                break;

            default:
                lastError = process_CG_TS(lastCmd, rxPayload, rxPayloadLen, gTxPayload, &gTxPayloadLen);
                break;
        }

        //
        txSize += (gTxPayloadLen);


        /* remove the handled packet, and move on to next packet */
        StreamPacketProcessed(0);
    }

    return txSize;
}


// --------------------------------------------------------------------------
uint8_t process_CG_GENERIC(uint8_t cmd, uint8_t *rxPayload, uint16_t rxPayloadLen, uint8_t *txPayload, uint16_t *txPayloadLen)
{
    uint8_t retCode = (uint8_t)ERR_PROTO;
    
    switch(cmd)
    {   // commands
        case STUHFL_CC_GET_VERSION:{
            STUHFL_T_Version swVersion, hwVersion;
            retCode = GetBoardVersion(&swVersion, &hwVersion);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_VERSION_FW, sizeof(STUHFL_T_Version), &swVersion);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_VERSION_HW, sizeof(STUHFL_T_Version), &hwVersion);
            break;
        }
        case STUHFL_CC_GET_INFO:{
            STUHFL_T_Version_Info swInfo, hwInfo;
            retCode = GetBoardInfo(&swInfo, &hwInfo);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_INFO_FW, sizeof(STUHFL_T_Version_Info), &swInfo);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_INFO_HW, sizeof(STUHFL_T_Version_Info), &hwInfo);
            break;
        }
        case STUHFL_CC_REBOOT:
          Reboot();
		  // will never return
          break;
        case STUHFL_CC_ENTER_BOOTLOADER:
    	  EnterBootloader();
		  // will never return
          break;
          
        case STUHFL_CC_UPGRADE:
        default:
            retCode = (uint8_t)ERR_PROTO;
            break;
    }
    
    return retCode;
}

// --------------------------------------------------------------------------
uint8_t process_CG_UL(uint8_t cmd, uint8_t *rxPayload, uint16_t rxPayloadLen, uint8_t *txPayload, uint16_t *txPayloadLen)
{
    uint8_t     retCode = (uint8_t)ERR_NONE;
    uint32_t    lenLength = 0;
    switch(cmd)
    {   // commands
        case STUHFL_CC_GET_PARAM:{
            int nextPacketOffset = 0;
            while(nextPacketOffset < rxPayloadLen){
                //
                lenLength = STUHFL_T_LEN_LENGTH( rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG)] );
                uint8_t packetType = rxPayload[nextPacketOffset];
                
                if(packetType == STUHFL_TAG_REGISTER){                    
                    STUHFL_T_ST25RU3993_Register *rcvReg = (STUHFL_T_ST25RU3993_Register*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    STUHFL_T_ST25RU3993_Register reg;
                    reg.addr = rcvReg->addr;
                    retCode |= GetRegister(&reg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Register), &reg);
                }                            
                else if(packetType == STUHFL_TAG_RWD_CONFIG){
                    STUHFL_T_ST25RU3993_RwdConfig *rcvCfg = (STUHFL_T_ST25RU3993_RwdConfig*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    STUHFL_T_ST25RU3993_RwdConfig cfg;
                    cfg.id = rcvCfg->id;
                    retCode |= GetRwdCfg(&cfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_RwdConfig), &cfg);
                } 
                //
                else if(packetType == STUHFL_TAG_ANTENNA_POWER){
                    STUHFL_T_ST25RU3993_Antenna_Power antPwr;
                    retCode |= GetAntennaPower(&antPwr);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Antenna_Power), &antPwr);
                } 
                //
                else if(packetType == STUHFL_TAG_FREQ_RSSI){
                    STUHFL_T_ST25RU3993_Freq_Rssi *rcvFreqRSSI = (STUHFL_T_ST25RU3993_Freq_Rssi*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    STUHFL_T_ST25RU3993_Freq_Rssi freqRSSI;
                    freqRSSI.frequency = rcvFreqRSSI->frequency;
                    retCode |= GetFreqRSSI(&freqRSSI);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Freq_Rssi), &freqRSSI);
                }                
                else if(packetType == STUHFL_TAG_FREQ_REFLECTED){
                    STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info *rcvFreqReflectedPower = (STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info freqReflectedPower;
                    freqReflectedPower.frequency = rcvFreqReflectedPower->frequency;
                    freqReflectedPower.applyTunerSetting = rcvFreqReflectedPower->applyTunerSetting;
                    retCode |= GetFreqReflectedPower(&freqReflectedPower);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info), &freqReflectedPower);
                }                
                else if(packetType == STUHFL_TAG_CHANNEL_LIST){
                    STUHFL_T_ST25RU3993_ChannelList *rcvChannelList = (STUHFL_T_ST25RU3993_ChannelList*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    STUHFL_T_ST25RU3993_ChannelList channelList;
                    channelList.antenna = rcvChannelList->antenna;
                    channelList.persistent = rcvChannelList->persistent;
                    retCode |= GetChannelList(&channelList);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_ChannelList), &channelList);
                }
                else if(packetType == STUHFL_TAG_FREQ_PROFILE_INFO){
                    STUHFL_T_ST25RU3993_Freq_Profile_Info freqProfileInfo;
                    retCode |= GetFreqProfileInfo(&freqProfileInfo);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Freq_Profile_Info), &freqProfileInfo);
                }
                else if(packetType == STUHFL_TAG_FREQ_HOP){
                    STUHFL_T_ST25RU3993_Freq_Hop freqHop;
                    retCode |= GetFreqHop(&freqHop);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Freq_Hop), &freqHop);
                }
                else if(packetType == STUHFL_TAG_FREQ_LBT){
                    STUHFL_T_ST25RU3993_Freq_LBT freqLBT;
                    retCode |= GetFreqLBT(&freqLBT);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Freq_LBT), &freqLBT);                    
                }
                //
                else if(packetType == STUHFL_TAG_GEN2TIMINGS){
                    STUHFL_T_Gen2_Timings gen2Timings;
                    retCode |= GetGen2Timings(&gen2Timings);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_Gen2_Timings), &gen2Timings);
                }
                else if(packetType == STUHFL_TAG_GEN2PROTOCOL_CFG){
                    STUHFL_T_ST25RU3993_Gen2Protocol_Cfg gen2ProtocolCfg;
                    retCode |= GetGen2ProtocolCfg(&gen2ProtocolCfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg), &gen2ProtocolCfg);
                }
#if GB29768
                else if(packetType == STUHFL_TAG_GB29768PROTOCOL_CFG){
                    STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg gb29768ProtocolCfg;
                    retCode |= GetGb29768ProtocolCfg(&gb29768ProtocolCfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg), &gb29768ProtocolCfg);
                }
#endif
                else if(packetType == STUHFL_TAG_TXRX_CFG){
                    STUHFL_T_ST25RU3993_TxRx_Cfg txRxCfg;
                    retCode |= GetTxRxCfg(&txRxCfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_TxRx_Cfg), &txRxCfg);
                }
                else if(packetType == STUHFL_TAG_PA_CFG){
                    STUHFL_T_ST25RU3993_PA_Cfg paCfg;
                    retCode |= GetPA_Cfg(&paCfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_PA_Cfg), &paCfg);
                }
                else if(packetType == STUHFL_TAG_GEN2INVENTORY_CFG){
                    STUHFL_T_ST25RU3993_Gen2Inventory_Cfg invGen2Cfg;
                    retCode |= GetGen2InventoryCfg(&invGen2Cfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg), &invGen2Cfg);
                }                
#if GB29768
                else if(packetType == STUHFL_TAG_GB29768INVENTORY_CFG){
                    STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg invGb29768Cfg;
                    retCode |= GetGb29768InventoryCfg(&invGb29768Cfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg), &invGb29768Cfg);
                }                
#endif                
                //
                else if(packetType == STUHFL_TAG_TUNING){
                    STUHFL_T_ST25RU3993_Tuning *rcvTuning = (STUHFL_T_ST25RU3993_Tuning*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    STUHFL_T_ST25RU3993_Tuning tuning;
                    tuning.antenna = rcvTuning->antenna;
                    retCode |= GetTuning(&tuning);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Tuning), &tuning);
                }                
                else if(packetType == STUHFL_TAG_TUNING_TABLE_ENTRY){
                    STUHFL_T_ST25RU3993_TuningTableEntry *rcvTuningTableEntry = (STUHFL_T_ST25RU3993_TuningTableEntry*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    STUHFL_T_ST25RU3993_TuningTableEntry tuningTableEntry;
                    tuningTableEntry.entry = rcvTuningTableEntry->entry;
                    retCode |= GetTuningTableEntry(&tuningTableEntry);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_TuningTableEntry), &tuningTableEntry);
                }                
                else if(packetType == STUHFL_TAG_TUNING_TABLE_INFO){
                    STUHFL_T_ST25RU3993_TuningTableInfo *rcvTuningTableInfo = (STUHFL_T_ST25RU3993_TuningTableInfo*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    STUHFL_T_ST25RU3993_TuningTableInfo tuningTableInfo;
                    tuningTableInfo.profile = rcvTuningTableInfo->profile;
                    retCode |= GetTuningTableInfo(&tuningTableInfo);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_TuningTableInfo), &tuningTableInfo);
                }                
                else if(packetType == STUHFL_TAG_TUNING_CAPS){
                    STUHFL_T_ST25RU3993_TuningCaps *rcvTuningCaps = (STUHFL_T_ST25RU3993_TuningCaps*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    STUHFL_T_ST25RU3993_TuningCaps tuningCaps;
                    tuningCaps.antenna = rcvTuningCaps->antenna;
                    tuningCaps.channelListIdx  = rcvTuningCaps->channelListIdx;
                    retCode |= GetTuningCaps(&tuningCaps);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_TuningCaps), &tuningCaps);
                }

                // move forward by adding the length of the TAG, the LENGTH and the content of the TLV
                uint16_t packetLen = (lenLength == 2) ? ((rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + 1] << 8) | (rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG)] & 0x7F)) : rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG)]; 
                nextPacketOffset += (packetLen + (sizeof(STUHFL_T_TAG) + lenLength));                
            }
            break;
        }
        case STUHFL_CC_SET_PARAM:{
            int nextPacketOffset = 0;
            while(nextPacketOffset < rxPayloadLen){
                //
                lenLength = STUHFL_T_LEN_LENGTH(rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG)]);
                uint8_t packetType = rxPayload[nextPacketOffset];
            
                if(packetType == STUHFL_TAG_REGISTER){                            
                    STUHFL_T_ST25RU3993_Register *reg = (STUHFL_T_ST25RU3993_Register*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetRegister(reg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Register), reg);
                }                            
                else if(packetType == STUHFL_TAG_RWD_CONFIG){
                    STUHFL_T_ST25RU3993_RwdConfig *rwdCfg = (STUHFL_T_ST25RU3993_RwdConfig*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetRwdCfg(rwdCfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_RwdConfig), rwdCfg);
                }
                //
                else if(packetType == STUHFL_TAG_ANTENNA_POWER){
                    STUHFL_T_ST25RU3993_Antenna_Power *antPwr = (STUHFL_T_ST25RU3993_Antenna_Power*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetAntennaPower(antPwr);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Antenna_Power), antPwr);
                }
                //
//                else if(packetType == STUHFL_TAG_FREQ_RSSI){
//                }
//                else if(packetType == STUHFL_TAG_FREQ_REFLECTED){
//                }
                else if(packetType == STUHFL_TAG_CHANNEL_LIST){
                    STUHFL_T_ST25RU3993_ChannelList *channelList = (STUHFL_T_ST25RU3993_ChannelList*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetChannelList(channelList);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_ChannelList), channelList);
                }
                else if(packetType == STUHFL_TAG_FREQ_PROFILE){
                    STUHFL_T_ST25RU3993_Freq_Profile *freqProfile = (STUHFL_T_ST25RU3993_Freq_Profile*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetFreqProfile(freqProfile);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Freq_Profile), freqProfile);
                }
                else if(packetType == STUHFL_TAG_FREQ_PROFILE_ADD2CUSTOM){
                    STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom *freqProfileAdd = (STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetFreqProfileAdd2Custom(freqProfileAdd);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom), freqProfileAdd);
                }
//                else if(packetType == STUHFL_TAG_FREQ_PROFILE_INFO){
//                }
                else if(packetType == STUHFL_TAG_FREQ_HOP){
                    STUHFL_T_ST25RU3993_Freq_Hop *freqHop = (STUHFL_T_ST25RU3993_Freq_Hop*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetFreqHop(freqHop);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Freq_Hop), freqHop);
                }
                else if(packetType == STUHFL_TAG_FREQ_LBT){
                    STUHFL_T_ST25RU3993_Freq_LBT *freqLBT = (STUHFL_T_ST25RU3993_Freq_LBT*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetFreqLBT(freqLBT);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Freq_LBT), freqLBT);
                }
                else if(packetType == STUHFL_TAG_FREQ_CONT_MOD){
                    STUHFL_T_ST25RU3993_Freq_ContMod *freqContMod = (STUHFL_T_ST25RU3993_Freq_ContMod*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetFreqContMod(freqContMod);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Freq_ContMod), freqContMod);
                }
                //
                else if(packetType == STUHFL_TAG_GEN2TIMINGS){
                    STUHFL_T_Gen2_Timings *gen2Timings = (STUHFL_T_Gen2_Timings*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetGen2Timings(gen2Timings);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_Gen2_Timings), gen2Timings);
                }
                else if(packetType == STUHFL_TAG_GEN2PROTOCOL_CFG){
                    STUHFL_T_ST25RU3993_Gen2Protocol_Cfg *gen2ProtocolCfg = (STUHFL_T_ST25RU3993_Gen2Protocol_Cfg*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetGen2ProtocolCfg(gen2ProtocolCfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg), gen2ProtocolCfg);
                }
#if GB29768
                else if(packetType == STUHFL_TAG_GB29768PROTOCOL_CFG){
                    STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg *gb29768ProtocolCfg = (STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetGb29768ProtocolCfg(gb29768ProtocolCfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg), gb29768ProtocolCfg);
                }
#endif
                else if(packetType == STUHFL_TAG_TXRX_CFG){
                    STUHFL_T_ST25RU3993_TxRx_Cfg *txRxCfg = (STUHFL_T_ST25RU3993_TxRx_Cfg*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetTxRxCfg(txRxCfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_TxRx_Cfg), txRxCfg);
                }
                else if(packetType == STUHFL_TAG_PA_CFG){
                    STUHFL_T_ST25RU3993_PA_Cfg *paCfg = (STUHFL_T_ST25RU3993_PA_Cfg*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetPA_Cfg(paCfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_PA_Cfg), paCfg);
                }
                else if(packetType == STUHFL_TAG_GEN2INVENTORY_CFG){
                    STUHFL_T_ST25RU3993_Gen2Inventory_Cfg *invGen2Cfg = (STUHFL_T_ST25RU3993_Gen2Inventory_Cfg*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetGen2InventoryCfg(invGen2Cfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg), invGen2Cfg);
                }
#if GB29768
                else if(packetType == STUHFL_TAG_GB29768INVENTORY_CFG){
                    STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg *invGb29768Cfg = (STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetGb29768InventoryCfg(invGb29768Cfg);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg), invGb29768Cfg);
                }
#endif
                //
                else if(packetType == STUHFL_TAG_TUNING){
                    STUHFL_T_ST25RU3993_Tuning *tuning = (STUHFL_T_ST25RU3993_Tuning*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetTuning(tuning);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_Tuning), tuning);
                }
                else if(packetType == STUHFL_TAG_TUNING_TABLE_ENTRY){
                    STUHFL_T_ST25RU3993_TuningTableEntry *tuningTableEntry = (STUHFL_T_ST25RU3993_TuningTableEntry*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetTuningTableEntry(tuningTableEntry);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_TuningTableEntry), tuningTableEntry);
                }
                else if(packetType == STUHFL_TAG_TUNING_TABLE_DEFAULT){
                    STUHFL_T_ST25RU3993_TunerTableSet *tuningTableSet = (STUHFL_T_ST25RU3993_TunerTableSet*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetTuningTableDefault(tuningTableSet);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_TunerTableSet), tuningTableSet);
                }
                else if(packetType == STUHFL_TAG_TUNING_TABLE_SAVE){
                    retCode |= SetTuningTableSave2Flash();
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, 0, NULL);
                }
                else if(packetType == STUHFL_TAG_TUNING_TABLE_EMPTY){
                    retCode |= SetTuningTableEmpty();
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, 0, NULL);
                }
                else if(packetType == STUHFL_TAG_TUNING_CAPS){
                    STUHFL_T_ST25RU3993_TuningCaps *tuning = (STUHFL_T_ST25RU3993_TuningCaps*)&rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + lenLength];
                    retCode |= SetTuningCaps(tuning);
                    *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], packetType, sizeof(STUHFL_T_ST25RU3993_TuningCaps), tuning);
                }

                // move forward by adding the length of the TAG, the LENGTH and the content of the TLV
                uint16_t packetLen = (lenLength == 2) ? ((rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG) + 1] << 8) | (rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG)] & 0x7F)) : rxPayload[nextPacketOffset + sizeof(STUHFL_T_TAG)]; 
                nextPacketOffset += (packetLen + (sizeof(STUHFL_T_TAG) + lenLength));                
            }
            break;
        }
        
        case STUHFL_CC_TUNE:{
            lenLength = STUHFL_T_LEN_LENGTH(rxPayload[sizeof(STUHFL_T_TAG)]);
            STUHFL_T_ST25RU3993_Tune *tune = (STUHFL_T_ST25RU3993_Tune*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Tune(tune);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_TUNE, sizeof(STUHFL_T_ST25RU3993_Tune), tune);
            break;
        }
        case STUHFL_CC_TUNE_CHANNEL:{
            lenLength = STUHFL_T_LEN_LENGTH(rxPayload[sizeof(STUHFL_T_TAG)]);
            STUHFL_T_ST25RU3993_TuneCfg *tuneCfg = (STUHFL_T_ST25RU3993_TuneCfg*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = TuneChannel(tuneCfg);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_TUNE_CHANNEL, sizeof(STUHFL_T_ST25RU3993_TuneCfg), tuneCfg);
            break;
        }
        default:
            retCode = (uint8_t)ERR_PROTO;
            break;
    }  
    
    return retCode;
}

// --------------------------------------------------------------------------
uint8_t process_CG_SL(uint8_t cmd, uint8_t *rxPayload, uint16_t rxPayloadLen, uint8_t *txPayload, uint16_t *txPayloadLen)
{
    uint8_t     retCode = (uint8_t)ERR_PROTO;
    uint32_t    lenLength = STUHFL_T_LEN_LENGTH(rxPayload[sizeof(STUHFL_T_TAG)]);

    switch(cmd)
    {   // commands
        case STUHFL_CC_GEN2_INVENTORY:{
            STUHFL_T_Inventory_Option *invOption = (STUHFL_T_Inventory_Option*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];            
            setInventoryFromHost(true, false);
            retCode = Gen2_Inventory(invOption, NULL);
            // until here all TAG information and statistics are already send out
            // send out empty packet to close the inventory communication
            *txPayloadLen = 0;
            break;        
        }

        case STUHFL_CC_GEN2_SELECT:{
            STUHFL_T_Gen2_Select *selData = (STUHFL_T_Gen2_Select*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gen2_Select(selData);
            break;
        }
        
        case STUHFL_CC_GEN2_READ:{
            STUHFL_T_Read *readData = (STUHFL_T_Read*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gen2_Read(readData);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_GEN2_READ, sizeof(STUHFL_T_Read), readData);
            break;
        }
        
        case STUHFL_CC_GEN2_WRITE:{
            STUHFL_T_Write *writeData = (STUHFL_T_Write*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gen2_Write(writeData);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_GEN2_WRITE, sizeof(STUHFL_T_Write), writeData);
            break;
        }
        
        case STUHFL_CC_GEN2_BLOCKWRITE:{
            STUHFL_T_BlockWrite *blockWrite = (STUHFL_T_BlockWrite*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gen2_BlockWrite(blockWrite);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_GEN2_BLOCKWRITE, sizeof(STUHFL_T_BlockWrite), blockWrite);
            break;
        }
        
        case STUHFL_CC_GEN2_LOCK:{
            STUHFL_T_Gen2_Lock *lockData = (STUHFL_T_Gen2_Lock*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gen2_Lock(lockData);
            break;
        }
        
        case STUHFL_CC_GEN2_KILL:{
            STUHFL_T_Kill *killData = (STUHFL_T_Kill*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gen2_Kill(killData);
            break;
        }

        case STUHFL_CC_GEN2_GENERIC_CMD:{
            STUHFL_T_Gen2_GenericCmdSnd *genericCmdSnd = (STUHFL_T_Gen2_GenericCmdSnd*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            STUHFL_T_Gen2_GenericCmdRcv genericCmdRcv;
            retCode = Gen2_GenericCmd(genericCmdSnd, &genericCmdRcv);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_GEN2_GENERIC, sizeof(STUHFL_T_Gen2_GenericCmdRcv), &genericCmdRcv);
            break;
        }

        case STUHFL_CC_GEN2_QUERY_MEASURE_RSSI_CMD:{
            STUHFL_T_Gen2_QueryMeasureRssi *queryMeasureRssi = (STUHFL_T_Gen2_QueryMeasureRssi*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            STUHFL_T_Gen2_QueryMeasureRssi queryMeasureRssiReply;
            queryMeasureRssiReply.frequency = queryMeasureRssi->frequency;
            queryMeasureRssiReply.measureCnt = queryMeasureRssi->measureCnt;
            retCode = Gen2_QueryMeasureRssi(&queryMeasureRssiReply);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_GEN2_QUERY_MEASURE_RSSI, sizeof(STUHFL_T_Gen2_QueryMeasureRssi), &queryMeasureRssiReply);
            break;
        }
#if GB29768
        case STUHFL_CC_GB29768_INVENTORY:{
            STUHFL_T_Inventory_Option *invOption = (STUHFL_T_Inventory_Option*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];            
            setInventoryFromHost(true, false);
            retCode = Gb29768_Inventory(invOption, NULL);
            // until here all TAG information and statistics are already send out
            // send out empty packet to close the inventory communication
            *txPayloadLen = 0;
            break;        
        }

        case STUHFL_CC_GB29768_SORT:{
            STUHFL_T_Gb29768_Sort *sortData = (STUHFL_T_Gb29768_Sort*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gb29768_Sort(sortData);
            break;
        }
        
        case STUHFL_CC_GB29768_READ:{
            STUHFL_T_Read *readData = (STUHFL_T_Read*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gb29768_Read(readData);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_GB29768_READ, sizeof(STUHFL_T_Read), readData);
            break;
        }
        
        case STUHFL_CC_GB29768_WRITE:{
            STUHFL_T_Write *writeData = (STUHFL_T_Write*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gb29768_Write(writeData);
            *txPayloadLen += addTlvExt(&txPayload[*txPayloadLen], STUHFL_TAG_GB29768_WRITE, sizeof(STUHFL_T_Write), writeData);
            break;
        }
        
        case STUHFL_CC_GB29768_LOCK:{
            STUHFL_T_Gb29768_Lock *lockData = (STUHFL_T_Gb29768_Lock*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gb29768_Lock(lockData);
            break;
        }

        case STUHFL_CC_GB29768_KILL:{
            STUHFL_T_Kill *killData = (STUHFL_T_Kill*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gb29768_Kill(killData);
            break;
        }

        case STUHFL_CC_GB29768_ERASE:{
            STUHFL_T_Gb29768_Erase *eraseData = (STUHFL_T_Gb29768_Erase*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            retCode = Gb29768_Erase(eraseData);
            break;
        }
#endif        
        default:
            retCode = (uint8_t)ERR_PROTO;
            break;
    }
    
    return retCode;
}

// --------------------------------------------------------------------------
uint8_t process_CG_AL(uint8_t cmd, uint8_t *rxPayload, uint16_t rxPayloadLen, uint8_t *txPayload, uint16_t *txPayloadLen)
{
    uint8_t     retCode = (uint8_t)ERR_PROTO;
    uint32_t    lenLength = STUHFL_T_LEN_LENGTH(rxPayload[sizeof(STUHFL_T_TAG)]);
    STUHFL_T_Inventory_Option *invOption;
    
    switch(cmd)
    {   // commands
        case STUHFL_CC_INVENTORY_START:
#ifdef USE_INVENTORY_EXT
        case STUHFL_CC_INVENTORY_START_W_SLOT_STATISTICS:
            setInventoryFromHost(true, (cmd == STUHFL_CC_INVENTORY_START_W_SLOT_STATISTICS));
#else        
            setInventoryFromHost(true, false);
#endif  // USE_INVENTORY_EXT
            invOption = (STUHFL_T_Inventory_Option*)&rxPayload[sizeof(STUHFL_T_TAG) + lenLength];
            /* NOTE: Inventory runner is just configured here, to run it the main loop in FW calls periodically the cycle function */
            retCode = inventoryRunnerSetup(invOption, NULL, NULL, NULL);
            break;
        case STUHFL_CC_INVENTORY_STOP:
            retCode = InventoryRunnerStop();
            break;

        case STUHFL_CC_INVENTORY_DATA:
            retCode = handleInventoryData(false, true);
            // all data should be send here already..
            *txPayloadLen = 0;
            break;
        
        default:
            retCode = (uint8_t)ERR_PROTO;
            break;
    }

    return retCode;
}

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void StreamDispatcherInit(void)
{
    StreamDispatcherGetLastError();
    StreamInitialize(rxBuffer, txBuffer);
}

/*------------------------------------------------------------------------- */
uint8_t StreamDispatcherGetLastError(void)
{
    return lastError;
}

/*------------------------------------------------------------------------- */
uint16_t StreamDispatcherGetLastCmd(void)
{
    return lastCmd;
}

/*------------------------------------------------------------------------- */
void ProcessIO(void)
{
    if(StreamReady())
    {
        /* read out data from stream driver */
        if(StreamReceive())
        {
            StreamTransmit(processReceivedPackets());
        }
    }
}

/**
  * @}
  */
/**
  * @}
  */
