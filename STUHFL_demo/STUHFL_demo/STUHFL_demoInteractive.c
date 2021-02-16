/**
  ******************************************************************************
  * @file           STUHFL_demoInteractive.c
  * @brief          Interactive demo file.
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

#include "sysinfoapi.h"

extern bool _kbhit(void);
extern int _getch(void);

static int lastPrintMenuTime;

void printMenu(void)
{
    log2Screen(false, false, "\nChoose an action below:\n");
    log2Screen(false, false, "\tv: Version from HOST side\n");
    log2Screen(false, false, "\tT: Toggle tuning mechanism\n");
    log2Screen(false, false, "\tt: Tune frequencies\n");
    log2Screen(false, false, "\tg: Gen2Inventory from HOST side\n");
    log2Screen(false, false, "\ti: InventoryRunner from HOST side (1000 rounds)\n");
    log2Screen(false, false, "\tI: InventoryRunner from HOST side (infinite loop)\n");
    log2Screen(false, false, "\tr: Read/Write tag from HOST side\n");
    log2Screen(false, false, "\tG: Gen2 Generic Command from HOST side\n");
    log2Screen(false, false, "\n");
    log2Screen(false, false, "\tq: quit\n");
    log2Screen(true, true, "");

    lastPrintMenuTime = GetTickCount();
}

/**
  * @brief          Gen2 Inventory demo.<br>
  *                 Launch a Gen2 inventory and prints all detected tags
  *
  * @retval         None
  */
void demo_inventoryGen2(void)
{
    setupGen2Config(false, true, ANTENNA_1);

    // apply data storage location, where the found TAGs shall be stored
    STUHFL_T_Inventory_Tag tagData[MAX_TAGS_PER_ROUND];

    // Set inventory data and print all found tags
    STUHFL_T_Inventory_Data invData = STUHFL_O_INVENTORY_DATA_INIT();
    invData.tagList = tagData;
    invData.tagListSizeMax = MAX_TAGS_PER_ROUND;

    STUHFL_T_Inventory_Option invOption = STUHFL_O_INVENTORY_OPTION_INIT();  // Init with default values

    Gen2_Inventory(&invOption, &invData);

    printTagList(&invOption, &invData);
}

/**
  * @brief      Interactive demo (enabled by default). <br>
  *             Launch commands to Board from HOST through an interactive menu (cf below) <br>
  *             Menu:                                   <br>
  *               v: Version from HOST side             <br>
  *               g: Gen2Inventory from HOST side       <br>
  *               i: InventoryRunner from HOST side     <br>
  *               r: Read/Write tag from HOST side      <br>
  *               q: quit                               <br>
  *             <br>
  *             <b>Nota:</b> Menu selection can last up to 2s to be taken into account
  *
  * @param      None
  *
  * @retval     None
  */
void demo_Interactive()
{
    int key = 0;

    printMenu();

    while (key != 'q' && key != 'Q') {
        if ((GetTickCount() - lastPrintMenuTime) > 7*1000) {  // Wait 7s until screen is cleared
            printMenu();
        }

        if (_kbhit()) {
            bool pressAKey = false;

            printMenu();
            key = _getch();
            printf("%c\n\n", key);
            pressAKey = true;
            switch (key) {
            case 'v':
                printf("Getting FW version\n");
                demo_GetVersion();
                break;
            case 'g':
                printf("Launching Gen2 Inventory\n");
                demo_inventoryGen2();
                break;
            case 't':
                printf("Reset Tuning\n");
                demo_resetFreqsTuning(11);
                printf("Tuning Profile: algo: %d\n", TUNING_ALGO_MEDIUM);
                demo_tuneFreqs(TUNING_ALGO_MEDIUM);
                printTuning();
                break;
            case 'i':
                printf("Launching Inventory Runner for 1000 rounds ...\n");
                demo_InventoryRunner(1000, false);
                break;
            case 'I':
                printf("Launching Inventory Runner with infinite loop ...\n");
                demo_InventoryRunner(0, false);
                break;
            case 'r':
                printf("Reading/Writing data on tag\n");
                demo_evalAPI_Gen2RdWr();
                printf("\n");
                break;
            case 'G':
                printf("Gen2 Generic Command from HOST side\n");
                demo_evalAPI_Gen2GenericCommand();
                printf("\n");
                break;
            case 'T':
                printf("Toggling Tuning mechanism:\n");
                toggleTuningMechanism();
                break;

            case 'q':
                printf("Exiting\n");
                pressAKey = false;
                break;
            default:
                printMenu();
                pressAKey = false;
                break;
            }

            if (pressAKey) {
                printf("\nTask finished .. press a key to continue\n");
                key = _getch();
                printMenu();
            }
        }
    }
}
