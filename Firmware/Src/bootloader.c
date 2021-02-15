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
 *  @brief Bootloader Code
 *
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup Bootloader
  * @{
  */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include <stdint.h>
#include <string.h>
#if defined(STM32WB55xx)
#include "stm32wbxx_hal.h"
#endif
#if defined(STM32L476xx)
#include "stm32l4xx_hal.h"
#endif
#include "bootloader.h"

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
/*        Whole SRAM: 0x20000000 - 0x20017FFF
Bootloader-MagicWord: 0x20017FF0 - 0x20017FFF (16 bytes)
      Remaining SRAM: 0x20000000 - 0x20017FEF 
*/
#define SYS_MEM_ADDR                    0x1FFF0000
#define BOOTLOADER_MAGIC_INFO_ADDR      0x20017FF0

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */
/** magic word written on specific memory address to indicate bootloader should be started */
#if !JIGEN || !defined(STM32WB55xx)
const char* BtlMagicInfo = ".EnterBooloader.";
#endif  /*!JIGEN || !STM32WB55xx */

/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */
/** enters the bootloader on the specified memory address */
#if !JIGEN || !defined(STM32WB55xx)
static void bootloaderInit(void);
#endif  /*!JIGEN || !STM32WB55xx */

/*
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 */
#if !JIGEN || !defined(STM32WB55xx)
static void bootloaderInit(void)
{
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
    __set_MSP(*(__IO uint32_t*) SYS_MEM_ADDR);

    void (*Bootloader)(void) = (void(*)(void)) *((uint32_t *) (SYS_MEM_ADDR + 4));
    Bootloader();

    while(1);
}
#endif  /*!JIGEN || !STM32WB55xx */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */
void bootloaderCheck(void)
{
#if !JIGEN || !defined(STM32WB55xx)
    if(0 == memcmp((const void*)BOOTLOADER_MAGIC_INFO_ADDR, BtlMagicInfo, strlen(BtlMagicInfo)))
    {
        *(uint32_t *)BOOTLOADER_MAGIC_INFO_ADDR = 0;
        bootloaderInit();
    }
#endif  /*!JIGEN || !STM32WB55xx */
}

/*------------------------------------------------------------------------- */
void bootloaderEnterAndReset(void)
{
#if !JIGEN || !defined(STM32WB55xx)
    memcpy((void*)BOOTLOADER_MAGIC_INFO_ADDR, BtlMagicInfo, strlen(BtlMagicInfo));
    HAL_NVIC_SystemReset();

    while(1);
#endif  /*!JIGEN || !STM32WB55xx */
}

/**
  * @}
  */
/**
  * @}
  */
