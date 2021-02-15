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
 *  @brief This file is the include file for the adc_driver.c file.
 *
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup ADC
  * @{
  */

#ifndef __ADC_DRIVER_H__
#define __ADC_DRIVER_H__

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#define ADC_HANDLE       hadc1

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

/** triggers one ADC measurement */
void adcStartOneMeasurement(void);

/** returns whether ADC measurement is complete */
uint8_t adcValueReady(void);

/** returns measured ADC value (12 bit, 0 - 4095) */
uint16_t adcGetValue(void);

/** automatically called from ADC IRQ once measurement is done, saves value */
void adcIsr(void);

#endif

/**
  * @}
  */
/**
  * @}
  */
