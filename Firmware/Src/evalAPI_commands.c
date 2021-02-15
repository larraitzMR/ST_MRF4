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
 *  @brief Functions which handle commands received via UART.
 *
 *  This file contains all functions for processing commands received via UART.
 *  It implements the parsing of reports data, executing the requested command
 *  and sending data back.
 *
 *  A description of the protocol between host and FW is included in the
 *  documentation for commands().
 *  The documentation of the various appl command functions (call*) will only discuss the
 *  payload of the command data and will not include the header information for
 *  every transmitted packet, as this is already described for commands().
 *
 *  The frequency hopping is also done in this file before calling protocol/device
 *  specific functions.
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup Commands
  * @{
  */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "platform.h"
#include "errno_application.h"
#include "flash_access.h"
#include "global.h"
#include "gen2.h"
#include "logger.h"
#include "gb29768.h"
#include "timer.h"
#include "st25RU3993.h"
#include "st25RU3993_config.h"
#include "tuner.h"
#include "adc_driver.h"
#include "leds.h"
#include "bootloader.h"

#include "stream_dispatcher.h"
#include "evalAPI_commands.h"
#include "stuhfl_evalAPI.h"
#include "stuhfl.h"

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#define MAXTAG                  64      /** Maximum number of tags that will be sent at once, this must be inline with the UART_TX_BUFFER_SIZE */

#define SESSION_GEN2            1       /** Identifier for gen2 protocol session */
#define SESSION_GB29768         2       /** Identifier for GB29768 protocol session */
        
#define MAX_SELECTS             2       /** Maximum of consecutive select commands currently supported. */
        
#define MAX_SENDING_TIME_SUB    10      /** value in ms, stop transmission when max. allocation - #define reached, select next freq when max. allocation + #define reached */

#define MAX_GEN2_DELAYED_RESPONSE_MS      20U
/*
 ******************************************************************************
 * STRUCTS
 ******************************************************************************
 */

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
uint8_t usedAntenna = ANTENNA_1;        /** Stores which antenna port is currently in use */
freq_t frequencies[NUM_SAVED_PROFILES]; /** Contains the list of used frequencies.*/
extern tuningTable_t tuningTable[NUM_SAVED_PROFILES];
uint8_t externalPA = 1;                 /** Use external or internal PA */

/**Profile settings which can be set via the following EVAL API call
 * SetFreqProfile(STUHFL_T_ST25RU3993_Freq_Profile *freqProfile);
 * We default to EUROPE settings.*/
uint8_t profile = PROFILE_EUROPE;


typedef struct {
    STUHFL_T_ST25RU3993_ChannelList channelList;
    uint16_t    tunedIQ[MAX_FREQUENCY];
    freq_t      frequencies;
    uint8_t     freqIndex;
} ChannelList_Tuning_t;
static ChannelList_Tuning_t gChannelListTuning[FW_NB_ANTENNA];


/*
******************************************************************************
* EXTERNAL VARIABLES
******************************************************************************
*/
extern uint8_t *gTxPayload;
extern uint16_t gTxPayloadLen;
extern volatile uint16_t st25RU3993Response;

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/

#ifdef USE_INVENTORY_EXT
/** Slot info storage */
static STUHFL_T_Inventory_Slot_Info_Data *slotInfoData = NULL;
static STUHFL_T_Inventory_Slot_Info_Data slotInfoDataTmp;
/** Slot storage location*/
static volatile uint32_t slotWrPos = 0;        /** Write pointer to slot statistics buffer. is always used as mod MAXSLOT */
static volatile uint32_t slotRdPos = 0;        /** Read pointer to slot statistics buffer. is always used as mod MAXSLOT */

static uint32_t gPrevSlotTime = 0;
#endif  // USE_INVENTORY_EXT

/** Inventory TAG storage */
static STUHFL_T_Inventory_Data *cycleData = NULL;
static STUHFL_T_Inventory_Data cycleDataTmp;
static STUHFL_T_Inventory_Tag tagListTmp[MAXTAG];
/** TAG storage pointers */
static volatile uint32_t tagWrPos = 0;         /** Write pointer to tags array. is always used as mod MAXTAG */
static volatile uint32_t tagRdPos = 0;         /** Read pointer to tags array. is always used as mod MAXTAG */

/** TAG selection */
static tag_t *selectedTag = NULL;                   /** Pointer to data of currently selected Tag.*/
static tag_t gCurrentTag;                            /** Actual selected Transponder */
static uint8_t numSelects;                          /** Internal number of currently configured select commands. */
static STUHFL_T_Gen2_Select selParams[MAX_SELECTS];   /** Parameters for select commands, can be changed via callSelectTag()*/

static uint8_t freqIndex = 0;                  /** Inventory frequency counter (used to calculate round) */
static uint32_t gStartScanTimestamp = 0;
static uint32_t gLastHeartBeatTick;

/*
tari;        < Tari setting
blf;         < GEN2_LF_40, ...
coding;      < GEN2_COD_FM0, ...
trext;       < 1 if the preamble is long, i.e. with pilot tone
sel;         < For QUERY Sel field
session;     < GEN2_SESSION_S0, ...
target;      < For QUERY Target field
toggleTarget;   < Target toggling
T4Min;       < T4Min value
*/
/** Default Gen2 configuration, can be changed callConfigGen2(). */
static gen2Config_t gen2Configuration = {GEN2_TARI_25_00, GEN2_LF_256, GEN2_COD_MILLER8, TREXT_LEAD_ON, 0, GEN2_SESSION_S0, GEN2_TARGET_A, false, TIMING_USE_DEFAULT_T4MIN};
static uint16_t inventoryDelay = 0; /** Delay between Inventory Rounds */

#if GB29768
//typedef struct
//{
//    uint8_t condition;   /*< Condition setting */
//    uint8_t session;     /*< GB29768_SESSION_S0, ... */
//    uint8_t target;      /*< For QUERY Target field */
//    bool    trext;       /*< 1 if the lead code is sent, 0 otherwise */
//    uint8_t blf;         /*< backscatter link frequency factor */
//    uint8_t coding;      /*< GB29768_COD_FM0, GB29768_COD_MILLER2, ... */
//    uint8_t tc;          /*< GB29768_TC_12_5, GB29768_TC_6_25 */
//    uint8_t toggleTarget; /**< Target A/B toggle */
//    uint8_t targetDepletionMode; /**< I/O Param: If set to 1 and the target shall be toggled in inventory an additional inventory round before the target is toggled will be executed. This gives "weak" transponders an additional chance to reply. */
//} gb29768Config_t;
static gb29768Config_t gb29768Config = {GB29768_CONDITION_ALL, GB29768_SESSION_S0, GB29768_TARGET_0, TREXT_LEAD_ON, GB29768_BLF_320, GB29768_COD_MILLER2, GB29768_TC_12_5, true, false};
static uint8_t numSorts;                    /** Internal number of currently configured sort commands. */
static gb29768SortParams_t sortParams[MAX_SELECTS]; /** Parameters for select commands, can be changed via callSelectTag()*/
static gb29768_Anticollision_t       gGbtAntiCollision;

static void inventoryGb29768(void);
static uint16_t powerAndSelectTagGb29768(void);
static void performSelectsGb29768(void);
#if 0
static void lockUnlockTagGb29768(void);
static void eraseTagGb29768(void);
static void killTagGb29768(void);
#endif

#endif

#ifdef ANTENNA_SWITCH
/* Antenna switch */
static uint8_t alternateAntennaEnable = 0;          /** Enable/Disable alternating antenna during scan */
static uint16_t alternateAntennaInterval = 1;       /** Alternate antenna interval (in inventory rounds) */
static uint16_t alternateAntennaRoundCounter = 0;   /** Intermediate round count for comparison with interval */
static uint8_t vlna = 3;                            /** VLNA setting. This is only used with ELANCE boards, all other board will ignore this setting */
#if (alternateAntennaEnable && usedAntenna!=ANTENNA_1)
    #error "Wrong Configuration"
#endif

#else
#if (usedAntenna != ANTENNA_1)
    #error "Wrong Configuration"
#endif
#endif


#ifdef TUNER
/** Tuning/Tuned LEDs **/
static bool tunedFrequencies[FW_NB_ANTENNA][MAX_FREQUENCY]; /** Used for tuned LED, tuned frequencies */

/** AutoTuning */
static uint8_t autoTuningAlgo = TUNING_ALGO_FAST;   /** Algorithm used for auto (re)tuning during scan */
static uint8_t autoTuningLevel = 20;                /** Deviation level for retuning in percentage */
static uint16_t autoTuningInterval = 7;             /** Auto tuning check interval (in inventory rounds) */
static uint16_t autoTuningRoundCounter = 0;         /** Intermediate round count for comparison with interval */
static uint16_t tunedLedInterval = 1;               /** Tuned LED interval (in inventory rounds) */
static uint16_t tunedLedRoundCounter = 0;           /** Intermediate round count for comparison with interval */
#endif

/** Adaptive Sensitivity */
static uint8_t gAdaptiveSensitivityEnable = 0;       /** Enable/Disable adaptive sensitivity feature */
static uint16_t gAdaptiveSensitivityInterval = 5;    /** Adaptive sensitivity interval (in inventory rounds) */
static uint16_t gAdaptiveSensitivityRoundCounter = 0;/** Intermediate round count for comparison with interval */

/** action flags */
static bool    saveTuningTable = false;     /** save tuning table asap */
static uint8_t saveChannelList = 0x00;      /** channel list entries to be saved asap */
static bool    cyclicInventory = false;     /** set until inventory runner shall be active */

/** session/profile dependent params*/
static int8_t rssiLogThreshold;             /** If rssi measurement is above this threshold the channel is regarded as used and the system will hop to the next frequency. Otherwise this frequency is used */
static uint8_t skipLBTcheck;                /** Variable to decide if LBT check should be performed. */
static uint16_t idleTime = 0;               /** Profile settings which can be set via GUI commands. We default to Europe settings. Before starting the frequency hop this time (in ms) will be waited for.*/
static uint16_t maxSendingTime = 400;       /** Profile settings which can be set via GUI commands. We default to Europe settings. Maximum allocation time (in ms) of a channel.*/
static uint16_t listeningTime = 1;          /** Profile settings which can be set via GUI commands. We default to Europe settings. Measure rssi for this time (in ms) before deciding if the channel should be used.*/
static uint8_t pwrMode = ANTENNA_POWER_MODE_OFF;
static uint16_t maxSendingLimit;            /** Maximal channel arbitration time in ms (internal value of maxSendingTime) */
static uint8_t maxSendingLimitTimedOut;     /** Will be set to 1 when #maxSendingLimit in continueCheckTimeout() timed out. */
static uint8_t continuousModulation = CONTINUOUS_MODULATION_MODE_OFF;
static uint8_t backupRxnorespwait;
static uint8_t backupRxwait;
static uint8_t autoAckMode;                 /** If set to 0 inventory round commandos will be triggered by the FW, otherwise the autoACK feature of the reader will be used which sends the required Gen2 commands automatically.*/
static uint32_t scanDuration = 0;           /** Inventory scan duration (in rounds) */
static uint8_t fastInventory;               /** If set to 0 normal inventory round will be performed, if set to 1 fast inventory rounds will be performed. The value is set in callStartStop() and callInventoryGen2().For details on normal/fast inventory rounds see parameter singulate of gen2SearchForTags().*/
static uint8_t readTIDinInventoryRound;     /** If set to 1 an read of the TID will be performed in inventory rounds. The value is set in callStartStop() and callInventoryGen2(). For details see parameter followTagCommand of gen2SearchForTags() and gen2ReadTID(tag_t *tag). */
static uint8_t targetDepletionMode; 		/** If set to 1 and target shall be toggled in inventory an additional inventory round before the target is toggled will be executed. This gives "weak" transponders an additional change to reply. */
static uint8_t rssiMode;                    /** Value for register ST25RU3993_REG_STATUSPAGE. This defines what RSSI value is sent to the host along with the tag data. The value is set in callStartStop() and callInventoryGen2(). */
static uint8_t readerPowerDownMode;         /** Currently configured power down mode of reader. Available modes are: #POWER_DOWN, #POWER_NORMAL, #POWER_NORMAL_RF and #POWER_STANDBY */
static int8_t inventoryResult;              /** To be communicated to GUI, basically result of hopFrequencies(), having information on skipped rounds */
static uint8_t gCurrentSession = 0;          /** Currently used protocol, valid values are: SESSION_GEN2 and SESSION_GB29768. start with invalid session (neither Gen2 nor GB29768)*/

/** Adaptive Q settings */
static gen2AdaptiveQ_Params_t           gGen2AdaptiveQSettings;
/**/
static uint8_t gReportOptions = 0x00;


/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */

static void inventoryGen2(void);    
static int8_t startTagConnection(void); 
static void stopTagConnection(void);

/**
  * This function is used as callback parameter for gen2SearchForTags() and 
  * will be called whenever a TAG is found.
  * It will return 1 as long as more TAG could be processed.
  * If no more TAGs could be processed it will return 0 and this will terminate
  * the inventory loop.
  *
  * @return 1 if inventory should be continued.
  */
static bool tagFoundCallback(tag_t *tag);
static void slotFinishedCallback(uint32_t slotTime, uint16_t eventMask, uint8_t Q);
static STUHFL_T_RET_CODE (*inventoryCycleCallback)(STUHFL_T_Inventory_Data *cycleData) = NULL;
static STUHFL_T_RET_CODE (*inventoryFinishedCallback)(STUHFL_T_Inventory_Data *cycleData) = NULL;
#if STUHFL_HOST_COMMUNICATION
static bool gInventoryFromHost = false;
#endif

#ifdef USE_INVENTORY_EXT
static STUHFL_T_RET_CODE (*inventorySlotCallback)(STUHFL_T_Inventory_Slot_Info_Data *slotData) = NULL;
static bool gSlotStatistics = false;
#endif

/**
  * This function can be used as callback parameter for gen2SearchForTags().
  * It will return 1 as long as allocation timeout has not been exceeded yet.
  * If allocation timeout has occurred it will return 0.
  *
  * @return 1 if allocation timeout has not been exceeded yet.
  */
static bool continueCheckTimeout(void);
static bool continueCheckTimeoutMaxAllocation(void);

/** This function checks the current session, if necessary closes it
and opens a new session. Valid session values are: #SESSION_GEN2, #SESSION_GB29768. */
static void checkAndOpenSession(uint8_t newSession);

static void pseudoRandomContinuousModulation(void);
static void etsiModeContinuousModulation(void);

static uint16_t powerAndSelectTagGen2(void);

static void initTagInfo(void);

static void updateTuningCaps(bool setCaps, uint8_t ant, uint8_t idx, STUHFL_T_ST25RU3993_Caps * caps, bool newTuning);

#ifdef TUNER
static void applyTunerSettingForFreq(uint32_t freq);
#endif

static int8_t hopFrequencies(void);
static void hopChannelRelease(void);
#ifdef ANTENNA_SWITCH        
static void switchAntenna(uint8_t antenna);
#endif
/** Handles the configured power down mode of the reader. The power down mode
  * is define in readerPowerDownMode variable and can be changed via callReaderConfig().
  * Available modes are: #POWER_DOWN, #POWER_NORMAL, #POWER_NORMAL_RF and #POWER_STANDBY
  */
static void powerDownReader(void);

/**
  * Handles power up of reader. Basically reverts changes done by powerDownReader().
  */
static void powerUpReader(void);

/** These functions increase/decrease or adapt the sensitivity setting
  */
static void increaseRoundCounter(void);

static void performSelectsGen2(void);
static uint8_t writeTagMemGen2(uint32_t memAdress, const tag_t *tag, const uint8_t *dataBuf, uint8_t dataLengthWords, uint8_t memType, int8_t *status, uint8_t *tagError);
static int8_t writeBlockTagMemGen2(uint32_t memAdress, const tag_t *tag, const uint8_t *dataBuf, uint8_t dataLengthWords, uint8_t memType, uint8_t *tagError);

/**This function reads the TID Memory of a specific Tag.\n
  *
  * The maximum length is limited by MAX_TID_LENGTH.
  * @attention This command works on the one tag which is currently in the open
  *            state
  *
  * @param *tag Pointer to the tag_t structure.
  *
  * @return The function returns an errorcode.
                  0x00 means no error occurred.
                  0xFF unknown error occurred.
                  Any other value is the backscattered error code from the tag.
 */
static int8_t gen2ReadTID(tag_t *tag);



#ifdef TUNER
static uint8_t reflectedPowerTooHigh(void);
static void runHillClimb(uint8_t algo, bool fpd);
static void resetTunedFrequencies(void);
static void setTunedFrequency(uint8_t antenna, uint8_t entry, bool tuned);
static void computeTunedLed(void);
static uint8_t getTuningStatus(void);
#endif


#ifdef EMBEDDED_TESTS
#include "stuhfl_testsAPI.h"
#include "../Firmware/storedata_tests.c"
#else
#define StoreTestData(a,b,c)
#define StoreInternalDataForTest(a,b,c)
#define StoreGen2GenericCommandDataForTest(a,b,c,d,e)
#define ResetAndStoreTestDataMaxStoredTags()
#define CheckAndStoreTestDataMaxStoredTags(a)
#endif

#ifdef GB29768FRAMES_DEBUG
extern uint8_t     gDebugFrames[MAX_DEBUG_FRAMESSIZE];
extern uint32_t    gDebugFramesSize;
#endif

/*
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 */

static bool isGen2ChipErrorCode(int8_t err)
{
    return ((err >= ERR_GEN2_ERRORCODE_NONSPECIFIC) && (err <= ERR_GEN2_ERRORCODE_OTHER));    // Invert comparisons as errors are handled with negative values
}

static bool isGb29768ChipErrorCode(int8_t err)
{
    return ((err >= ERR_GB29768_AUTH_ERROR) && (err <= ERR_GB29768_PERMISSION_ERROR));  // Invert comparisons as errors are handled with negative values
}
 
/*------------------------------------------------------------------------- */
static bool continueCheckTimeout(void)
{
    if(maxSendingLimit == 0){
        return true;
    }
    uint16_t rtcTime = rtcTimerValue();
    
    if(rtcTime >= maxSendingLimit){
        maxSendingLimitTimedOut = 1;
        return false;
    }
    
    return true;
}

/*------------------------------------------------------------------------- */
static bool continueCheckTimeoutMaxAllocation(void)
{
    if(rtcTimerValue() >= (maxSendingLimit+(MAX_SENDING_TIME_SUB*2))){
        return false;
    }
    return true;
}

/*------------------------------------------------------------------------- */
static void checkAndOpenSession(uint8_t newSession)
{
    switch(gCurrentSession)
    {
        case SESSION_GEN2:      gen2Close(); break;
#if GB29768 
        case SESSION_GB29768:   gb29768Close(); break;
#endif  
        default:                break;
    }

    switch(newSession)
    {
        case SESSION_GEN2:      gen2Open(&gen2Configuration); break;
#if GB29768
        case SESSION_GB29768:   gb29768Open(&gb29768Config); break;
#endif
        default:                break;
    }

    gCurrentSession = newSession;
}

/*------------------------------------------------------------------------- */
static void handleLEDs(void)
{
#if !JIGEN && !NO_LEDS
    /* Handle LEDs after every USB command, can be scanning, register access or anything */
    if(!HAL_GPIO_ReadPin(GPIO_EN_PORT,GPIO_EN_PIN))
    { /* Disabled: Power down mode */

        // Crystal not stable anymore
        SET_OSC_LED_OFF();

        // PLL not locked anymore
        SET_PLL_LED_OFF();
        SET_RF_LED_OFF();
    }
    else
    { /* Enabled */
        uint8_t reg = st25RU3993SingleRead(ST25RU3993_REG_AGCANDSTATUS);

        if(0x01 & reg)
        { /* Crystal stable */
            SET_OSC_LED_ON();
        }
        else
        {
            SET_OSC_LED_OFF();
        }
        if(0x02 & reg)
        { /* PLL locked */
            SET_PLL_LED_ON();
        }
        else
        {
            SET_PLL_LED_OFF();
        }
        if((0x04 & reg) && (!externalPA || (externalPA && GET_GPIO(GPIO_PA_LDO_PORT,GPIO_PA_LDO_PIN))))
        { /* RF stable */
            SET_RF_LED_ON();
        }
        else
        {
            SET_RF_LED_OFF();
        }
    }
#endif
}

/*------------------------------------------------------------------------- */
static void etsiModeContinuousModulation(void)
{
    uint16_t rxbits = 0;
    uint16_t rtcTimerValueStart = rtcTimerValue();

    st25RU3993AntennaPower(1);
    do
    {
        st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_NAK, 0, 0, 0, &rxbits, 0, 0, 1);
        st25RU3993ClrResponse();
    } while((rtcTimerValue()-rtcTimerValueStart) < 30); // should be between 10-50 ms (here: 30)
    st25RU3993AntennaPower(0);  
    delay_ms(5);    // should be between 1-10 ms (here: 5)
}

/*------------------------------------------------------------------------- */
static void pseudoRandomContinuousModulation()
{
    static uint16_t rnd;
    uint8_t buf[2];
    uint16_t rxbits = 0;

    // prepare a pseudo random value    
    rnd ^= (uint16_t)HAL_GetTick();
    rnd ^= (uint16_t)rand();            // add random number to get some additional entropy

    // now send the data
    buf[0] = (rnd & 0xFF);
    buf[1] = (rnd >> 8) & 0xFF;
    st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_TRANSMCRC, buf, 16, 0, &rxbits, 0, 0, 1);
}

/*------------------------------------------------------------------------- */
static int8_t startTagConnection(void)
{
    uint32_t numTags;

    int8_t status = hopFrequencies();
    if (status != GEN2_OK) { 
        return status;
    }

#if GB29768
    if(gCurrentSession == SESSION_GB29768) {
        // Enter direct mode before any RF but after hop freqs
        st25RU3993EnterDirectMode();
        numTags = powerAndSelectTagGb29768();
    }
    else
#endif
    {
        numTags = powerAndSelectTagGen2();
    }
    if (numTags == 0) { 
        status = GEN2_ERR_SELECT;
    }
    return status;
}    

/*------------------------------------------------------------------------- */
static void stopTagConnection(void)
{
#if GB29768
    if(gCurrentSession == SESSION_GB29768) {
        // Exit direct mode before hop freqs
        st25RU3993ExitDirectMode();
    }
#endif
    hopChannelRelease();
}    

/*------------------------------------------------------------------------- */
static uint16_t powerAndSelectTagGen2(void)
{
    gen2Config_t saveCfg;
    memcpy(&saveCfg, &gen2Configuration, sizeof(gen2Config_t));

    // Reset selectedTag
    selectedTag = NULL;

    // start with target A
    gen2Configuration.target = 0;
    gen2Configuration.sel = 0;

    // use the SL flag for the QUERY when we did select with target SL
    for(uint32_t i=0 ; i<numSelects ; i++) {
        if(selParams[i].target == GEN2_TARGET_SL){
            gen2Configuration.sel = 3;
            break;
        }
    }

    
    checkAndOpenSession(SESSION_GEN2);

    gen2SearchForTagsParams_t       p;
    STUHFL_T_Inventory_Statistics   statistics;
    gen2AdaptiveQ_Params_t          localAdaptiveQ;    

    p.tag = &gCurrentTag;
    p.singulate = true;
    p.toggleSession = false;

    p.statistics = &statistics;
    memset(p.statistics, 0x00, sizeof(STUHFL_T_Inventory_Statistics));

    p.adaptiveQ = &localAdaptiveQ;
    p.adaptiveQ->isEnabled = false;
    p.adaptiveQ->qfp = 0.0;
    p.adaptiveQ->minQ = 0;
    p.adaptiveQ->maxQ = 15;
    p.adaptiveQ->options = 0;
    for(int i = 0; i < NUM_C_VALUES; i++){
        p.adaptiveQ->c2[i] = 35;
        p.adaptiveQ->c1[i] = 15;
    }

    
    p.cbTagFound = NULL;
    p.cbSlotFinished = NULL;
    p.cbContinueScanning = NULL;
    p.cbFollowTagCommand = NULL;

    //
    performSelectsGen2();
    uint32_t numTags = gen2SearchForTags(true, &p);
    
    if(numTags == 0)
    {
        gen2Configuration.target = 1;   // change target (A --> B)
        gen2Configure(&gen2Configuration);

        performSelectsGen2();
        numTags = gen2SearchForTags(true, &p);
    }

#ifdef ANTENNA_SWITCH
    if(numTags == 0 && alternateAntennaEnable)
    {
        usedAntenna++;
        usedAntenna %= FW_NB_ANTENNA;
        switchAntenna(usedAntenna);

        gen2Configuration.target = 0;
        gen2Configure(&gen2Configuration);

        performSelectsGen2();
        numTags = gen2SearchForTags(true, &p);
        
        if(numTags == 0)
        {
            gen2Configuration.target = 1;   // change target (A --> B)
            gen2Configure(&gen2Configuration);

            performSelectsGen2();
            numTags = gen2SearchForTags(true, &p);
        }
    }
#endif
    if(numTags) {
        selectedTag = p.tag;
    }

    memcpy(&gen2Configuration, &saveCfg, sizeof(gen2Config_t));
    gen2Configure(&gen2Configuration);

    return numTags;
}

/*------------------------------------------------------------------------- */
#if GB29768
static uint16_t powerAndSelectTagGb29768(void)
{
    gb29768Config_t saveCfg;
    memcpy(&saveCfg, &gb29768Config, sizeof(gb29768Config_t));

    // Reset selectedTag
    selectedTag = NULL;

    gb29768_SearchForTagsParams_t   p;
    p.tag = &gCurrentTag;

    STUHFL_T_Inventory_Statistics   statistics;
    p.statistics = &statistics;
    memset(p.statistics, 0x00, sizeof(STUHFL_T_Inventory_Statistics));

    // No Anticollision, gets only first answer
    p.antiCollision.endThreshold = 0;
    p.antiCollision.ccnThreshold = 0xFF;
    p.antiCollision.cinThreshold = 0;
    
    p.singulate = true;
    p.toggleSession = false;

    p.cbTagFound = NULL;
    p.cbSlotFinished = NULL;
    p.cbContinueScanning = NULL;
    p.cbFollowTagCommand = NULL;

    // start with target A
    gb29768Config.session = GB29768_SESSION_S0;
    gb29768Config.target = GB29768_TARGET_0;
    gb29768Config.condition = GB29768_CONDITION_ALL;
    for (uint32_t i=0 ; i<numSorts ; i++){
        if(sortParams[i].target == GB29768_TARGET_MATCHINGFLAG){
            // MATCHINGFLAG has been selected
            // enable matching flag only on flags set to 1 considering select rule=GB29768_RULE_MATCH0_ELSE_1 (0x03) is used to invert selection
            gb29768Config.condition = GB29768_CONDITION_FLAG1;
            break;
        }
    }

    if (gCurrentSession == SESSION_GB29768) {
        gb29768Configure(&gb29768Config);
    }
    else {
        checkAndOpenSession(SESSION_GB29768);
    }    

    performSelectsGb29768();
    uint32_t numTags = gb29768SearchForTags(&p);
    
    if(numTags == 0)
    {
        gb29768Config.target = GB29768_TARGET_1;   // change target (0 --> 1)
        gb29768Configure(&gb29768Config);
        performSelectsGb29768();
        numTags = gb29768SearchForTags(&p);
    }

#ifdef ANTENNA_SWITCH
    if(numTags == 0 && alternateAntennaEnable)
    {
        st25RU3993ExitDirectMode();
        usedAntenna++;
        usedAntenna %= FW_NB_ANTENNA;
        switchAntenna(usedAntenna);

        st25RU3993EnterDirectMode();
        gb29768Config.target = GB29768_TARGET_0;
        gb29768Configure(&gb29768Config);
        performSelectsGb29768();
        numTags = gb29768SearchForTags(&p);
        
        if(numTags == 0)
        {
            gb29768Config.target = GB29768_TARGET_1;   // change target (0 --> 1)
            gb29768Configure(&gb29768Config);
            performSelectsGb29768();
            numTags = gb29768SearchForTags(&p);
        }
    }
#endif

    if(numTags) {
        selectedTag = p.tag;
    }
    else {
    }

    memcpy(&gb29768Config, &saveCfg, sizeof(gb29768Config_t));
    gb29768Configure(&gb29768Config);

    return numTags;
}
#endif

/*------------------------------------------------------------------------- */
static void initTagInfo(void)
{
    memset(&gCurrentTag, 0, sizeof(tag_t));
}

#ifdef TUNER
/*------------------------------------------------------------------------- */
static void setTuningIndexFromFreq(uint32_t freq)
{
    //find the best matching frequency
    uint32_t best = FREQUENCY_MAX_VALUE;
    uint8_t  idx = 0;

    if (profile == PROFILE_NEWTUNING) {
        for(uint8_t i=0; (i<gChannelListTuning[usedAntenna].channelList.nFrequencies) && (best > 0) ; i++)
        {
            uint32_t diff = (gChannelListTuning[usedAntenna].channelList.item[i].frequency > freq) ? gChannelListTuning[usedAntenna].channelList.item[i].frequency - freq : freq - gChannelListTuning[usedAntenna].channelList.item[i].frequency;
            if(diff < best)
            {
                idx = i;
                best = diff;
            }
        }
        gChannelListTuning[usedAntenna].channelList.currentChannelListIdx = idx;
    }
    else {
        for(uint8_t i=0; (i<tuningTable[profile].tableSize) && (best > 0) ; i++)
        {
            uint32_t diff = (tuningTable[profile].freq[i] > freq) ? tuningTable[profile].freq[i] - freq : freq - tuningTable[profile].freq[i];
            if(diff < best)
            {
                idx = i;
                best = diff;
            }
        }
        tuningTable[profile].currentEntry = idx;
    }
}

/*------------------------------------------------------------------------- */
static void applyTunerSettingForFreq(uint32_t freq)
{
    STUHFL_T_ST25RU3993_Caps caps;
    STUHFL_T_ST25RU3993_Caps *pCaps;

    if (profile == PROFILE_NEWTUNING) {
        if(gChannelListTuning[usedAntenna].channelList.nFrequencies == 0) {    //tuning is disabled
            return;
        }
        pCaps = &gChannelListTuning[usedAntenna].channelList.item[ gChannelListTuning[usedAntenna].channelList.currentChannelListIdx ].caps;
    }
    else {
        if(tuningTable[profile].tableSize == 0) {    //tuning is disabled
            return;
        }
        caps.cin = tuningTable[profile].cin[usedAntenna][ tuningTable[profile].currentEntry ];
        caps.clen = tuningTable[profile].clen[usedAntenna][ tuningTable[profile].currentEntry ];
        caps.cout = tuningTable[profile].cout[usedAntenna][ tuningTable[profile].currentEntry ];
        pCaps = &caps;
    }

    setTuningIndexFromFreq(freq);

    //apply the found parameters if enabled for the current antenna
    tunerSetTuning(pCaps->cin, pCaps->clen, pCaps->cout);
}
#endif

/*------------------------------------------------------------------------- */
#ifdef TUNER
static uint8_t reflectedPowerTooHigh(void)
{
    uint16_t reflectedPower;
    
    // set sensitivity to 0 as saved reflected power has been retrieved with sensitivity=0
    if(externalPA) {
        //uint8_t regValRXMixerGain = st25RU3993SingleRead(ST25RU3993_REG_RXMIXERGAIN);
        int8_t sensitivity = st25RU3993GetSensitivity();
        st25RU3993SetSensitivity(0);
        reflectedPower = tunerGetReflected();
        //st25RU3993SingleWrite(ST25RU3993_REG_RXMIXERGAIN, regValRXMixerGain);
        st25RU3993SetSensitivity(sensitivity);
    }
    else {
        reflectedPower = tunerGetReflected();
    }
    uint16_t reflectedPowerSaved;
    if (profile == PROFILE_NEWTUNING) {
        reflectedPowerSaved = gChannelListTuning[usedAntenna].tunedIQ[ gChannelListTuning[usedAntenna].channelList.currentChannelListIdx ];
    }
    else {
        reflectedPowerSaved = tuningTable[profile].tunedIQ[usedAntenna][ tuningTable[profile].currentEntry ];
    }
    uint16_t deviation = (reflectedPower > reflectedPowerSaved ? reflectedPower-reflectedPowerSaved : reflectedPowerSaved-reflectedPower);
    
    if(deviation >= (uint16_t)autoTuningLevel){
        // if reflected power deviation is bigger then limit redo tuning
        return 1;
    }
    return 0;
}

/*------------------------------------------------------------------------- */
static void computeTunedLed(void)
{
    uint8_t  currentEntry = (profile == PROFILE_NEWTUNING) ? gChannelListTuning[usedAntenna].channelList.currentChannelListIdx : tuningTable[profile].currentEntry;
    if(tunedFrequencies[usedAntenna][currentEntry]) {
        SET_TUNED_LED_ON();
    }
    else {
        SET_TUNED_LED_OFF();
    }
}

/*------------------------------------------------------------------------- */
static void resetTunedFrequencies(void)
{
    // No freq is currently tuned
    memset((uint8_t *)tunedFrequencies, 0, FW_NB_ANTENNA*MAX_FREQUENCY*sizeof(bool));

    computeTunedLed();
}

/*------------------------------------------------------------------------- */
static void setTunedFrequency(uint8_t antenna, uint8_t entry, bool tuned)
{
    tunedFrequencies[antenna][entry] = tuned;
    computeTunedLed();
}

/*------------------------------------------------------------------------- */
static uint8_t getTuningStatus(void)
{
    uint8_t  currentEntry = (profile == PROFILE_NEWTUNING) ? gChannelListTuning[usedAntenna].channelList.currentChannelListIdx : tuningTable[profile].currentEntry;
    return tunedFrequencies[usedAntenna][currentEntry] ? TUNING_STATUS_TUNED : TUNING_STATUS_UNTUNED;
}

/*------------------------------------------------------------------------- */
static void runHillClimb(uint8_t algo, bool fpd)
{
    if (algo == TUNING_ALGO_NONE) {
        return;
    }

    //uint8_t regValRXMixerGain = 0;
    int8_t sensitivity = 0;

    // tuning LED on while tuning
    SET_TUNING_LED_ON();

    // set sensitivity to 0 during tuning
    if(externalPA)
    {
        //regValRXMixerGain = st25RU3993SingleRead(ST25RU3993_REG_RXMIXERGAIN);
        sensitivity = st25RU3993GetSensitivity();
        st25RU3993SetSensitivity(0);
    }

    STUHFL_T_ST25RU3993_Caps caps;
    uint16_t    tunedIQ;
    uint8_t     currentEntry;
    if (profile == PROFILE_NEWTUNING) {
        currentEntry = gChannelListTuning[usedAntenna].channelList.currentChannelListIdx;
        memcpy(&caps, &gChannelListTuning[usedAntenna].channelList.item[currentEntry].caps, sizeof(STUHFL_T_ST25RU3993_Caps));
    }
    else {
        currentEntry = tuningTable[profile].currentEntry;
        caps.cin = tuningTable[profile].cin[usedAntenna][currentEntry];
        caps.clen = tuningTable[profile].clen[usedAntenna][currentEntry];
        caps.cout = tuningTable[profile].cout[usedAntenna][currentEntry];
    }

    switch(algo)
    {
        case TUNING_ALGO_FAST:      tunerOneHillClimb(&caps, &tunedIQ, fpd, 30);  break;         // Simple: hill climb from current setting
        case TUNING_ALGO_SLOW:      tunerMultiHillClimb(&caps, &tunedIQ, fpd);  break;           // advanced: hill climb from more points
        case TUNING_ALGO_MEDIUM:    tunerEnhancedMultiHillClimb(&caps, &tunedIQ, fpd);  break;   // Enhanced Multi Hill
        default:                    tunerOneHillClimb(&caps, &tunedIQ, fpd, 30);  break;         // Simple: default algorithm
    }

    if (profile == PROFILE_NEWTUNING) {
        updateTuningCaps(false, usedAntenna, currentEntry, &caps, true);              // Update caps with values resulting from tuning
        gChannelListTuning[usedAntenna].tunedIQ[currentEntry] = tunedIQ;
    }
    else {
        updateTuningCaps(false, usedAntenna, currentEntry, &caps, false);              // Update caps with values resulting from tuning
        tuningTable[profile].tunedIQ[usedAntenna][currentEntry] = tunedIQ;
    }

    if(externalPA)
    {
        //st25RU3993SingleWrite(ST25RU3993_REG_RXMIXERGAIN, regValRXMixerGain);
        st25RU3993SetSensitivity(sensitivity);
    }
    // tuning LED off
    SET_TUNING_LED_OFF();
}
#endif  // TUNER

/*------------------------------------------------------------------------- */
static STUHFL_T_RET_CODE runChannelTuning(uint32_t freq, uint8_t algo, bool fpd)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    uint8_t     pllOk = false;

#ifdef TUNER
    switch (algo) { 
        case TUNING_ALGO_FAST:
        case TUNING_ALGO_SLOW:
        case TUNING_ALGO_MEDIUM:
            powerUpReader();
            for(uint32_t tries=0 ; (tries<5) && (!pllOk) ; tries++) {
                pllOk = st25RU3993SetBaseFrequency(freq);
            }
            if(pllOk)
            {
                st25RU3993AntennaPower(1);

                setTuningIndexFromFreq(freq);
                runHillClimb(algo, fpd);

                st25RU3993AntennaPower(0);
                err = ERR_NONE;
            }
            else {
                err = (STUHFL_T_RET_CODE)ERR_IO;
            }
            powerDownReader();
            break;
        case TUNING_ALGO_NONE:
            setTuningIndexFromFreq(freq);
            err = ERR_NONE;
            break;
        default:
            err = (STUHFL_T_RET_CODE)ERR_REQUEST;
            break;
    }
#endif

    return err;
}

/*------------------------------------------------------------------------- */
static void updateTuningCaps(bool setCaps, uint8_t ant, uint8_t idx, STUHFL_T_ST25RU3993_Caps * refCaps, bool newTuning)
{
    STUHFL_T_ST25RU3993_Caps caps;

    caps.cin  = (refCaps->cin >  MAX_DAC_VALUE ? MAX_DAC_VALUE : refCaps->cin);
    caps.clen = (refCaps->clen > MAX_DAC_VALUE ? MAX_DAC_VALUE : refCaps->clen);
    caps.cout = (refCaps->cout > MAX_DAC_VALUE ? MAX_DAC_VALUE : refCaps->cout);
    
    if (newTuning) {
        memcpy(&gChannelListTuning[ant].channelList.item[idx].caps, &caps, sizeof(STUHFL_T_ST25RU3993_Caps));
    }
    else {
        // Update Tuning Table
        tuningTable[profile].cin[ant][idx]  = caps.cin;
        tuningTable[profile].clen[ant][idx] = caps.clen;
        tuningTable[profile].cout[ant][idx] = caps.cout;
    }
    
    // Update registers
    if (setCaps) {
        tunerSetCap(TUNER_CIN,  caps.cin);
        tunerSetCap(TUNER_CLEN, caps.clen);
        tunerSetCap(TUNER_COUT, caps.cout);
    }
    
    setTunedFrequency(ant, idx, true);
}

/*------------------------------------------------------------------------- */
static void increaseRoundCounter(void)
{
    cycleData->statistics.roundCnt++;
#ifdef TUNER
    autoTuningRoundCounter++;
    tunedLedRoundCounter++;
#endif
    gAdaptiveSensitivityRoundCounter++;
#ifdef ANTENNA_SWITCH
    alternateAntennaRoundCounter++;
#endif
}

/*------------------------------------------------------------------------- */
static int8_t hopFrequencies(void)
{
    uint8_t rssiLogIQSum = 0;
    uint8_t pllOk;
    uint16_t idleDelay;
    uint8_t i;
    uint8_t rssiLogI = 0;
    uint8_t rssiLogQ = 0;
    uint8_t regValRFOutput = 0;

    uint8_t *pFreqIndex;
    freq_t  *pFrequencies;

    if (profile == PROFILE_NEWTUNING) {
        pFreqIndex = &gChannelListTuning[usedAntenna].freqIndex;
        pFrequencies = &gChannelListTuning[usedAntenna].frequencies;
    }
    else {
        pFreqIndex = &freqIndex;
        pFrequencies = &frequencies[profile];
    }

    // select next frequency
    (*pFreqIndex)++;
    if(*pFreqIndex >= pFrequencies->numFreqs) {
        *pFreqIndex = 0;
    }

    if(externalPA && (rssiLogThreshold < 31) && !skipLBTcheck)
    {
        //PA eTX<3-2> disable
        regValRFOutput = st25RU3993SingleRead(ST25RU3993_REG_RFOUTPUTCONTROL);
        st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, regValRFOutput & 0xC3);

        // skip Listen Before Talk (LBT) if flag is set OR
        // skip rssi measurement if threshold is too high and I + Q is always below this threshold OR
        // if the reference divider is too high so that the target frequency cannot be set
        uint8_t reg_pll = st25RU3993SingleRead(ST25RU3993_REG_PLLMAIN1);
        uint8_t ref;
        switch(reg_pll&0x70)
        {
            case 0x40:
            {
                ref=125;
                break;
            }
            case 0x50:
            {
                ref=100;
                break;
            }
            case 0x60:
            {
                ref=50;
                break;
            }
            case 0x70:
            {
                ref=25;
                break;
            }
            default:
            {
                ref=0;
                break;
            }
        }
        if(ref <= ST25RU3993_RSSI_FREQUENCY_OFFSET)
        {
            uint8_t regValRFOutput;

            // LBT idle time set in GUI
            if(idleTime)
            {
                rtcTimerReset();       // start timer for idle delay

                idleDelay = rtcTimerValue();
                if(idleTime > idleDelay)
                {
                    // wait for idle time
                    delay_ms(idleTime-idleDelay);
                }
            }

            // go once through all available frequencies for LBT
            regValRFOutput = st25RU3993SingleRead(ST25RU3993_REG_RFOUTPUTCONTROL);
            st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, regValRFOutput & 0xC3);
            int8_t sensitivity = st25RU3993GetSensitivity();
            for(i=0 ; i<pFrequencies->numFreqs ; i++)
            {
                uint8_t rssiLogIQSumPlusOffset;
                uint8_t rssiLogIQSumMinusOffset;

                // Listen Before Talk (LBT)
                // change for RSSI PLL frequency, dependency on filter setting --> see comment of define ST25RU3993_RSSI_FREQUENCY_OFFSET
                // measure RSSI at [freq - OFFSET]
                pllOk = st25RU3993SetBaseFrequency((pFrequencies->freq[*pFreqIndex]-ST25RU3993_RSSI_FREQUENCY_OFFSET));
                setTuningIndexFromFreq((pFrequencies->freq[*pFreqIndex]-ST25RU3993_RSSI_FREQUENCY_OFFSET));
                if(pllOk)
                {                    
                    st25RU3993GetRSSIMaxSensitive(listeningTime, &rssiLogI, &rssiLogQ);
                    rssiLogIQSumMinusOffset = rssiLogI + rssiLogQ;

                    // measure RSSI at [freq + OFFSET]
                    pllOk = st25RU3993SetBaseFrequency((pFrequencies->freq[*pFreqIndex]+ST25RU3993_RSSI_FREQUENCY_OFFSET));
                    setTuningIndexFromFreq((pFrequencies->freq[*pFreqIndex]+ST25RU3993_RSSI_FREQUENCY_OFFSET));
                    if(pllOk)
                    {
                        st25RU3993GetRSSIMaxSensitive(listeningTime, &rssiLogI, &rssiLogQ);
                        rssiLogIQSumPlusOffset = rssiLogI + rssiLogQ;

                        // take minimum RSSI of both [freq +/- OFFSET]
                        if(rssiLogIQSumPlusOffset>rssiLogIQSumMinusOffset)
                        {
                            rssiLogIQSum = rssiLogIQSumPlusOffset;
                        }
                        else
                        {
                            rssiLogIQSum = rssiLogIQSumMinusOffset;
                        }

                        if(rssiLogIQSum <= rssiLogThreshold)
                        {
                            // Found free frequency, now we can return
                            break;
                        }
                        else{
                        }
                    }
                    
                }
                
                // Frequency not free
                // select next frequency
                (*pFreqIndex)++;
                if(*pFreqIndex >= pFrequencies->numFreqs) {
                    *pFreqIndex = 0;
                }
            }
            // reset
            st25RU3993SetSensitivity(sensitivity);
            st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, regValRFOutput);
        }
        pllOk = st25RU3993SetBaseFrequency(pFrequencies->freq[*pFreqIndex]);
        setTuningIndexFromFreq(pFrequencies->freq[*pFreqIndex]);
        if(pllOk)
        {
            st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, regValRFOutput);
        }
    }
    else
    {
        pllOk = st25RU3993SetBaseFrequency(pFrequencies->freq[*pFreqIndex]);
        setTuningIndexFromFreq(pFrequencies->freq[*pFreqIndex]);
    }

    if(!pllOk)
    {
        return GEN2_ERR_CHANNEL_TIMEOUT;
    }

    // maximum allocation time set per channel in GUI
    maxSendingLimit = (maxSendingTime-MAX_SENDING_TIME_SUB);
    
    // RSSI (measured in LBT mode, or 0 otherwise) is below threshold
    if(rssiLogIQSum <= rssiLogThreshold)
    {
        rtcTimerReset();
        maxSendingLimitTimedOut = 0;
#ifdef TUNER
        applyTunerSettingForFreq(pFrequencies->freq[*pFreqIndex]);
#endif

        st25RU3993AntennaPower(1);

#ifdef TUNER
        // Check if we have to do a re-tune (autotune)
        uint8_t algo = autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD;
        if((algo != TUNING_ALGO_NONE) && (autoTuningRoundCounter >= autoTuningInterval)) {
            if(reflectedPowerTooHigh()) {
                runHillClimb(algo, (autoTuningAlgo & TUNING_ALGO_ENABLE_FPD) ? true : false);
            }
            autoTuningRoundCounter = 0;
        }

        // Manage tune status
        uint8_t  currentEntry = (profile == PROFILE_NEWTUNING) ? gChannelListTuning[usedAntenna].channelList.currentChannelListIdx : tuningTable[profile].currentEntry;
        setTunedFrequency(usedAntenna, currentEntry, !reflectedPowerTooHigh());
#endif  // TUNER
    }
    else
    {
        // No free channel found
        maxSendingLimitTimedOut = 1;
        st25RU3993AntennaPower(0);
        return GEN2_ERR_CHANNEL_TIMEOUT;
    }

    return GEN2_OK;
}

/*------------------------------------------------------------------------- */
static void hopChannelRelease(void)
{
    if(readerPowerDownMode != POWER_NORMAL_RF)
    {
        st25RU3993AntennaPower(0);
    }
}

/*------------------------------------------------------------------------- */
#ifdef ANTENNA_SWITCH
static void switchAntenna(uint8_t antenna)
{
    if(antenna == ANTENNA_1)
    {
#if DISCOVERY
        SET_GPIO(GPIO_ANTENNA_PORT,GPIO_ANTENNA_PIN);
#elif EVAL
        RESET_GPIO(GPIO_ANT_SW_V1_PORT,GPIO_ANT_SW_V1_PIN);
        SET_GPIO(GPIO_ANT_SW_V2_PORT,GPIO_ANT_SW_V2_PIN);
#elif ELANCE
        RESET_GPIO(GPIO1_GPIO_Port, GPIO1_Pin);
        RESET_GPIO(GPIO2_GPIO_Port, GPIO2_Pin);
#elif JIGEN
#else
        #error "Undefined Code for selected board"
#endif

        // Update LEDs
        SET_ANT1_LED_ON();
        SET_ANT2_LED_OFF();
    }
    else if(antenna == ANTENNA_2)
    {
#if DISCOVERY
        RESET_GPIO(GPIO_ANTENNA_PORT,GPIO_ANTENNA_PIN);
#elif EVAL
        SET_GPIO(GPIO_ANT_SW_V1_PORT,GPIO_ANT_SW_V1_PIN);
        RESET_GPIO(GPIO_ANT_SW_V2_PORT,GPIO_ANT_SW_V2_PIN);
#elif ELANCE
        RESET_GPIO(GPIO1_GPIO_Port, GPIO1_Pin);
        SET_GPIO(GPIO2_GPIO_Port, GPIO2_Pin);
#elif JIGEN
#else
        #error "Undefined Code for selected board"
#endif

        SET_ANT1_LED_OFF();
        SET_ANT2_LED_ON();
    }
#if ELANCE
    else if(antenna == ANTENNA_3)
    {
        SET_GPIO(GPIO1_GPIO_Port, GPIO1_Pin);
        RESET_GPIO(GPIO2_GPIO_Port, GPIO2_Pin);
    }
    else if(antenna == ANTENNA_4)
    {
        SET_GPIO(GPIO1_GPIO_Port, GPIO1_Pin);
        SET_GPIO(GPIO2_GPIO_Port, GPIO2_Pin);
    }
#endif
}
#endif

/*------------------------------------------------------------------------- */
static void powerDownReader(void)
{
    switch(readerPowerDownMode)
    {
        case POWER_DOWN:
        {
            st25RU3993EnterPowerDownMode();
            break;
        }
        case POWER_NORMAL:
        {
            st25RU3993EnterPowerNormalMode();
            break;
        }
        case POWER_NORMAL_RF:
        {
            st25RU3993EnterPowerNormalRfMode();
            break;
        }
        case POWER_STANDBY:
        {
            st25RU3993EnterPowerStandbyMode();
            break;
        }
        default:
        {
            st25RU3993EnterPowerDownMode();
            break;
        }
    }
}

/*------------------------------------------------------------------------- */
static void powerUpReader(void)
{
    switch(readerPowerDownMode)
    {
        case POWER_DOWN:
        {
            st25RU3993ExitPowerDownMode();
            break;
        }
        case POWER_NORMAL:
        {
            st25RU3993ExitPowerNormalMode();
            break;
        }
        case POWER_NORMAL_RF:
        {
            st25RU3993ExitPowerNormalRfMode();
            break;
        }
        case POWER_STANDBY:
        {
            st25RU3993ExitPowerStandbyMode();
            break;
        }
        default:
        {
            st25RU3993ExitPowerDownMode();
            break;
        }
    }
}


/*------------------------------------------------------------------------- */
static void performSelectsGen2(void)
{
    for(uint32_t i=0 ; i<numSelects ; i++) {
        gen2Select(&selParams[i]);
    }
    StoreTestData(TEST_ID_GEN2_SELECT_T4MIN, sizeof(uint16_t), (uint8_t *)&gen2Configuration.T4Min);
}
/*------------------------------------------------------------------------- */
static int8_t gen2ReadTID(tag_t *tag)
{
    uint8_t nbWords = 4;
    
    tag->tidlen = 0;

    uint8_t len = 0;
    int8_t ret = gen2ReadFromTag(tag, MEM_TID, 0, &nbWords, tag->tid);     // Read 4 words at once
    if(ret == ERR_NONE) {
        len = 8;    // the 8 bytes were read
    }
    if (ret == ERR_NOMEM) {
        // Less data than the expected 4 words, not an error => will read TID word per Word
        ret = ERR_NONE;
    }

    nbWords = 1;
    // Complete the TID by reading word per word
    while((ret == ERR_NONE) && (len < MAX_TID_LENGTH)){
        ret = gen2ReadFromTag(tag, MEM_TID, len/2, &nbWords, &tag->tid[len]);
        len += 2;
    }

    if (ret == ERR_NONE) {
        tag->tidlen = len;
    }
    else if (ret == ERR_NOMEM) {
        // Reached last TID byte
        ret = ERR_NONE;         // Not an error
        tag->tidlen = len - 2;  // Remove the 2 extra bytes
    }
    else {
        ret = ERR_CHIP_NORESP;
    }
    
    return ret;
}
/*------------------------------------------------------------------------- */
static uint8_t writeTagMemGen2(uint32_t memAdress, const tag_t *tag, const uint8_t *dataBuf, uint8_t dataLengthWords, uint8_t memType, int8_t *status, uint8_t *tagError)
{
    int8_t   error = ERR_NONE;
    uint8_t  lengthWord = 0;

    while ((lengthWord < dataLengthWords) && (error == ERR_NONE))
    {
        bool continueLoop;
        uint32_t count = 4;
        
        do
        {
            uint32_t cmdEndTick = HAL_GetTick() + MAX_GEN2_DELAYED_RESPONSE_MS;
            error = gen2WriteWordToTag(tag, memType, memAdress + lengthWord, &dataBuf[2*lengthWord], tagError);
            do
            {
                if (((error == ST25RU3993_ERR_PREAMBLE) || (error == ST25RU3993_ERR_CRC)) && (HAL_GetTick() < cmdEndTick)) {
                    // Reader probably received burst from tag
                    // Ignore it and keep on waiting for tag response
                    error = gen2ContinueCommand(tagError);
                    continueLoop = (HAL_GetTick() < cmdEndTick);
                }
                else {
                    if ((error != ERR_NONE) && !isGen2ChipErrorCode(error)) {
                        delay_ms(20);           // T5: Potentially transaction is still ongoing, avoid metastable eeprom and stuff

                        continueLoop = continueCheckTimeout();
                        if (! maxSendingLimitTimedOut) {
                            // Check anyway data were not written through a read (faster than launching another useless write sequence)
                            uint8_t readBuf[2];
                            uint8_t nbWords = 1;
                            if (gen2ReadFromTag((tag_t *)tag, memType, memAdress + lengthWord, &nbWords, readBuf) == ERR_NONE) {
                                const uint8_t *p_dataBuf = &dataBuf[2*lengthWord];
                                if ((p_dataBuf[0] == readBuf[0]) && (p_dataBuf[1] == readBuf[1])) {
                                    // Eventually data were dully written
                                    error = ERR_NONE;
                                }
                            }
                        }
                    }
                    continueLoop = false;
                }
            } while((error != ERR_NONE) && !isGen2ChipErrorCode(error) && continueLoop);    // Loop on tag bursts
            
            count--;
            continueLoop = continueCheckTimeout();
        } while((error != ERR_NONE) && !isGen2ChipErrorCode(error) && (count) && continueLoop); // Give them 7 possible trials before giving up

        if(error == ERR_NONE) {
            lengthWord++;

            if (maxSendingLimitTimedOut) {
                error = GEN2_ERR_CHANNEL_TIMEOUT;
            }
        }
    }

    *status = error;
    return lengthWord;
}
/*------------------------------------------------------------------------- */
static int8_t writeBlockTagMemGen2(uint32_t memAdress, const tag_t *tag, const uint8_t *dataBuf, uint8_t dataLengthWords, uint8_t memType, uint8_t *tagError)
{
    int8_t      error;
    bool        continueLoop;
    uint32_t    count = 4;

    do
    {
        uint32_t cmdEndTick = HAL_GetTick() + MAX_GEN2_DELAYED_RESPONSE_MS;
        error = gen2WriteBlockToTag(tag, memType, memAdress, dataBuf, dataLengthWords, tagError);
        do
        {
            if (((error == ST25RU3993_ERR_PREAMBLE) || (error == ST25RU3993_ERR_CRC)) && (HAL_GetTick() < cmdEndTick)) {
                // Reader probably received burst from tag
                // Ignore it and keep on waiting for tag response
                error = gen2ContinueCommand(tagError);
                continueLoop = (HAL_GetTick() < cmdEndTick);
            }
            else {
                if ((error != ERR_NONE) && !isGen2ChipErrorCode(error)) {
                    delay_ms(20);           // T5: Potentially transaction is still ongoing, avoid metastable eeprom and stuff

                    continueLoop = continueCheckTimeout();
                    if (! maxSendingLimitTimedOut) {
                        // Check anyway data were not written through a read (faster than launching another useless write sequence)
                        uint8_t readBuf[MAX_READ_DATA_LEN];
                        uint8_t nbWordsToread = (dataLengthWords < MAX_READ_DATA_LEN/2) ? dataLengthWords : MAX_READ_DATA_LEN/2;
                        if (gen2ReadFromTag((tag_t *)tag, memType, memAdress, &nbWordsToread, readBuf) == ERR_NONE) {
                            if (memcmp(readBuf, dataBuf, nbWordsToread*2) == 0) {       // Ensures at least all readable at once data are identical
                                // Eventually data were dully written
                                error = ERR_NONE;
                            }
                        }
                    }
                }
                continueLoop = false;
            }
        } while((error != ERR_NONE) && !isGen2ChipErrorCode(error) && continueLoop);     // Loop on tag bursts

        continueLoop = continueCheckTimeout();
        count--;
    } while((error != ERR_NONE) && !isGen2ChipErrorCode(error) && (count) && continueLoop); // Give them 7 possible trials before giving up

    return error;
}
/*------------------------------------------------------------------------- */
void doSaveTuningTable(uint8_t p)
{
    if(saveTuningTable)
    {
        saveTuningTable = false;
        saveTuningTableToFlash(p);
    }
    if(saveChannelList)
    {
        for (uint32_t i=0 ; i<FW_NB_ANTENNA ; i++) {
            if ((saveChannelList & (1<<i)) != 0x00) {
                saveChannelListToFlash(i, &gChannelListTuning[i].channelList);
            }
        }
        saveChannelList = 0x00;
    }    
}

/*------------------------------------------------------------------------- */
void initCommands(void)
{
    randomizeFrequencies(profile);      // randomize frequency list, required for FCC

    gCurrentSession = 0;
    cyclicInventory = false;
    setInventoryFromHost(false, false);
    continuousModulation = CONTINUOUS_MODULATION_MODE_OFF;
    fastInventory = false;
    readTIDinInventoryRound = false;
    targetDepletionMode = false;
    autoAckMode = true;
    rssiMode = 0x06;    // rssi at 2nd byte
    numSelects = 0;
#if GB29768
    numSorts = 0;
    gGbtAntiCollision.endThreshold = 2;
    gGbtAntiCollision.ccnThreshold = 3;
    gGbtAntiCollision.cinThreshold = 4;
#endif
    
    //
    cycleData = &cycleDataTmp;
    cycleData->tagList = tagListTmp;        
    cycleData->tagListSizeMax = MAXTAG;
    cycleData->tagListSize = 0;

#ifdef USE_INVENTORY_EXT
    //
    slotInfoData = &slotInfoDataTmp;
    slotInfoData->slotInfoListSize = 0;
#endif
    
    //
    memset(&cycleData->statistics, 0x00, sizeof(STUHFL_T_Inventory_Statistics));
    cycleData->statistics.Q = 4;
    
    gGen2AdaptiveQSettings.isEnabled = true;
    gGen2AdaptiveQSettings.options = 0;
    for(int i = 0; i < NUM_C_VALUES; i++){
        gGen2AdaptiveQSettings.c2[i] = 35;
        gGen2AdaptiveQSettings.c1[i] = 15;
    }
    gGen2AdaptiveQSettings.qfp = 4.0;
    gGen2AdaptiveQSettings.minQ = 0;
    gGen2AdaptiveQSettings.maxQ = 15;
    
    // rssi measures 2x 4 bits (I,Q), summed up this is a value of 0...30
    if(profile == PROFILE_JAPAN)
    {
        skipLBTcheck = 0;
        listeningTime = 5;
        maxSendingTime = 400;
        rssiLogThreshold = 4;   // -74 dBm (value -78)
    }
    else    // PROFILE_EUROPE / PROFILE_USA / PROFILE_CHINA / PROFILE_CHINA2
    {
        skipLBTcheck = 1;
        listeningTime = 1;
        maxSendingTime = 400;
        rssiLogThreshold = 31;  // -47 dBm (value -78), LBT check will always pass
    }
    readerPowerDownMode = POWER_NORMAL;


#ifdef ANTENNA_SWITCH
    if (    (usedAntenna != ANTENNA_2)
#if ELANCE
        &&  (usedAntenna != ANTENNA_3)
        &&  (usedAntenna != ANTENNA_4)
#endif
    ) {
        usedAntenna = ANTENNA_1;    // Force value to handle ANTENNA_ALT
    }
    switchAntenna(usedAntenna);
#endif

#ifdef TUNER
    resetTunedFrequencies();
#endif

    for (uint32_t i=0 ; i<FW_NB_ANTENNA ; i++) {
        gChannelListTuning[i].freqIndex = 0;
        gChannelListTuning[i].frequencies.numFreqs = 0;
        gChannelListTuning[i].channelList.antenna = i;
        gChannelListTuning[i].channelList.persistent = false;
        gChannelListTuning[i].channelList.nFrequencies = 0;
        gChannelListTuning[i].channelList.currentChannelListIdx = 0;
    }

    powerUpReader();
    st25RU3993AntennaPower(0);
    powerDownReader();
}

/*------------------------------------------------------------------------- */
uint8_t doCyclicInventory(void)
{
    //cyclicInventory = true;
    if(cyclicInventory)
    {
#if GB29768
        if(gCurrentSession == SESSION_GB29768)
        {
            inventoryGb29768();
        }
        else
#endif
        {
            inventoryGen2();
        }
        
#ifdef ANTENNA_SWITCH
        if(alternateAntennaEnable)
        {
            if(alternateAntennaRoundCounter >= alternateAntennaInterval)
            {
                alternateAntennaRoundCounter = 0;
                
                // cycle
                usedAntenna++;
                usedAntenna %= FW_NB_ANTENNA;
                switchAntenna(usedAntenna);
            }
        }
#endif

#ifdef TUNER
        // Tuned LED
        if(tunedLedRoundCounter >= tunedLedInterval)
        {
            tunedLedRoundCounter = 0;
            computeTunedLed();
        }
#endif
        return 1;
    }
    return 0;
}
/*------------------------------------------------------------------------- */
void doContinuousModulation(void)
{
    if(continuousModulation != CONTINUOUS_MODULATION_MODE_OFF)
    {
        uint16_t rxbits = 0;

        powerUpReader();
        st25RU3993AntennaPower(1);
        switch(continuousModulation) {
            case CONTINUOUS_MODULATION_MODE_PSEUDO_RANDOM:
                // pseudo random continuous modulation
                pseudoRandomContinuousModulation();
                break;
            case CONTINUOUS_MODULATION_MODE_ETSI:
                // ETSI mode (10-50 ms pulse, 1-10 ms pause)
                etsiModeContinuousModulation();
                break;
            case CONTINUOUS_MODULATION_MODE_STATIC:
                // static modulation
                st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_NAK, 0, 0, 0, &rxbits, 0, 0, 1);
                break;
        }
        st25RU3993ClrResponse();
    }
}



/*
 ******************************************************************************
 * GLOBAL/LOCAL FUNCTIONS
 ******************************************************************************
 */

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


STUHFL_T_RET_CODE setInventoryFromHost(bool fromHost, bool slotStatistics)
{
#if STUHFL_HOST_COMMUNICATION
    gInventoryFromHost = fromHost;
#endif
#ifdef USE_INVENTORY_EXT
    gSlotStatistics = slotStatistics;
#endif
    return ERR_NONE;
}

STUHFL_T_RET_CODE GetBoardVersion(STUHFL_T_Version *swVersion, STUHFL_T_Version *hwVersion)
{
    swVersion->major = FW_VERSION_MAJOR;
    swVersion->minor = FW_VERSION_MINOR;
    swVersion->micro = FW_VERSION_MICRO;
    swVersion->build = FW_VERSION_BUILD;
    
    hwVersion->major = HW_VERSION_MAJOR;
    hwVersion->minor = HW_VERSION_MINOR;
    hwVersion->micro = HW_VERSION_MICRO;
    hwVersion->build = HW_VERSION_NANO;    
    return ERR_NONE;
}

/** FW version which will be reported to host */
const char fwInfo[] = "STUHFL SDK Evaluation FW @ STM32L4";
const char hwInfo[] = "ST25RU3993-EVAL Board";
STUHFL_T_RET_CODE GetBoardInfo(STUHFL_T_Version_Info *swVersionInfo, STUHFL_T_Version_Info *hwVersionInfo)
{
    memset(swVersionInfo, 0, sizeof(STUHFL_T_Version_Info));
    memset(hwVersionInfo, 0, sizeof(STUHFL_T_Version_Info));
    
    swVersionInfo->infoLength = sizeof(fwInfo) / sizeof(fwInfo[0]);//34;
	hwVersionInfo->infoLength = sizeof(hwInfo) / sizeof(hwInfo[0]);//21;
    memcpy(swVersionInfo->info, fwInfo, swVersionInfo->infoLength);
    memcpy(hwVersionInfo->info, hwInfo, hwVersionInfo->infoLength);
    return ERR_NONE;
}

void Reboot()
{
	HAL_NVIC_SystemReset();
    while(1);
}

void EnterBootloader()
{
    bootloaderEnterAndReset();
}

// **************************************************************************
// Setter Register
// **************************************************************************
STUHFL_T_RET_CODE SetRegister(STUHFL_T_ST25RU3993_Register *reg)
{
    if (((reg->addr >= 0x40) && (reg->addr <= 0x7F)) || (reg->addr >= 0xAC)) {
        // between 0x40-0x7F or 0xAC-0xFF
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }
    
    powerUpReader();    
    if(reg->addr < 0x80){   
        st25RU3993SingleWrite(reg->addr, reg->data);
    }
    else{   
        st25RU3993SingleCommand(reg->addr);
    }
    handleLEDs();

    StoreInternalDataForTest(TEST_ID_SETREGISTER, ERR_NONE, reg);

    return ERR_NONE;
}
STUHFL_T_RET_CODE SetRegisterMultiple(STUHFL_T_ST25RU3993_Register **reg, uint8_t numReg)
{
    STUHFL_T_RET_CODE ret = ERR_NONE;
    for(int i = 0; i < numReg; i++){
        ret |= SetRegister((*reg)+i);
    }
    return ret;
}

// **************************************************************************
// Getter Register
// **************************************************************************
STUHFL_T_RET_CODE GetRegister(STUHFL_T_ST25RU3993_Register *reg)
{
    if (((reg->addr >= 0x40) && (reg->addr <= 0x7F)) || (reg->addr >= 0xAC)) {
        // between 0x40-0x7F or 0xAC-0xFF
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    powerUpReader();
    reg->data = st25RU3993SingleRead(reg->addr);
    handleLEDs();

    StoreTestData(TEST_ID_GETREGISTER, sizeof(STUHFL_T_ST25RU3993_Register), (uint8_t *)reg);

    return ERR_NONE;
}
STUHFL_T_RET_CODE GetRegisterMultiple(uint8_t numReg, STUHFL_T_ST25RU3993_Register **reg)
{
    STUHFL_T_RET_CODE ret = ERR_NONE;
    for(int i = 0; i < numReg; i++){
        ret |= GetRegister((*reg)+i);
    }
    return ret;
}

// **************************************************************************
// Setter RwdCfg
// **************************************************************************
STUHFL_T_RET_CODE SetRwdCfg(STUHFL_T_ST25RU3993_RwdConfig *rwdCfg)
{
    switch(rwdCfg->id)
    {
        case STUHFL_RWD_CFG_ID_PWR_DOWN_MODE:
        {
            // before changing configuration we have to power up
            powerUpReader();
            readerPowerDownMode = rwdCfg->value;
            powerDownReader();
            break;
        }
        default: 
            StoreInternalDataForTest(TEST_ID_SETRWDCFG, (STUHFL_T_RET_CODE)ERR_PARAM, rwdCfg);
            return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    StoreInternalDataForTest(TEST_ID_SETRWDCFG, ERR_NONE, rwdCfg);
    
    return ERR_NONE;
}

// **************************************************************************
// Getter RwdCfg
// **************************************************************************
STUHFL_T_RET_CODE GetRwdCfg(STUHFL_T_ST25RU3993_RwdConfig *rwdCfg)
{
    switch(rwdCfg->id)
    {
        case STUHFL_RWD_CFG_ID_PWR_DOWN_MODE:
        {
            rwdCfg->value = readerPowerDownMode;
            break;
        }
        case STUHFL_RWD_CFG_ID_EXTVCO:
        {
#ifdef EXTVCO 
            rwdCfg->value = 1;
#else 
            rwdCfg->value = 0; 
#endif
            break;
        }
        case STUHFL_RWD_CFG_ID_PA:
        {
            rwdCfg->value = externalPA;
            break;
        }
        case STUHFL_RWD_CFG_ID_INP:
        {
#ifdef BALANCEDINP 
            rwdCfg->value = 0;
#else   // defined SINGLEINP
            rwdCfg->value = 1;
#endif
            break;
        }
        case STUHFL_RWD_CFG_ID_ANTENNA_SWITCH:
        {
#ifdef ANTENNA_SWITCH
            rwdCfg->value = 1;
#else 
            rwdCfg->value = 0; 
#endif
            break;
        }
        case STUHFL_RWD_CFG_ID_TUNER:
        {
            rwdCfg->value = 
#ifdef TUNER
#ifdef TUNER_CONFIG_CIN
            (TUNER_CIN)|
#endif
#ifdef TUNER_CONFIG_CLEN
            (TUNER_CLEN)|
#endif
#ifdef TUNER_CONFIG_COUT
            (TUNER_COUT)|
#endif
            0;
#else
            0;
#endif
            break;
        }
        case STUHFL_RWD_CFG_ID_HARDWARE_ID_NUM:
        {
            rwdCfg->value = HARDWARE_ID_NUM;
            break;
        }
        default: 
            StoreTestData(TEST_ID_GETRWDCFG, 0, (uint8_t *)rwdCfg);    // Error: Reset Tests data
            return (uint8_t)ERR_PARAM;
    }
    
    StoreTestData(TEST_ID_GETRWDCFG, sizeof(STUHFL_T_ST25RU3993_RwdConfig), (uint8_t *)rwdCfg);
    
    return ERR_NONE;
}



// **************************************************************************
// Setter AntennaPower
// **************************************************************************
STUHFL_T_RET_CODE SetAntennaPower(STUHFL_T_ST25RU3993_Antenna_Power *antPwr)
{
    if (antPwr->frequency > FREQUENCY_MAX_VALUE) {
        return (uint8_t)ERR_PARAM;
    }

    STUHFL_T_RET_CODE ret = (STUHFL_T_RET_CODE)ERR_IO;
    switch(antPwr->mode){
        case ANTENNA_POWER_MODE_OFF:
        {
            powerUpReader();
            st25RU3993AntennaPower(0);
            powerDownReader();
            setTuningIndexFromFreq(antPwr->frequency);

            pwrMode = antPwr->mode;
            ret = ERR_NONE;
            break;
        }
        case ANTENNA_POWER_MODE_ON:
        {
            uint8_t pllOk;
            uint8_t counter;
            setTuningIndexFromFreq(antPwr->frequency);

            powerUpReader();
            counter = 0;
            do{
                pllOk = st25RU3993SetBaseFrequency(antPwr->frequency);
                counter++;
            } while(!pllOk && counter<15);
            if(pllOk){
                st25RU3993AntennaPower(1);
                maxSendingLimit = antPwr->timeout;
                maxSendingLimitTimedOut = 0;
                if(maxSendingLimit > 0){
                    rtcTimerReset();
                    while(continueCheckTimeout()) {
                        __NOP();
                    }
                    st25RU3993AntennaPower(0);
                }

                pwrMode = antPwr->mode;
                ret = ERR_NONE;
            }
            if (!pllOk || (maxSendingLimit > 0)) {  // power down reader only if unlimited power is not enabled
                powerDownReader();
            }
            break;
        }
        default:
            StoreInternalDataForTest(TEST_ID_SETANTENNAPOWER, (STUHFL_T_RET_CODE)ERR_PARAM, antPwr);
            return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    StoreInternalDataForTest(TEST_ID_SETANTENNAPOWER, ret, antPwr);
    
    return ret;
}

// **************************************************************************
// Setter AntennaPower
// **************************************************************************
STUHFL_T_RET_CODE GetAntennaPower(STUHFL_T_ST25RU3993_Antenna_Power *antPwr)
{
    antPwr->mode = pwrMode;
    antPwr->timeout = maxSendingLimit;
    antPwr->frequency = st25RU3993GetBaseFrequency();

    StoreTestData(TEST_ID_GETANTENNAPOWER, sizeof(STUHFL_T_ST25RU3993_Antenna_Power), (uint8_t *)antPwr);

    return ERR_NONE;
}

// **************************************************************************
// Setter Frequency
// **************************************************************************
STUHFL_T_RET_CODE SetChannelList(STUHFL_T_ST25RU3993_ChannelList *channelList)
{
    if(   (channelList->nFrequencies > MAX_FREQUENCY) 
       || (channelList->currentChannelListIdx >= MAX_FREQUENCY)
       || (channelList->antenna >= FW_NB_ANTENNA)
    ) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }
    for(uint32_t i = 0; i<channelList->nFrequencies; i++) {
        if (channelList->item[i].frequency > FREQUENCY_MAX_VALUE) {
            return (STUHFL_T_RET_CODE)ERR_PARAM;
        }
    }

    if (   (channelList->currentChannelListIdx > channelList->nFrequencies)
        || ((channelList->currentChannelListIdx == channelList->nFrequencies) && (channelList->nFrequencies != 0))) {
        return (STUHFL_T_RET_CODE)ERR_REQUEST;
    }

    memcpy(&gChannelListTuning[ channelList->antenna ], channelList, sizeof(STUHFL_T_ST25RU3993_ChannelList));

    // Update frequencies list
    gChannelListTuning[ channelList->antenna ].frequencies.numFreqs = channelList->nFrequencies;
    gChannelListTuning[ channelList->antenna ].freqIndex = channelList->currentChannelListIdx;
    for(uint32_t i = 0; i < channelList->nFrequencies ; i++){
        updateTuningCaps(false, channelList->antenna, i, &channelList->item[i].caps, true);

        gChannelListTuning[ channelList->antenna ].frequencies.freq[i] = channelList->item[i].frequency;
        gChannelListTuning[ channelList->antenna ].tunedIQ[i] = 0xFFFF;
    }

#ifdef TUNER
    resetTunedFrequencies();
#endif

    //
    if(channelList->persistent){
        saveChannelList |= (1<<channelList->antenna);
    }

    StoreInternalDataForTest(TEST_ID_SETCHANNELLIST, ERR_NONE, channelList);

    return ERR_NONE;
}

STUHFL_T_RET_CODE SetFreqProfile(STUHFL_T_ST25RU3993_Freq_Profile *freqProfile)
{
    if ((freqProfile->profile >= NUM_SAVED_PROFILES) && (freqProfile->profile != PROFILE_NEWTUNING)) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;    // Out of range profile
    }

    // change current freq profile
    if(freqProfile->profile != profile)
    {
        profile = freqProfile->profile;

        randomizeFrequencies(freqProfile->profile);
#ifdef TUNER
        resetTunedFrequencies();    // Changed profile, reset list of tuned freqs
#endif
    }     

    StoreInternalDataForTest(TEST_ID_SETFREQPROFILE, ERR_NONE, freqProfile);

    return ERR_NONE;
}
STUHFL_T_RET_CODE SetFreqProfileAdd2Custom(STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom *freqProfileAdd)
{
    if(profile != PROFILE_CUSTOM){
        return (STUHFL_T_RET_CODE)ERR_REQUEST;    // profile is not PROFILE_CUSTOM
    }

    if(freqProfileAdd->frequency > FREQUENCY_MAX_VALUE) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;    // Out of range frequency
    }

    // append to custom profile
    if(freqProfileAdd->clearList) {
        frequencies[PROFILE_CUSTOM].numFreqs = 0;
        tuningTable[PROFILE_CUSTOM].tableSize = 0;
    }
    
    frequencies[PROFILE_CUSTOM].numFreqs++;
    if(frequencies[PROFILE_CUSTOM].numFreqs > MAX_FREQUENCY) {
        frequencies[PROFILE_CUSTOM].numFreqs = MAX_FREQUENCY;
    }
    frequencies[PROFILE_CUSTOM].freq[(frequencies[PROFILE_CUSTOM].numFreqs-1)] = freqProfileAdd->frequency;

    // Update tuning table with new freq
    tuningTable[PROFILE_CUSTOM].tableSize++;
    if(tuningTable[PROFILE_CUSTOM].tableSize > MAX_FREQUENCY) {
        tuningTable[PROFILE_CUSTOM].tableSize = MAX_FREQUENCY;
    }
    tuningTable[PROFILE_CUSTOM].freq[(tuningTable[PROFILE_CUSTOM].tableSize-1)] = freqProfileAdd->frequency;
    
    // Ensures all freqs in tuning table for CUSTOM profile are sorted
    bool permutedFreqs;
    do
    {
        permutedFreqs = false;
        for (uint32_t i=0 ; i<tuningTable[PROFILE_CUSTOM].tableSize-1 ; i++) {
            if (tuningTable[PROFILE_CUSTOM].freq[i] > tuningTable[PROFILE_CUSTOM].freq[i+1]) {
                // Permuting freqs
                uint32_t tmpFreq = tuningTable[PROFILE_CUSTOM].freq[i];
                tuningTable[PROFILE_CUSTOM].freq[i] = tuningTable[PROFILE_CUSTOM].freq[i+1];
                tuningTable[PROFILE_CUSTOM].freq[i+1] = tmpFreq;
                
                // Permuting tuning values
                for (uint32_t j=0 ; j<FW_NB_ANTENNA ; j++) {
                    tuningTable[PROFILE_CUSTOM].cin[j][i]   = tuningTable[PROFILE_CUSTOM].cin[j][i+1];
                    tuningTable[PROFILE_CUSTOM].clen[j][i]  = tuningTable[PROFILE_CUSTOM].clen[j][i+1];
                    tuningTable[PROFILE_CUSTOM].cout[j][i]  = tuningTable[PROFILE_CUSTOM].cout[j][i+1];
                    tuningTable[PROFILE_CUSTOM].tunedIQ[j][i] = tuningTable[PROFILE_CUSTOM].tunedIQ[j][i+1];
                }
                
                permutedFreqs = true;
            }
        }
    } while(permutedFreqs);

#ifdef TUNER
    // Added custom freq, reset list of tuned freqs
    resetTunedFrequencies();
#endif

    StoreInternalDataForTest(TEST_ID_SETFREQPROFILEADD2CUSTOM, ERR_NONE, freqProfileAdd);

    return ERR_NONE;
}
STUHFL_T_RET_CODE SetFreqHop(STUHFL_T_ST25RU3993_Freq_Hop *freqHop)
{
    // set parameters for frequency hopping
    maxSendingTime  =  freqHop->maxSendingTime;
    if(maxSendingTime < 40)
    {   maxSendingTime = 40;
    }

    StoreInternalDataForTest(TEST_ID_SETFREQHOP, ERR_NONE, freqHop);

    return ERR_NONE;
}
STUHFL_T_RET_CODE SetFreqLBT(STUHFL_T_ST25RU3993_Freq_LBT *freqLBT)
{
    // set parameters for frequency hopping
    listeningTime  =  freqLBT->listeningTime;
    idleTime  =  freqLBT->idleTime;
    rssiLogThreshold =  freqLBT->rssiLogThreshold;
    skipLBTcheck =  freqLBT->skipLBTcheck;

    StoreInternalDataForTest(TEST_ID_SETFREQLBT, ERR_NONE, freqLBT);

    return ERR_NONE;
}
STUHFL_T_RET_CODE SetFreqContMod(STUHFL_T_ST25RU3993_Freq_ContMod *freqContMod)
{
    powerUpReader();
    if(freqContMod->enableContMod) // enable
    {
        uint8_t pllOk;
        uint8_t counter;
        backupRxnorespwait = st25RU3993SingleRead(ST25RU3993_REG_RXNORESPONSEWAITTIME);
        backupRxwait       = st25RU3993SingleRead(ST25RU3993_REG_RXWAITTIME);
        st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, 0);
        st25RU3993SingleWrite(ST25RU3993_REG_RXWAITTIME, 0);
        counter = 0;
        do
        {
            pllOk = st25RU3993SetBaseFrequency(freqContMod->frequency);
            counter++;
        } while(!pllOk && counter<15);
        setTuningIndexFromFreq(freqContMod->frequency);
        if(pllOk)
        {
            checkAndOpenSession(SESSION_GEN2);

            maxSendingLimit = freqContMod->maxSendingTime;
            maxSendingLimitTimedOut = 0;
            st25RU3993AntennaPower(1);
            delay_ms(1);
            rtcTimerReset();
            if(maxSendingLimit == 0)    // unlimited
            {
                switch (freqContMod->modulationMode) {
                    case CONTINUOUS_MODULATION_MODE_STATIC:
                    case CONTINUOUS_MODULATION_MODE_PSEUDO_RANDOM:
                    case CONTINUOUS_MODULATION_MODE_ETSI:
                        continuousModulation = freqContMod->modulationMode;
                        break;
                }
            }
            else
            {
                uint16_t rxbits = 0;
                do
                {
                    switch(freqContMod->modulationMode) {
                        case CONTINUOUS_MODULATION_MODE_STATIC:
                            // static modulation
                            st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_NAK, 0, 0, 0, &rxbits, 0, 0, 1);
                            break;
                        case CONTINUOUS_MODULATION_MODE_PSEUDO_RANDOM:
                            // pseudo random continuous modulation
                            pseudoRandomContinuousModulation();
                            break;
                        case CONTINUOUS_MODULATION_MODE_ETSI:
                            // ETSI mode (10-50 ms pulse, 1-10 ms pause)
                            etsiModeContinuousModulation();
                            break;
                    }
                    st25RU3993ClrResponse();
                } while(continueCheckTimeout());
                
                st25RU3993AntennaPower(0);
                st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, backupRxnorespwait);
                st25RU3993SingleWrite(ST25RU3993_REG_RXWAITTIME, backupRxwait);
            }
        }
        else
        {
            powerDownReader();
            return (STUHFL_T_RET_CODE)ERR_IO;
        }
    }
    else if(continuousModulation != CONTINUOUS_MODULATION_MODE_OFF)
    {
        continuousModulation = CONTINUOUS_MODULATION_MODE_OFF;

        st25RU3993AntennaPower(0);
        st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, backupRxnorespwait);
        st25RU3993SingleWrite(ST25RU3993_REG_RXWAITTIME, backupRxwait);
    }
    powerDownReader();

    return ERR_NONE;
}

// **************************************************************************
// Getter Frequency
// **************************************************************************
STUHFL_T_RET_CODE GetChannelList(STUHFL_T_ST25RU3993_ChannelList *channelList)
{
    STUHFL_T_RET_CODE ret = ERR_NONE;

    if (channelList->antenna >= FW_NB_ANTENNA) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    if(channelList->persistent){
        loadChannelListFromFlash(channelList->antenna, channelList);
    }
    else {
        memcpy(channelList, &gChannelListTuning[ channelList->antenna ].channelList, sizeof(STUHFL_T_ST25RU3993_ChannelList));
    }

    StoreTestData(TEST_ID_GETCHANNELLIST, (ret == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_ChannelList) : 0, (uint8_t *)channelList);
    StoreTestData(TEST_ID_GETCHANNELLIST_IQS, (ret == ERR_NONE) ? sizeof(STUHFL_T_Test_CHANNELLIST_IQS) : 0, (uint8_t *)gChannelListTuning[ channelList->antenna ].tunedIQ);

    return ret;
}

STUHFL_T_RET_CODE GetFreqRSSI(STUHFL_T_ST25RU3993_Freq_Rssi *freqRSSI)
{
    STUHFL_T_RET_CODE   err = ERR_NONE;

    freqRSSI->rssiLogI = 0;
    freqRSSI->rssiLogQ = 0;

    if(freqRSSI->frequency > FREQUENCY_MAX_VALUE) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;    // Out of range frequency
    }

    // get RSSI
    if(externalPA)
    {
        uint8_t regValRFOutput;
        uint8_t pllOk;

        powerUpReader();
        regValRFOutput = st25RU3993SingleRead(ST25RU3993_REG_RFOUTPUTCONTROL);
        st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, regValRFOutput & 0xC3);
        pllOk = st25RU3993SetBaseFrequency(freqRSSI->frequency);
        setTuningIndexFromFreq(freqRSSI->frequency);
        if(pllOk)
        {
            st25RU3993GetRSSI(listeningTime, &freqRSSI->rssiLogI, &freqRSSI->rssiLogQ);
            st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, regValRFOutput);
            err = ERR_NONE;
        }
        else
        {
            st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, regValRFOutput);
            err = (STUHFL_T_RET_CODE)ERR_IO;
        }
        powerDownReader();
    }
    else
    {
        err = (STUHFL_T_RET_CODE)ERR_REQUEST;
    }

    StoreTestData(TEST_ID_GETFREQRSSI, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_Freq_Rssi) : 0, (uint8_t *)freqRSSI);

    return err;
}
STUHFL_T_RET_CODE GetFreqReflectedPower(STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info *freqReflectedPower)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    uint8_t pllOk = false;

    // get reflected Power
    freqReflectedPower->reflectedI = 0xFF;
    freqReflectedPower->reflectedQ = 0xFF;

    if(freqReflectedPower->frequency > FREQUENCY_MAX_VALUE) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;    // Out of range frequency
    }

    powerUpReader();
    for(uint8_t tries=0;(tries<5) && (!pllOk);tries++) {
        pllOk = st25RU3993SetBaseFrequency(freqReflectedPower->frequency);
    }
    setTuningIndexFromFreq(freqReflectedPower->frequency);
    if(pllOk)
    {
        uint16_t iqTogetherReflected;
#ifdef TUNER
        if(freqReflectedPower->applyTunerSetting){        
            applyTunerSettingForFreq(freqReflectedPower->frequency);
        }
        else{
            // Use default tuning..
            tunerSetTuning(15,15,15);
        }
#endif
        st25RU3993AntennaPower(1);
        //delay_us(5000); // more Settling Time for the Tuning
        iqTogetherReflected = st25RU3993GetReflectedPower();
        freqReflectedPower->reflectedI = (int8_t)(iqTogetherReflected & 0xFF);         // I
        freqReflectedPower->reflectedQ = (int8_t)((iqTogetherReflected >> 8)&0xFF);    // Q
        err = ERR_NONE;
    }
    else
    {
        err = (STUHFL_T_RET_CODE)ERR_IO;
    }
    powerDownReader();

    StoreTestData(TEST_ID_GETFREQREFLECTEDPOWER, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info) : 0, (uint8_t *)freqReflectedPower);

    return err;
}
STUHFL_T_RET_CODE GetFreqProfileInfo(STUHFL_T_ST25RU3993_Freq_Profile_Info *freqProfileInfo)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    
    uint8_t i;
    uint32_t guiMinFreq = FREQUENCY_MAX_VALUE;
    uint32_t guiMaxFreq = 0;

    freq_t  *pFrequencies = (profile == PROFILE_NEWTUNING) ? &gChannelListTuning[usedAntenna].frequencies : &frequencies[profile];

    // get frequency list parameters
    for(i = 0; i < pFrequencies->numFreqs; i++)
    {
        guiMinFreq = (pFrequencies->freq[i] < guiMinFreq) ? pFrequencies->freq[i] : guiMinFreq;
        guiMaxFreq = (pFrequencies->freq[i] > guiMaxFreq) ? pFrequencies->freq[i] : guiMaxFreq;
    }

    freqProfileInfo->profile = profile;
    freqProfileInfo->minFrequency = (pFrequencies->numFreqs == 0) ? 0 : guiMinFreq;
    freqProfileInfo->maxFrequency = (pFrequencies->numFreqs == 0) ? 0 : guiMaxFreq;
    freqProfileInfo->numFrequencies = pFrequencies->numFreqs;    

    StoreTestData(TEST_ID_GETFREQPROFILEINFO, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_Freq_Profile_Info) : 0, (uint8_t *)freqProfileInfo);

    return err;
}
STUHFL_T_RET_CODE GetFreqHop(STUHFL_T_ST25RU3993_Freq_Hop *freqHop)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    freqHop->maxSendingTime = maxSendingTime;

    StoreTestData(TEST_ID_GETFREQHOP, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_Freq_Hop) : 0, (uint8_t *)freqHop);

    return err;
}
STUHFL_T_RET_CODE GetFreqLBT(STUHFL_T_ST25RU3993_Freq_LBT *freqLBT)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    freqLBT->listeningTime = listeningTime;
    freqLBT->idleTime = idleTime;
    freqLBT->rssiLogThreshold = rssiLogThreshold;
    freqLBT->skipLBTcheck = skipLBTcheck;    

    StoreTestData(TEST_ID_GETFREQLBT, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_Freq_LBT) : 0, (uint8_t *)freqLBT);

    return err;
}

// **************************************************************************
// Setter SW Cfg
// **************************************************************************
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGen2Timings(STUHFL_T_Gen2_Timings *gen2Timings)
{
    gen2Configuration.T4Min       = gen2Timings->T4Min;
    StoreInternalDataForTest(TEST_ID_SETGEN2TIMINGS, ERR_NONE, gen2Timings);
    return ERR_NONE;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGen2ProtocolCfg(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg *gen2ProtocolCfg)
{
    if (   (gen2ProtocolCfg->tari > GEN2_TARI_25_00)
        || (gen2ProtocolCfg->coding > GEN2_COD_MILLER8)
        || (gen2ProtocolCfg->blf > GEN2_LF_640)) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    gen2Configuration.blf       = gen2ProtocolCfg->blf;
    gen2Configuration.coding    = gen2ProtocolCfg->coding;
    gen2Configuration.trext     = gen2ProtocolCfg->trext;
    gen2Configuration.tari      = gen2ProtocolCfg->tari;
    //
    gGen2AdaptiveQSettings.qfp  = (float)cycleData->statistics.Q;
    
    StoreInternalDataForTest(TEST_ID_SETGEN2PROTOCOLCFG, ERR_NONE, gen2ProtocolCfg);

    powerUpReader();
    checkAndOpenSession(SESSION_GEN2);
    powerDownReader();    
    return ERR_NONE;
}
#if GB29768
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGb29768ProtocolCfg(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg *gb29768ProtocolCfg)
{
    STUHFL_T_RET_CODE err = ERR_NONE;

    if (   (gb29768ProtocolCfg->blf > GB29768_BLF_640)
        || (gb29768ProtocolCfg->coding > GB29768_COD_MILLER8)
        || (gb29768ProtocolCfg->tc > GB29768_TC_12_5)) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    gb29768Config.trext     = gb29768ProtocolCfg->trext;
    gb29768Config.blf       = gb29768ProtocolCfg->blf;
    gb29768Config.coding    = gb29768ProtocolCfg->coding;
    gb29768Config.tc        = gb29768ProtocolCfg->tc;
    StoreInternalDataForTest(TEST_ID_SETGB29768PROTOCOLCFG, ERR_NONE, gb29768ProtocolCfg);

    powerUpReader();
    checkAndOpenSession(SESSION_GB29768);
    powerDownReader();
    return err;
}
#endif
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTxRxCfg(STUHFL_T_ST25RU3993_TxRx_Cfg *txRxCfg)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    st25RU3993SetSensitivity(txRxCfg->rxSensitivity);
    st25RU3993SetTxOutputLevel(txRxCfg->txOutputLevel);

#ifdef ANTENNA_SWITCH
    switch(txRxCfg->usedAntenna) {
        case ANTENNA_1:
        case ANTENNA_2:
#if ELANCE // only ELANCE boards have 4 Antennas
        case ANTENNA_3:
        case ANTENNA_4:
#endif
            usedAntenna = txRxCfg->usedAntenna;
            alternateAntennaEnable = 0;
            alternateAntennaInterval = 1;
            break;
        case ANTENNA_ALT:
            usedAntenna = ANTENNA_1;
            alternateAntennaEnable = 1;
            alternateAntennaInterval = txRxCfg->alternateAntennaInterval;
            break;
        default:
            return (STUHFL_T_RET_CODE)ERR_PARAM;

    }
    switchAntenna(usedAntenna);

#if ELANCE // only ELANCE boards have VLNA configuration
    vlna = txRxCfg->vlna;
    if (vlna & 0x01) { SET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin); } else { RESET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin); }
    if (vlna & 0x02) { SET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin); } else { RESET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin); }   
#endif
#endif  // ANTENNA_SWITCH

    StoreInternalDataForTest(TEST_ID_SETTXRXCFG, err, txRxCfg);

    return err;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetPA_Cfg(STUHFL_T_ST25RU3993_PA_Cfg *paCfg)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    uint8_t myBuf[4];
    
    externalPA = paCfg->useExternal;
    if(externalPA)
    {
        // External PA
#if EVAL
        RESET_GPIO(GPIO_PA_SW_V1_PORT,GPIO_PA_SW_V1_PIN);
        SET_GPIO(GPIO_PA_SW_V2_PORT,GPIO_PA_SW_V2_PIN);
#elif ELANCE
        // No PA Switches on ELANCE
#elif JIGEN
        // No PA Switches on JIGEN
#elif DISCOVERY
        // No PA Switches on DISCOVERY 
#else
        #error "Undefined Code for selected board"
#endif

        SET_INTPA_LED_OFF();
        SET_EXTPA_LED_ON();

        // REGULATOR and PA Bias Register 0x0B
        // pa_bias<1-0> | rvs_rf<2-1> | rvs<2-0>
        //     0 0      |    0 1 1    |  0 1 1   = 0x1B
        st25RU3993SingleWrite(ST25RU3993_REG_REGULATORCONTROL, 0x1B);

        // RF Output and LO Control Register 0x0C
        // eTX<7-5> | - | eTx<3-0>
        //   1 0 0  | 0 | 0 0 1 0  = 0x82
        st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, 0x82);

        // Modulator Control Register 1 0x13
        //  - | main_mod | aux_mod | - | e_lpf | ask_rate<1-0>
        //  0 |     0    |    1    |0 0|   0   |     0 0       = 0x20
        myBuf[0] = 0x20;

        // Modulator Control Register 2 0x14
        // ook_ask | pr_ask | del_len<5-0>
        //    1    |    1   | 0 1 1 1 0 1  = 0xDD
        myBuf[1] = 0xDD;

        // Modulator Control Register 3 0x15
        // trfon<1-0> | lin_mode | TX_lev<4-0>
        //     0 0    |     0    |  0 0 0 1 0  = 0x02 for EU/USA
        //     0 0    |     0    |  0 0 1 1 1  = 0x07 for JAPAN
        if(profile == PROFILE_JAPAN)
        {
// TODO: how here to identify profile JAPAN with ChannelList mechanism ?
            myBuf[2] = 0x07;
        }
        else
        {
            myBuf[2] = 0x02;
        }
        st25RU3993ContinuousWrite(ST25RU3993_REG_MODULATORCONTROL1, myBuf, 3);
    }
    else
    {
        // Internal PA
#if EVAL
        SET_GPIO(GPIO_PA_SW_V1_PORT,GPIO_PA_SW_V1_PIN);
        RESET_GPIO(GPIO_PA_SW_V2_PORT,GPIO_PA_SW_V2_PIN);
#elif ELANCE
        // No PA Switches on ELANCE
#elif JIGEN
        // No PA Switches on JIGEN
#elif DISCOVERY
        // No PA Switches on DISCOVERY
#else
        #error "Undefined Code for selected board"
#endif

        SET_INTPA_LED_ON();
        SET_EXTPA_LED_OFF();

        // REGULATOR and PA Bias Register 0x0B
        // pa_bias<1-0> | rvs_rf<2-1> | rvs<2-0>
        //      1 0     |    0 1 1    |  0 1 1   = 0x1B
        st25RU3993SingleWrite(ST25RU3993_REG_REGULATORCONTROL, 0xBF);

        // RF Output and LO Control Register 0x0C
        // eTX<7-5> | - | eTx<3-0>
        //   0 1 1  | 0 | 1 0 0 0  = 0x68
        st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, 0x68);

        // Modulator Control Register 1 0x13
        //  - | main_mod | aux_mod | - | e_lpf | ask_rate<1-0>
        //  0 |     1    |    0    |0 0|   0   |     0 0       = 0x40
        myBuf[0] = 0x40;

        // Modulator Control Register 2 0x14
        // ook_ask | pr_ask | del_len<5-0>
        //    1    |    1   | 0 1 1 1 0 1  = 0xDD
        myBuf[1] = 0xDD;

        // Modulator Control Register 3 0x15
        // trfon<1-0> | lin_mode | TX_lev<4-0>
        //     0 0    |    0     |  0 0 1 1 1  = 0x07
        myBuf[2] = 0x01;
        st25RU3993ContinuousWrite(ST25RU3993_REG_MODULATORCONTROL1, myBuf, 3);
    }

    StoreInternalDataForTest(TEST_ID_SETPA_CFG, err, paCfg);
    
    return err;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGen2InventoryCfg(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg *invGen2Cfg)
{
    if (   (invGen2Cfg->session > GEN2_SESSION_S3)
        || (invGen2Cfg->target > GEN2_TARGET_B)
        || ((invGen2Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD) > TUNING_ALGO_MEDIUM)       // remove FPD bit
        || (invGen2Cfg->startQ > MAXGEN2Q)) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;    // Out of range
    }
       
    fastInventory = invGen2Cfg->fastInv;
    autoAckMode = invGen2Cfg->autoAck;
    readTIDinInventoryRound = invGen2Cfg->readTID;
    cycleData->statistics.Q = invGen2Cfg->startQ;     
    gGen2AdaptiveQSettings.isEnabled     = invGen2Cfg->adaptiveQEnable != 0 ? true : false;                 
    gGen2AdaptiveQSettings.options       = invGen2Cfg->adjustOptions;          
    for(int i = 0; i < NUM_C_VALUES; i++){
        gGen2AdaptiveQSettings.c2[i] = invGen2Cfg->C2[i];     
        gGen2AdaptiveQSettings.c1[i] = invGen2Cfg->C1[i];   
    }
    gGen2AdaptiveQSettings.minQ = (invGen2Cfg->minQ) > 15 ? 15 : invGen2Cfg->minQ;
    gGen2AdaptiveQSettings.maxQ = (invGen2Cfg->maxQ) > 15 ? 15 : invGen2Cfg->maxQ;
    
#ifdef TUNER
    autoTuningInterval  = invGen2Cfg->autoTuningInterval;
    autoTuningLevel     = invGen2Cfg->autoTuningLevel;   
    autoTuningAlgo      = invGen2Cfg->autoTuningAlgo;  
#endif  // TUNER

    gAdaptiveSensitivityEnable   = invGen2Cfg->adaptiveSensitivityEnable;
    gAdaptiveSensitivityInterval = invGen2Cfg->adaptiveSensitivityInterval;   

    gen2Configuration.sel      = invGen2Cfg->sel;
    gen2Configuration.session  = invGen2Cfg->session;
    gen2Configuration.target   = invGen2Cfg->target;
    gen2Configuration.toggleTarget = invGen2Cfg->toggleTarget;
    targetDepletionMode        = invGen2Cfg->targetDepletionMode;

    // and reset autotuning round counter
    autoTuningRoundCounter = 0;

    // apply settings
    powerUpReader();
    checkAndOpenSession(SESSION_GEN2);
    powerDownReader(); 

    StoreInternalDataForTest(TEST_ID_SETGEN2INVENTORYCFG, ERR_NONE, invGen2Cfg);
    
    return ERR_NONE;
}
#if GB29768
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetGb29768InventoryCfg(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg *invGb29768Cfg)
{
    if (   (invGb29768Cfg->condition > GB29768_CONDITION_FLAG0)
        || (invGb29768Cfg->session > GB29768_SESSION_S3)
        || (invGb29768Cfg->target > GB29768_TARGET_1)
        || ((invGb29768Cfg->autoTuningAlgo & ~TUNING_ALGO_ENABLE_FPD) > TUNING_ALGO_MEDIUM)) {         // remove FPD bit
        return (STUHFL_T_RET_CODE)ERR_PARAM;    // Out of range
    }

#ifdef TUNER
    autoTuningInterval = invGb29768Cfg->autoTuningInterval;
    autoTuningLevel = invGb29768Cfg->autoTuningLevel;   
    autoTuningAlgo = invGb29768Cfg->autoTuningAlgo; 
#endif  // TUNER

    gAdaptiveSensitivityEnable = invGb29768Cfg->adaptiveSensitivityEnable;
    gAdaptiveSensitivityInterval = invGb29768Cfg->adaptiveSensitivityInterval;   
    // and reset autotuning round counter
    autoTuningRoundCounter = 0;

    gb29768Config.condition = invGb29768Cfg->condition;
    gb29768Config.session   = invGb29768Cfg->session;
    gb29768Config.target    = invGb29768Cfg->target;
    gb29768Config.toggleTarget = invGb29768Cfg->toggleTarget;
    targetDepletionMode     = invGb29768Cfg->targetDepletionMode;

    gGbtAntiCollision.endThreshold = invGb29768Cfg->endThreshold;
    gGbtAntiCollision.ccnThreshold = invGb29768Cfg->ccnThreshold;
    gGbtAntiCollision.cinThreshold = invGb29768Cfg->cinThreshold;

    readTIDinInventoryRound = invGb29768Cfg->readTID;

    powerUpReader();
    checkAndOpenSession(SESSION_GB29768);
    powerDownReader(); 

    StoreInternalDataForTest(TEST_ID_SETGB29768INVENTORYCFG, ERR_NONE, invGb29768Cfg);
    
    return ERR_NONE;
}
#endif

// **************************************************************************
// Getter SW Cfg
// **************************************************************************
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGen2Timings(STUHFL_T_Gen2_Timings *gen2Timings)
{
    gen2Timings->T4Min        = gen2Configuration.T4Min;
    StoreTestData(TEST_ID_GETGEN2TIMINGS, sizeof(STUHFL_T_Gen2_Timings), (uint8_t *)gen2Timings);
    return ERR_NONE;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGen2ProtocolCfg(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg *gen2ProtocolCfg)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    gen2ProtocolCfg->blf        = gen2Configuration.blf;
	gen2ProtocolCfg->coding     = gen2Configuration.coding;
	gen2ProtocolCfg->trext      = gen2Configuration.trext;
	gen2ProtocolCfg->tari       = gen2Configuration.tari;

    StoreTestData(TEST_ID_GETGEN2PROTOCOLCFG, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_Gen2Protocol_Cfg) : 0, (uint8_t *)gen2ProtocolCfg);

    return err;
}
#if GB29768
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGb29768ProtocolCfg(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg *gb29768ProtocolCfg)
{

    STUHFL_T_RET_CODE err = ERR_NONE;
    
    gb29768ProtocolCfg->trext   = gb29768Config.trext;
    gb29768ProtocolCfg->blf     = gb29768Config.blf;
    gb29768ProtocolCfg->coding  = gb29768Config.coding;
    gb29768ProtocolCfg->tc      = gb29768Config.tc;

    StoreTestData(TEST_ID_GETGB29768PROTOCOLCFG, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg) : 0, (uint8_t *)gb29768ProtocolCfg);

    return err;
}
#endif
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTxRxCfg(STUHFL_T_ST25RU3993_TxRx_Cfg *txRxCfg)
{
    STUHFL_T_RET_CODE err = ERR_NONE;

    txRxCfg->rxSensitivity = st25RU3993GetSensitivity();
    txRxCfg->txOutputLevel = st25RU3993GetTxOutputLevel();
    
#ifdef ANTENNA_SWITCH
    txRxCfg->usedAntenna = alternateAntennaEnable ? ANTENNA_ALT : usedAntenna;
    txRxCfg->alternateAntennaInterval = alternateAntennaInterval;
    txRxCfg->vlna = vlna;
#else
    txRxCfg->usedAntenna = 0;
    txRxCfg->alternateAntennaInterval = 0;
    txRxCfg->vlna = 0;
#endif

    StoreTestData(TEST_ID_GETTXRXCFG, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_TxRx_Cfg) : 0, (uint8_t *)txRxCfg);

    return err;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetPA_Cfg(STUHFL_T_ST25RU3993_PA_Cfg *paCfg)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    paCfg->useExternal = externalPA;

    StoreTestData(TEST_ID_GETPA_CFG, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_PA_Cfg) : 0, (uint8_t *)paCfg);

    return err;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGen2InventoryCfg(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg *invGen2Cfg)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    memset(invGen2Cfg, 0, sizeof(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg));
    invGen2Cfg->fastInv = fastInventory;
    invGen2Cfg->autoAck = autoAckMode;
    invGen2Cfg->readTID = readTIDinInventoryRound;
	invGen2Cfg->startQ      = cycleData->statistics.Q;
    invGen2Cfg->adaptiveQEnable   = gGen2AdaptiveQSettings.isEnabled ? 1: 0;
    invGen2Cfg->adjustOptions = gGen2AdaptiveQSettings.options;
    for(int i = 0; i < NUM_C_VALUES; i++){
        invGen2Cfg->C2[i] = gGen2AdaptiveQSettings.c2[i];
        invGen2Cfg->C1[i] = gGen2AdaptiveQSettings.c1[i];
    }
    invGen2Cfg->minQ = gGen2AdaptiveQSettings.minQ;
    invGen2Cfg->maxQ = gGen2AdaptiveQSettings.maxQ;

#ifdef TUNER
    invGen2Cfg->autoTuningInterval  = autoTuningInterval;
    invGen2Cfg->autoTuningLevel     = autoTuningLevel;
    invGen2Cfg->autoTuningAlgo      = autoTuningAlgo;
#else
    invGen2Cfg->autoTuningInterval  = 0;
    invGen2Cfg->autoTuningLevel     = 0;
    invGen2Cfg->autoTuningAlgo      = TUNING_ALGO_NONE;
#endif

    invGen2Cfg->adaptiveSensitivityEnable   = gAdaptiveSensitivityEnable;
    invGen2Cfg->adaptiveSensitivityInterval = gAdaptiveSensitivityInterval;

    invGen2Cfg->sel         = gen2Configuration.sel;
    invGen2Cfg->session     = gen2Configuration.session;
    invGen2Cfg->target      = gen2Configuration.target;
    invGen2Cfg->toggleTarget        = gen2Configuration.toggleTarget;
    invGen2Cfg->targetDepletionMode = targetDepletionMode;

    StoreTestData(TEST_ID_GETGEN2INVENTORYCFG, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_Gen2Inventory_Cfg) : 0, (uint8_t *)invGen2Cfg);

    return err;
}
#if GB29768
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetGb29768InventoryCfg(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg *invGb29768Cfg)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    memset(invGb29768Cfg, 0, sizeof(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg));
#ifdef TUNER
    invGb29768Cfg->autoTuningInterval = autoTuningInterval;
    invGb29768Cfg->autoTuningLevel = autoTuningLevel;
    invGb29768Cfg->autoTuningAlgo = autoTuningAlgo;
#else
    invGb29768Cfg->autoTuningInterval  = 0;
    invGb29768Cfg->autoTuningLevel     = 0;
    invGb29768Cfg->autoTuningAlgo      = TUNING_ALGO_NONE;
#endif

    invGb29768Cfg->adaptiveSensitivityEnable = gAdaptiveSensitivityEnable;
    invGb29768Cfg->adaptiveSensitivityInterval = gAdaptiveSensitivityInterval;

    invGb29768Cfg->condition = gb29768Config.condition;
    invGb29768Cfg->session = gb29768Config.session;
    invGb29768Cfg->target = gb29768Config.target;
    invGb29768Cfg->toggleTarget = gb29768Config.toggleTarget;
    invGb29768Cfg->targetDepletionMode = targetDepletionMode;

    invGb29768Cfg->endThreshold = gGbtAntiCollision.endThreshold;
    invGb29768Cfg->ccnThreshold = gGbtAntiCollision.ccnThreshold;
    invGb29768Cfg->cinThreshold = gGbtAntiCollision.cinThreshold;

    invGb29768Cfg->readTID = readTIDinInventoryRound;

    StoreTestData(TEST_ID_GETGB29768INVENTORYCFG, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg) : 0, (uint8_t *)invGb29768Cfg);

    return err;
}
#endif

// **************************************************************************
// Setter Tuning
// **************************************************************************
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningCaps(STUHFL_T_ST25RU3993_TuningCaps *tuning)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    
#ifdef TUNER
// DEBUG only
#if ELANCE || EVAL
    if((tuning->antenna == 0xFF)&&
       (tuning->caps.cin == 0xFF)    &&
       (tuning->caps.clen == 0xFF)   &&        
       (tuning->caps.cout == 0xFF))
    {
        sweepCaps();
        return ERR_NONE;
    }
#endif
// DEBUG only

    if (   (tuning->channelListIdx >= MAX_FREQUENCY)
        || (tuning->antenna >= FW_NB_ANTENNA)
    ) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }
    if (tuning->channelListIdx >= gChannelListTuning[ tuning->antenna ].channelList.nFrequencies) {
        return (STUHFL_T_RET_CODE)ERR_REQUEST;
    }

    updateTuningCaps(true, tuning->antenna, tuning->channelListIdx, &tuning->caps, true);

    StoreInternalDataForTest(TEST_ID_SETTUNINGCAPS, err, tuning);

#else
    err = (STUHFL_T_RET_CODE)ERR_REQUEST;
#endif

    return err;
}

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuning(STUHFL_T_ST25RU3993_Tuning *tuning)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    
#ifdef TUNER
// DEBUG only
#if ELANCE || EVAL
    if((tuning->antenna == 0xFF)&&
       (tuning->cin == 0xFF)    &&
       (tuning->clen == 0xFF)   &&        
       (tuning->cout == 0xFF))
    {
        sweepCaps();
        return ERR_NONE;
    }
#endif
// DEBUG only

    uint8_t ant = tuning->antenna;
    if (    (ant >= FW_NB_ANTENNA)
#ifdef ANTENNA_SWITCH
        &&  (ant != ANTENNA_ALT)
#endif	// ANTENNA_SWITCH 
    ) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }
#ifdef ANTENNA_SWITCH
    if (ant == ANTENNA_ALT) {
        ant = ANTENNA_1;
    }
#endif  // ANTENNA_SWITCH 

    if (profile == PROFILE_NEWTUNING) {
        return (STUHFL_T_RET_CODE)ERR_REQUEST;
    }
    
    STUHFL_T_ST25RU3993_Caps    caps;
    caps.cin = tuning->cin;
    caps.clen = tuning->clen;
    caps.cout = tuning->cout;
    updateTuningCaps(true, ant, tuningTable[profile].currentEntry, &caps, false);

    StoreInternalDataForTest(TEST_ID_SETTUNING, err, tuning);

#else
    err = (STUHFL_T_RET_CODE)ERR_REQUEST;
#endif

    return err;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningTableEntry(STUHFL_T_ST25RU3993_TuningTableEntry *tuningTableEntry)
{
    if (profile == PROFILE_NEWTUNING) {
        return (STUHFL_T_RET_CODE)ERR_REQUEST;
    }

    // set current entry
    if(tuningTableEntry->entry >= tuningTable[profile].tableSize){
        // Check parameters
        if(tuningTableEntry->freq > FREQUENCY_MAX_VALUE){
            return (STUHFL_T_RET_CODE)ERR_PARAM;
        }
    
        // add entry to end
        tuningTable[profile].tableSize++;
        if(tuningTable[profile].tableSize > MAX_FREQUENCY) {
            // limit table to maximum possible frequencies
            // if exceeds MAX_FREQUENCY, all subsequent table update will be done in last table entry
            tuningTable[profile].tableSize = MAX_FREQUENCY;
        }
        tuningTable[profile].currentEntry = tuningTable[profile].tableSize-1;

        // Only set frequency for new entries
        tuningTable[profile].freq[tuningTable[profile].currentEntry] = tuningTableEntry->freq;
    }
    else{
        // set entry
        tuningTable[profile].currentEntry = tuningTableEntry->entry;
    }

    // Set values for each antenna
    for (uint8_t ant=0 ; ant<FW_NB_ANTENNA ; ant++) {
        if(tuningTableEntry->applyCapValues[ant]){
            STUHFL_T_ST25RU3993_Caps    caps;
            caps.cin = tuningTableEntry->cin[ant];
            caps.clen = tuningTableEntry->clen[ant];
            caps.cout = tuningTableEntry->cout[ant];
            updateTuningCaps(false, ant, tuningTable[profile].currentEntry, &caps, false);

            tuningTable[profile].tunedIQ[ant][tuningTable[profile].currentEntry] = ((tuningTableEntry->cin[ant] > MAX_DAC_VALUE) || (tuningTableEntry->clen[ant] > MAX_DAC_VALUE) || (tuningTableEntry->cout[ant] > MAX_DAC_VALUE)) ? 0xFFFF : tuningTableEntry->IQ[ant];
        }
    }
    
    StoreInternalDataForTest(TEST_ID_SETTUNINGTABLEENTRY, ERR_NONE, tuningTableEntry);
    
    return ERR_NONE;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningTableDefault(STUHFL_T_ST25RU3993_TunerTableSet *set)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    if (set->profile >= NUM_SAVED_PROFILES) {
        return (set->profile == PROFILE_NEWTUNING) ? (STUHFL_T_RET_CODE)ERR_REQUEST : (STUHFL_T_RET_CODE)ERR_PARAM;
    }
    setDefaultTuningTable(set->profile, &tuningTable[set->profile], set->freq);
    saveTuningTableToFlash(set->profile);
    return err;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningTableSave2Flash(void)
{
    STUHFL_T_RET_CODE err = ERR_NONE;
    saveTuningTable = true;
    return err;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV SetTuningTableEmpty(void)
{
    if (profile != PROFILE_NEWTUNING) {
        tuningTable[profile].tableSize = 0;
    }
    return ERR_NONE;
}

// **************************************************************************
// Getter Tuning
// **************************************************************************
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTuningCaps(STUHFL_T_ST25RU3993_TuningCaps *tuning)
{
    STUHFL_T_RET_CODE err = ERR_NONE;

    if (   (tuning->channelListIdx >= MAX_FREQUENCY)
        || (tuning->antenna >= FW_NB_ANTENNA)
    ) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    if (tuning->channelListIdx >= gChannelListTuning[ tuning->antenna ].channelList.nFrequencies) {
        return (STUHFL_T_RET_CODE)ERR_REQUEST;
    }

    memcpy(&tuning->caps, &gChannelListTuning[ tuning->antenna ].channelList.item[ tuning->channelListIdx ].caps, sizeof(STUHFL_T_ST25RU3993_Caps));

    StoreTestData(TEST_ID_GETTUNINGCAPS, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_TuningCaps) : 0, (uint8_t *)tuning);

    return err;
}

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTuning(STUHFL_T_ST25RU3993_Tuning *tuning)
{
    STUHFL_T_RET_CODE err = ERR_NONE;

    uint8_t ant = tuning->antenna;
    if (    (ant >= FW_NB_ANTENNA)
#ifdef ANTENNA_SWITCH
        &&  (ant != ANTENNA_ALT)
#endif	// ANTENNA_SWITCH 
    ) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }
#ifdef ANTENNA_SWITCH
    if (ant == ANTENNA_ALT) {
        ant = ANTENNA_1;
    }
#endif  // ANTENNA_SWITCH 
    if (profile == PROFILE_NEWTUNING) {
        return (STUHFL_T_RET_CODE)ERR_REQUEST;
    }

    tuning->cin = tuningTable[profile].cin[ant][tuningTable[profile].currentEntry];
    tuning->clen = tuningTable[profile].clen[ant][tuningTable[profile].currentEntry];
    tuning->cout = tuningTable[profile].cout[ant][tuningTable[profile].currentEntry];

    StoreTestData(TEST_ID_GETTUNING, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_Tuning) : 0, (uint8_t *)tuning);

    return err;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTuningTableEntry(STUHFL_T_ST25RU3993_TuningTableEntry *tuningTableEntry)
{
    STUHFL_T_RET_CODE err = ERR_NONE;

    if (profile == PROFILE_NEWTUNING) {     // Management of PROFILE_NEWTUNING is added for tests convenience
        if(tuningTableEntry->entry >= gChannelListTuning[usedAntenna].channelList.nFrequencies){
            return (STUHFL_T_RET_CODE)ERR_PARAM;
        }

        // Force applyCapValues to 0
        memset(tuningTableEntry->applyCapValues, 0x00, MAX_ANTENNA);
    
        tuningTableEntry->freq = gChannelListTuning[usedAntenna].channelList.item[ tuningTableEntry->entry ].frequency;

        // Get values of each antenna
        for (uint8_t ant=0 ; ant<FW_NB_ANTENNA ; ant++) {
            tuningTableEntry->cin[ant] = gChannelListTuning[ant].channelList.item[ tuningTableEntry->entry ].caps.cin;
            tuningTableEntry->clen[ant] = gChannelListTuning[ant].channelList.item[ tuningTableEntry->entry ].caps.clen;
            tuningTableEntry->cout[ant] = gChannelListTuning[ant].channelList.item[ tuningTableEntry->entry ].caps.cout;
            tuningTableEntry->IQ[ant] = gChannelListTuning[ant].tunedIQ[ tuningTableEntry->entry ];
        }
    }
    else {
        if(tuningTableEntry->entry >= tuningTable[profile].tableSize){
            return (STUHFL_T_RET_CODE)ERR_PARAM;
        }

        // Force applyCapValues to 0
        memset(tuningTableEntry->applyCapValues, 0x00, MAX_ANTENNA);
        
        // get frequency        
        tuningTableEntry->freq = tuningTable[profile].freq[tuningTableEntry->entry];
        
        // Get values of each antenna
        for (uint8_t ant=0 ; ant<FW_NB_ANTENNA ; ant++) {
            tuningTableEntry->cin[ant] = tuningTable[profile].cin[ant][tuningTableEntry->entry];
            tuningTableEntry->clen[ant] = tuningTable[profile].clen[ant][tuningTableEntry->entry];
            tuningTableEntry->cout[ant] = tuningTable[profile].cout[ant][tuningTableEntry->entry];
            tuningTableEntry->IQ[ant] = tuningTable[profile].tunedIQ[ant][tuningTableEntry->entry];            
        }
    }

    StoreTestData(TEST_ID_GETTUNINGTABLEENTRY, (err == ERR_NONE) ? sizeof(STUHFL_T_ST25RU3993_TuningTableEntry) : 0, (uint8_t *)tuningTableEntry);
    return err;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV GetTuningTableInfo(STUHFL_T_ST25RU3993_TuningTableInfo *tuningTableInfo)
{
    if ((tuningTableInfo->profile >= NUM_SAVED_PROFILES) && (tuningTableInfo->profile != PROFILE_NEWTUNING)) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;    // Out of range profile
    }
    if (tuningTableInfo->profile == PROFILE_NEWTUNING) {
        return (STUHFL_T_RET_CODE)ERR_REQUEST;
    }

    tuningTableInfo->numEntries = tuningTable[tuningTableInfo->profile].tableSize;

    StoreTestData(TEST_ID_GETTUNINGTABLEINFO, sizeof(STUHFL_T_ST25RU3993_TuningTableInfo), (uint8_t *)tuningTableInfo);

    return ERR_NONE;
}

// **************************************************************************
// Tuning
// **************************************************************************
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV Tune(STUHFL_T_ST25RU3993_Tune *tune)
{
    STUHFL_T_RET_CODE err = (STUHFL_T_RET_CODE)ERR_REQUEST;
#ifdef TUNER
    uint8_t algo = tune->algo & ~TUNING_ALGO_ENABLE_FPD;            // Removes FPD bit
    switch (algo) {
        case TUNING_ALGO_FAST:
        case TUNING_ALGO_SLOW:
        case TUNING_ALGO_MEDIUM:
            powerUpReader();    
            st25RU3993AntennaPower(1);
            runHillClimb(algo, (tune->algo & TUNING_ALGO_ENABLE_FPD) ? true : false);
            powerDownReader();
            err = ERR_NONE;
            break;
        case TUNING_ALGO_NONE:
            err = ERR_NONE;
            break;
    }
#endif

    StoreInternalDataForTest(TEST_ID_TUNE, err, tune);

    return err;
}
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV TuneChannel(STUHFL_T_ST25RU3993_TuneCfg *tuneCfg)
{
    STUHFL_T_RET_CODE err = ERR_NONE;

    if(   (tuneCfg->algorithm > TUNING_ALGO_MEDIUM) 
       || (tuneCfg->antenna >= FW_NB_ANTENNA)
       || (tuneCfg->channelListIdx >= MAX_FREQUENCY)
    ) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    if (   (profile != PROFILE_NEWTUNING)
        || (tuneCfg->antenna != usedAntenna)
        || (tuneCfg->channelListIdx >= gChannelListTuning[ tuneCfg->antenna ].channelList.nFrequencies)
       ) {
        return (STUHFL_T_RET_CODE)ERR_REQUEST;
    }

    err = runChannelTuning(gChannelListTuning[ tuneCfg->antenna ].channelList.item[ tuneCfg->channelListIdx ].frequency, tuneCfg->algorithm, tuneCfg->enableFPD);
    if (err == ERR_NONE) {
        // .. and save if requested
        if(tuneCfg->save2Flash){
            saveChannelList |= (1<<tuneCfg->antenna);
        }    
    }

    StoreInternalDataForTest(TEST_ID_TUNECHANNEL, err, tuneCfg);

    return err;
}


// **************************************************************************
// Inventory Runner
// **************************************************************************
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV inventoryRunnerSetup(STUHFL_T_Inventory_Option *invOption, STUHFL_T_InventoryCycle cycleCallback, STUHFL_T_InventoryFinished finishedCallback, STUHFL_T_Inventory_Data *invData)
{
    cyclicInventory = true;    
    inventoryCycleCallback = cycleCallback;
    inventoryFinishedCallback = finishedCallback;

    rssiMode = invOption->rssiMode;
    scanDuration = invOption->roundCnt;
    inventoryDelay = invOption->inventoryDelay;
    gReportOptions = invOption->reportOptions;

    if(invData == NULL){
        cycleData = &cycleDataTmp;
        cycleData->tagList = tagListTmp;        
        cycleData->tagListSizeMax = MAXTAG;
    }else{
        cycleData = invData;
        cycleData->statistics.Q = cycleDataTmp.statistics.Q;    // SetGen2ProtocolCfg that sets Q has been done on cycleDataTmp
    }

    // Reset tagListSize & all statistics (save & restore Q)
    cycleData->tagListSize = 0;
    StoreTestData(TEST_ID_INVENTORY_LISTSIZE, sizeof(cycleData->tagListSize), (uint8_t *)&cycleData->tagListSize);
    uint8_t currentQ = cycleData->statistics.Q;
    memset((uint8_t *)&cycleData->statistics, 0x00, sizeof(STUHFL_T_Inventory_Statistics));
    cycleData->statistics.Q = currentQ;
    // finally remember start time for relative time measurement
    gStartScanTimestamp = 0;
    gLastHeartBeatTick = gStartScanTimestamp;     // Set Heart Beat tick initial value

#ifdef USE_INVENTORY_EXT
    // prepare slot statistics
    slotInfoData->slotSync.slotIdBase = 0;
    slotInfoData->slotSync.timeStampBase = 0;
    slotWrPos = slotRdPos = 0;    
#endif
    
    //
    computeTunedLed();    
    return ERR_NONE;
}

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV InventoryRunnerStart(STUHFL_T_Inventory_Option *invOption, STUHFL_T_InventoryCycle cycleCallback, STUHFL_T_InventoryFinished finishedCallback, STUHFL_T_Inventory_Data *invData)
{
    if(ERR_NONE != inventoryRunnerSetup(invOption, cycleCallback, finishedCallback, invData)){
        return (STUHFL_T_RET_CODE)ERR_REQUEST;
    }

    // start the runner
    while(doCyclicInventory()) {
        __NOP();
    }
   
    cycleData->statistics.tuningStatus = getTuningStatus();
    StoreTestData(TEST_ID_INVENTORY_STATISTICS, sizeof(STUHFL_T_Inventory_Statistics), (uint8_t *)&cycleData->statistics);
   
    return ERR_NONE;
}

STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV InventoryRunnerStop(void)
{
    cyclicInventory = false;    
    inventoryCycleCallback = NULL;
    setInventoryFromHost(false, false);
        
    SET_CRC_LED_OFF();
    SET_NORESP_LED_OFF();

    SET_TAG_LED_OFF();
    SET_TUNING_LED_OFF();

    return ERR_NONE;
}
// **************************************************************************
// Cycle
// **************************************************************************
void cycle(void)
{    
    /* main trigger for operation commands.
       process UART based host communication */
#if STUHFL_HOST_COMMUNICATION
    ProcessIO();
#endif

    // execute all cyclic opertaions if necessary
    doCyclicInventory();
    doContinuousModulation();
    doSaveTuningTable(profile);
    
    LOGFLUSH();
}

// **************************************************************************
// Gen2
// **************************************************************************
STUHFL_T_RET_CODE Gen2_Inventory(STUHFL_T_Inventory_Option *invOption, STUHFL_T_Inventory_Data *invData)
{
    // to reuse the doCyclicInventory() function we force exact one inventory here    
    invOption->roundCnt = 1;

    STUHFL_T_RET_CODE ret = InventoryRunnerStart(invOption, NULL, NULL, (STUHFL_T_Inventory_Data *)invData);
    doCyclicInventory();  
    ret |= InventoryRunnerStop();
    return ret;
}

STUHFL_T_RET_CODE Gen2_Select(STUHFL_T_Gen2_Select *selData)
{
    if(selData->mode == GEN2_SELECT_MODE_CLEAR_LIST){
        powerUpReader();
        checkAndOpenSession(SESSION_GEN2);
        powerDownReader();

        numSelects = 0;

        StoreTestData(TEST_ID_GEN2_SELECT, 0, (uint8_t *)selData);  // Reset tests data
        return ERR_NONE;
    }

    if (   (selData->memBank > GEN2_MEMORY_BANK_USER)
        || (selData->target > GEN2_TARGET_SL)
        || (selData->mode > GEN2_SELECT_MODE_CLEAR_AND_ADD)
        ) {
        StoreTestData(TEST_ID_GEN2_SELECT, 0, (uint8_t *)selData);  // Reset tests data
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    powerUpReader();
    checkAndOpenSession(SESSION_GEN2);
    if(selData->mode == GEN2_SELECT_MODE_ADD2LIST){
        if(numSelects >= MAX_SELECTS){
            numSelects = MAX_SELECTS;
            powerDownReader();

            StoreTestData(TEST_ID_GEN2_SELECT, 0, (uint8_t *)selData);  // Reset tests data
            return (STUHFL_T_RET_CODE)ERR_NOMEM;
        }
    }
    else if(selData->mode == GEN2_SELECT_MODE_CLEAR_AND_ADD){
        numSelects = 0;
    }

    memcpy(&selParams[numSelects], selData, sizeof(STUHFL_T_Gen2_Select));
    StoreTestData(TEST_ID_GEN2_SELECT, sizeof(STUHFL_T_Gen2_Select), (uint8_t *)&selParams[numSelects]);
    
    initTagInfo();
    numSelects++;
    powerDownReader();
    return ERR_NONE;
}

STUHFL_T_RET_CODE Gen2_Read(STUHFL_T_Read *readData)
{
    int8_t      status = ERR_NONE;
    uint32_t    retries = 6;
    uint8_t     membank = readData->memBank;
    uint32_t    wrdPtr = readData->wordPtr;
    uint8_t     nbWords = (readData->bytes2Read < MAX_READ_DATA_LEN) ? ((readData->bytes2Read+1)/2) : (MAX_READ_DATA_LEN/2);    // Read enough words for odd and even lengths or Read max data size

    if (nbWords == 0) {
        // Requested size=0, tries first to read max data size
        nbWords = MAX_READ_DATA_LEN/2;
    }
        
    memset(readData->data, 0, MAX_READ_DATA_LEN);

    bool stopLoop = false;
    do
    {
        status = startTagConnection();
        if(status == ERR_NONE){
            status = gen2AccessTag(selectedTag, readData->pwd);
            if(status == ERR_NONE){
                uint8_t tmpBuf[MAX_READ_DATA_LEN];
                status = gen2ReadFromTag(selectedTag, membank, wrdPtr, &nbWords, tmpBuf);

                // Check sending timeout
                if (status == ERR_NONE) {
                    if ((readData->bytes2Read+1)/2 != nbWords) {
                        // nbWords has been modified since initial request => update read bytes accordingly
                        readData->bytes2Read = nbWords*2;
                    }

                    memcpy(readData->data, tmpBuf, readData->bytes2Read);
                }
                else {
                    if (status == ERR_NOMEM) {
                        if (nbWords) {
                            if (readData->bytes2Read == 0) {
                                // Trying to read max data size with requested size=0 has failed
                                // Tag area contains less data than max size, tries then with size=0 which will be de facto below max size
                                nbWords = 0;
                            }
                            else {
                                // Requested more data than tag memory reduces then data to be read
                                nbWords--;
                                stopLoop = (nbWords == 0);        // Stops reading data if no more data to read
                            }
                        }
                        else {
                            stopLoop = true;
                        }
                    }

                    continueCheckTimeout();
                    if(maxSendingLimitTimedOut) {
                        status = GEN2_ERR_CHANNEL_TIMEOUT;
                    }
                }
            }
        }
        stopTagConnection();

        if (status != ERR_NOMEM) {
            retries--;
            stopLoop = isGen2ChipErrorCode(status);     // Will stop Loop if Gen2 Error code and not ERR_NOMEM
        }
    } while((status != ERR_NONE) && (!stopLoop) && (retries));

    StoreTestData(TEST_ID_GEN2_READ, (status == ERR_NONE) ? sizeof(STUHFL_T_Read) : 0, (uint8_t *)readData);
    
    return (STUHFL_T_RET_CODE)status;
}

STUHFL_T_RET_CODE Gen2_Write(STUHFL_T_Write *writeData)
{
    int8_t      status;
    uint8_t     membank = writeData->memBank;
    uint32_t    address = writeData->wordPtr;
    uint32_t    retries = 4;

    do
    {
        status = startTagConnection();
        if(status == ERR_NONE){
            status = gen2AccessTag(selectedTag, writeData->pwd);
            if(status == ERR_NONE){
                writeTagMemGen2(address, selectedTag, writeData->data, 1, membank, &status, &writeData->tagReply);
            }
        }
        stopTagConnection();

        retries--;
    } while((status != ERR_NONE) && (retries));


    StoreTestData(TEST_ID_GEN2_WRITE, (status == ERR_NONE) ? sizeof(STUHFL_T_Write) : 0, (uint8_t *)writeData);
    return (STUHFL_T_RET_CODE)status;
}

STUHFL_T_RET_CODE Gen2_BlockWrite(STUHFL_T_BlockWrite *blockWrite)
{
    int8_t      status;
    uint32_t    retries = 4;

    if (blockWrite->nbWords > MAX_BLOCKWRITE_DATA_LEN) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    do
    {
        status = startTagConnection();
        if(status == ERR_NONE){
            status = gen2AccessTag(selectedTag, blockWrite->pwd);
            if(status == ERR_NONE){
                status = writeBlockTagMemGen2(blockWrite->wordPtr, selectedTag, blockWrite->data, blockWrite->nbWords, blockWrite->memBank, &blockWrite->tagReply);
            }
        }
        stopTagConnection();

        retries--;
    } while((status != ERR_NONE) && (retries));


    StoreTestData(TEST_ID_GEN2_BLOCKWRITE, (status == ERR_NONE) ? sizeof(STUHFL_T_BlockWrite) : 0, (uint8_t *)blockWrite);
    return (STUHFL_T_RET_CODE)status;
}

STUHFL_T_RET_CODE Gen2_Lock(STUHFL_T_Gen2_Lock *lockData)
{
    const uint8_t *mask = lockData->mask;

    int8_t status = startTagConnection();
    if(status == ERR_NONE){
        status=gen2AccessTag(selectedTag, lockData->pwd);
        if(status == ERR_NONE){
            bool    continueLoop;

            uint32_t cmdEndTick = HAL_GetTick() + MAX_GEN2_DELAYED_RESPONSE_MS;
            status = gen2LockTag(selectedTag, mask, &lockData->tagReply);
            do
            {
                if (((status == ST25RU3993_ERR_PREAMBLE) || (status == ST25RU3993_ERR_CRC)) && (HAL_GetTick() < cmdEndTick)) {
                    // Reader probably received burst from tag
                    // Ignore it and keep on waiting for tag response
                    status = gen2ContinueCommand(&lockData->tagReply);
                    continueLoop = (HAL_GetTick() < cmdEndTick);
                }
                else {
                    if ((status != ERR_NONE) && !isGen2ChipErrorCode(status)) {
                        delay_ms(20); // T5: Potentially transaction is still ongoing, avoid metastable eeprom and stuff
                    }
                    continueLoop = false;
                }
            } while((status != ERR_NONE) && !isGen2ChipErrorCode(status) && continueLoop);
        }
    }
    stopTagConnection();

    StoreTestData(TEST_ID_GEN2_LOCK, (status == ERR_NONE) ? sizeof(STUHFL_T_Gen2_Lock) : 0, (uint8_t *)lockData);
    return (STUHFL_T_RET_CODE)status;
}

STUHFL_T_RET_CODE Gen2_Kill(STUHFL_T_Kill *killData)
{
    const uint8_t* password = killData->pwd;
    uint8_t rfu   = killData->rfu;
    uint8_t recom = killData->recom;

    int8_t status = startTagConnection();    
    if(status == ERR_NONE){
        bool    continueLoop;

        uint32_t cmdEndTick = HAL_GetTick() + MAX_GEN2_DELAYED_RESPONSE_MS;
        status = gen2KillTag(selectedTag, password, rfu, recom, &killData->tagReply);
        do
        {
            if (((status == ST25RU3993_ERR_PREAMBLE) || (status == ST25RU3993_ERR_CRC)) && (HAL_GetTick() < cmdEndTick)) {
                // Reader probably received burst from tag
                // Ignore it and keep on waiting for tag response
                status = gen2ContinueCommand(&killData->tagReply);
                continueLoop = (HAL_GetTick() < cmdEndTick);
            }
            else {
                if ((status != ERR_NONE) && !isGen2ChipErrorCode(status)) {
                    delay_ms(20); // T5: Potentially transaction is still ongoing, avoid metastable eeprom and stuff
                }
                continueLoop = false;
            }
        } while((status != ERR_NONE) && !isGen2ChipErrorCode(status) && continueLoop);
    }
    stopTagConnection();

    StoreTestData(TEST_ID_GEN2_KILL, (status == ERR_NONE) ? sizeof(STUHFL_T_Kill) : 0, (uint8_t *)killData);

    return (STUHFL_T_RET_CODE)status;
}

STUHFL_T_RET_CODE Gen2_GenericCmd(STUHFL_T_Gen2_GenericCmdSnd *genericCmdSnd, STUHFL_T_Gen2_GenericCmdRcv *genericCmdRcv)
{
    int8_t status = ERR_NONE;
    uint16_t toTagSize;
    uint16_t fromTagSize;
    uint8_t toTagBuffer[GEN2_GENERIC_CMD_MAX_SND_DATA_BYTES];
    uint8_t cmd, norestime;
    uint16_t dataBytes, leftBits;

    genericCmdRcv->rcvDataByteCnt = 0;

    /* Precompute toTag info */
    cmd = genericCmdSnd->cmd;
    norestime = genericCmdSnd->noResponseTime;
    switch (cmd) {
        case ST25RU3993_CMD_TRANSMCRC:      fromTagSize = genericCmdSnd->expectedRcvDataBitCnt + 16; break;
        case ST25RU3993_CMD_TRANSMCRCEHEAD: fromTagSize = genericCmdSnd->expectedRcvDataBitCnt + 17; break;
        default:                            fromTagSize = genericCmdSnd->expectedRcvDataBitCnt;      break;
    }
    toTagSize = genericCmdSnd->sndDataBitCnt;
    dataBytes = (toTagSize + 7) / 8;
    leftBits = (dataBytes * 8) - toTagSize;    
    if(genericCmdSnd->appendRN16) { 
        toTagSize += 16; 
    }
        
    if((toTagSize >= (GEN2_GENERIC_CMD_MAX_SND_DATA_BYTES*8)) || (fromTagSize >= (GEN2_GENERIC_CMD_MAX_RCV_DATA_BYTES*8))){
        status = ERR_REQUEST;
        StoreGen2GenericCommandDataForTest(cmd, norestime, toTagBuffer, 0, genericCmdRcv);
        return (STUHFL_T_RET_CODE)status;
    }

    insertBitStream(toTagBuffer, genericCmdSnd->sndData, dataBytes, 8);

    /* Send generic command to tag */ 
    uint32_t    retries = 5;
    do
    {
        status = startTagConnection();   
        if(status == ERR_NONE){
            status = gen2AccessTag(selectedTag, genericCmdSnd->pwd);
            if(status == ERR_NONE){
                if(genericCmdSnd->appendRN16) { 
                    if(leftBits == 0) {
                        insertBitStream(&toTagBuffer[dataBytes], selectedTag->handle, 2, 8);
                    }
                    else {
                        insertBitStream(&toTagBuffer[dataBytes-1], selectedTag->handle, 2, leftBits);
                    }
                }

                uint32_t delayedRespTime = (norestime == 0xFF) ? MAX_GEN2_DELAYED_RESPONSE_MS : (norestime*26U + 999U)/1000U;    // Either 20ms or norestime*26us(converted to upper ms)
                uint32_t cmdEndTick = HAL_GetTick() + delayedRespTime;

                // Get Values from Stream
                bool waitForTxIrq = retries%2;              // Alternates waitForTxIrq between true and false as the command may need it or not
                uint16_t tmpFromTagSize = fromTagSize;      // Use intermediate variable as st25RU3993TxRxGen2Bytes() modifies it

                status = st25RU3993TxRxGen2Bytes(cmd, toTagBuffer, toTagSize, genericCmdRcv->rcvData, &tmpFromTagSize, norestime, 0, waitForTxIrq);
                while (((status == ST25RU3993_ERR_PREAMBLE) || (status == ST25RU3993_ERR_CRC)) && (HAL_GetTick() < cmdEndTick)) {
                    // Reader probably received burst from tag
                    // Ignore it and keep on waiting for tag response
                    tmpFromTagSize = fromTagSize;
                    status = st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_ENABLERX, NULL, 0, genericCmdRcv->rcvData, &tmpFromTagSize, norestime, 0, 0);
                }

                genericCmdRcv->rcvDataByteCnt = (tmpFromTagSize + 7) / 8;
                if(status == ERR_NONE){
                    delay_ms(delayedRespTime);          // T5: Potentially transaction is still ongoing, avoid metastable eeprom and stuff
                }
            }
        }
        stopTagConnection();
        
        retries--;
    } while((status != ERR_NONE) && (retries));

    StoreGen2GenericCommandDataForTest(cmd, norestime, toTagBuffer, toTagSize, genericCmdRcv);
    
    return (STUHFL_T_RET_CODE)status;
}


STUHFL_T_RET_CODE Gen2_QueryMeasureRssi(STUHFL_T_Gen2_QueryMeasureRssi *queryMeasureRssi)
{
    int8_t status = ERR_NONE;
    uint8_t pllOk;
    uint32_t freq;

    gen2Config_t saveCfg;

    // 
    freq = queryMeasureRssi->frequency;
    pllOk = st25RU3993SetBaseFrequency(freq);
    setTuningIndexFromFreq(freq);
    //
    memcpy(&saveCfg, &gen2Configuration, sizeof(gen2Config_t));

    if(pllOk){  
        // 
        checkAndOpenSession(SESSION_GEN2);
        // start with target A
        gen2Configuration.target = 0;
        gen2Configuration.sel = 0;

        // use the SL flag for the QUERY when we did select with target SL
        for(uint32_t i=0 ; i<numSelects ; i++) {
            if(selParams[i].target == GEN2_TARGET_SL){
                gen2Configuration.sel = 3;
                break;
            }
        }
        
        //
        gen2SearchForTagsParams_t       p;
        STUHFL_T_Inventory_Statistics   statistics;
        gen2AdaptiveQ_Params_t          localAdaptiveQ; 
        
        gen2Configure(&gen2Configuration);  // Required before performSelect to ensure correct T4Min is used
        performSelectsGen2();
        for(int i = 0; i < queryMeasureRssi->measureCnt; i++){
            p.tag = &gCurrentTag;
            p.singulate = true;
            p.toggleSession = false;

            p.statistics = &statistics;
            memset(p.statistics, 0x00, sizeof(STUHFL_T_Inventory_Statistics));

            p.adaptiveQ = &localAdaptiveQ;
            p.adaptiveQ->isEnabled = false;
            p.adaptiveQ->qfp = 0.0;
            p.adaptiveQ->minQ = 0;
            p.adaptiveQ->maxQ = 15;
            p.adaptiveQ->options = 0;
            for(int i = 0; i < NUM_C_VALUES; i++){
                p.adaptiveQ->c2[i] = 35;
                p.adaptiveQ->c1[i] = 15;
            }
            p.cbTagFound = NULL;
            p.cbSlotFinished = NULL;
            p.cbContinueScanning = NULL;
            p.cbFollowTagCommand = NULL;

            // look for exact 1 TAG
            gen2Configure(&gen2Configuration);
//            if(gen2QueryMeasureRSSI(p.statistics->Q, &p.tag->agc, &p.tag->rssiLogI, &p.tag->rssiLogQ, &p.tag->rssiLinI, &p.tag->rssiLinQ) == ERR_NONE){
//                queryMeasureRssi->agc[i] = p.tag->agc;
//                queryMeasureRssi->rssiLogI[i] = p.tag->rssiLogI;
//                queryMeasureRssi->rssiLogQ[i] = p.tag->rssiLogQ;
//                queryMeasureRssi->rssiLinI[i] = p.tag->rssiLinI;                                               
//                queryMeasureRssi->rssiLinQ[i] = p.tag->rssiLinQ;
//            }
            if(gen2SearchForTags(true, &p)){
                queryMeasureRssi->agc[i] = p.tag->agc;
                queryMeasureRssi->rssiLogI[i] = p.tag->rssiLogI;
                queryMeasureRssi->rssiLogQ[i] = p.tag->rssiLogQ;
                queryMeasureRssi->rssiLinI[i] = p.tag->rssiLinI;                                               
                queryMeasureRssi->rssiLinQ[i] = p.tag->rssiLinQ;
                gen2Configuration.target = gen2Configuration.target ? 0 : 1;    // togle target
            }
            else{
                queryMeasureRssi->agc[i] = 0xFF;
                queryMeasureRssi->rssiLogI[i] = 0x0F;
                queryMeasureRssi->rssiLogQ[i] = 0x0F;
                queryMeasureRssi->rssiLinI[i] = 0xFF;
                queryMeasureRssi->rssiLinQ[i] = 0xFF;
            }
            
            
        }
    }
    // revert configuration..
    memcpy(&gen2Configuration, &saveCfg, sizeof(gen2Config_t));
    gen2Configure(&gen2Configuration);

    return (STUHFL_T_RET_CODE)status;
}


// **************************************************************************
// Gen2 Inventory loop implementation
// **************************************************************************
STUHFL_T_RET_CODE handleInventoryData(bool skipTAGs, bool forceFlush)
{
    uint32_t element = tagWrPos - tagRdPos;

    // build up Tx data and send out if either we are forced or the max number of transponders that can be transmitted once is reached
    if(forceFlush || ((element % cycleData->tagListSizeMax) == 0)){
        // overrun, we need to discard a few tags
        // and limit no of tags to max we can send at once
        if(element > cycleData->tagListSizeMax){
            tagRdPos = tagWrPos - cycleData->tagListSizeMax;
            element = cycleData->tagListSizeMax;
        }
    
        // Update Tuning Status before sending to User
        cycleData->statistics.tuningStatus = getTuningStatus();
        cycleData->statistics.timestamp = HAL_GetTick() - gStartScanTimestamp;    // make timestamp relative to start of scan

#if STUHFL_HOST_COMMUNICATION
        if (gInventoryFromHost) {
            //
            uint8_t *data = gTxPayload;
            uint16_t *dataLen = &gTxPayloadLen;

            uint16_t tagIdx;

            // reset length
            *dataLen = 0;
    
            // build TLV from header info data
            *dataLen += addTlvExt(&data[*dataLen], STUHFL_TAG_INVENTORY_STATISTICS, sizeof(STUHFL_T_Inventory_Statistics), &cycleData->statistics);

            if(!skipTAGs){
                // tag information
                for (uint32_t i=0 ; i<element ; ++i) {
                    tagIdx = tagRdPos % cycleData->tagListSizeMax;
                    
                    // make timestamp relative to start of scan
                    cycleData->tagList[tagIdx].timestamp -= gStartScanTimestamp;

                    // append TLVs
                    *dataLen += addTlvExt(&data[*dataLen], STUHFL_TAG_INVENTORY_TAG_INFO_HEADER, (uint16_t)(sizeof(STUHFL_T_Inventory_Tag) - sizeof(STUHFL_T_Inventory_Tag_XPC) - sizeof(STUHFL_T_Inventory_Tag_EPC) - sizeof(STUHFL_T_Inventory_Tag_TID)), &(cycleData->tagList[tagIdx]));
                    *dataLen += addTlvExt(&data[*dataLen], STUHFL_TAG_INVENTORY_TAG_EPC, cycleData->tagList[tagIdx].epc.len, &cycleData->tagList[tagIdx].epc.data);
                    *dataLen += addTlvExt(&data[*dataLen], STUHFL_TAG_INVENTORY_TAG_TID, cycleData->tagList[tagIdx].tid.len, &cycleData->tagList[tagIdx].tid.data);
                    *dataLen += addTlvExt(&data[*dataLen], STUHFL_TAG_INVENTORY_TAG_XPC, cycleData->tagList[tagIdx].xpc.len, &cycleData->tagList[tagIdx].xpc.data);
                    // finally append the separator
                    *dataLen += addTlvExt(&data[*dataLen], STUHFL_TAG_INVENTORY_TAG_FINISHED, 0, NULL);
                    tagRdPos++;        
                }
            }

            // update time for next heartbeat
            if(gReportOptions & INVENTORYREPORT_HEARTBEAT){
                gLastHeartBeatTick = HAL_GetTick();
            }

            // finally send out..
            uartStreamTransmitBuffer(0, ((STUHFL_CG_AL << 8) | STUHFL_CC_INVENTORY_DATA), data, *dataLen);
        }
        else
#endif  /* STUHFL_HOST_COMMUNICATION */
        {
            if (!skipTAGs) {
                // Make timestamp relative to start of scan for all tags
                for (uint32_t i=0 ; i<element ; ++i) {
                    cycleData->tagList[i].timestamp -= gStartScanTimestamp;
                }

                // All tags processed
                tagRdPos += element;
            }
            
            // update time for next heartbeat
            if(gReportOptions & INVENTORYREPORT_HEARTBEAT){
                gLastHeartBeatTick = HAL_GetTick();
            }

            // Run User CallBack
            if(inventoryCycleCallback){
                uint16_t    tagListSizeAfterCallBack;
                if (skipTAGs) {
                    // CallBack will be run with NO TAG: will set back size to flush tags next time
                    tagListSizeAfterCallBack = cycleData->tagListSize;

                    // Run CallBack for statistics only: artificially force NO TAG
                    cycleData->tagListSize = 0;
                    StoreTestData(TEST_ID_INVENTORY_LISTSIZE, sizeof(cycleData->tagListSize), (uint8_t *)&cycleData->tagListSize);
                }
                else {
                    // CallBack will process all tags: will reset size
                    tagListSizeAfterCallBack = 0;
                }

                inventoryCycleCallback(cycleData);

                // Update Tag List Size
                cycleData->tagListSize = tagListSizeAfterCallBack;
                StoreTestData(TEST_ID_INVENTORY_LISTSIZE, sizeof(cycleData->tagListSize), (uint8_t *)&cycleData->tagListSize);
            }
        }
    }

    return ERR_NONE;
}

#ifdef USE_INVENTORY_EXT
static STUHFL_T_RET_CODE handleInventorySlotData(bool forceFlush)
{    
    uint32_t element = slotWrPos - slotRdPos;
    
    // send out packet when list is full or forced..
    if(forceFlush || ((element % INVENTORYREPORT_SLOT_INFO_LIST_SIZE) == 0)){
        //
#if STUHFL_HOST_COMMUNICATION
        if (gInventoryFromHost) {            
            if(element){
                //
                uint8_t *data = gTxPayload;
                uint16_t *dataLen = &gTxPayloadLen;

                // reset dataLen and build TLV 
                *dataLen = 0;
                *dataLen += addTlvExt(&data[*dataLen], STUHFL_TAG_INVENTORY_SLOT_INFO_SYNC, sizeof(STUHFL_T_Inventory_Slot_Info_Sync), &slotInfoData->slotSync);

                while(element --){
                    // append TLVs (build up slot info details )
                    *dataLen += addTlvExt(&data[*dataLen], STUHFL_TAG_INVENTORY_SLOT_INFO, sizeof(STUHFL_T_Inventory_Slot_Info), &slotInfoData->slotInfoList[slotRdPos % INVENTORYREPORT_SLOT_INFO_LIST_SIZE]);
                    slotRdPos++;
                }
                
                // update time for next heartbeat
                if(gReportOptions & INVENTORYREPORT_HEARTBEAT){
                    gLastHeartBeatTick = HAL_GetTick();
                }

                // finally send out..
                uartStreamTransmitBuffer(0, ((STUHFL_CG_AL << 8) | STUHFL_CC_INVENTORY_DATA), data, *dataLen);
            }
        }
        else
#endif  /* STUHFL_HOST_COMMUNICATION */
        {
            // All slots processed
            slotRdPos += element;

            // update time for next heartbeat
            if(gReportOptions & INVENTORYREPORT_HEARTBEAT){
                gLastHeartBeatTick = HAL_GetTick();
            }

            if(inventorySlotCallback){
                inventorySlotCallback(slotInfoData);
            }
        }
    }

    //
    return ERR_NONE;
}
#endif  // USE_INVENTORY_EXT

static bool tagFoundCallback(tag_t *tag)
{
    // add into tag history queue 
    uint16_t tagIdx = 0;
    uint8_t  len;

    // update number of tags
    if (cycleData->tagListSize < cycleData->tagListSizeMax) {
        cycleData->tagListSize++;
        StoreTestData(TEST_ID_INVENTORY_LISTSIZE, sizeof(cycleData->tagListSize), (uint8_t *)&cycleData->tagListSize);
    }
#if STUHFL_HOST_COMMUNICATION
    if (gInventoryFromHost) {    
        tagIdx = tagWrPos % cycleData->tagListSizeMax;    
    }
    else
#endif  // STUHFL_HOST_COMMUNICATION
    {
        tagIdx = cycleData->tagListSize - 1;    /* If exceeds tagListSizeMax limit, all remaining tags will overwrite last tag entry   */
    }
    tagWrPos++; // tagWrPos is incremented even with gInventoryFromHost==false as is used to check the call to inventoryCycleCallback()

    cycleData->tagList[tagIdx].antenna = usedAntenna;
    cycleData->tagList[tagIdx].timestamp = tag->timeStamp;
    cycleData->tagList[tagIdx].agc = tag->agc;
    cycleData->tagList[tagIdx].rssiLogI = tag->rssiLogI;
    cycleData->tagList[tagIdx].rssiLogQ = tag->rssiLogQ;
    cycleData->tagList[tagIdx].rssiLinI = tag->rssiLinI;
    cycleData->tagList[tagIdx].rssiLinQ = tag->rssiLinQ;
    cycleData->tagList[tagIdx].pc[0] = tag->pc[0];
    cycleData->tagList[tagIdx].pc[1] = tag->pc[1];        

    // EPC
    len = tag->epclen;
    if(len){
        if(len > MAX_EPC_LENGTH) {
            len = MAX_EPC_LENGTH;
        }
        memcpy(cycleData->tagList[tagIdx].epc.data, tag->epc, len);
    }
    cycleData->tagList[tagIdx].epc.len = len;

    // XPC
    len = tag->xpclen;
    if(len){
        if(len > MAX_XPC_LENGTH){
            len = MAX_XPC_LENGTH;
        }
        memcpy(cycleData->tagList[tagIdx].xpc.data, tag->xpc, len);
    }
    cycleData->tagList[tagIdx].xpc.len = len;

    // TID
    len = readTIDinInventoryRound ? tag->tidlen : 0;
    if(len){
        if(len > MAX_TID_LENGTH){
            len = MAX_TID_LENGTH;
        }
        memcpy(cycleData->tagList[tagIdx].tid.data, tag->tid, len);
    }
    cycleData->tagList[tagIdx].tid.len = len;
    
    StoreTestData(TEST_ID_INVENTORYRUNNERSTART, sizeof(STUHFL_T_Inventory_Tag), (uint8_t *)&cycleData->tagList[tagIdx]);
    StoreTestData(TEST_ID_GEN2_INVENTORY,       sizeof(STUHFL_T_Inventory_Tag), (uint8_t *)&cycleData->tagList[tagIdx]);
#if STUHFL_HOST_COMMUNICATION
    CheckAndStoreTestDataMaxStoredTags((gInventoryFromHost) ? tagWrPos - tagRdPos : cycleData->tagListSize);
#else
    CheckAndStoreTestDataMaxStoredTags(cycleData->tagListSize);
#endif

    // data available, 
    // lets handover and store data or if storage is full send out..
    handleInventoryData(false, false);
    
    // return true if further TAGs could be processed
    return true;
}

static void slotFinishedCallback(uint32_t slotTime, uint16_t eventMask, uint8_t Q)
{
    // make timestamps relative to first executed slot
    if(gStartScanTimestamp == 0){
        gStartScanTimestamp = slotTime;
        gLastHeartBeatTick = slotTime;
    }

#ifdef USE_INVENTORY_EXT
    // process slot statistics
    if(gSlotStatistics){
        uint32_t deltaSlotCnt = ((slotWrPos - slotRdPos) % INVENTORYREPORT_SLOT_INFO_LIST_SIZE);
        if((INVENTORYREPORT_SLOT_INFO_LIST_SIZE - deltaSlotCnt) == 16){
            // in case we are near to a full slot info list we flush ready enough the already found
            // TAG statistics data, if any TAGs already available
            // This ensures that sending of TAG and SLOT statistics will not block each other ..
            if((tagWrPos - tagRdPos) > 0){
                handleInventoryData(false, true);
            }
        }        

        // sync info
        if((slotWrPos - slotRdPos) == 0){
            // prepare sync values
            slotInfoData->slotSync.slotIdBase = slotWrPos;
            slotInfoData->slotSync.timeStampBase = (slotTime - gStartScanTimestamp);
            gPrevSlotTime = slotTime; 
        }
        // slot data
        slotInfoData->slotInfoList[slotWrPos % INVENTORYREPORT_SLOT_INFO_LIST_SIZE].eventMask = eventMask;
        slotInfoData->slotInfoList[slotWrPos % INVENTORYREPORT_SLOT_INFO_LIST_SIZE].Q = Q;
        slotInfoData->slotInfoList[slotWrPos % INVENTORYREPORT_SLOT_INFO_LIST_SIZE].sensitivity = st25RU3993GetLastSetSensitivity();
        slotInfoData->slotInfoList[slotWrPos % INVENTORYREPORT_SLOT_INFO_LIST_SIZE].deltaT = (slotTime - gPrevSlotTime);
        gPrevSlotTime = slotTime;    
        slotWrPos++;    
        // send out when list is full
        handleInventorySlotData(false);
    }
#endif  // USE_INVENTORY_EXT

    if(gReportOptions & INVENTORYREPORT_HEARTBEAT){
        // send out heartbeat
        if (gLastHeartBeatTick + INVENTORYREPORT_HEART_BEAT_DURATION_MS <= slotTime) {
            gLastHeartBeatTick = slotTime;
            handleInventoryData(true, true);
        }
    }

#ifdef ADAPTIVE_SENSITIVITY
    if(gAdaptiveSensitivityEnable && (gCurrentSession == SESSION_GEN2)){        
        static uint32_t gSensitivityJudgeCnt = 0;
    
        //
        // Slot status
        //
        if(eventMask & (EVENT_TAG_FOUND | EVENT_COLLISION)){
            gSensitivityJudgeCnt = 0;
        }
        if(eventMask & EVENT_EMPTY_SLOT){
            gSensitivityJudgeCnt++;
            if((gSensitivityJudgeCnt % gAdaptiveSensitivityInterval) == 0){
                // X empty slots in a row
                st25RU3993IncreaseSensitivity();
            }
        }
        
        //
        // Error status
        //
        if(eventMask & (EVENT_PREAMBLE_ERR | EVENT_CRC_ERR | EVENT_RX_COUNT_ERR | EVENT_STOPBIT_ERR)){
            st25RU3993DecreaseSensitivity();
            gSensitivityJudgeCnt = 0;
        }
        if(eventMask & (EVENT_HEADER_ERR | EVENT_SKIP_FOLLOW_CMD)){
            // Do nothing
        }

        //
        // Actions
        //
        if(eventMask & EVENT_RESEND_ACK){
            st25RU3993DecreaseSensitivity();
            gSensitivityJudgeCnt = 0;
        }
//        if(eventMask & EVENT_QUERY_REP){
//        }
    }
#endif

}

static void inventoryGen2(void)
{
    uint16_t tagCnt = 0;
    
    gen2SearchForTagsParams_t p;
    
    tag_t tag;
    p.tag = &tag;
    p.adaptiveQ = &gGen2AdaptiveQSettings;    
    p.statistics = &cycleData->statistics;

    p.toggleSession = true;
    p.singulate = fastInventory ? false : true;
    
    p.cbTagFound = tagFoundCallback;
    p.cbSlotFinished =  slotFinishedCallback;
    p.cbContinueScanning = continueCheckTimeout;
    p.cbFollowTagCommand = NULL;
    
    ResetAndStoreTestDataMaxStoredTags();
    StoreTestData(TEST_ID_INVENTORY_LISTSIZE, sizeof(cycleData->tagListSize), (uint8_t *)&cycleData->tagListSize);

    //
    inventoryResult = hopFrequencies();

    // To be done after hopFrequencies()
    cycleData->statistics.tuningStatus = getTuningStatus();
    StoreTestData(TEST_ID_INVENTORY_STATISTICS, sizeof(STUHFL_T_Inventory_Statistics), (uint8_t *)&cycleData->statistics);   // done after hopFrequencies()

    if(numSelects > 0){
        // Use SL flag to only address specific tags
        gen2Configuration.sel = 0x3;
    }
    else{
        // inventory used on all tags
        gen2Configuration.sel = 0x0;
    }
    if(gen2Configuration.session == GEN2_SESSION_S0){
        gen2Configuration.target = 0;
    }
    gen2Configure(&gen2Configuration);
    
    if(inventoryResult == ERR_NONE)
    {
        st25RU3993SingleWrite(ST25RU3993_REG_STATUSPAGE, rssiMode);     // 0x29
        if(rssiMode == RSSI_MODE_PEAK){
            //if we use peak rssi mode, we have to send anti collision commands
            st25RU3993SingleCommand(ST25RU3993_CMD_ANTI_COLL_ON);
        }
        if(readTIDinInventoryRound){
            p.cbFollowTagCommand = gen2ReadTID;
        }

#ifdef ADC
        adcStartOneMeasurement();
#endif

        do
        {   
            // update statistics first to have data already available with the first found TAGs
            cycleData->statistics.sensitivity = st25RU3993GetSensitivity();
            cycleData->statistics.frequency = (profile == PROFILE_NEWTUNING) ? gChannelListTuning[usedAntenna].frequencies.freq[ gChannelListTuning[usedAntenna].freqIndex ] : frequencies[profile].freq[freqIndex];
            cycleData->statistics.tuningStatus = getTuningStatus();
            StoreTestData(TEST_ID_INVENTORY_STATISTICS, sizeof(STUHFL_T_Inventory_Statistics), (uint8_t *)&cycleData->statistics);

            //
            // Search for TAGs ..
            //
            performSelectsGen2();
            tagCnt = gen2SearchForTags(!autoAckMode, &p);
            
            // update counters
            increaseRoundCounter();
            
            // update LEDs
            if(tagCnt){ 
                SET_TAG_LED_ON(); 
                SET_NORESP_LED_OFF(); 
            }
            else{ 
                SET_TAG_LED_OFF(); 
                SET_NORESP_LED_ON(); 
            } 
            
            // toggle Target ?
            if(gen2Configuration.toggleTarget){                
                if(targetDepletionMode){
                    // in target deplition mode toggle only when no TAGs and no collisions occured
                    if((!tagCnt) && (!cycleData->statistics.collisionCnt)){
                        // change target (A --> B or B --> A)
                        gen2Configuration.target = gen2Configuration.target ? 0x00 : 0x01;
                        gen2Configure(&gen2Configuration);
                    }
                }
                else{
                    // change target (A --> B or B --> A)
                    gen2Configuration.target = gen2Configuration.target ? 0x00 : 0x01;
                    gen2Configure(&gen2Configuration);
                }                
            }

            // check if finished and terminate 
            if (scanDuration && (cycleData->statistics.roundCnt >= scanDuration)) {
                // requested rounds executed: stops inventory
                cyclicInventory = false;
            }
#if STUHFL_HOST_COMMUNICATION
            else {
                if (gInventoryFromHost) {
                    // check if any cmd received ..
                    ProcessIO();    // process received commands, WARNING: received InventoryRunnerStop() may set cyclicInventory=false & gInventoryFromHost=false
                }   
            }
#endif
        } while(cyclicInventory && continueCheckTimeout());

        // In case next round is done on other antenna/frequency: Flush out last pending statistics & tags data ...
#ifdef USE_INVENTORY_EXT
        if (gSlotStatistics){
            handleInventorySlotData(true);
        }        
#endif
        handleInventoryData(false, true);   // must be called after handleInventorySlotData(..) here

#ifdef ADC
        if(adcValueReady()){
            cycleData->statistics.adc = adcGetValue();
            adcStartOneMeasurement();
        }
#endif

        if(rssiMode == RSSI_MODE_PEAK){
            //if we use peak rssi mode, we have to send anti collision commands
            st25RU3993SingleCommand(ST25RU3993_CMD_ANTI_COLL_OFF);
        }
    }
  
    // notify finished
    if(inventoryFinishedCallback){
        inventoryFinishedCallback(cycleData);
    }

    // change frequency
    hopChannelRelease();

    if(cyclicInventory){
        // wait until max. allocation has been reached
        while(continueCheckTimeoutMaxAllocation()) {
            __NOP();
        }
        delay_ms(inventoryDelay);
    }
    else{
        setInventoryFromHost(false, false);

        SET_CRC_LED_OFF();
        SET_NORESP_LED_OFF();

        SET_TAG_LED_OFF();
        SET_TUNING_LED_OFF();
    }


    cycleData->statistics.tuningStatus = getTuningStatus();
    StoreTestData(TEST_ID_INVENTORY_STATISTICS, sizeof(STUHFL_T_Inventory_Statistics), (uint8_t *)&cycleData->statistics);
}



















































/*------------------------------------------------------------------------- */
/*------------------------------------------------------------------------- */
/*------------------------------------------------------------------------- */
/*---------------------------   GBT   ------------------------------------- */
/*------------------------------------------------------------------------- */
/*------------------------------------------------------------------------- */
/*------------------------------------------------------------------------- */
#if GB29768
/*------------------------------------------------------------------------- */
STUHFL_T_RET_CODE Gb29768_Inventory(STUHFL_T_Inventory_Option *invOption, STUHFL_T_Inventory_Data *invData)
{
    return Gen2_Inventory(invOption, invData);
}

/*------------------------------------------------------------------------- */
static int8_t gb29768ReadTID(tag_t *tag)
{
    uint8_t     buf[MAX_READ_DATA_LEN];
    uint16_t    datalen = 0;
    int8_t      ret;
    
    tag->tidlen=0;

    ret = gb29768AccessTag(tag, GB29768_AREA_TAGINFO, PW_CATEGORY_READ_L, NULL);        // No Password for TAG Info area
    if (ret == GB29768_OKAY) {
        ret = gb29768AccessTag(tag, GB29768_AREA_TAGINFO, PW_CATEGORY_READ_H, NULL);    // No Password for TAG Info area
        if(ret == GB29768_OKAY){
            ret = gb29768ReadFromTag(tag, GB29768_AREA_TAGINFO, 0, &datalen, buf);        // if size is 0, length is updated to read size
            if (ret == GB29768_OKAY) {
                tag->tidlen = (datalen < MAX_TID_LENGTH) ? datalen : MAX_TID_LENGTH;
                memcpy(tag->tid, buf, tag->tidlen);
            }
        }
    }
    if (ret != ERR_NONE) {
        ret = ERR_CHIP_NORESP;
    }
    return ret;
}

/*------------------------------------------------------------------------- */
static void inventoryGb29768(void)
{    
    uint16_t tagCnt = 0;

    gb29768_SearchForTagsParams_t p;
    tag_t tag;
    
    p.tag = &tag;
    p.statistics = &cycleData->statistics;

    memcpy(&p.antiCollision, &gGbtAntiCollision, sizeof(gb29768_Anticollision_t));

    p.toggleSession = true;
    p.singulate = true;
    
    p.cbTagFound = tagFoundCallback;
    p.cbSlotFinished = slotFinishedCallback;
    p.cbContinueScanning = continueCheckTimeout;
    p.cbFollowTagCommand = NULL;
    
    ResetAndStoreTestDataMaxStoredTags();
    StoreTestData(TEST_ID_INVENTORY_LISTSIZE, sizeof(cycleData->tagListSize), (uint8_t *)&cycleData->tagListSize);

    //
    inventoryResult = hopFrequencies();

    // To be done after hopFrequencies()
    cycleData->statistics.tuningStatus = getTuningStatus();
    StoreTestData(TEST_ID_INVENTORY_STATISTICS, sizeof(STUHFL_T_Inventory_Statistics), (uint8_t *)&cycleData->statistics);   // done after hopFrequencies()

    // set condition
    gb29768Config.condition = GB29768_CONDITION_ALL;
    for (uint32_t i=0 ; i<numSorts ; i++) {
        if(sortParams[i].target == GB29768_TARGET_MATCHINGFLAG){
            // MATCHINGFLAG has been selected
            // enable matching flag only on flags set to 1 considering select rule=GB29768_RULE_MATCH0_ELSE_1 (0x03) is used to invert selection
            gb29768Config.condition = GB29768_CONDITION_FLAG1;
            break;
        }
    }
    if(gb29768Config.session == GB29768_SESSION_S0){
        gb29768Config.target = GB29768_TARGET_0;
    }
    gb29768Configure(&gb29768Config);
    
    if(inventoryResult == ERR_NONE)
    {
        if(readTIDinInventoryRound) {
            p.cbFollowTagCommand = gb29768ReadTID;
        }

#if 0
        st25RU3993SingleWrite(ST25RU3993_REG_STATUSPAGE, rssiMode);     // 0x29
        if(rssiMode == RSSI_MODE_PEAK){
            //if we use peak rssi mode, we have to send anti collision commands
            st25RU3993SingleCommand(ST25RU3993_CMD_ANTI_COLL_ON);
        }
#endif

#ifdef ADC
        adcStartOneMeasurement();
#endif
        do
        {
            // update rest of statistics
            cycleData->statistics.sensitivity = st25RU3993GetSensitivity();
            cycleData->statistics.frequency = (profile == PROFILE_NEWTUNING) ? gChannelListTuning[usedAntenna].frequencies.freq[ gChannelListTuning[usedAntenna].freqIndex ] : frequencies[profile].freq[freqIndex];
            cycleData->statistics.tuningStatus = getTuningStatus();
            StoreTestData(TEST_ID_INVENTORY_STATISTICS, sizeof(STUHFL_T_Inventory_Statistics), (uint8_t *)&cycleData->statistics);

            // Search for TAGs ..
            st25RU3993EnterDirectMode();
            performSelectsGb29768();
            tagCnt = gb29768SearchForTags(&p);
            st25RU3993ExitDirectMode();

            // update counters
            increaseRoundCounter();
            
            // update LEDs
            if(tagCnt){
                SET_TAG_LED_ON();
                SET_NORESP_LED_OFF();
            }
            else{
                SET_TAG_LED_OFF();
                SET_NORESP_LED_ON();
            }

            // toggle Target ?
            if(gb29768Config.toggleTarget)
            {                
                if(targetDepletionMode){
                    // in target deplition mode toggle only when no TAGs and no collisions occured
                    if((!tagCnt) && (!cycleData->statistics.collisionCnt)){
                        // change target (A --> B or B --> A)
                        gb29768Config.target = gb29768Config.target ? GB29768_TARGET_0 : GB29768_TARGET_1;
                        gb29768Configure(&gb29768Config);
                    }
                }
                else{
                    // change target (A --> B or B --> A)
                    gb29768Config.target = gb29768Config.target ? GB29768_TARGET_0 : GB29768_TARGET_1;
                    gb29768Configure(&gb29768Config);
                }                
            }

            if (scanDuration && (cycleData->statistics.roundCnt >= scanDuration)) {
                // requested rounds executed: stops inventory
                cyclicInventory = false;
            }
#if STUHFL_HOST_COMMUNICATION
            else {
                if (gInventoryFromHost) {
                    // check if any cmd received ..
                    ProcessIO();    // process received commands, WARNING: received InventoryRunnerStop() may set cyclicInventory=false & gInventoryFromHost=false
                }
            }
#endif
        } while(cyclicInventory && continueCheckTimeout());

        // In case next round is done on other antenna/frequency: Flush out last pending statistics & tags data ...
#ifdef USE_INVENTORY_EXT
        if (gSlotStatistics){
            handleInventorySlotData(true);
        }
#endif
        handleInventoryData(false, true);   // must be called after handleInventorySlotData(..) here

#ifdef ADC
        if(adcValueReady())
        {
            cycleData->statistics.adc = adcGetValue();
            adcStartOneMeasurement();
        }
#endif

#if 0
        if(rssiMode == RSSI_MODE_PEAK){
            //if we use peak rssi mode, we have to send anti collision commands
            st25RU3993SingleCommand(ST25RU3993_CMD_ANTI_COLL_OFF);
        }
#endif
    }

    // notify finished
    if(inventoryFinishedCallback) {
        inventoryFinishedCallback(cycleData);
    }

    hopChannelRelease();

    // wait until max. allocation has been reached
    if(cyclicInventory) {
        while(continueCheckTimeoutMaxAllocation()) {
            __NOP();
        }
        delay_ms(inventoryDelay);
    }
    else {
        setInventoryFromHost(false, false);

        SET_CRC_LED_OFF();
        SET_NORESP_LED_OFF();

        SET_TAG_LED_OFF();
        SET_TUNING_LED_OFF();
    }

    cycleData->statistics.tuningStatus = getTuningStatus();
    StoreTestData(TEST_ID_INVENTORY_STATISTICS, sizeof(STUHFL_T_Inventory_Statistics), (uint8_t *)&cycleData->statistics);
}

/*------------------------------------------------------------------------- */
static void performSelectsGb29768(void)
{
    for(uint32_t i=0 ; i<numSorts ; i++) {
        gb29768Sort(&sortParams[i]);
    }
}

/*------------------------------------------------------------------------- */
STUHFL_T_RET_CODE Gb29768_Sort(STUHFL_T_Gb29768_Sort *sortData)
{
    // Special case for mode = CLEAR_LIST, ignore all other parameters => no parameter check needed
    if(sortData->mode == GB29768_SELECT_MODE_CLEAR_LIST){
        powerUpReader();
        checkAndOpenSession(SESSION_GB29768);
        powerDownReader();

        numSorts = 0;

        StoreTestData(TEST_ID_GB29768_SORT, 0, (uint8_t *)sortParams);  // Reset tests data
        return ERR_NONE;
    }
    
    if (   (((sortData->storageArea & 0xF0) != GB29768_AREA_USER) && ((sortData->storageArea & 0xCF) != 0x00))
        || (sortData->target > GB29768_TARGET_MATCHINGFLAG)
        || (sortData->rule > GB29768_RULE_MATCH0_ELSE_1)
        || (sortData->mode > GB29768_SELECT_MODE_CLEAR_AND_ADD)
        ) {
        StoreTestData(TEST_ID_GB29768_SORT, 0, (uint8_t *)sortParams);  // Reset tests data
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    powerUpReader();
    checkAndOpenSession(SESSION_GB29768);
    if(sortData->mode == GB29768_SELECT_MODE_ADD2LIST){
        if(numSorts >= MAX_SELECTS){
            numSorts = MAX_SELECTS;
            powerDownReader();

            StoreTestData(TEST_ID_GB29768_SORT, 0, (uint8_t *)sortParams);  // Reset tests data
            return (STUHFL_T_RET_CODE)ERR_NOMEM;
        }
    }
    else if(sortData->mode == GB29768_SELECT_MODE_CLEAR_AND_ADD){
        numSorts = 0;
    }
    
    sortParams[numSorts].target = sortData->target;
    sortParams[numSorts].rule = sortData->rule;
    sortParams[numSorts].storageArea = sortData->storageArea;
    sortParams[numSorts].bitPointer = sortData->bitPointer;
    sortParams[numSorts].bitLength = sortData->bitLength;
    memcpy(sortParams[numSorts].mask, sortData->mask, sortParams[numSorts].bitLength/8);

    StoreTestData(TEST_ID_GB29768_SORT, sizeof(gb29768SortParams_t), (uint8_t *)&sortParams[numSorts]);
    
    initTagInfo();
    numSorts++;
    powerDownReader();
    return ERR_NONE;
}

/*------------------------------------------------------------------------- */
STUHFL_T_RET_CODE Gb29768_Write(STUHFL_T_Write *writeData)
{
    int8_t      status;
    uint8_t     storageArea = writeData->memBank;
    uint32_t    wordPtr = writeData->wordPtr;
    uint32_t    trials = 8;

    if (((storageArea & 0xF0) != GB29768_AREA_USER) && ((storageArea & 0xCF) != 0x00)) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    do
    {
        status = startTagConnection();
        if(status == ERR_NONE){
            status  = gb29768AccessTag(selectedTag, storageArea, PW_CATEGORY_WRITE_L, &(writeData->pwd[0]));
            if (status == GB29768_OKAY) {
                status = gb29768AccessTag(selectedTag, storageArea, PW_CATEGORY_WRITE_H, &(writeData->pwd[2]));
            }
            if(status == GB29768_OKAY){
                status = gb29768WriteToTag(selectedTag, storageArea, wordPtr, 1, writeData->data);
            }
        }
        stopTagConnection();

        trials--;
    } while((status != ERR_NONE) && !isGb29768ChipErrorCode(status) && (trials));


    StoreTestData(TEST_ID_GB29768_WRITE, (status == ERR_NONE) ? sizeof(STUHFL_T_Write) : 0, (uint8_t *)writeData);

    powerDownReader();
    return (STUHFL_T_RET_CODE)status;
}

/*------------------------------------------------------------------------- */
STUHFL_T_RET_CODE Gb29768_Read(STUHFL_T_Read *readData)
{
    int8_t      status = ERR_NONE;
    uint32_t    trials = 6;
    uint8_t     storageArea = readData->memBank;
    uint32_t    pointer = readData->wordPtr;
    uint16_t    datalen;

    if (((storageArea & 0xF0) != GB29768_AREA_USER) && ((storageArea & 0xCF) != 0x00)) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    if (readData->bytes2Read > MAX_READ_DATA_LEN) {
        readData->bytes2Read = MAX_READ_DATA_LEN;
    }
    datalen = readData->bytes2Read;
    memset(readData->data, 0, MAX_READ_DATA_LEN);
    do
    {
        status = startTagConnection();
        if(status == ERR_NONE){
            status  = gb29768AccessTag(selectedTag, storageArea, PW_CATEGORY_READ_L, &(readData->pwd[0]));
            if (status == GB29768_OKAY) {
                status = gb29768AccessTag(selectedTag, storageArea, PW_CATEGORY_READ_H, &(readData->pwd[2]));
            }
            if(status == GB29768_OKAY){
                status = gb29768ReadFromTag(selectedTag, storageArea, (uint16_t)pointer, &datalen, readData->data);        // if size is 0, length is updated to read size
                if (status == ERR_NONE) {
                    readData->bytes2Read = (uint8_t)datalen;
                }
                else {
                    // Check sending timeout
                    continueCheckTimeout();
                    if(maxSendingLimitTimedOut) {
                        status = GB29768_TAG_ACCESS_TIMEOUT_ERROR;
                    }
                }
            }
        }
        stopTagConnection();
        trials--;
    } while((status != ERR_NONE) && !isGb29768ChipErrorCode(status) && (trials));

    StoreTestData(TEST_ID_GB29768_READ, (status == ERR_NONE) ? sizeof(STUHFL_T_Read) : 0, (uint8_t *)readData);
    
    powerDownReader();
    return (STUHFL_T_RET_CODE)status;
}

/*------------------------------------------------------------------------- */
STUHFL_T_RET_CODE Gb29768_Lock(STUHFL_T_Gb29768_Lock *lockData)
{
    if (   (((lockData->storageArea & 0xF0) != GB29768_AREA_USER) && ((lockData->storageArea & 0xCF) != 0x00))
        || (lockData->configuration > GB29768_CONFIGURATION_SECURITYMODE)
        || ((lockData->configuration == GB29768_CONFIGURATION_ATTRIBUTE) && (lockData->action > GB29768_ACTION_ATTRIBUTE_UNREADUNWRITE))
        || ((lockData->configuration == GB29768_CONFIGURATION_SECURITYMODE) && (lockData->action == GB29768_ACTION_SECMODE_RESERVED))
        || ((lockData->configuration == GB29768_CONFIGURATION_SECURITYMODE) && (lockData->action > GB29768_ACTION_SECMODE_AUTH_SECCOMM))
        ) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    int8_t status = startTagConnection();
    if(status == ERR_NONE){
        status  = gb29768AccessTag(selectedTag, lockData->storageArea, PW_CATEGORY_LOCK_L, &(lockData->pwd[0]));
        if (status == GB29768_OKAY) {
            status = gb29768AccessTag(selectedTag, lockData->storageArea, PW_CATEGORY_LOCK_H, &(lockData->pwd[2]));
        }
        if(status == GB29768_OKAY){
            status = gb29768LockTag(selectedTag, lockData->storageArea, lockData->configuration, lockData->action);
        }
    }
    stopTagConnection();

    StoreTestData(TEST_ID_GB29768_LOCK, (status == ERR_NONE) ? sizeof(STUHFL_T_Gb29768_Lock) : 0, (uint8_t *)lockData);

    return (STUHFL_T_RET_CODE)status;
}

/*------------------------------------------------------------------------- */
STUHFL_T_RET_CODE Gb29768_Erase(STUHFL_T_Gb29768_Erase *eraseData)
{
    if (((eraseData->storageArea & 0xF0) != GB29768_AREA_USER) && ((eraseData->storageArea & 0xCF) != 0x00)) {
        return (STUHFL_T_RET_CODE)ERR_PARAM;
    }

    int8_t status = startTagConnection();
    if(status == ERR_NONE){
        status  = gb29768AccessTag(selectedTag, eraseData->storageArea, PW_CATEGORY_WRITE_L, &(eraseData->pwd[0]));       // Use WRITE passwd category
        if (status == GB29768_OKAY) {
            status = gb29768AccessTag(selectedTag, eraseData->storageArea, PW_CATEGORY_WRITE_H, &(eraseData->pwd[2]));   // Use WRITE passwd category
        }
        if(status == GB29768_OKAY){
            status = gb29768EraseTag(selectedTag, eraseData->storageArea, eraseData->bytePtr, eraseData->bytes2Erase);
        }
    }
    stopTagConnection();

    StoreTestData(TEST_ID_GB29768_ERASE, (status == ERR_NONE) ? sizeof(STUHFL_T_Gb29768_Erase) : 0, (uint8_t *)eraseData);

    return (STUHFL_T_RET_CODE)status;
}

/*------------------------------------------------------------------------- */
STUHFL_T_RET_CODE Gb29768_Kill(STUHFL_T_Kill *killData)
{
    int8_t status = startTagConnection();    
    if(status == ERR_NONE){
        status  = gb29768AccessTag(selectedTag, GB29768_AREA_SECURITY, PW_CATEGORY_KILL_L, &(killData->pwd[0]));      // Use Security area by default
        if (status == GB29768_OKAY) {
            status = gb29768AccessTag(selectedTag, GB29768_AREA_SECURITY, PW_CATEGORY_KILL_H, &(killData->pwd[2]));  // Use Security area by default
        }
        if(status == GB29768_OKAY){
            status = gb29768KillTag(selectedTag);
        }
    }
    stopTagConnection();

    StoreTestData(TEST_ID_GB29768_KILL, (status == ERR_NONE) ? sizeof(STUHFL_T_Kill) : 0, (uint8_t *)killData);

    return (STUHFL_T_RET_CODE)status;
}
#endif


/**
  * @}
  */
/**
  * @}
  */
