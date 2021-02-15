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
 *  @brief This file is the include file for the appl_commands.c file.
 *
 *  This file contains defines for handling appl commands, eg: The defines for the
 *  index of command functions in call_ftk_() and expected rx and tx data sizes.
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup Commands
  * @{
  */

#ifndef __EVAL_API_COMMANDS_H__
#define __EVAL_API_COMMANDS_H__

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "st25RU3993_config.h"
#include "st25RU3993_public.h"

#include "stuhfl.h"
#include "stuhfl_helpers.h"
#include "stuhfl_evalAPI.h"

/** init */
void initCommands(void);

/** Cycle all cyclic operations */
void cycle(void);

/* */
STUHFL_T_RET_CODE handleInventoryData(bool skipTAGs, bool forceFlush);

/* */
STUHFL_T_RET_CODE inventoryRunnerSetup(STUHFL_T_Inventory_Option *invOption, STUHFL_T_InventoryCycle cycleCallback, STUHFL_T_InventoryFinished finishedCallback, STUHFL_T_Inventory_Data *invData);

STUHFL_T_RET_CODE setInventoryFromHost(bool fromHost, bool slotStatistics);



#endif //__EVAL_API_COMMANDS_H__

/**
  * @}
  */
/**
  * @}
  */
