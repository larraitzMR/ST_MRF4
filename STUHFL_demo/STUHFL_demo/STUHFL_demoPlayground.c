/**
  ******************************************************************************
  * @file           STUHFL_demoPlayground.c
  * @brief          Inventory Runner + Gen2 Inventory related demos
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

/**
  * @brief      Inventory Runner demo entry point with auto tags number detection
  *
  * @param[in]  rounds: Inventory expected rounds (0: infinite loop)
  *
  * @retval     None
  */
void demo_Playground()
{
    // place your code here ..

    uint32_t rounds = 2000;
    bool singleTag = false;
    // run inventory for 2000 rounds with multiple TAGs
    demo_InventoryRunner(rounds, singleTag);
}
