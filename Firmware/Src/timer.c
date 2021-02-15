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
 *  @brief This file includes all functionality to use some hardware timers
 *
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup Timer
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "tim.h"
#include "platform.h"
#include "timer.h"

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
volatile uint16_t rtcTimerMsValue = 1;

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
/** multiplier value used for delays, depending on MCU frequency */
static uint32_t multiplier;

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void delayInitialize(void)
{
    uint32_t freq = HAL_RCC_GetSysClockFreq();

    // while loop takes up 4 cycles, so divide by 4M to get 1 us delay
    multiplier = freq/4000000;
}

/*------------------------------------------------------------------------- */
void udelay(uint16_t usec)
{
    uint32_t usec32;
    if (usec >= 3) {
        // usec32 = (usec * 16) - 10
        usec32 = (usec * multiplier) - 4;
    }
    else {
        // REMINDER: timings < 3us
        // usec32 = (usec * multiplier * 85)/100;   // below 3us, drift of +10%, correct it taking into account calculation time
        usec32 = usec;
    }
    while(usec32--) {
        __NOP();    // Avoid code optimization
    }    
}

/*------------------------------------------------------------------------- */
void nsdelay(uint32_t nsec)
{
    uint32_t cycles = NS_TO_CYCLES(nsec)-(nsec < 8000 ? 25 : 20);    // -25/-20: compensate function call overhead
    do
    {
        __NOP();            // Avoid code optimization
    } while(GET_DWT() < cycles);    
}

/*------------------------------------------------------------------------- */
void rtcTimerReset(void)
{
    rtcTimerMsValue = 1;   //start with 1ms to get immediate stop at 0ms delays
}

/*------------------------------------------------------------------------- */
uint16_t rtcTimerValue(void)
{
    return rtcTimerMsValue;
}

void rtcTimerSet(uint16_t t)
{
    rtcTimerMsValue = t;
}

/*------------------------------------------------------------------------- */
void ledTimerStart(void)
{
#if EVAL || DISCOVERY || HAS_LED_TIMER
    HAL_TIM_Base_Start_IT(&TIMER_LED_HANDLE);
#endif
}

/*------------------------------------------------------------------------- */
void ledTimerStop(void)
{
#if EVAL || DISCOVERY || HAS_LED_TIMER
    HAL_TIM_Base_Stop_IT(&TIMER_LED_HANDLE);
#endif

#if !JIGEN && !NO_LEDS
    HAL_GPIO_WritePin(GPIO_LED_MCU_PORT,GPIO_LED_MCU_PIN,GPIO_PIN_SET);
#endif
}

/*------------------------------------------------------------------------- */
void bitbangTimerStart(void)
{
    HAL_TIM_Base_Start_IT(&TIMER_BB_HANDLE);
}

/*------------------------------------------------------------------------- */
void bitbangTimerStop(void)
{
    HAL_TIM_Base_Stop_IT(&TIMER_BB_HANDLE);
}

/*------------------------------------------------------------------------- */
void buzzerTimerStart(uint16_t freq)
{
#if EVAL
    /*
    freq = 20 Hz     --> prescaler = 64.000
    freq = 20.000 Hz --> prescaler = 64
    */
    uint16_t prescaler;
    uint32_t temp;

    freq = (freq > 20480 ? 20480 : freq);   // prescaler of 65535, 16 bit allowed
    freq = (freq < 1 ? 1 : freq);           // prescaler of 2

    temp = (1280000/freq);
    prescaler = temp;

    TIMER_BUZZER_HANDLE.Init.Prescaler = prescaler-1;
    HAL_TIM_PWM_Init(&TIMER_BUZZER_HANDLE);
    HAL_TIM_PWM_Start(&TIMER_BUZZER_HANDLE, TIMER_BUZZER_CHANNEL);
#endif
}

/*------------------------------------------------------------------------- */
void buzzerTimerStop(void)
{
#if EVAL
    HAL_TIM_PWM_Stop(&TIMER_BUZZER_HANDLE, TIMER_BUZZER_CHANNEL);
    HAL_GPIO_WritePin(GPIO_BUZZER_PORT,GPIO_BUZZER_PIN,GPIO_PIN_RESET);
#endif
}

/**
  * @}
  */
/**
  * @}
  */
