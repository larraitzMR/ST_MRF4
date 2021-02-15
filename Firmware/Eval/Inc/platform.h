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
 *  @brief This file provides platform (board) specific macros and declarations
 *
 *  Contains configuration macros which are used throughout the code base,
 *  to enable/disable board specific features.
 */

/** @addtogroup Low_Level
  * @{
  */
/** @addtogroup Platform
  * @{
  */

#ifndef PLATFORM_H_
#define PLATFORM_H_

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "gpio.h"
#include "st25RU3993_config.h"
#include "st25RU3993.h"

/*
 ******************************************************************************
 * DEFINES (DO NOT CHANGE)
 ******************************************************************************
 */
/** Macro for enable external IRQ */
#define ENEXTIRQ()          {irqEnable=1; st25RU3993Isr();}
#define DISEXTIRQ()         irqEnable=0

/** Macro for reset/set GPIO */
#define SET_GPIO(X,Y)       HAL_GPIO_WritePin((X),(Y),GPIO_PIN_SET)
#define RESET_GPIO(X,Y)     HAL_GPIO_WritePin((X),(Y),GPIO_PIN_RESET)
#define GET_GPIO(X,Y)       HAL_GPIO_ReadPin((X),(Y))

/** define for stopMode parameter of writeReadST25RU3993() */
#define STOP_NONE           0
#define STOP_SGL            1
#define STOP_CONT           2

#define DIRECT_MODE_ENABLE_SENDER()      RESET_GPIO(GPIO_SPI_SCK_PORT,GPIO_SPI_SCK_PIN)
#define DIRECT_MODE_ENABLE_RECEIVER()    SET_GPIO(GPIO_SPI_SCK_PORT,GPIO_SPI_SCK_PIN)

#define DM_TX_SET           SET_GPIO(GPIO_SPI_MOSI_PORT,GPIO_SPI_MOSI_PIN)
#define DM_TX_RESET         RESET_GPIO(GPIO_SPI_MOSI_PORT,GPIO_SPI_MOSI_PIN)

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
// Logging
/** LOGGER_ON to enable, LOGGER_OFF to disable */
#define USE_LOGGER              LOGGER_ON
/** Define this to 1 if you want to have logging for appl commands. */
#define LOG_APPL_COMMANDS       0
#define VERBOSE_LOG             0
/** Define this to 1 if you want to have logging for gen2 commands. */
#define LOG_GEN2                0
/** Define this to 1 if you want to have logging for st25ru3993. */
#define LOG_ST25RU3993          0

// GPIOs to ST25RU3993
#if DISCOVERY
    #define GPIO_SPI_CS_PORT        SPI_NCS_GPIO_Port
    #define GPIO_SPI_CS_PIN         SPI_NCS_Pin

    #define GPIO_SPI_SCK_PORT       SPI_SCK_GPIO_Port
    #define GPIO_SPI_SCK_PIN        SPI_SCK_Pin

    #define GPIO_SPI_MOSI_PORT      SPI_MOSI_GPIO_Port
    #define GPIO_SPI_MOSI_PIN       SPI_MOSI_Pin

    #define GPIO_SPI_MISO_PORT      SPI_MISO_GPIO_Port
    #define GPIO_SPI_MISO_PIN       SPI_MISO_Pin

    #define GPIO_EN_PORT            EN_GPIO_Port
    #define GPIO_EN_PIN             EN_Pin

    #define GPIO_IRQ_PORT           IRQ_GPIO_Port
    #define GPIO_IRQ_PIN            IRQ_Pin

    // tuner GPIOs
    #define GPIO_CIN_PORT           ETC1_GPIO_Port
    #define GPIO_CIN_PIN            ETC1_Pin

    #define GPIO_CLEN_PORT          ETC2_GPIO_Port
    #define GPIO_CLEN_PIN           ETC2_Pin

    #define GPIO_COUT_PORT          ETC3_GPIO_Port
    #define GPIO_COUT_PIN           ETC3_Pin

    // antenna select GPIO
    #define GPIO_ANTENNA_PORT       ANTENNA_GPIO_Port
    #define GPIO_ANTENNA_PIN        ANTENNA_Pin

    // PA select GPIO
    #define GPIO_PA_CTRL_PORT       PA_SW_CTRL_GPIO_Port
    #define GPIO_PA_CTRL_PIN        PA_SW_CTRL_Pin

    // LED GPIOs
    #define GPIO_LED_MCU_PORT       LED_MCU_GPIO_Port
    #define GPIO_LED_MCU_PIN        LED_MCU_Pin

    #define GPIO_LED_ANT1_PORT      LED_ANT1_GPIO_Port
    #define GPIO_LED_ANT1_PIN       LED_ANT1_Pin

    #define GPIO_LED_ANT2_PORT      LED_ANT2_GPIO_Port
    #define GPIO_LED_ANT2_PIN       LED_ANT2_Pin

    #define GPIO_LED_TUNED_PORT     LED_TUNED_GPIO_Port
    #define GPIO_LED_TUNED_PIN      LED_TUNED_Pin

    #define GPIO_LED_TUNING_PORT    LED_TUNING_GPIO_Port
    #define GPIO_LED_TUNING_PIN     LED_TUNING_Pin

    #define GPIO_LED_RF_PORT        LED_RF_GPIO_Port
    #define GPIO_LED_RF_PIN         LED_RF_Pin

    #define GPIO_LED_EXTPA_PORT     LED_EXTPA_GPIO_Port
    #define GPIO_LED_EXTPA_PIN      LED_EXTPA_Pin

    #define GPIO_LED_INTPA_PORT     LED_INTPA_GPIO_Port
    #define GPIO_LED_INTPA_PIN      LED_INTPA_Pin

    #define GPIO_LED_PLL_PORT       LED_PLL_GPIO_Port
    #define GPIO_LED_PLL_PIN        LED_PLL_Pin

    #define GPIO_LED_OSC_PORT       LED_OSC_GPIO_Port
    #define GPIO_LED_OSC_PIN        LED_OSC_Pin

    #define GPIO_LED_NORESP_PORT    LED_NO_RESP_GPIO_Port
    #define GPIO_LED_NORESP_PIN     LED_NO_RESP_Pin

    #define GPIO_LED_CRC_PORT       LED_CRC_ERR_GPIO_Port
    #define GPIO_LED_CRC_PIN        LED_CRC_ERR_Pin

    #define GPIO_LED_TAG_PORT       LED_TAG_GPIO_Port
    #define GPIO_LED_TAG_PIN        LED_TAG_Pin

    // LDO GPIOs
    #define GPIO_RF_LDO_PORT        RF_LDO_GPIO_Port
    #define GPIO_RF_LDO_PIN         RF_LDO_Pin

    #define GPIO_PA_LDO_PORT        PA_LDO_GPIO_Port
    #define GPIO_PA_LDO_PIN         PA_LDO_Pin

    // OAD GPIOs
    #define GPIO_OAD_PORT           OAD_GPIO_Port
    #define GPIO_OAD_PIN            OAD_Pin

    #define GPIO_OAD2_PORT          OAD2_GPIO_Port
    #define GPIO_OAD2_PIN           OAD2_Pin

    // GAIN GPIOs
    #define GPIO_G8_I_PORT          G8_I_GPIO_Port
    #define GPIO_G8_I_PIN           G8_I_Pin

    #define GPIO_G16_I_PORT         G16_I_GPIO_Port
    #define GPIO_G16_I_PIN          G16_I_Pin

    #define GPIO_G8_Q_PORT          G8_Q_GPIO_Port
    #define GPIO_G8_Q_PIN           G8_Q_Pin

    #define GPIO_G16_Q_PORT         G16_Q_GPIO_Port
    #define GPIO_G16_Q_PIN          G16_Q_Pin

#elif EVAL
    #define GPIO_SPI_CS_PORT        NCS_GPIO_Port
    #define GPIO_SPI_CS_PIN         NCS_Pin

    #define GPIO_SPI_SCK_PORT       CLK_GPIO_Port
    #define GPIO_SPI_SCK_PIN        CLK_Pin

    #define GPIO_SPI_MOSI_PORT      MOSI_GPIO_Port
    #define GPIO_SPI_MOSI_PIN       MOSI_Pin

    #define GPIO_SPI_MISO_PORT      MISO_GPIO_Port
    #define GPIO_SPI_MISO_PIN       MISO_Pin

    #define GPIO_EN_PORT            EN_GPIO_Port
    #define GPIO_EN_PIN             EN_Pin

    #define GPIO_IRQ_PORT           IRQ_GPIO_Port
    #define GPIO_IRQ_PIN            IRQ_Pin

    // tuner GPIOs
    #define GPIO_CIN_PORT           ETC_1_GPIO_Port
    #define GPIO_CIN_PIN            ETC_1_Pin

    #define GPIO_CLEN_PORT          ETC_2_GPIO_Port
    #define GPIO_CLEN_PIN           ETC_2_Pin

    #define GPIO_COUT_PORT          ETC_3_GPIO_Port
    #define GPIO_COUT_PIN           ETC_3_Pin

    // antenna select GPIO
    #define GPIO_ANT_SW_V1_PORT     ANT_SW_V1_GPIO_Port
    #define GPIO_ANT_SW_V1_PIN      ANT_SW_V1_Pin

    #define GPIO_ANT_SW_V2_PORT     ANT_SW_V2_GPIO_Port
    #define GPIO_ANT_SW_V2_PIN      ANT_SW_V2_Pin

    // PA select GPIO
    #define GPIO_PA_SW_V1_PORT      PA_SW_V1_GPIO_Port
    #define GPIO_PA_SW_V1_PIN       PA_SW_V1_Pin

    #define GPIO_PA_SW_V2_PORT      PA_SW_V2_GPIO_Port
    #define GPIO_PA_SW_V2_PIN       PA_SW_V2_Pin

    // LED GPIOs
    #define GPIO_LED_MCU_PORT       MCU_LED_GPIO_Port
    #define GPIO_LED_MCU_PIN        MCU_LED_Pin

    #define GPIO_LED_ANT1_PORT      ANT1_LED_GPIO_Port
    #define GPIO_LED_ANT1_PIN       ANT1_LED_Pin

    #define GPIO_LED_ANT2_PORT      ANT2_LED_GPIO_Port
    #define GPIO_LED_ANT2_PIN       ANT2_LED_Pin

    #define GPIO_LED_TUNED_PORT     TUNED_LED_GPIO_Port
    #define GPIO_LED_TUNED_PIN      TUNED_LED_Pin

    #define GPIO_LED_TUNING_PORT    TUNING_LED_GPIO_Port
    #define GPIO_LED_TUNING_PIN     TUNING_LED_Pin

    #define GPIO_LED_RF_PORT        RF_LED_GPIO_Port
    #define GPIO_LED_RF_PIN         RF_LED_Pin

    #define GPIO_LED_EXTPA_PORT     extPA_LED_GPIO_Port
    #define GPIO_LED_EXTPA_PIN      extPA_LED_Pin

    #define GPIO_LED_INTPA_PORT     intPA_LED_GPIO_Port
    #define GPIO_LED_INTPA_PIN      intPA_LED_Pin

    #define GPIO_LED_PLL_PORT       PLL_LED_GPIO_Port
    #define GPIO_LED_PLL_PIN        PLL_LED_Pin

    #define GPIO_LED_OSC_PORT       OSC_LED_GPIO_Port
    #define GPIO_LED_OSC_PIN        OSC_LED_Pin

    #define GPIO_LED_NORESP_PORT    NO_RESP_LED_GPIO_Port
    #define GPIO_LED_NORESP_PIN     NO_RESP_LED_Pin

    #define GPIO_LED_CRC_PORT       CRC_ERR_LED_GPIO_Port
    #define GPIO_LED_CRC_PIN        CRC_ERR_LED_Pin

    #define GPIO_LED_TAG_PORT       TAG_LED_GPIO_Port
    #define GPIO_LED_TAG_PIN        TAG_LED_Pin

    // LDO GPIOs
    #define GPIO_PA_LDO_PORT        PA_LDO_EN_GPIO_Port
    #define GPIO_PA_LDO_PIN         PA_LDO_EN_Pin

    // Buzzer GPIOs
    #define GPIO_BUZZER_PORT        Buzzer_GPIO_Port
    #define GPIO_BUZZER_PIN         Buzzer_Pin

    // Bluetooth GPIOs
    #define GPIO_BTC_BOOT_PORT      BTC_BOOT_GPIO_Port
    #define GPIO_BTC_BOOT_PIN       BTC_BOOT_Pin

    #define GPIO_BTC_RESET_PORT     BTC_RESET_GPIO_Port
    #define GPIO_BTC_RESET_PIN      BTC_RESET_Pin

#elif JIGEN
    #define GPIO_SPI_CS_PORT        NCS_GPIO_Port
    #define GPIO_SPI_CS_PIN         NCS_Pin

    #define GPIO_SPI_SCK_PORT       CLK_GPIO_Port
    #define GPIO_SPI_SCK_PIN        CLK_Pin

    #define GPIO_SPI_MOSI_PORT      MOSI_GPIO_Port
    #define GPIO_SPI_MOSI_PIN       MOSI_Pin

    #define GPIO_SPI_MISO_PORT      MISO_GPIO_Port
    #define GPIO_SPI_MISO_PIN       MISO_Pin

    #define GPIO_EN_PORT            EN_GPIO_Port
    #define GPIO_EN_PIN             EN_Pin

    #define GPIO_IRQ_PORT           IRQ_GPIO_Port
    #define GPIO_IRQ_PIN            IRQ_Pin

    // tuner GPIOs
    #define GPIO_CIN_PORT           ETC_1_GPIO_Port
    #define GPIO_CIN_PIN            ETC_1_Pin

    #define GPIO_CLEN_PORT          ETC_2_GPIO_Port
    #define GPIO_CLEN_PIN           ETC_2_Pin

    #define GPIO_COUT_PORT          ETC_3_GPIO_Port
    #define GPIO_COUT_PIN           ETC_3_Pin

    // LED GPIOs
    #define GPIO_LED_RF_PORT        RF_LED_GPIO_Port
    #define GPIO_LED_RF_PIN         RF_LED_Pin

    #define GPIO_LED_PLL_PORT       PLL_LED_GPIO_Port
    #define GPIO_LED_PLL_PIN        PLL_LED_Pin

    #define GPIO_LED_NORESP_PORT    NO_RESP_LED_GPIO_Port
    #define GPIO_LED_NORESP_PIN     NO_RESP_LED_Pin

    #define GPIO_LED_CRC_PORT       CRC_ERR_LED_GPIO_Port
    #define GPIO_LED_CRC_PIN        CRC_ERR_LED_Pin

    // LDO GPIOs
    #define GPIO_PA_LDO_PORT        PA_LDO_EN_GPIO_Port
    #define GPIO_PA_LDO_PIN         PA_LDO_EN_Pin

#elif ELANCE
    #define GPIO_SPI_CS_PORT        NCS_GPIO_Port
    #define GPIO_SPI_CS_PIN         NCS_Pin

    #define GPIO_SPI_SCK_PORT       CLK_GPIO_Port
    #define GPIO_SPI_SCK_PIN        CLK_Pin

    #define GPIO_SPI_MOSI_PORT      MOSI_GPIO_Port
    #define GPIO_SPI_MOSI_PIN       MOSI_Pin

    #define GPIO_SPI_MISO_PORT      MISO_GPIO_Port
    #define GPIO_SPI_MISO_PIN       MISO_Pin

    #define GPIO_EN_PORT            EN_GPIO_Port
    #define GPIO_EN_PIN             EN_Pin

    #define GPIO_IRQ_PORT           IRQ_GPIO_Port
    #define GPIO_IRQ_PIN            IRQ_Pin

    // tuner GPIOs
    #define GPIO_CIN_PORT           ETC_1_GPIO_Port
    #define GPIO_CIN_PIN            ETC_1_Pin

    #define GPIO_CLEN_PORT          ETC_2_GPIO_Port
    #define GPIO_CLEN_PIN           ETC_2_Pin

    #define GPIO_COUT_PORT          ETC_3_GPIO_Port
    #define GPIO_COUT_PIN           ETC_3_Pin

    // antenna select GPIO
//    #define GPIO_ANT_SW_V1_PORT     V1_GPIO_Port
//    #define GPIO_ANT_SW_V1_PIN      V1_Pin

//    #define GPIO_ANT_SW_V2_PORT     V2_GPIO_Port
//    #define GPIO_ANT_SW_V2_PIN      V2_Pin

//    #define GPIO_ANT_SW_V3_PORT     V3_GPIO_Port
//    #define GPIO_ANT_SW_V3_PIN      V3_Pin

    // PA select GPIO
//    #define GPIO_PA_SW_V1_PORT      PA_SW_V1_GPIO_Port
//    #define GPIO_PA_SW_V1_PIN       PA_SW_V1_Pin

//    #define GPIO_PA_SW_V2_PORT      PA_SW_V2_GPIO_Port
//    #define GPIO_PA_SW_V2_PIN       PA_SW_V2_Pin

    // LED GPIOs

    #define GPIO_LED_CRC_PORT       Error_GPIO_Port
    #define GPIO_LED_CRC_PIN        Error_Pin    
    
    #define GPIO_LED_MCU_PORT       Activity_GPIO_Port
    #define GPIO_LED_MCU_PIN        Activity_Pin
    
    #define GPIO_LED_PLL_PORT       GPIO1_GPIO_Port
    #define GPIO_LED_PLL_PIN        GPIO1_Pin

    #define GPIO_LED_OSC_PORT       GPIO2_GPIO_Port
    #define GPIO_LED_OSC_PIN        GPIO2_Pin

    #define GPIO_LED_RF_PORT        GPIO3_GPIO_Port
    #define GPIO_LED_RF_PIN         GPIO3_Pin
    
//    #define GPIO_LED_ANT1_PORT      ANT1_LED_GPIO_Port
//    #define GPIO_LED_ANT1_PIN       ANT1_LED_Pin

//    #define GPIO_LED_ANT2_PORT      ANT2_LED_GPIO_Port
//    #define GPIO_LED_ANT2_PIN       ANT2_LED_Pin

    // shared LED
    #define GPIO_LED_TUNED_PORT     GPIO4_GPIO_Port
    #define GPIO_LED_TUNED_PIN      GPIO4_Pin
    #define GPIO_LED_TUNING_PORT    GPIO4_GPIO_Port
    #define GPIO_LED_TUNING_PIN     GPIO4_Pin

    #define GPIO_LED_NORESP_PORT    GPIO5_GPIO_Port
    #define GPIO_LED_NORESP_PIN     GPIO5_Pin

    #define GPIO_LED_TAG_PORT       GPIO6_GPIO_Port
    #define GPIO_LED_TAG_PIN        GPIO6_Pin


    // LDO GPIOs
    #define GPIO_URF_LDO_PORT       UHF_LDO_EN_GPIO_Port
    #define GPIO_HRF_LDO_PIN        UHF_LDO_EN_Pin

    #define GPIO_PA_LDO_PORT        PA_LDO_EN_GPIO_Port
    #define GPIO_PA_LDO_PIN         PA_LDO_EN_Pin

    // Buzzer GPIOs
    #define GPIO_BUZZER_PORT        Buzzer_GPIO_Port
    #define GPIO_BUZZER_PIN         Buzzer_Pin

    // Bluetooth GPIOs
    #define GPIO_BTC_BOOT_PORT      BTC_BOOT_GPIO_Port
    #define GPIO_BTC_BOOT_PIN       BTC_BOOT_Pin

    #define GPIO_BTC_RESET_PORT     BTC_RESET_GPIO_Port
    #define GPIO_BTC_RESET_PIN      BTC_RESET_Pin
#else
    #error "GPIOs undefined for selected board"
#endif

/*
 ******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************
 */
/** This function initializes uController I/O ports for the configured board
  *
  * This function does not take or return a parameter.
  */
void platformInit(void);

/** Deinitializes SPI interface to enter direct mode */
void setPortDirect(void);

/** Initializes SPI interface to exit direct mode */
void setPortNormal(void);

#endif /* PLATFORM_H_ */

/**
  * @}
  */
/**
  * @}
  */
