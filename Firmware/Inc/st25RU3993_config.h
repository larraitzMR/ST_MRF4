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
 *  @brief Configuration file for all AS99x firmware
 *
 *  Do configuration of the driver in here.
 */

/** @addtogroup ST25RU3993
  * @{
  */

#ifndef _ST25RU3993_CONFIG_H
#define _ST25RU3993_CONFIG_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "stuhfl.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#if (DISCOVERY + EVAL + JIGEN + ELANCE) > 1
    #error "FW is configured for more than 1 board at the same time"
    #error "Add 'DISCOVERY=1' to the project settings if you have a DISCOVERY board"
    #error "Add 'EVAL=1' to the project settings if you have a EVAL board"
    #error "Add 'JIGEN=1' to the project settings if you have a JIGEN board"
    #error "Add 'ELANCE=1' to the project settings if you have a ELANCE board"
#endif
#if DISCOVERY==0 && EVAL==0 && JIGEN==0 && ELANCE==0
    #error "Unknown board"
    #error "Add 'DISCOVERY=1' to the project settings if you have a DISCOVERY board"
    #error "Add 'EVAL=1' to the project settings if you have a EVAL board"
    #error "Add 'JIGEN=1' to the project settings if you have a JIGEN board"
    #error "Add 'ELANCE=1' to the project settings if you have a ELANCE board"
#endif

/** Define this to 1 if st25RU3993Initialize() should perform a proper selftest,
  testing connection ST25RU3993, crystal, pll, antenna */
#define ST25RU3993_DO_SELFTEST  1

/** Define this to 1 if you want debug messages in st25RU3993.c or want to have
 debug output on OAD pins*/
#define ST25RU3993_DEBUG        0

/*
******************************************************************************
* DEFINES (DO NOT CHANGE)
******************************************************************************
*/

/** Set this to 1 to enable gb29768 support. */
#if JIGEN
#define GB29768 0
#else
#define GB29768 1
#endif

#define FW_VERSION_MAJOR    2
#define FW_VERSION_MINOR    3
#define FW_VERSION_MICRO    0
#define FW_VERSION_BUILD    0

#define HW_VERSION_MAJOR    1
#define HW_VERSION_MINOR    1
#define HW_VERSION_MICRO    0
#define HW_VERSION_NANO     0

#if DISCOVERY
#define HARDWARE_ID         "DISCOVERY"
#define HARDWARE_ID_NUM     DISCOVERY_HW_ID
#endif
#if EVAL
#define HARDWARE_ID         "EVAL"
#define HARDWARE_ID_NUM     EVAL_HW_ID
#endif
#if JIGEN
#define HARDWARE_ID         "JIGEN"
#define HARDWARE_ID_NUM     JIGEN_HW_ID
#endif
#if ELANCE
#define HARDWARE_ID         "ELANCE"
#define HARDWARE_ID_NUM     ELANCE_HW_ID
#endif

/* Set the following configuration switches depending on the board setup:
 * INTVCO or EXTVCO
 * SINGLEINP or BALANCEDINP
 * ANTENNA_SWITCH if 2 antenna ports are available
 * TUNER if antenna tuning is available
 * TUNER_CONFIG configures which tuner caps are available (bitmask)
*/
#if DISCOVERY
    #define INTVCO
    #define BALANCEDINP
    #define ANTENNA_SWITCH
    #define TUNER
    #define TUNER_CONFIG_CIN
    #define TUNER_CONFIG_CLEN
    #define TUNER_CONFIG_COUT
#endif
#if EVAL
    #define INTVCO
    #define BALANCEDINP
    #define ANTENNA_SWITCH
    #define TUNER
    #define TUNER_CONFIG_CIN
    #define TUNER_CONFIG_CLEN
    #define TUNER_CONFIG_COUT
    #define ADC
    #define BTC
#endif
#if JIGEN
    #define INTVCO
    #define BALANCEDINP
    #define TUNER
    #define TUNER_CONFIG_CIN
    #define TUNER_CONFIG_CLEN
    #define TUNER_CONFIG_COUT
#endif
#if ELANCE
    #define INTVCO
    #define SINGLEINP
    #define ANTENNA_SWITCH
    #define TUNER
    #define ADC
    #define TUNER_CONFIG_CIN
    #define TUNER_CONFIG_CLEN
    #define TUNER_CONFIG_COUT
#endif
/*
******************************************************************************
* CHECK FOR CONFIGURATION ERRORS
******************************************************************************
*/
#if GB29768
    #define CRC16_ENABLE 1
    #define CRC5_ENABLE  1
#endif

#define CRC16_ENABLE 1

#endif

/**
  * @}
  */
