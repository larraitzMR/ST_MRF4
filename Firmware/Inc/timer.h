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
 *  @brief This file is the include file for the timer.c file.
 *
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup Timer
  * @{
  */

#ifndef __TIMER_H__
#define __TIMER_H__

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
#if ELANCE
#define TIMER_BB_HANDLE         htim5
#elif JIGEN && defined(STM32WB55xx)
#define TIMER_BB_HANDLE         htim2
#else
#define TIMER_BB_HANDLE         htim5
#endif

#if EVAL || DISCOVERY || HAS_LED_TIMER
#define TIMER_LED_HANDLE        htim4
#endif

#if EVAL
#define TIMER_BUZZER_HANDLE     htim16
#define TIMER_BUZZER_CHANNEL    TIM_CHANNEL_1
#endif

/* Porting note: replace delay functions which with functions provided with your controller or use timer */
#define delay_ms(ms)    { HAL_Delay(ms); }
#define delay_us(us)    { udelay(us); }

#define NS_TO_CYCLES(ns)        (((ns)*2)/25)

#define INIT_DWT()             { CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;    \
                                 DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;               \
                                 RESET_DWT();                                       \
                                 }
#define RESET_DWT()             DWT->CYCCNT = 0
#define GET_DWT()               (DWT->CYCCNT)


/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
/**
  Needs to be called prior to first delay function to initialize
  */
void delayInitialize(void);

/**
  Delays usec microseconds
  */
void udelay(uint16_t usec);

/**
  Delays nsec nanoseconds (versus last RESET_DWT())
  */
void nsdelay(uint32_t nsec);

/** Reset the internal count value incremented by the RTC

  Real Time Clock:
  Freq = RTCCLK(LSE) / (AsynchPrediv + 1)*(SynchPrediv + 1)
    AsyncPrediv = 0 - 127
    SyncPrediv  = 0 - 32767
    e.g.: Freq = 32768 Hz / (127 + 1)*(255+1)
          Freq = 32768 Hz / (128 * 256) = 1 Hz
          Period = 1/Freq = 1 sec.

  currently used:
  Freq = 32768 Hz / (0 + 1)*(326+1)
  Freq = 32768 Hz / (1 * 327) = 100,208 Hz
  Period = 1/Freq = 9,979 ms
*/
void rtcTimerReset(void);
void rtcTimerSet(uint16_t t);
/**
 * Measure time passed since last call of rtcTimerReset(void). The resolution is
 * 10ms. The return value is in ms.
 */
uint16_t rtcTimerValue(void);

/**
  Start timer to toggle MCU LED. It has a period of 300ms.
  Prescaler = 64000-1 --> 64MHz freq --> 1000 Hz timer freq --> 1 ms basis
  Period = 300-1 --> 300 * basis --> 300 ms
  */
void ledTimerStart(void);

/**
 * Stops the led timer.
 */
void ledTimerStop(void);

/**
  Start timer to send out bytes in direct mode. It has a period of 13us.
  Prescaler = 32-1 --> 64MHz freq --> 2000.000 Hz timer freq --> 0.5 us basis
  Period = 25-1 --> 25 * basis --> 12.5 us
  */
void bitbangTimerStart(void);

/**
 * Stops the bitbang timer.
 */
void bitbangTimerStop(void);

/**
  Start timer to control buzzer PWM
  Prescaler = 64-1 --> 64MHz freq --> 1000.000 Hz timer freq --> 1 us basis
  Period = 50-1 --> 50 * basis --> 50 us
  Pulse = 25-1 --> 25 * basis --> 25 us
  --> Signal of 20.000 Hz

  Prescaler = 64000-1 --> 64MHz freq --> 1000 Hz timer freq --> 1 ms basis
  Period = 50-1 --> 50 * basis --> 50 ms
  Pulse = 25-1 --> 25 * basis --> 25 ms
  --> Signal of 20 Hz
  */
void buzzerTimerStart(uint16_t freq);

/**
 * Stops the buzzer PWM timer.
 */
void buzzerTimerStop(void);

#endif

/**
  * @}
  */
/**
  * @}
  */
