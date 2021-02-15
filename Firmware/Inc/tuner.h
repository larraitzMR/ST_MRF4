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

  #ifndef __TUNER_H__
#define __TUNER_H__

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include <stdbool.h>
#include "st25RU3993_config.h"
#include "stuhfl_dl_ST25RU3993.h"

/*
******************************************************************************
* STRUCTS
******************************************************************************
*/

#if ELANCE
#define MAX_DAC_VALUE   127
#else
#define MAX_DAC_VALUE   31
#endif

#ifdef ANTENNA_SWITCH
#if ELANCE
#define FW_NB_ANTENNA   4
#else
#define FW_NB_ANTENNA   2
#endif
#else
#define FW_NB_ANTENNA   1
#endif


/** @struct tuningTable_t
  * This struct stores the whole information of the tuning parameters. */
typedef struct
{
    /** number of entries in the table */
    uint8_t tableSize;
    /** currently active entry in the table. */
    uint8_t currentEntry;
    /** frequency which is assigned to this tune parameters. Note: Only updated with new entries */
    uint32_t freq[MAX_FREQUENCY];
    /** Cin  tuning parameter for all antennas */
    uint8_t cin[FW_NB_ANTENNA][MAX_FREQUENCY];
    /** Clen tuning parameter for all antennas */
    uint8_t clen[FW_NB_ANTENNA][MAX_FREQUENCY];
    /** Cout tuning parameter for all antennas */
    uint8_t cout[FW_NB_ANTENNA][MAX_FREQUENCY];
    /** Reflected power which was measured after last tuning. */
    uint16_t tunedIQ[FW_NB_ANTENNA][MAX_FREQUENCY];
} tuningTable_t;

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
/**
 * Simple automatic tuning function. This function tries to find an optimized tuner setting (minimal reflected power).
 * The function starts at the current tuner setting and modifies the tuner caps until a
 * setting with a minimum of reflected power is found. When changing the tuner further leads to
 * an increase of reflected power the algorithm stops.
 * Note that, although the algorithm has been optimized to not immediately stop at local minima
 * of reflected power, it still might not find the tuner setting with the lowest reflected
 * power. The algorithm of tunerMultiHillClimb() is probably producing better results, but it
 * is slower.
 * @param maxSteps Number of maximum steps which should be performed when searching for tuning optimum.
 */
void tunerOneHillClimb(STUHFL_T_ST25RU3993_Caps *caps, uint16_t *tunedIQ, bool doFalsePositiveDetection, uint8_t maxSteps);

/**
 * Sophisticated automatic tuning function. This function tries to find an optimized tuner setting (minimal reflected power).
 * The function splits the 3-dimensional tuner-setting-space (axis are Cin, Clen and Cout) into segments
 * and searches in each segment (by using tunerOneHillClimb()) for its local minimum of reflected power.
 * The tuner setting (point in tuner-setting-space) which has the lowest reflected power is
 * returned in parameter res.
 * This function has a much higher probability to find the tuner setting with the lowest reflected
 * power than tunerOneHillClimb() but on the other hand takes much longer.
 */
void tunerMultiHillClimb(STUHFL_T_ST25RU3993_Caps *caps, uint16_t *tunedIQ, bool doFalsePositiveDetection);

/**
 * Enhanced Sophisticated automatic tuning function. This function tries to find an optimized tuner setting (minimal reflected power).
 * The function splits the 3-dimensional tuner-setting-space (axis are Cin, Clen and Cout) into segments
 * and get reflected power for each of them. A tunerOneHillClimb() is then run on the 3 segments with minimum of reflected power.
 * The tuner setting (point in tuner-setting-space) which has the lowest reflected power is then
 * returned in parameter res.
 * This function has a much higher probability to find the tuner setting with the lowest reflected
 * power than tunerOneHillClimb() and is faster than tunerMultiHillClimb().
 */
void tunerEnhancedMultiHillClimb(STUHFL_T_ST25RU3993_Caps *caps, uint16_t *tunedIQ, bool doFalsePositiveDetection);

/**
 * Applies the values cin, clen and cout to the tuner network. The parameter values are
 * register values of the associated components not values in Farad.
 * The maximum value of val depends on the used component on the board. For the PE64904 the maximum value is 31.
 * @param cin Value which is applied to cap Cin of the tuner network.
 * @param clen Value which is applied to cap Clen of the tuner network.
 * @param cout Value which is applied to cap Cout of the tuner network.
 */
void tunerSetTuning(uint8_t cin, uint8_t clen, uint8_t cout);

void loadTuningTablesFromFlashAll(void);
void loadTuningTablesFromFlash(uint8_t p);
void loadChannelListFromFlash(uint8_t antenna, STUHFL_T_ST25RU3993_ChannelList *channelList);

void saveTuningTableToFlash(uint8_t p);
void saveChannelListToFlash(uint8_t antenna, STUHFL_T_ST25RU3993_ChannelList *channelList);

void clearFlashPage(uint8_t p);

void randomizeFrequencies(uint8_t p);

void initializeFrequencies(void);

void setDefaultTuningTable(uint8_t new_profile, tuningTable_t *table, uint32_t freq);

/**
 * Measures the reflected power by utilizing st25RU3993GetReflectedPower()
 * This function is used by the automatic tuning algorithms
 * to find the tuner setting with the lowest reflected power.
 *
 * @note Antenna has to be switched on manually before
 *
 * @return Reflected power measured with the current tuner settings.
 */
uint16_t tunerGetReflected(void);

void tunerSetCap(uint8_t component, uint8_t val);
void sweepCaps(void);

#endif

/**
  * @}
  */
/**
  * @}
  */
