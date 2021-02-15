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
 *  @brief Bitbang Engine
 *
 *  The bitbang module is needed to support serial protocols via bitbang technique.
 *
 *  Nota: The implementation of GB-29768 protocol through bitbang is currently 
 *        highly depend on the ARM feature: DWT.
 *        All STM32 currently use this feature, porting the current GB29768 bitbang 
 *        on other MCUs can only be ensured if this MCU has equivalent feature.
 *
 */

/** @addtogroup Protocol
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <string.h>
#include <stdbool.h>
#include "st25RU3993_config.h"
#include "platform.h"
#include "bitbang.h"
#include "timer.h"
#include "st25RU3993.h"
#include "leds.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#if GB29768

#define READ_PULSE_PIN()        ((GPIO_SPI_MISO_PORT->IDR & GPIO_SPI_MISO_PIN) ? 1 : 0)


#define MAX_PULSE_TIME_NS(blf)       (GB29768_TIMING_T1max_NS(blf) - GB29768_TIMING_T1min_NS(blf))  // Max waiting time for first short pulse
#define MEDIAN_PULSE_TIME_NS(blf)    (GB29768_Blf2NS(blf) / 2)                                      // (Tpri/2) = 1562ns @320kHz
#define LONG_PULSE_TIME_NS(blf)      (GB29768_Blf2NS(blf))                                          // Tpri = 3125ns  @320kHz
#define VIOLATION_PULSE_TIME_NS(blf) ((GB29768_Blf2NS(blf)*150) / 100)                              // Tpri*150% = 4688ns  @320kHz
#define FLUSH_PULSE_TIME_NS(blf)     ((GB29768_TIMING_T2min_NS(blf)*150) / 100)                     // Min pulse length to consider tag response is over
#define MAX_VERYLONG_WAIT_NS         20000000           // Wait 20 ms max for Write and Erase commands

#define MAX_PULSE_CYCLES(blf)        NS_TO_CYCLES( MAX_PULSE_TIME_NS(blf)    )
#define MEDIAN_PULSE_CYCLES(blf)     NS_TO_CYCLES( MEDIAN_PULSE_TIME_NS(blf) )
#define LONG_PULSE_CYCLES(blf)       NS_TO_CYCLES( LONG_PULSE_TIME_NS(blf)   )
#define VIOLATION_PULSE_CYCLES(blf)  NS_TO_CYCLES( VIOLATION_PULSE_TIME_NS(blf)   )
#define FLUSH_PULSE_CYCLES(blf)      NS_TO_CYCLES( FLUSH_PULSE_TIME_NS(blf)  )

#define MILLER2_DWTPULSELOOP_DURATION_CYCLES       20

#ifdef GB29768PULSES_DEBUG
#define DEBUG_STORE_PULSES_VAL(v)                              \
        if (gDebugPulsesNb < MAX_RESPONSE_PULSES_DEBUG) {      \
            gDebugPulses[gDebugPulsesNb] = (uint8_t)(v);       \
            gDebugPulsesNb++;                                  \
        }

#define SET_DEBUG_DELTA_VAL(val,delta)          (val) |= (uint8_t)((((delta) >> 2) << 1))       /* Add delta value for debug */

#define SET_DWTDELTA_IRQ_DEBUG_BIT(delta)       (delta) = (((delta) & 0xFFFFFFFC) | 0x04)       /* Debug: Reset bits 0&1 + set bit 2 (IRQ happened) */
#define RESET_DWTDELTA_IRQ_DEBUG_BIT(delta)     (delta) &= 0xFFFFFFF8                           /* Debug: Reset bits 0, 1 & 2 */
#else
#define DEBUG_STORE_PULSES_VAL(v)
#define SET_DEBUG_DELTA_VAL(val,delta)

#define SET_DWTDELTA_IRQ_DEBUG_BIT(delta)  
#define RESET_DWTDELTA_IRQ_DEBUG_BIT(delta)
#endif

#define MILLER2_GET_NEW_PULSE_DWT_NOPULSECHECK(delta,value,maxCycles)                              \
    {                                                                                               \
        uint8_t value_new;                                                                          \
        do {                                                                                        \
            value_new = READ_PULSE_PIN();                                                           \
            (delta) = GET_DWT();                                                                    \
        } while(((delta) < (maxCycles)) && (value_new == (value)));                                 \
        RESET_DWT();                                                                                \
        (value) = value_new;     /* Store pulse new value */                                        \
    }

#define MILLER_PULSE_PROCESSING_MAX_CYCLES      (60*4)      // Estimated 60 commands at maximum, multiplied by 4 cycles per command 
#define FM0_PULSE_PROCESSING_MAX_CYCLES         (60*4)      // Estimated 50 commands at maximum, multiplied by 4 cycles per command 
#define MILLER2_GET_NEW_PULSE_DWT(delta,value,maxCycles,missed,processingMaxCycles)                 \
    {                                                                                               \
        uint32_t    timeStamp = GET_DWT();                                                          \
        uint8_t     value_new;                                                                      \
        missed = false;                                                                             \
        if (timeStamp < processingMaxCycles) {                                                      \
            do {                                                                                    \
                timeStamp += MILLER2_DWTPULSELOOP_DURATION_CYCLES;                                  \
                                                                                                    \
                value_new = READ_PULSE_PIN();                                                       \
                (delta) = GET_DWT();                                                                \
            } while(((delta) < (maxCycles)) && (value_new == (value)));                             \
        }                                                                                           \
        else {                                                                                      \
            /* Cycles required to get back here are ABOVE the max number of cycles */               \
            /* => an IRQ has most probably happened */                                              \
            (delta) = timeStamp;                                                                    \
        }                                                                                           \
        RESET_DWT();                                                                                \
                                                                                                    \
        /* Error correction */                                                                      \
        if ((delta) >= timeStamp) {                                                                 \
            /* Delta is not in line with cycles spent in loop above */                              \
            /* => an IRQ has been processed in between */                                           \
            /* => remove cycles taken by IRQ */                                                     \
            (delta) >>= 1;                      /* Divide delta by 2 to emulate cycles removal */   \
            SET_DWTDELTA_IRQ_DEBUG_BIT(delta);                                                      \
                                                                                                    \
            /* Manage missed short pulse due to IRQ */                                              \
            value_new = READ_PULSE_PIN();                                                           \
            missed = (value_new == (value));   /* value has not changed => missed short pulse */    \
        }                                                                                           \
        else {                                                                                      \
            RESET_DWTDELTA_IRQ_DEBUG_BIT(delta);                                                    \
        }                                                                                           \
        (value) = value_new;     /* Store pulse new value */                                        \
    }
#endif  /* GB29768 */

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
#if GB29768
static uint8_t  gPulses[MAX_RESPONSE_PULSES];

#ifdef GB29768PULSES_DEBUG
uint8_t     gDebugPulses[MAX_RESPONSE_PULSES_DEBUG];
uint32_t    gDebugPulsesNb;
#endif
#endif

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
#if GB29768
/*------------------------------------------------------------------------- */
gb29768_error_t gb29768GetMillerBits(uint8_t *bits, uint32_t *nbBits, gb29768Config_t *gbCfg, uint8_t lastCommand)
{
    uint32_t    totalCounters = 0;
    uint32_t    index;
    uint32_t    leadPulses;
    uint8_t     currentPulse;
    uint8_t     previousPulse;
    uint8_t     val;
    bool        receivedFirstLongPulse = false;
    uint32_t    countDownToForcedLongPulse;
    uint32_t    maxPulseCycles;
    uint32_t    minLeadPulses;                              // Very minimum pulses to be received as lead code
    uint32_t    delta;
    bool        missedShortPulse;

    // Preprocessed values
    uint32_t    halfBitLength = (1 << gbCfg->coding) - 1;    // Bit length depends on Miller coding length
    uint32_t    bitLength = (1 << gbCfg->coding) * 2;
    uint32_t    bitLengthButOne = bitLength-1;

    uint32_t    aboveMedianPulseCycles = (MEDIAN_PULSE_CYCLES(gbCfg->blf)*135)/100;                  // (Tpri/2) => 1562 ns * 135% = 2100 ns  => Threshold between short and long pulses
    uint32_t    aboveLongPulseCycles = (LONG_PULSE_CYCLES(gbCfg->blf)*112)/100;                      // Tpri => 3125 ns * 112% = 3500 ns      => Threshold to exceed long pulse

    uint32_t    reducedAboveMedianPulseCycles = aboveMedianPulseCycles - aboveLongPulseCycles/10;    // (Tpri/2) & TPri => 2100 ns - 350 ns = 1750 ns
    uint32_t    reducedAboveLongPulseCycles = (aboveLongPulseCycles*90)/100;                         // TPri => 3500 ns * 90% = 3150 ns

    uint32_t    revisedAboveMedianPulseCycles = aboveMedianPulseCycles;

    *nbBits = 0;

    switch (lastCommand)
    {
        case GB29768_CMD_WRITE:
        case GB29768_CMD_ERASE:
        case GB29768_CMD_LOCK:
        case GB29768_CMD_KILL:
            maxPulseCycles = NS_TO_CYCLES(MAX_VERYLONG_WAIT_NS);
            minLeadPulses = bitLength * 16;             // Lead code is forced to TRext=1
            break;
        default:
            maxPulseCycles = MAX_PULSE_CYCLES(gbCfg->blf);
            minLeadPulses = bitLength * (gbCfg->trext ? 16 : 4);
            break;
    }

#ifdef GB29768PULSES_DEBUG
    gDebugPulsesNb = 0;
#endif

    INIT_DWT();

    // Wait for first edge
    currentPulse = READ_PULSE_PIN();
    MILLER2_GET_NEW_PULSE_DWT_NOPULSECHECK(delta, currentPulse, maxPulseCycles);                    // Caution: currentPulse, delta & missedShortPulse are modified by the macro
    totalCounters += delta;
    if (totalCounters > maxPulseCycles) {
        // Stability check timeout
        return GB29768_NO_RESPONSE;
    }

    // Check stability of signal and wait for first pulse
    do
    {
        previousPulse = currentPulse;
        MILLER2_GET_NEW_PULSE_DWT_NOPULSECHECK(delta, currentPulse, maxPulseCycles);                // Caution: currentPulse, delta & missedShortPulse are modified by the macro
        totalCounters += delta;
        if (totalCounters > maxPulseCycles) {
            // Stability check timeout
            return GB29768_NO_RESPONSE;
        }
    } while(delta >= aboveMedianPulseCycles);    // pulse is too large    

    DEBUG_STORE_PULSES_VAL(gbCfg->coding);                               // Store used coding
    DEBUG_STORE_PULSES_VAL(aboveMedianPulseCycles >> 1);                 // Store mediancycles (divided by 2 to fit in 8 bits)
    DEBUG_STORE_PULSES_VAL(aboveLongPulseCycles >> 1);                   // Store longcycles (divided by 2 to fit in 8 bits)
    SET_DEBUG_DELTA_VAL(previousPulse, delta);          // Add delta value for debug; Caution: previousPulse is modified by the macro
    DEBUG_STORE_PULSES_VAL(previousPulse);              // Store first pulse

    // Starts processing
    leadPulses = 1;
    maxPulseCycles = aboveLongPulseCycles;
    do
    {
        // ***********************************************************************
        //                CAUTION: this loop code is time critical !!!
        // Algorithm must be optimal and no function call is allowed (only macros)


        previousPulse = currentPulse;
        MILLER2_GET_NEW_PULSE_DWT(delta, currentPulse, maxPulseCycles, missedShortPulse, MILLER_PULSE_PROCESSING_MAX_CYCLES);                  // Caution: currentPulse, delta & missedShortPulse are modified by the macro

        // Store pulse at least as a short one
        val = previousPulse;
        SET_DEBUG_DELTA_VAL(val, delta);            // Add delta value for debug; Caution: val is modified by the macro

        // Check pulse duration
        if(delta > revisedAboveMedianPulseCycles) {      // Long pulse duration
            DEBUG_STORE_PULSES_VAL(val);

            // Starts bits management
            if (receivedFirstLongPulse) {
                gPulses[index++] = val;                      // Store pulse value
            }
            else {
                DEBUG_STORE_PULSES_VAL(0xFF);           // First long pulse beacon

                if (leadPulses < minLeadPulses) {
                    // Wrong lead code
                    return GB29768_NO_RESPONSE;
                }
                receivedFirstLongPulse = true;

                index = 0;

                // Consider first bit is received
                bits[0] = 0;
                *nbBits = 1;
            }
            countDownToForcedLongPulse = 4;         //  Max places between 2 potential long pulses is 4 whatever the Miller coding
        }

        DEBUG_STORE_PULSES_VAL(val);

        // Bits management
        if (receivedFirstLongPulse) {
            gPulses[index++] = val;                         // Store pulse value

            if (missedShortPulse) {
                val = previousPulse ? 0 : 1;                // Store inverted pulse value (missed pulse)
                SET_DEBUG_DELTA_VAL(val, delta);            // Add delta value for debug; Caution: val is modified by the macro
                DEBUG_STORE_PULSES_VAL(val);

                gPulses[index++] = val;
            }

            if (index >= bitLength) {
                bits[(*nbBits)++] = (gPulses[halfBitLength] == gPulses[halfBitLength+1]) ? 1 : 0;     // Middle pulses are equal => bit=1 else bit=0

                index %= bitLength;     // Process new bit: skip previous bit pulses ...
                switch(index) {         // ... and keep new bit ones
                    case 2: gPulses[1] = gPulses[bitLength+1];
                        /* Fall through */
                    case 1: gPulses[0] = gPulses[bitLength];
                        /* Fall through */
                    case 0:
                        break;
                    default:
                        // should not happen, but just in case ...
                        memcpy(gPulses, &gPulses[bitLength], index);
                        break;
                }
            }

            // Error correction
            if ((index == halfBitLength) || (index == bitLengthButOne)) {
                countDownToForcedLongPulse--;

                if (countDownToForcedLongPulse) {
                    // Middle or last pulse: A long pulse is subject to happen at these positions => reduce threshold
                    if ((delta <= revisedAboveMedianPulseCycles) || (missedShortPulse)) {
                        // ShortPulse or MissedShortPulse: short or long pulse is allowed
                        revisedAboveMedianPulseCycles = reducedAboveMedianPulseCycles;
                    }
                    else {
                        // Long Pulse: A long pulse is not allowed immediately after another long pulse
                        revisedAboveMedianPulseCycles = reducedAboveLongPulseCycles;
                    }
                }
                else {
                    // Max nb of pulses between 2 long pulses: A long pulse is mandatory => set threshold to 0
                    revisedAboveMedianPulseCycles = 0;              
                }
            }
            else {
                // Other places: A long pulse is not allowed at these positions
                revisedAboveMedianPulseCycles = aboveLongPulseCycles;
            }
            
            if ((index >= (MAX_RESPONSE_PULSES-2)) || (*nbBits >= MAX_RESPONSE_BITS)) {
                return GB29768_PROTO_ERROR;
            }
        }
        else {
            leadPulses++;
        }
    } while (delta < maxPulseCycles);

    return GB29768_OKAY;
}

/*------------------------------------------------------------------------- */
gb29768_error_t gb29768GetFM0Bits(uint8_t *bits, uint32_t *nbBits, gb29768Config_t *gbCfg, uint8_t lastCommand)
{
    uint32_t    totalCounters = 0;
    uint32_t    index;
    uint32_t    leadPulses;
    uint8_t     currentPulse;
    uint8_t     previousPulse;
    uint8_t     val;
    bool        receivedFirstLongPulse = false;
    bool        missedShortPulse;
    uint32_t    maxPulseCycles;
    uint32_t    minLeadPulses;                              // Very minimum pulses to be received as lead code
    uint32_t    minFirstPulseCycles;
    uint32_t    maxFirstPulseCycles;
    uint32_t    delta;
    bool        trext;

    // Preprocessed values
    uint32_t    aboveMedianPulseCycles = (MEDIAN_PULSE_CYCLES(gbCfg->blf)*135)/100;                  // (Tpri/2) => 1562ns * 135% = 2100 ns  => Threshold between short and long pulses
    uint32_t    aboveLongPulseCycles = (LONG_PULSE_CYCLES(gbCfg->blf)*115)/100;                      // Tpri => 3125ns * 115% = 3593 ns      => Threshold to exceed long pulse
    uint32_t    aboveViolationLongPulseCycles = (VIOLATION_PULSE_CYCLES(gbCfg->blf)*105)/100;        // 1.5Tpri => 4688ns * 105% = 4922 ns   => Threshold to exceed violation long pulse

    uint32_t    augmentedAboveMedianPulseCycles = (aboveMedianPulseCycles*112)/100;                  // aboveMedianPulseCycles => 2100 ns * 112% = 2352 ns  => Threshold of augmented short pulses
    uint32_t    reducedAboveLongPulseCycles = (aboveLongPulseCycles*90)/100;                         // aboveLongPulseCycles => 3593 * 90% = 3233 ns

    uint32_t    revisedAboveMedianPulseCycles = augmentedAboveMedianPulseCycles;
    uint32_t    revisedAboveLongPulseCycles = aboveLongPulseCycles;

    *nbBits = 0;

    switch (lastCommand)
    {
        case GB29768_CMD_WRITE:
        case GB29768_CMD_ERASE:
        case GB29768_CMD_LOCK:
        case GB29768_CMD_KILL:
            maxPulseCycles = NS_TO_CYCLES(MAX_VERYLONG_WAIT_NS);
            // TRext is forced to TRUE
            trext = true;
            break;
        default:
            maxPulseCycles = MAX_PULSE_CYCLES(gbCfg->blf);
            trext = gbCfg->trext;
            break;
    }
    if (trext) {
        minLeadPulses = 23;
        maxFirstPulseCycles = revisedAboveMedianPulseCycles;
        minFirstPulseCycles = 0;
    }
    else {
        minLeadPulses = 0;
        maxFirstPulseCycles = aboveLongPulseCycles;
        minFirstPulseCycles = revisedAboveMedianPulseCycles;
    }

#ifdef GB29768PULSES_DEBUG
    gDebugPulsesNb = 0;
#endif

    INIT_DWT();

    // Wait for first edge
    currentPulse = READ_PULSE_PIN();
    MILLER2_GET_NEW_PULSE_DWT_NOPULSECHECK(delta, currentPulse, maxPulseCycles);                    // Caution: currentPulse, delta & missedShortPulse are modified by the macro
    totalCounters += delta;
    if (totalCounters > maxPulseCycles) {
        // Stability check timeout
        return GB29768_NO_RESPONSE;
    }

    // Check stability of signal and wait for first pulse
    do
    {
        previousPulse = currentPulse;
        MILLER2_GET_NEW_PULSE_DWT_NOPULSECHECK(delta, currentPulse, maxPulseCycles);                // Caution: currentPulse, delta & missedShortPulse are modified by the macro
        totalCounters += delta;
        if (totalCounters > maxPulseCycles) {
            // Stability check timeout
            return GB29768_NO_RESPONSE;
        }
    } while((delta >= maxFirstPulseCycles) || (delta <= minFirstPulseCycles));      // pulse has correct length

    DEBUG_STORE_PULSES_VAL(gbCfg->coding);                               // Store used coding
    DEBUG_STORE_PULSES_VAL(aboveMedianPulseCycles >> 1);                 // Store mediancycles (divided by 2 to fit in 8 bits)
    DEBUG_STORE_PULSES_VAL(aboveLongPulseCycles >> 1);                   // Store longcycles (divided by 2 to fit in 8 bits)
    SET_DEBUG_DELTA_VAL(previousPulse, delta);          // Add delta value for debug; Caution: previousPulse is modified by the macro
    DEBUG_STORE_PULSES_VAL(previousPulse);              // Store first pulse

    // Starts processing
    if (trext) {
        leadPulses = 1;
    }
    else {
        DEBUG_STORE_PULSES_VAL(previousPulse);             // First pulse is a long one

        receivedFirstLongPulse = true;
        index = 0;
        // Consider first bit is received
        bits[0] = 1;
        *nbBits = 1;
    }

    maxPulseCycles = aboveLongPulseCycles;
    do
    {
        // ***********************************************************************
        //                CAUTION: this loop code is time critical !!!
        // Algorithm must be optimal and no function call is allowed (only macros)


        previousPulse = currentPulse;
        MILLER2_GET_NEW_PULSE_DWT(delta, currentPulse, maxPulseCycles, missedShortPulse, FM0_PULSE_PROCESSING_MAX_CYCLES);                  // Caution: currentPulse, delta & missedShortPulse are modified by the macro

        // Store pulse at least as a short one
        val = previousPulse;
        SET_DEBUG_DELTA_VAL(val, delta);    // Add delta value for debug; Caution: val is modified by the macro

        // Check pulse duration
        if(delta > revisedAboveMedianPulseCycles) {      // Long pulse duration
            // Starts bits management
            if (!receivedFirstLongPulse) {
                if (leadPulses < minLeadPulses) {
                    // Wrong lead code
                    DEBUG_STORE_PULSES_VAL(0xFB);
                    return GB29768_NO_RESPONSE;
                }
                receivedFirstLongPulse = true;
                index = 0;
            }
            DEBUG_STORE_PULSES_VAL(val);
            gPulses[index++] = val;                      // Store pulse value
        }

        DEBUG_STORE_PULSES_VAL(val);

        // Bits management
        if (receivedFirstLongPulse) {
            gPulses[index++] = val;                         // Store pulse value

            if (delta > revisedAboveLongPulseCycles) {
                // Violation long pulses awaited for 6th bit of lead code
                DEBUG_STORE_PULSES_VAL(val);
                DEBUG_STORE_PULSES_VAL(0xFF);

                gPulses[index++] = val;                         // Store pulse value
            }
    
            if (missedShortPulse) {
                val = previousPulse ? 0 : 1;                // Store inverted pulse value (missed pulse)
                SET_DEBUG_DELTA_VAL(val, delta);      // Add delta value for debug; Caution: val is modified by the macro
                DEBUG_STORE_PULSES_VAL(val);

                gPulses[index++] = val;
            }

            if (index >= 2) {
                bits[(*nbBits)++] = (gPulses[0] == gPulses[1]) ? 1 : 0;     // Both pulses are equal => bit=1 else bit=0

                switch(index) {
                    case 5:
                        gPulses[0] = gPulses[4];
                        /* Fall through */
                    case 4:
                        bits[(*nbBits)++] = (gPulses[2] == gPulses[3]) ? 1 : 0;     // Both pulses are equal => bit=1 else bit=0
                        break;
                    case 3:
                        gPulses[0] = gPulses[2];
                        break;
                    default:
                        break;
                }
                index %= 2;
            }

            // Error correction
            maxPulseCycles = aboveLongPulseCycles;
            revisedAboveLongPulseCycles = aboveLongPulseCycles;
            if (index == 1) {
                switch(*nbBits) {
                    case 3:
                        // 3rd bit of Lead code: bits violation => Long pulse mandatory
                        revisedAboveMedianPulseCycles = 0;              
                        break;
                    case 6:
                        // 6th bit of Lead code: bits violation => Violation Long pulse awaited
                        revisedAboveMedianPulseCycles = aboveMedianPulseCycles;              
                        if (delta < maxPulseCycles) {   // Check delta before changing maxPulseCycles => will exit if above current maxPulseCycles
                            maxPulseCycles = aboveViolationLongPulseCycles;
                            revisedAboveLongPulseCycles = augmentedAboveMedianPulseCycles;
                        }
                        break;
                    default:
                        // A long pulse is not allowed at this position => Short pulse awaited
                        revisedAboveMedianPulseCycles = reducedAboveLongPulseCycles;
                        break;
                }
            }
            else {
                switch(*nbBits) {
                    case 3:
                        // 3rd bit of Lead code: bits violation => Short pulse mandatory
                        revisedAboveMedianPulseCycles = aboveLongPulseCycles;              
                        break;
                    case 8:
                        // New bit starts: short or long pulse is allowed
                        // Violation Long pulse from lead code received, ensures delta does not exceed maxPulseCycles
                        delta = aboveMedianPulseCycles; 
                        // After bits violation short pulse lasts a bit longer => augments pulse duration
                        revisedAboveMedianPulseCycles = augmentedAboveMedianPulseCycles;
                        break;
                    default:
                        // New bit starts: short or long pulse is allowed
                        revisedAboveMedianPulseCycles = aboveMedianPulseCycles;
                        break;
                }
            }
    
            if ((index >= (MAX_RESPONSE_PULSES-3)) || (*nbBits >= MAX_RESPONSE_BITS)) {
                return GB29768_PROTO_ERROR;
            }
        }
        else {
            leadPulses++;
        }
    } while (delta < maxPulseCycles);

    DEBUG_STORE_PULSES_VAL(0xFD);

    return GB29768_OKAY;
}

gb29768_error_t gb29768RxFlush(uint32_t blf)
{
    uint32_t    delta;
    uint8_t     currentPulse;
    uint32_t    flushPulseCycles = FLUSH_PULSE_CYCLES(blf);
    uint32_t    maxPulseCycles = NS_TO_CYCLES( 2*GB29768_TIMING_T2max_NS(blf) );
    uint32_t    totalCounters = 0;

    INIT_DWT();

    // Read current pulse
    currentPulse = READ_PULSE_PIN();
    do
    {
        MILLER2_GET_NEW_PULSE_DWT_NOPULSECHECK(delta, currentPulse, flushPulseCycles);              // Caution: currentPulse, delta & missedShortPulse are modified by the macro
        totalCounters += delta;
        if (totalCounters > maxPulseCycles) {
            break;
        }
    } while(delta < flushPulseCycles);            // Still some pulses

    return GB29768_OKAY;
}

#endif  /* GB29768 */

/**
  * @}
  */
/**
  * @}
  */
