/**
  ******************************************************************************
  * @file           STUHFL_demoInventoryRunner.c
  * @brief          Inventory Runner related demos
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
#include "stuhfl_platform.h"

#include "STUHFL_demo.h"

extern bool _kbhit(void);
extern int _getch(void);

void logInventory(STUHFL_T_ACTION_CYCLE_DATA data);

static STUHFL_T_RET_CODE InventoryCycle_DemoInvRunner(STUHFL_T_Inventory_Data *data);
static STUHFL_T_RET_CODE cbInventoryCycleFinished(STUHFL_T_Inventory_Data *cycleData);

static uint32_t startTickTime = 0;
static uint32_t totalTAGs = 0;

#define UPDATE_CYCLE_TIME_MS 400
static float maxReadRate = 0.0;
static float readRate = 0.0;

/**
  * @brief      Execute Inventory Runnner with infinite loop (ended with InventoryRunnerStop()).
  *
  * @param[in]  rounds: Inventory expected rounds (0: infinite loop)
  * @param[in]  singleTag: true: expect a single tag in teh field, false: otherwise
  *
  * @retval     None
  */
void demo_InventoryRunner(uint32_t rounds, bool singleTag)
{
    setupGen2Config(false, true, ANTENNA_1);

    // Set optimal configuration for current test according to detected tags
    STUHFL_T_ST25RU3993_Gen2Inventory_Cfg invGen2Cfg = STUHFL_O_ST25RU3993_GEN2INVENTORY_CFG_INIT();
    GetGen2InventoryCfg(&invGen2Cfg);
    invGen2Cfg.startQ = singleTag ? 0 : 4;
    invGen2Cfg.adaptiveQEnable = !singleTag;
    invGen2Cfg.targetDepletionMode = false;
    SetGen2InventoryCfg(&invGen2Cfg);

    //
    STUHFL_T_ST25RU3993_Gen2Protocol_Cfg gen2ProtocolCfg = STUHFL_O_ST25RU3993_GEN2PROTOCOL_CFG_INIT();
    GetGen2ProtocolCfg(&gen2ProtocolCfg);
    gen2ProtocolCfg.tari = GEN2_TARI_12_50;
    gen2ProtocolCfg.blf = GEN2_LF_320;
    gen2ProtocolCfg.coding = GEN2_COD_MILLER2;
    gen2ProtocolCfg.trext = true;
    SetGen2ProtocolCfg(&gen2ProtocolCfg);

    STUHFL_T_Inventory_Tag tagData[MAX_TAGS_PER_ROUND];

    // apply data storage location, where the found TAGs shall be stored
    STUHFL_T_Inventory_Data cycleData = STUHFL_O_INVENTORY_DATA_INIT();
    cycleData.tagList = tagData;
    cycleData.tagListSizeMax = MAX_TAGS_PER_ROUND;

    STUHFL_T_Inventory_Option invOption = STUHFL_O_INVENTORY_OPTION_INIT();
    invOption.roundCnt = rounds;        // Define expected rounds (0: infinite loop)
    invOption.reportOptions = INVENTORYREPORT_HEARTBEAT;     // Enable HeartBeat for read rate calculation accuracy

    // before we start initalize statistic counters
    startTickTime = 0;
    totalTAGs = 0;
    maxReadRate = 0.0;
    readRate = 0.0;

    // blocking call
    InventoryRunnerStart(&invOption, InventoryCycle_DemoInvRunner, cbInventoryCycleFinished, &cycleData);
}

/**
  * @brief      Inventory Runner run call back.
  *
  * @param[in]  data: Gen2 Inventory data
  *
  * @retval     ERR_NONE: No error happened
  * @retval     otherwise: An error occurred
  *
  */
static STUHFL_T_RET_CODE InventoryCycle_DemoInvRunner(STUHFL_T_Inventory_Data *data)
{
    static uint32_t localCycleTime = 0;
    uint32_t millis = getMilliCount();

    STUHFL_T_Inventory_Data *invData = ((STUHFL_T_Inventory_Data *)data);

    //if (invData->rfu == 0) {
    // count successful reads
    totalTAGs = invData->statistics.tagCnt;
    if (startTickTime == 0) {
        startTickTime = invData->tagListSize ? invData->tagList[0].timestamp : invData->statistics.timestamp;
    }

    // screen update should not be done too frequent, as long
    // it will block receiption during inventory loop
    if ((millis - localCycleTime) < UPDATE_CYCLE_TIME_MS) {
        return ERR_NONE;
    }
    localCycleTime = millis;

    logInventory(data);

#if defined(WIN32) || defined(WIN64)
    // 'q' can be pressed to end inventory
    if (_kbhit()) {
        if (_getch() == 'q') {
            InventoryRunnerStop();
        }
    }
#elif defined (POSIX)
#endif
    return ERR_NONE;
}

/**
  * @brief      Inventory Runner end call back.
  *
  * @param[in]  data: Gen2 Inventory data
  *
  * @retval     ERR_NONE: No error happened
  * @retval     otherwise: An error occurred
  *
  */
static STUHFL_T_RET_CODE cbInventoryCycleFinished(STUHFL_T_Inventory_Data *cycleData)
{
    log2Screen(false, true, "    Inventory stopped after %d cycles\n    Processing Inventory end ...\n", cycleData->statistics.roundCnt);
    return ERR_NONE;
}

/**
  * @brief      Ouputs Gen2 Inventory data <br>
  *             extra ouput details can be enabled with define SHOW_TAG_DETAIL
  *
  * @param[in]  data: Gen2 Inventory data
  *
  * @retval     None
  */
void logInventory(STUHFL_T_ACTION_CYCLE_DATA data)
{
    STUHFL_T_Inventory_Data *invData = ((STUHFL_T_Inventory_Data *)data);
    uint32_t duration = (invData->tagListSize ? invData->tagList[0].timestamp : invData->statistics.timestamp) - startTickTime;
    readRate = duration ? ((float)totalTAGs * ((float)1000.0 / (float)duration)) : (float)0.0;
    if (readRate > maxReadRate) {
        maxReadRate = readRate;
    }

    log2Screen(false, false, "--- Inventory Info ---\nDuration    : %d ms\n", duration);
    log2Screen(false, false, "#TAGs total : %d \n", totalTAGs);
    log2Screen(false, false, "ReadRate    : %.2f (Max: %.2f)\n\n", readRate, maxReadRate);

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

#if defined(SHOW_TAG_DETAIL)
    if (invData->tagListSize > 6) {
        log2Screen(false, false, "\n\n--- too many tags to display ---\n");
    } else {

        log2Screen(false, false, "--- TAG Info ---\n");

        // print transponder information for TagList
        for (int tags = 0; tags < invData->tagListSize ; tags++) {
            log2Screen(false, false, "\n\n--- %03d ---\n", tags + 1);
            log2Screen(false, false, "agc         : %d\n", invData->tagList[tags].agc);
            log2Screen(false, false, "rssiLogI    : %d\n", invData->tagList[tags].rssiLogI);
            log2Screen(false, false, "rssiLogQ    : %d\n", invData->tagList[tags].rssiLogQ);
            log2Screen(false, false, "rssiLinI    : %d\n", invData->tagList[tags].rssiLinI);
            log2Screen(false, false, "rssiLinQ    : %d\n", invData->tagList[tags].rssiLinQ);
            for (int i = 0; i < MAX_PC_LENGTH; i++) {
                log2Screen(false, false, "%02x ", invData->tagList[tags].pc[i]);
            }
            log2Screen(false, false, "\nepcLen      : %d\n", invData->tagList[tags].epclen);
            log2Screen(false, false, "epc         : ");
            for (int i = 0; i < min(MAX_EPC_LENGTH, invData->tagList[tags].epclen); i++) {
                log2Screen(false, false, "%02x ", invData->tagList[tags].epc[i]);
            }
            log2Screen(false, false, "\ntidLen      : %d\n", invData->tagList[tags].tidLen);
            log2Screen(false, false, "tid         : ");
            for (int i = 0; i < min(MAX_TID_LENGTH, invData->tagList[tags].tidLen); i++) {
                log2Screen(false, false, "%02x ", invData->tagList[tags].tid[i]);
            }
        }
    }
#endif

#if defined(WIN32) || defined(WIN64)
    log2Screen(false, false, "\n\nPress 'q' to end inventory ...");
#elif defined(POSIX)
#endif


    log2Screen(true, true, "");

}
