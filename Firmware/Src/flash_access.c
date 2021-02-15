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
 *  @brief Routines to erase, program and read flash on STM32L4
 *
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup Flash_Access
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "flash_access.h"
#include "platform.h"
#include "timer.h"
#include "logger.h"

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void flashErasePage(uint32_t addr, uint8_t numPages)
{
    FLASH_EraseInitTypeDef pEraseInit;
    uint32_t pageError;

    pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    pEraseInit.Page = ((addr-STM32_FLASH_BASE)/FLASH_PAGE_SIZE_IN_BYTES);
#if !JIGEN || !defined(STM32WB55xx)
    pEraseInit.Banks = (pEraseInit.Page > 255 ? FLASH_BANK_2 : FLASH_BANK_1);
#endif
    pEraseInit.NbPages = numPages;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    HAL_FLASHEx_Erase(&pEraseInit,&pageError);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    HAL_FLASH_Lock();

    delay_ms(25);
}

/*------------------------------------------------------------------------- */
void flashProgram64Bit(uint32_t *addr, const uint64_t data)
{
    HAL_FLASH_Unlock();
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, *addr, data);
    HAL_FLASH_Lock();

    *addr += 8;
    delay_ms(3);
}

/**
  * @}
  */
/**
  * @}
  */
