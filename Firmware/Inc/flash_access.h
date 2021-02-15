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
 *  @brief routines to erase, program and read flash on STM32L4
 *
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup Flash_Access
  * @{
  */

#ifndef FLASH_ACCESS_H
#define FLASH_ACCESS_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include "st25RU3993_config.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#if JIGEN && defined(STM32WB55xx)
#define STM32_FLASH_NPAGES          128
#define STM32_FLASH_PAGESIZE        4096
#define STM32_FLASH_BASE            0x08000000     // 0x08000000-0x0807FFFF: FLASH memory
#else
#define STM32_FLASH_NPAGES          512
#define STM32_FLASH_PAGESIZE        2048
#define STM32_FLASH_BASE            0x08000000     // 0x08000000-0x080FFFFF: FLASH memory
#endif

#define FLASH_PAGE_SIZE_IN_BYTES        STM32_FLASH_PAGESIZE
#define FLASH_PAGE_SIZE_IN_WORDS        ((FLASH_PAGE_SIZE_IN_BYTES)/2)

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/

/**
 *****************************************************************************
 *  @brief erases a flash page
 *
 *  Function to erase a complete flash page
 *****************************************************************************
 */
void flashErasePage(uint32_t addr, uint8_t numPages);

/**
 *****************************************************************************
 *  @brief programs 8 byte to flash
 *
 *  Function to program 8 byte to flash.
 *
 *  @param addr - is the 16-bit address page+row to be programmed
 *      The function increments the *addr automatically
 *  @param data - data to be written to flash
 *****************************************************************************
 */
void flashProgram64Bit(uint32_t *addr, const uint64_t data);

#endif /* FLASH_ACCESS_H */

/**
  * @}
  */
/**
  * @}
  */
