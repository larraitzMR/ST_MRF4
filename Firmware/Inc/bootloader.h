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
 *  @brief Bootloader Code Header File
 *
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup Bootloader
  * @{
  */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

/*
 ******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************
 */

/** Checks if the Bootloader needs to be entered by checking a magic word
  * in the flash memory */
void bootloaderCheck(void);

/** Writes magic word in flash memory and triggers a soft reset */
void bootloaderEnterAndReset(void);

#endif

/**
  * @}
  */
/**
  * @}
  */
