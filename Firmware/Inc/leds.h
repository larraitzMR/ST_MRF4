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
 *  @brief Leds macros definition
 *
 *  This file defines all leds related macros in a platform agnostic mode.
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup Leds
  * @{
  */

#ifndef _LEDS_H
#define _LEDS_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "platform.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#if !JIGEN && !NO_LEDS
#define SET_TUNED_LED_ON()      RESET_GPIO(GPIO_LED_TUNED_PORT,GPIO_LED_TUNED_PIN)
#define SET_TUNED_LED_OFF()     SET_GPIO(GPIO_LED_TUNED_PORT,GPIO_LED_TUNED_PIN)
#define SET_TUNING_LED_ON()     RESET_GPIO(GPIO_LED_TUNING_PORT,GPIO_LED_TUNING_PIN)
#define SET_TUNING_LED_OFF()    SET_GPIO(GPIO_LED_TUNING_PORT,GPIO_LED_TUNING_PIN)
#define SET_TAG_LED_ON()        RESET_GPIO(GPIO_LED_TAG_PORT,GPIO_LED_TAG_PIN)
#define SET_TAG_LED_OFF()       SET_GPIO(GPIO_LED_TAG_PORT,GPIO_LED_TAG_PIN)
#define SET_PLL_LED_ON()        RESET_GPIO(GPIO_LED_PLL_PORT,GPIO_LED_PLL_PIN)
#define SET_PLL_LED_OFF()       SET_GPIO(GPIO_LED_PLL_PORT,GPIO_LED_PLL_PIN)
#define SET_RF_LED_ON()         RESET_GPIO(GPIO_LED_RF_PORT,GPIO_LED_RF_PIN)
#define SET_RF_LED_OFF()        SET_GPIO(GPIO_LED_RF_PORT,GPIO_LED_RF_PIN)
#else
#define SET_TUNED_LED_ON()      
#define SET_TUNED_LED_OFF()     
#define SET_TUNING_LED_ON()     
#define SET_TUNING_LED_OFF()    
#define SET_TAG_LED_ON()
#define SET_TAG_LED_OFF()
#define SET_PLL_LED_ON()        
#define SET_PLL_LED_OFF()       
#define SET_RF_LED_ON()         
#define SET_RF_LED_OFF()        
#endif

#if !NO_LEDS
#define SET_CRC_LED_ON()        RESET_GPIO(GPIO_LED_CRC_PORT,GPIO_LED_CRC_PIN)
#define SET_CRC_LED_OFF()       SET_GPIO(GPIO_LED_CRC_PORT,GPIO_LED_CRC_PIN)
#define SET_NORESP_LED_ON()     RESET_GPIO(GPIO_LED_NORESP_PORT,GPIO_LED_NORESP_PIN)
#define SET_NORESP_LED_OFF()    SET_GPIO(GPIO_LED_NORESP_PORT,GPIO_LED_NORESP_PIN)
#else
#define SET_CRC_LED_ON()    
#define SET_CRC_LED_OFF()   
#define SET_NORESP_LED_ON() 
#define SET_NORESP_LED_OFF()
#endif

#if !JIGEN && !NO_LEDS && !ELANCE
#define SET_OSC_LED_ON()        RESET_GPIO(GPIO_LED_OSC_PORT,GPIO_LED_OSC_PIN)
#define SET_OSC_LED_OFF()       SET_GPIO(GPIO_LED_OSC_PORT,GPIO_LED_OSC_PIN)
#else
#define SET_OSC_LED_ON()        
#define SET_OSC_LED_OFF()       
#endif

#if EVAL || DISCOVERY
#define SET_INTPA_LED_ON()      RESET_GPIO(GPIO_LED_INTPA_PORT,GPIO_LED_INTPA_PIN)
#define SET_INTPA_LED_OFF()     SET_GPIO(GPIO_LED_INTPA_PORT,GPIO_LED_INTPA_PIN)
#define SET_EXTPA_LED_ON()      RESET_GPIO(GPIO_LED_EXTPA_PORT,GPIO_LED_EXTPA_PIN)
#define SET_EXTPA_LED_OFF()     SET_GPIO(GPIO_LED_EXTPA_PORT,GPIO_LED_EXTPA_PIN)
#else
#define SET_INTPA_LED_ON()      
#define SET_INTPA_LED_OFF()     
#define SET_EXTPA_LED_ON()      
#define SET_EXTPA_LED_OFF()     
#endif

#if !JIGEN && !ELANCE
#define SET_ANT1_LED_ON()       RESET_GPIO(GPIO_LED_ANT1_PORT,GPIO_LED_ANT1_PIN)
#define SET_ANT1_LED_OFF()      SET_GPIO(GPIO_LED_ANT1_PORT,GPIO_LED_ANT1_PIN)
#define SET_ANT2_LED_ON()       RESET_GPIO(GPIO_LED_ANT2_PORT,GPIO_LED_ANT2_PIN)
#define SET_ANT2_LED_OFF()      SET_GPIO(GPIO_LED_ANT2_PORT,GPIO_LED_ANT2_PIN)
#else
#define SET_ANT1_LED_ON()       
#define SET_ANT1_LED_OFF()      
#define SET_ANT2_LED_ON()       
#define SET_ANT2_LED_OFF()      
#endif

/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/

#endif /* _LEDS_H */

/**
  * @}
  */
/**
  * @}
  */
