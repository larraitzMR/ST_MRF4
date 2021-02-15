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
 *  @brief This file provides declarations for tuner related functions.
 *
 */

/** @addtogroup ST25RU3993
  * @{
  */
/** @addtogroup Tuner
  * @{
  */

#ifndef __RFFE_H__
#define __RFFE_H__

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include "stm32l476xx.h"

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/
void RFFE_Init(GPIO_TypeDef *DataPort, uint16_t DataPin, GPIO_TypeDef *ClkPort, uint16_t ClkPin, GPIO_TypeDef *GpioPort, uint16_t GpioPin);
void RFFE_WriteRegister(uint8_t USID, uint8_t RegAddr, uint8_t Data);
void RFFE_ReadRegister(uint8_t USID, uint8_t RegAddr, uint8_t *Data);

#endif //__RFFE_H__

/**
  * @}
  */
/**
  * @}
  */
