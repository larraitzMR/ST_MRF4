/**
  ******************************************************************************
  * @file           STUHFL_demoEvalAPI.c
  * @brief          Main Gen2 protocol (inventory, select, read, write) related demos
  ******************************************************************************
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

#include "stuhfl.h"
#include "stuhfl_sl.h"
#include "stuhfl_sl_gen2.h"
#include "stuhfl_sl_gb29768.h"
#include "stuhfl_al.h"
#include "stuhfl_dl.h"
#include "stuhfl_evalAPI.h"
#include "stuhfl_err.h"

#include "STUHFL_demo.h"

static bool useNewTuningMechanism = false;

void demo_gen2Select(uint8_t *epc, uint8_t epcLen);
void demo_gen2Read(uint8_t *data);
void demo_gen2Write(uint8_t *data);
void demo_gen2GenericCommand(uint8_t cmd);

/**
  * @brief      Toggle Tuning mechanism to be used ().
  *
  * @retval     None
  */
void toggleTuningMechanism(void)
{
    useNewTuningMechanism = !useNewTuningMechanism;
    log2Screen(false, true, "\tTuning Mechanism: %s\n", useNewTuningMechanism ? "NEW" : "OLD" );

    // Setup Config accordignly
    setupGen2Config(false, true, ANTENNA_1);
}

/**
  * @brief      Starts frequency tuning for current profile and current antenna.
  *             This demo addresses both old and new tuning mechanism
  *
  * @param[in]  tuningAlgo: tuning algo
  *
  * @retval     None
  */
static void tuneFreqs(uint8_t tuningAlgo)
{
    if (tuningAlgo == TUNING_ALGO_NONE) {
        return;
    }

    if (useNewTuningMechanism) {
        STUHFL_T_ST25RU3993_TxRx_Cfg txRxCfg = STUHFL_O_ST25RU3993_TXRX_CFG_INIT();

        // Get current antenna
        GetTxRxCfg(&txRxCfg);

        STUHFL_T_ST25RU3993_ChannelList channelList = STUHFL_O_ST25RU3993_CHANNELLIST_INIT();
        channelList.persistent = false;
        channelList.antenna = txRxCfg.usedAntenna;
        GetChannelList(&channelList);

        for (uint32_t i=0 ; i<channelList.nFrequencies ; i++) {
            STUHFL_T_ST25RU3993_TuneCfg tuneCfg = STUHFL_O_ST25RU3993_TUNECFG_INIT();
            tuneCfg.channelListIdx = (uint8_t)i;
            tuneCfg.antenna = txRxCfg.usedAntenna;
            tuneCfg.algorithm = tuningAlgo;
            TuneChannel(&tuneCfg);
        }
    }
    else {
        // Get freq profile + number of frequencies
        STUHFL_T_ST25RU3993_Freq_Profile_Info   freqProfileInfo = STUHFL_O_ST25RU3993_FREQ_PROFILE_INFO_INIT();
        GetFreqProfileInfo(&freqProfileInfo);

        // Tune for each freq
        for (uint8_t i=0 ; i<freqProfileInfo.numFrequencies ; i++) {
            STUHFL_T_ST25RU3993_TuningTableEntry    tuningTableEntry = STUHFL_O_ST25RU3993_TUNINGTABLEENTRY_INIT();
            STUHFL_T_ST25RU3993_Antenna_Power       antPwr = STUHFL_O_ST25RU3993_ANTENNA_POWER_INIT();
            STUHFL_T_ST25RU3993_Tune                tune = STUHFL_O_ST25RU3993_TUNE_INIT();

            tuningTableEntry.entry = i;
            GetTuningTableEntry(&tuningTableEntry);               // Retrieve frequency related to this entry

            tuningTableEntry.entry = i;
            memset(tuningTableEntry.applyCapValues, false, MAX_ANTENNA);    // Do not apply caps, only set entry
            SetTuningTableEntry(&tuningTableEntry);

            antPwr.mode = ANTENNA_POWER_MODE_ON;
            antPwr.timeout = 0;
            antPwr.frequency = tuningTableEntry.freq;
            SetAntennaPower(&antPwr);

            tune.algo = tuningAlgo;
            Tune(&tune);

            antPwr.mode = ANTENNA_POWER_MODE_OFF;
            antPwr.timeout = 0;
            antPwr.frequency = tuningTableEntry.freq;
            SetAntennaPower(&antPwr);
        }
    }
}

/**
  * @brief      Define Channel List frequencies according to required profile.
  *
  * @param[in]  profile:        targeted country profile<br>
  * @param[out] channelList:    ChannelList frequencies<br>
  *
  * @retval     None
  */
static void defineChannelListFreqs(uint8_t profile, STUHFL_T_ST25RU3993_ChannelList *channelList)
{
    switch (profile) {
        case PROFILE_EUROPE:  *channelList = STUHFL_O_ST25RU3993_CHANNELLIST_EUROPE_INIT(); break;
        case PROFILE_USA:     *channelList = STUHFL_O_ST25RU3993_CHANNELLIST_USA_INIT();    break;
        case PROFILE_JAPAN:   *channelList = STUHFL_O_ST25RU3993_CHANNELLIST_JAPAN_INIT();  break;
        case PROFILE_CHINA:   *channelList = STUHFL_O_ST25RU3993_CHANNELLIST_CHINA_INIT();  break;
        case PROFILE_CHINA2:  *channelList = STUHFL_O_ST25RU3993_CHANNELLIST_CHINA2_INIT(); break;
        default:              *channelList = STUHFL_O_ST25RU3993_CHANNELLIST_INIT();        break;  // By default: single frequency
    }
}

/**
  * @brief      Configuration for Inventory Runner + Gen2 Inventory.
  *
  * @param[in]  singleTag:      true: single tag -> no adaptive Q +  Q = 0<br>
  *                             false: multiple tags -> adaptive Q + Q = 4<br>
  * @param[in]  freqHopping:    true: EU frequency profile with hopping<br>
  *                             false: EU frequency profile without hopping<br>
  * @param[in]  antenna:        ANTENNA_1: Using Antenna 1<br>
  *                             ANTENNA_2: Using Antenna 2<br>
  *
  * @retval     None
  */
void setupGen2Config(bool singleTag, bool freqHopping, int antenna)
{
    STUHFL_T_ST25RU3993_TxRx_Cfg TxRxCfg = STUHFL_O_ST25RU3993_TXRX_CFG_INIT();          // Set to FW default values
    TxRxCfg.usedAntenna = (uint8_t)antenna;
    SetTxRxCfg(&TxRxCfg);

    STUHFL_T_ST25RU3993_Gen2Inventory_Cfg invGen2Cfg = STUHFL_O_ST25RU3993_GEN2INVENTORY_CFG_INIT();     // Set to FW default values
    invGen2Cfg.fastInv = true;
    invGen2Cfg.autoAck = false;
    invGen2Cfg.startQ = singleTag ? 0 : 4;
    invGen2Cfg.adaptiveQEnable = !singleTag;
    invGen2Cfg.adaptiveSensitivityEnable = true;
    invGen2Cfg.toggleTarget = true;
    invGen2Cfg.targetDepletionMode = true;
    SetGen2InventoryCfg(&invGen2Cfg);

    //
    STUHFL_T_ST25RU3993_Gen2Protocol_Cfg gen2ProtocolCfg = STUHFL_O_ST25RU3993_GEN2PROTOCOL_CFG_INIT();  // Set to FW default values
    SetGen2ProtocolCfg(&gen2ProtocolCfg);

    STUHFL_T_ST25RU3993_Freq_LBT freqLBT = STUHFL_O_ST25RU3993_FREQ_LBT_INIT();                          // Set to FW default values
    freqLBT.listeningTime = 0;
    SetFreqLBT(&freqLBT);

    STUHFL_T_ST25RU3993_Freq_Profile freqProfile = STUHFL_O_ST25RU3993_FREQ_PROFILE_INIT();          // Set to FW default values
    if (useNewTuningMechanism) {
        STUHFL_T_ST25RU3993_ChannelList         channelList = STUHFL_O_ST25RU3993_CHANNELLIST_INIT();
        channelList.antenna = (uint8_t)antenna;
        channelList.persistent = false;
        channelList.currentChannelListIdx = 0;
        if (freqHopping) {
            defineChannelListFreqs(PROFILE_EUROPE, &channelList);
        }
        else {
            channelList.nFrequencies = 1;
            channelList.item[0].frequency = DEFAULT_FREQUENCY;
        }
        SetChannelList(&channelList);

        freqProfile.profile = PROFILE_NEWTUNING;    // Profile is now only used to switch to new mechanism
        SetFreqProfile(&freqProfile);
    }
    else {
        if (freqHopping) {
            // EU frequency profile with hopping
            freqProfile.profile = PROFILE_EUROPE;
            SetFreqProfile(&freqProfile);
        }
        else {
            freqProfile.profile = PROFILE_CUSTOM;
            SetFreqProfile(&freqProfile);

            // EU single frequency without hopping
            STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom freqCustom = STUHFL_O_ST25RU3993_FREQ_PROFILE_ADD2CUSTOM_INIT();            // Set to FW default values
            SetFreqProfileAdd2Custom(&freqCustom);
        }
    }

    STUHFL_T_ST25RU3993_Freq_Hop freqHop = STUHFL_O_ST25RU3993_FREQ_HOP_INIT();              // Set to FW default values
    SetFreqHop(&freqHop);

    STUHFL_T_Gen2_Select    Gen2Select = STUHFL_O_GEN2_SELECT_INIT();                        // Set to FW default values
    Gen2Select.mode = GEN2_SELECT_MODE_CLEAR_LIST;  // Clear all Select filters
    Gen2_Select(&Gen2Select);
    
    printf("Tuning Profile frequencies: algo: TUNING_ALGO_SLOW\n");
    tuneFreqs(TUNING_ALGO_SLOW);
}

/**
  * @brief      Getter functions typical use.<br>
  *             => RwdCfg, FreqProfile, FreqRSSI, FreqReflectedPower, FreqLBT, InventoryCfg
  *
  * @param      None
  *
  * @retval     None
  */
void demo_evalAPI_GetInfo()
{
    STUHFL_T_ST25RU3993_Register reg = STUHFL_O_ST25RU3993_REGISTER_INIT();
    STUHFL_T_ST25RU3993_RwdConfig rwdCfg = STUHFL_O_ST25RU3993_RWDCONFIG_INIT();
    STUHFL_T_ST25RU3993_Antenna_Power antPwr = STUHFL_O_ST25RU3993_ANTENNA_POWER_INIT();
    STUHFL_T_ST25RU3993_Freq_Rssi freqRSSI = STUHFL_O_ST25RU3993_FREQ_RSSI_INIT();
    STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info freqReflectedPower = STUHFL_O_ST25RU3993_FREQ_REFLECTEDPOWER_INFO_INIT();
    STUHFL_T_ST25RU3993_Freq_Profile_Info freqProfileInfo = STUHFL_O_ST25RU3993_FREQ_PROFILE_INFO_INIT();
    STUHFL_T_ST25RU3993_Freq_Profile freqProfile = STUHFL_O_ST25RU3993_FREQ_PROFILE_INIT();
    STUHFL_T_ST25RU3993_Freq_Hop freqHop = STUHFL_O_ST25RU3993_FREQ_HOP_INIT();
    STUHFL_T_ST25RU3993_Freq_LBT freqLBT = STUHFL_O_ST25RU3993_FREQ_LBT_INIT();

    STUHFL_T_ST25RU3993_Gen2Protocol_Cfg gen2ProtocolCfg = STUHFL_O_ST25RU3993_GEN2PROTOCOL_CFG_INIT();
    STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg gb29768ProtocolCfg = STUHFL_O_ST25RU3993_GB29768PROTOCOL_CFG_INIT();
    STUHFL_T_ST25RU3993_TxRx_Cfg TxRxCfg = STUHFL_O_ST25RU3993_TXRX_CFG_INIT();
    STUHFL_T_ST25RU3993_PA_Cfg paCfg = STUHFL_O_ST25RU3993_PA_CFG_INIT();
    STUHFL_T_ST25RU3993_Gen2Inventory_Cfg invGen2Cfg = STUHFL_O_ST25RU3993_GEN2INVENTORY_CFG_INIT();
    STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg invGb29768Cfg = STUHFL_O_ST25RU3993_GB29768INVENTORY_CFG_INIT();

    reg.addr = 0;
    GetRegister(&reg);

    rwdCfg.id = 0;
    GetRwdCfg(&rwdCfg);

    // Power ON
    antPwr.mode = ANTENNA_POWER_MODE_ON;
    antPwr.frequency = 866000;
    antPwr.timeout = 0;
    SetAntennaPower(&antPwr);

    if (useNewTuningMechanism) {
        STUHFL_T_ST25RU3993_ChannelList         channelList = STUHFL_O_ST25RU3993_CHANNELLIST_INIT();
        channelList.antenna = ANTENNA_1;
        channelList.persistent = false;
        channelList.currentChannelListIdx = 0;
        defineChannelListFreqs(PROFILE_EUROPE, &channelList);       // Define frequencies
        SetChannelList(&channelList);

        freqProfile.profile = PROFILE_NEWTUNING;    // Profile is now only used to switch to new mechanism
        SetFreqProfile(&freqProfile);
    }
    else {
        freqProfile.profile = PROFILE_EUROPE;
        SetFreqProfile(&freqProfile);
    }

    freqRSSI.frequency = 866000;
    GetFreqRSSI(&freqRSSI);

    freqReflectedPower.frequency = 866000;
    freqReflectedPower.applyTunerSetting = true;
    GetFreqReflectedPower(&freqReflectedPower);

    freqProfileInfo.profile = 0;
    GetFreqProfileInfo(&freqProfileInfo);

    GetFreqHop(&freqHop);
    GetFreqLBT(&freqLBT);

    GetGen2ProtocolCfg(&gen2ProtocolCfg);
    GetGb29768ProtocolCfg(&gb29768ProtocolCfg);
    GetTxRxCfg(&TxRxCfg);
    GetPA_Cfg(&paCfg);
    GetGen2InventoryCfg(&invGen2Cfg);
    GetGb29768InventoryCfg(&invGb29768Cfg);

    // Power OFF
    antPwr.mode = ANTENNA_POWER_MODE_OFF;
    antPwr.frequency = 866000;
    antPwr.timeout = 0;
    SetAntennaPower(&antPwr);

    //
    return;
}

/**
  * @brief      Resets frequency tuning for current profile and current antenna.
  *             This demo addresses both old and new tuning mechanism
  *
  * @param[in]  startVal: Reset value for all frequencies (dependenign on chosen tunign algo, used as seed for upcoming tunings)
  *
  * @retval     None
  */
void demo_resetFreqsTuning(uint8_t startVal)
{
    STUHFL_T_ST25RU3993_TxRx_Cfg txRxCfg = STUHFL_O_ST25RU3993_TXRX_CFG_INIT();

    // Get current antenna
    GetTxRxCfg(&txRxCfg);

    if (useNewTuningMechanism) {
        STUHFL_T_ST25RU3993_ChannelList channelList = STUHFL_O_ST25RU3993_CHANNELLIST_INIT();
        channelList.persistent = false;
        channelList.antenna = txRxCfg.usedAntenna;
        GetChannelList(&channelList);
        
        for (uint32_t i=0 ; i<channelList.nFrequencies ; i++) {
            channelList.item[i].caps.cin = startVal;
            channelList.item[i].caps.clen = startVal;
            channelList.item[i].caps.cout = startVal;
        }        
        SetChannelList(&channelList);
    }
    else 
    {
        // Get freq profile + Number of frequencies
        STUHFL_T_ST25RU3993_Freq_Profile_Info   freqProfileInfo = STUHFL_O_ST25RU3993_FREQ_PROFILE_INFO_INIT();
        GetFreqProfileInfo(&freqProfileInfo);

        // Set Tuning Table entry
        STUHFL_T_ST25RU3993_TuningTableEntry tuningTableEntry = STUHFL_O_ST25RU3993_TUNINGTABLEENTRY_INIT();
        // Update Tuning values for current antenna only
        memset(&tuningTableEntry, 0x00, sizeof(STUHFL_T_ST25RU3993_TuningTableEntry));
        tuningTableEntry.applyCapValues[txRxCfg.usedAntenna] = true;
        tuningTableEntry.cin[txRxCfg.usedAntenna]  = startVal;
        tuningTableEntry.clen[txRxCfg.usedAntenna] = startVal;
        tuningTableEntry.cout[txRxCfg.usedAntenna] = startVal;
        tuningTableEntry.IQ[txRxCfg.usedAntenna] = 0xFFFF;

        for (uint8_t i=0 ; i<freqProfileInfo.numFrequencies ; i++) {
            tuningTableEntry.entry = i;
            SetTuningTableEntry(&tuningTableEntry);
        }
    }
}

/**
  * @brief      Starts frequency tuning for current profile and current antenna.
  *             This demo addresses both old and new tuning mechanism
  *
  * @param[in]  tuningAlgo: tuning algo
  *
  * @retval     None
  */
void demo_tuneFreqs(uint8_t tuningAlgo)
{
    tuneFreqs(tuningAlgo);
}

/**
  * @brief      Prints frequency tuning values
  *
  * @retval     None
  */
void printTuning(void)
{
    // Get Antenna
    STUHFL_T_ST25RU3993_TxRx_Cfg txRxCfg = STUHFL_O_ST25RU3993_TXRX_CFG_INIT();
    GetTxRxCfg(&txRxCfg);

    log2Screen(false, true, "Antenna: %d, tuning mechanism: %s\n", txRxCfg.usedAntenna+1, useNewTuningMechanism ? "NEW" : "OLD");

    if (useNewTuningMechanism) {
        STUHFL_T_ST25RU3993_ChannelList         channelList = STUHFL_O_ST25RU3993_CHANNELLIST_INIT();
        channelList.antenna = txRxCfg.usedAntenna;
        channelList.persistent = false;
        GetChannelList(&channelList);

        for (uint32_t i=0 ; i<channelList.nFrequencies ; i++) {
            log2Screen(false, true, "\tFreq: %d, cin:%.2x, clen:%.2x, cout:%.2x\n", channelList.item[i].frequency,
                                            channelList.item[i].caps.cin, channelList.item[i].caps.clen, channelList.item[i].caps.cout);
        }
    }
    else {
        STUHFL_T_ST25RU3993_Freq_Profile_Info freqProfileInfo =  STUHFL_O_ST25RU3993_FREQ_PROFILE_INFO_INIT();
        STUHFL_T_ST25RU3993_TuningTableEntry  tuningTableEntry = STUHFL_O_ST25RU3993_TUNINGTABLEENTRY_INIT();
        // Get profile info
        GetFreqProfileInfo(&freqProfileInfo);

        for (uint32_t i=0 ; i<freqProfileInfo.numFrequencies ; i++) {
            tuningTableEntry.entry = (uint8_t)i;
            GetTuningTableEntry(&tuningTableEntry);
            log2Screen(false, true, "\tFreq: %d, cin:%.2x, clen:%.2x, cout:%.2x, IQ:%.4x\n", tuningTableEntry.freq,
                   tuningTableEntry.cin[txRxCfg.usedAntenna], tuningTableEntry.clen[txRxCfg.usedAntenna], tuningTableEntry.cout[txRxCfg.usedAntenna], tuningTableEntry.IQ[txRxCfg.usedAntenna] );
        }
    }
}

/**
  * @brief      Inventory + Read + Write demo.
  *
  * @param      None
  *
  * @retval     None
  */
void demo_evalAPI_Gen2RdWr()
{
    setupGen2Config(false, true, ANTENNA_1);

    // apply data storage location, where the found TAGs shall be stored
    STUHFL_T_Inventory_Tag tagData[1] = { STUHFL_O_INVENTORY_TAG_INIT() };

    STUHFL_T_Inventory_Data invData = STUHFL_O_INVENTORY_DATA_INIT();
    invData.tagList = tagData;
    invData.tagListSizeMax = 1;

    STUHFL_T_Inventory_Option invOption = STUHFL_O_INVENTORY_OPTION_INIT();  // Init with default values

    Gen2_Inventory(&invOption, &invData);

    printTagList(&invOption, &invData);

    if (invData.tagListSize) {
        uint8_t data[MAX_READ_DATA_LEN];
        uint8_t firstWord[2];

        demo_gen2Select(invData.tagList[0].epc.data, invData.tagList[0].epc.len);

        demo_gen2Read(data);
        memcpy(firstWord, data, 2);

        demo_gen2Write("\xA5\x5A");
        demo_gen2Read(data);

        demo_gen2Write(firstWord);
        demo_gen2Read(data);
    } else {
        log2Screen(false, true, "\nNo tag found\n");
    }
}

/**
  * @brief      Launch a single Gen2 Inventory
  *
  * @param[in]  None
  *
  * @retval     None
  */
void demo_evalAPI_Gen2Inventory(void)
{
    STUHFL_T_Inventory_Option invOption = STUHFL_O_INVENTORY_OPTION_INIT();

    STUHFL_T_Inventory_Tag tagData[MAX_TAGS_PER_ROUND] = { 0 };
    STUHFL_T_Inventory_Data invData = STUHFL_O_INVENTORY_DATA_INIT();
    invData.tagList = tagData;
    invData.tagListSizeMax = MAX_TAGS_PER_ROUND;

    Gen2_Inventory(&invOption, &invData);
    printTagList(&invOption, &invData);
}

/**
  * @brief      Gen2 Generic Command demo.<br>
  *             Reads 2 words (4 bytes) to user memory bank (@ address 0) on a pre-selected tag through generic commands
  *
  * @param[in]  None
  *
  * @retval     None
  */
void demo_evalAPI_Gen2GenericCommand(void)
{
    setupGen2Config(false, true, ANTENNA_1);

    // apply data storage location, where the found TAGs shall be stored
    STUHFL_T_Inventory_Tag tagData[1] = { STUHFL_O_INVENTORY_TAG_INIT() };

    STUHFL_T_Inventory_Data invData = STUHFL_O_INVENTORY_DATA_INIT();
    invData.tagList = tagData;
    invData.tagListSizeMax = 1;

    STUHFL_T_Inventory_Option invOption = STUHFL_O_INVENTORY_OPTION_INIT();  // Init with default values

    Gen2_Inventory(&invOption, &invData);

    printTagList(&invOption, &invData);

    if (invData.tagListSize) {
        demo_gen2Select(invData.tagList[0].epc.data, invData.tagList[0].epc.len);

        demo_gen2GenericCommand(GENERIC_CMD_TRANSMCRCEHEAD);
    } else {
        log2Screen(false, true, "\nNo tag found\n");
    }
}

/**
  * @brief          Inventory demo.<br>
  *                 Launch a Gen2 inventory and outputs all detected tags
  *
  * @param[in]      invOption: Gen2 inventory options
  * @param[in]      invData: Gen2 inventory data
  * @param[out]     None
  *
  * @retval         None
  */
void printTagList(STUHFL_T_Inventory_Option *invOption, STUHFL_T_Inventory_Data *invData)
{
    //
    log2Screen(false, false, "\n\n--- Gen2_Inventory Option ---\n");
    log2Screen(false, false, "rssiMode    : %d\n", invOption->rssiMode);
    log2Screen(false, false, "reportMode  : %d\n", invOption->reportOptions);
    log2Screen(false, false, "\n");

    log2Screen(false, false, "--- Round Info ---\n");
    log2Screen(false, false, "tuningStatus: %s\n", invData->statistics.tuningStatus == TUNING_STATUS_UNTUNED ? "UNTUNED" : (invData->statistics.tuningStatus == TUNING_STATUS_TUNING ? "TUNING" : "TUNED"));
    log2Screen(false, false, "roundCnt    : %d\n", invData->statistics.roundCnt);
    log2Screen(false, false, "sensitivity : %d\n", invData->statistics.sensitivity);
    log2Screen(false, false, "Q           : %d\n", invData->statistics.Q);
    log2Screen(false, false, "adc         : %d\n", invData->statistics.adc);
    log2Screen(false, false, "frequency   : %d\n", invData->statistics.frequency);
    log2Screen(false, false, "tagCnt      : %d\n", invData->statistics.tagCnt);
    log2Screen(false, false, "empty Slots : %d\n", invData->statistics.emptySlotCnt);
    log2Screen(false, false, "collisions  : %d\n", invData->statistics.collisionCnt);
    log2Screen(false, false, "preampleErr : %d\n", invData->statistics.preambleErrCnt);
    log2Screen(false, false, "crcErr      : %d\n\n", invData->statistics.crcErrCnt);

    // print transponder information for TagList
    for (int tagIdx = 0; tagIdx < invData->tagListSize; tagIdx++) {
        log2Screen(false, false, "\n\n--- %03d ---\n", tagIdx + 1);
        log2Screen(false, false, "agc         : %d\n", invData->tagList[tagIdx].agc);
        log2Screen(false, false, "rssiLogI    : %d\n", invData->tagList[tagIdx].rssiLogI);
        log2Screen(false, false, "rssiLogQ    : %d\n", invData->tagList[tagIdx].rssiLogQ);
        log2Screen(false, false, "rssiLinI    : %d\n", invData->tagList[tagIdx].rssiLinI);
        log2Screen(false, false, "rssiLinQ    : %d\n", invData->tagList[tagIdx].rssiLinQ);
        log2Screen(false, false, "pc          : ");
        for (int i = 0; i < MAX_PC_LENGTH; i++) {
            log2Screen(false, false, "%02x ", invData->tagList[tagIdx].pc[i]);
        }
        log2Screen(false, false, "\nepcLen      : %d\n", invData->tagList[tagIdx].epc.len);
        log2Screen(false, false, "epc         : ");
        for (int i = 0; i < invData->tagList[tagIdx].epc.len; i++) {
            log2Screen(false, false, "%02x ", invData->tagList[tagIdx].epc.data[i]);
        }
        log2Screen(false, false, "\ntidLen      : %d\n", invData->tagList[tagIdx].tid.len);
        log2Screen(false, false, "tid         : ");
        for (int i = 0; i < invData->tagList[tagIdx].tid.len; i++) {
            log2Screen(false, false, "%02x ", invData->tagList[tagIdx].tid.data[i]);
        }
    }
    log2Screen(false, true, "\n");
}

/**
  * @brief      Gen2 Select demo.<br>
  *             Set and launch Select
  *
  * @param[in]  epc: tag EPC to be selected
  * @param[in]  epcLen: tag EPC length
  *
  * @retval     None
  */
void demo_gen2Select(uint8_t *epc, uint8_t epcLen)
{
    STUHFL_T_Gen2_Select selData = STUHFL_O_GEN2_SELECT_INIT();

    selData.mode = GEN2_SELECT_MODE_CLEAR_AND_ADD;
    selData.target = GEN2_TARGET_SL;
    selData.action = 0;
    selData.memBank = GEN2_MEMORY_BANK_EPC;
    selData.maskAddress = 0x00000020;
    memcpy(selData.mask, epc, epcLen);
    selData.maskLen = (uint8_t)(epcLen * 8);
    selData.truncation = 0;

    Gen2_Select(&selData);

    log2Screen(false, false, "\n--- Selecting TAG ---\n");
    for (int i = 0; i < epcLen; i++) {
        log2Screen(false, false, "%02x ", epc[i]);
    }
    log2Screen(false, true, "\n");
}

/**
  * @brief      Gen2 Read demo.<br>
  *             Read 16 bytes from user memory bank on a pre-selected tag and outputs data
  *
  * @param[out] data: buffer data
  *
  * @retval     None
  */
void demo_gen2Read(uint8_t *data)
{
    STUHFL_T_Read readData = STUHFL_O_READ_INIT();

    readData.memBank = GEN2_MEMORY_BANK_USER;
    readData.wordPtr = 0;
    readData.bytes2Read = MAX_READ_DATA_LEN;
    memset(readData.pwd, 0, 4);

    if (Gen2_Read(&readData) == ERR_NONE) {
        memcpy(data, readData.data, readData.bytes2Read);   // Read may have eventually modified read length

        log2Screen(false, false, "\n--- Read data ---\n");
        for (int i = 0; i < readData.bytes2Read; i++) {
            log2Screen(false, false, "%02x ", readData.data[i]);
        }
    } else {
        log2Screen(false, false, "\nTag cannot be read");
    }
    log2Screen(false, true, "\n");
}

/**
  * @brief      Gen2 Write demo.<br>
  *             Write a word (2 bytes) to user memory bank (address 0) on a pre-selected tag
  *
  * @param[in]  data: Word to be written
  *
  * @retval     None
  */
void demo_gen2Write(uint8_t *data)
{
    STUHFL_T_Write writeData = STUHFL_O_WRITE_INIT();

    writeData.memBank = GEN2_MEMORY_BANK_USER;
    writeData.wordPtr = 0;
    memset(writeData.pwd, 0, 4);
    memcpy(writeData.data, data, 2);
    if (Gen2_Write(&writeData) == ERR_NONE) {
        log2Screen(false, true, "\n--- Write 1st word TAG data ---\n\t=> 0x%02x%02x\n", data[0], data[1]);
    } else {
        log2Screen(false, true, "\nTag cannot be written\n");
    }
}

/**
  * @brief      Gen2 Generic Command demo.<br>
  *             Reads 2 words (4 bytes) from user memory bank (@ address 0) through generic commands
  *
  * @param[in]  cmd: Generic Cmd transmission type to be used
  *
  * @retval     None
  */
void demo_gen2GenericCommand(uint8_t cmd)
{
    STUHFL_T_Gen2_GenericCmdSnd genericCmdSnd = STUHFL_O_GEN2_GENERICCMDSND_INIT();
    STUHFL_T_Gen2_GenericCmdRcv genericCmdRcv = STUHFL_O_GEN2_GENERICCMDRCV_INIT();

    uint8_t nbWords = 2;

    genericCmdSnd.cmd = cmd;
    genericCmdSnd.noResponseTime = 0xFF;
    genericCmdSnd.appendRN16 = true;
    genericCmdSnd.sndDataBitCnt = 26;       // 26: length of effective cmd data, headerbit + rn16 (if any) are computed by FW
    genericCmdSnd.sndData[0] = 0xC2;
    genericCmdSnd.sndData[1] = 0xC0;
    genericCmdSnd.sndData[2] = (uint8_t)(nbWords >> 2);
    genericCmdSnd.sndData[3] = (uint8_t)(nbWords << 6);
    genericCmdSnd.expectedRcvDataBitCnt = (uint16_t)((nbWords*2U*8U) + 16U + ((cmd == GENERIC_CMD_TRANSMCRCEHEAD) ? 0U : 1U));

    if (Gen2_GenericCmd(&genericCmdSnd, &genericCmdRcv) == ERR_NONE) {
        log2Screen(false, true, "--- Generic Cmd (0x%.2x, Reads %d bytes + handle): 0x", cmd, nbWords*2U);
        for (uint16_t i=0 ; i<genericCmdRcv.rcvDataByteCnt ; i++) {          // Data + handle
            log2Screen(false, false, "%.2x", genericCmdRcv.rcvData[i]);
        }
        log2Screen(false, true, "\n");
    }
    else {
        log2Screen(false, true, "Tag cannot be accessed\n");
    }
}

