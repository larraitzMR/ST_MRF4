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
 *  The file provides functions to initialize the MCU and the board
 *  For porting to another uController/board the functions in this file have to 
 *  be adapted.
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup Platform
  * @{
  */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include <string.h>
#include "spi.h"
#include "gpio.h"
#include "platform.h"
#include "logger.h"
#include "timer.h"
#include "spi_driver.h"
#include "uart_driver.h"
#include "st25RU3993_public.h"
#include "stream_dispatcher.h"
#include "evalAPI_commands.h"
#include "tuner.h"
#include "flash_access.h"
#include "leds.h"
#if ELANCE
#include "rffe.h"
#endif

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */
extern uint8_t externalPA;
uint8_t irqEnable = 0;

/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */
extern void MX_SPI1_DeInit(void);
extern void MX_GPIO_InitDirectMode(void);
extern void MX_GPIO_InitNormalMode(void);

/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */
//static void playForElise(void);

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */
void platformInit(void)
{
    // delay init
    delayInitialize();

    // logger init
    LOGINIT();

    // config LDO pins
#if DISCOVERY
    SET_GPIO(GPIO_RF_LDO_PORT,GPIO_RF_LDO_PIN);
    delay_ms(10);
#endif

    // Enable internal PA
    RESET_GPIO(GPIO_PA_LDO_PORT,GPIO_PA_LDO_PIN);

    //
#if ELANCE
    // Enable LDOs
    SET_GPIO(GPIO_URF_LDO_PORT, GPIO_HRF_LDO_PIN);
    delay_ms(10);

    // Switch default to antenna 1
    RESET_GPIO(GPIO1_GPIO_Port, GPIO1_Pin);
    RESET_GPIO(GPIO2_GPIO_Port, GPIO2_Pin);
    
    // initialize HVDAC
    RFFE_Init(DATA_GPIO_Port, DATA_Pin, CLK_RFFE_GPIO_Port, CLK_RFFE_Pin, NULL, NULL);

    // read USID of device
    uint8_t USID = 0x07;
    RFFE_ReadRegister(USID, 0x1F, &USID); 
    USID &= 0x0F;
    // set direct mode, no shadow register, no triggers
    RFFE_WriteRegister(USID, 28, 0x38); 

#endif

    // SPI init
    spiInitialize();

    // UART init
    uartInitialize();

    // PA init
    if(externalPA)
    {
#if DISCOVERY
        SET_GPIO(GPIO_PA_CTRL_PORT,GPIO_PA_CTRL_PIN);
#elif EVAL
        RESET_GPIO(GPIO_PA_SW_V1_PORT,GPIO_PA_SW_V1_PIN);
        SET_GPIO(GPIO_PA_SW_V2_PORT,GPIO_PA_SW_V2_PIN);
#elif ELANCE
        // No PA Switches on ELANCE
#elif JIGEN
        // No PA Switches on JIGEN
#else
        #error "Undefined Code for selected board"
#endif
        SET_INTPA_LED_OFF();
        SET_EXTPA_LED_ON();
    }
    else
    {
#if DISCOVERY
        RESET_GPIO(GPIO_PA_CTRL_PORT,GPIO_PA_CTRL_PIN);
#elif EVAL
        SET_GPIO(GPIO_PA_SW_V1_PORT,GPIO_PA_SW_V1_PIN);
        RESET_GPIO(GPIO_PA_SW_V2_PORT,GPIO_PA_SW_V2_PIN);
#elif ELANCE
        // No PA Switches on ELANCE
#elif JIGEN
        // No PA Switches on JIGEN
#else
        #error "Undefined Code for selected board"
#endif
        SET_INTPA_LED_ON();
        SET_EXTPA_LED_OFF();
    }

#if EVAL
    // Buzzer test
    //playForElise();
#endif

    initializeFrequencies();
    loadTuningTablesFromFlashAll(); // loads saved tuning tables from flash OR initializes with default values

    // Other initializations
    st25RU3993Initialize();

    initCommands();
    StreamDispatcherInit();

    // external interrupt init
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
void setPortDirect(void)
{
    MX_SPI1_DeInit();
    MX_GPIO_InitDirectMode();
}

/*------------------------------------------------------------------------- */
void setPortNormal(void)
{
    MX_GPIO_InitNormalMode();
    MX_SPI1_Init();
}

/*
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 */
/*
static void playForElise(void)
{
    // play e4
    delay_ms(600);
    buzzerTimerStart(330);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(50);
    // play d4# 
    buzzerTimerStart(311);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(50);
    // play e4
    buzzerTimerStart(330);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(50);
    // play d4# 
    buzzerTimerStart(311);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(50);
    // play e4
    buzzerTimerStart(330);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(50);
    // play b3
    buzzerTimerStart(247);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play d4
    buzzerTimerStart(294);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play c4
    buzzerTimerStart(262);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play a3
    buzzerTimerStart(220);
    delay_ms(900);
    buzzerTimerStop();
    delay_ms(100);
    // play d3
    buzzerTimerStart(147);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(50);
    //play f3
    buzzerTimerStart(175);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    //play a3
    buzzerTimerStart(220);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play b3
    buzzerTimerStart(247);
    delay_ms(900);
    buzzerTimerStop();
    delay_ms(100);
    // play f3
    buzzerTimerStart(175);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play a3#
    buzzerTimerStart(233);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play b3
    buzzerTimerStart(247);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play c4
    buzzerTimerStart(262);
    delay_ms(900);
    buzzerTimerStop();
    delay_ms(400);
    // play e4
    buzzerTimerStart(330);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play d4#
    buzzerTimerStart(311);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play e4
    buzzerTimerStart(330);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play d4#
    buzzerTimerStart(311);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play e4
    buzzerTimerStart(330);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play b3
    buzzerTimerStart(247);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play d4
    buzzerTimerStart(294);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play c4
    buzzerTimerStart(262);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play a3
    buzzerTimerStart(220);
    delay_ms(900);
    buzzerTimerStop();
    delay_ms(100);
    // play d3
    buzzerTimerStart(147);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play f3
    buzzerTimerStart(175);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play a3
    buzzerTimerStart(220);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play b3
    buzzerTimerStart(247);
    delay_ms(900);
    buzzerTimerStop();
    delay_ms(100);
    // play f3
    buzzerTimerStart(175);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play c4
    buzzerTimerStart(262);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play b3
    buzzerTimerStart(247);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play a3
    buzzerTimerStart(220);
    delay_ms(900);
    buzzerTimerStop();
    delay_ms(100);
    // play b3
    buzzerTimerStart(247);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
     // play c4
    buzzerTimerStart(262);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play d4
    buzzerTimerStart(294);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play e4
    buzzerTimerStart(330);
    delay_ms(900);
    buzzerTimerStop();
    delay_ms(100);
    // play g3
    buzzerTimerStart(196);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play f4
    buzzerTimerStart(349);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    //play e4
    buzzerTimerStart(329);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play d4
    buzzerTimerStart(294);
    delay_ms(900);
    buzzerTimerStop();
    delay_ms(100);
    // play e3
    buzzerTimerStart(165);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play e4
    buzzerTimerStart(330);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play d4
    buzzerTimerStart(294);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play c4
    buzzerTimerStart(262);
    delay_ms(900);
    buzzerTimerStop();
    delay_ms(100);
    // play d3
    buzzerTimerStart(147);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
      // play d4
    buzzerTimerStart(294);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play c4
    buzzerTimerStart(262);
    delay_ms(300);
    buzzerTimerStop();
    delay_ms(100);
    // play b3
    buzzerTimerStart(247);
    delay_ms(900);
    buzzerTimerStop();
    delay_ms(100);
}
*/

/**
  * @}
  */
/**
  * @}
  */
