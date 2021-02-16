/**
  ******************************************************************************
  * @file           STUHFL_demo.h
  * @brief          Main program header file
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

#if !defined __STUHFL_DEMO_H
#define __STUHFL_DEMO_H

//
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

// --------------------------------------------------------------------------
// screen logging
void log2Screen(bool clearScreen, bool flush, const char* format, ...);

// Utilities
void toggleTuningMechanism(void);
void setupGen2Config(bool singleTag, bool freqHopping, int antenna);
void printTagList(STUHFL_T_Inventory_Option *invOption, STUHFL_T_Inventory_Data *invData);
void printTuning(void);


// Showcase: EVAL API module
void demo_evalAPI_GetInfo();
void demo_evalAPI_Gen2RdWr();
void demo_evalAPI_Gen2Inventory();

// Showcase: Inventories runner
void demo_InventoryRunner(uint32_t rounds, bool singleTag);

// Showcase basic functionality
void demo_GetVersion();
void demo_DumpRegisters();
void demo_WriteRegister(uint8_t addr, uint8_t data);
void demo_DumpRwdCfg();
void demo_Power(bool on);
void demo_resetFreqsTuning(uint8_t startVal);
void demo_tuneFreqs(uint8_t tuningAlgo);
void demo_evalAPI_Gen2GenericCommand(void);

// Showcase: logging
void demo_LogLowLevel(bool enable);

// Playground ..
void demo_Playground();

#if defined(WIN32) || defined(WIN64)
void demo_Interactive();
#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__STUHFL_DEMO_H
