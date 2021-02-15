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
 *  @brief This file includes all functionality to use the ADC
 *
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup ADC
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "adc.h"
#include "platform.h"
#include "adc_driver.h"

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
static volatile uint8_t adcConversionDone = 1;
static volatile uint16_t adcValue = 0x0000;

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void adcStartOneMeasurement(void)
{
    
    if(adcConversionDone)
    {
        adcConversionDone = 0;
        HAL_ADC_Stop_IT(&ADC_HANDLE);
        HAL_ADC_Start_IT(&ADC_HANDLE);
    }
}

/*------------------------------------------------------------------------- */
uint8_t adcValueReady(void)
{
    return adcConversionDone;
}

/*------------------------------------------------------------------------- */
uint16_t adcGetValue(void)
{    
    return adcValue;
}

/*------------------------------------------------------------------------- */
void adcIsr(void)
{
    adcValue = HAL_ADC_GetValue(&ADC_HANDLE);
    adcConversionDone = 1;
    HAL_ADC_Stop_IT(&ADC_HANDLE);
}

/**
  * @}
  */
/**
  * @}
  */
