/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "platform.h"
#include "stream_dispatcher.h"
#include "evalAPI_commands.h"
#include "timer.h"
#include "bootloader.h"
#include "logger.h"
#include "bitbang.h"
#include "adc_driver.h"
#include "stdbool.h"

#if STUHFL_DEVICE_MODE
#include "stuhfl_evalAPI.h"
#endif

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
extern volatile uint16_t rtcTimerMsValue;

#if STUHFL_DEVICE_MODE
#include "stuhfl_evalAPI.h"
static STUHFL_T_RET_CODE cbInventoryCycle(STUHFL_T_Inventory_Data *cycleData);

uint32_t inventoryCycleCbCnt = 0;
static void doInventoryCfg(bool singleTag, bool freqHopping, int antenna);
#endif


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  bootloaderCheck();
  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_TIM5_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_TIM16_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
#if FAST_UART
  /* By default host UART communication is configured to 115200 baud. 
     EVAL boad FDTI converter is able to handle baudrates up to 3MB/sec */
  
  /* highest possible baudrate that is supported by FTDI converter IC of EVAL board */
  UART_HANDLE.Init.BaudRate = 3000000;       
  /* other available higher Baudrates: 230400; 460800; 500000; 576000; 921600; 1000000; 1152000; 1500000; 2000000; 2500000; */

  if (HAL_UART_Init(&UART_HANDLE) != HAL_OK){
    Error_Handler();
  }
#endif

  platformInit();
  ledTimerStart();
  LOG("FW Version = %02d.%02d.%02d.%02d\n",FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_MICRO, FW_VERSION_BUILD);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
#if STUHFL_DEVICE_MODE
 
  

  /** EVAL API based device code starts here .. */

  STUHFL_T_Inventory_Option invOption;
  STUHFL_T_Inventory_Data invData = { 0 };
  // apply data storage location, where the found TAGs shall be stored
  STUHFL_T_Inventory_Tag tagData[1] = { 0 };
  invData.tagList = tagData;
  // setup options
  invOption.rssiMode = RSSI_MODE_2NDBYTE;
  invOption.roundCnt = 1;
  invOption.inventoryDelay = 0;
  invOption.reportOptions = 0x00;
  // assign correct size of applied tag storage
  invData.tagListSizeMax = 1;
    
  doInventoryCfg(true, false, ANTENNA_1);  // single TAG
  //doInventoryCfg(false, false, ANTENNA_2);   // multiple TAGs
    
  //Gen2_Inventory(&invOption, &invData);

    
  inventoryCycleCbCnt = 0;
  InventoryRunnerStart(&invOption, cbInventoryCycle, NULL, &invData);
  /**
    * NOTE: Starting the inventory runner is a blocking call. Whenever 
    * transponders found the STUHFL_T_InventoryCycle callback is called.
    * Within this callback processing of the data should take place.
    * 
    * To stop the runner and return from the InventoryRunnerStart function
    * there are 2 possibilities.
    *
    *   1. A call to InventoryRunnerStop. Can be called from the callback
    *   2. When the number of requested rounds are executed
    *
  */
  
  for(int i = 1; i < 16; i++){
    invOption.roundCnt = i;
    inventoryCycleCbCnt = 0;
    InventoryRunnerStart(&invOption, cbInventoryCycle, NULL, &invData);
    invOption.roundCnt = 0;  
  }
#endif



  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    cycle();    
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure LSE Drive Capability 
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USART3
                              |RCC_PERIPHCLK_ADC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_HSI;
  PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
  PeriphClkInit.PLLSAI1.PLLSAI1N = 8;
  PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
  PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_ADC1CLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the main internal regulator output voltage 
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/*------------------------------------------------------------------------- */
/** overloaded function from stm32xxxx_hal_gpio.c, 
  * define GPIO_IRQ_PIN appropriately */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_IRQ_PIN)
    {
        st25RU3993Isr();
    }
}

/*------------------------------------------------------------------------- */
/** overloaded function from stm32xxxx_hal_tim.c, 
  * define TIMER_LED_HANDLE appropriately
  * define TIMER_BB_HANDLE appropriately */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
#if EVAL || DISCOVERY
    if(htim == &TIMER_LED_HANDLE)
        HAL_GPIO_TogglePin(GPIO_LED_MCU_PORT,GPIO_LED_MCU_PIN);
#endif
}

/*------------------------------------------------------------------------- */
/** overloaded function from stm32xxxx_hal_rtc.c */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    rtcTimerMsValue += 10;             // increase ms counter by 10 as period is 10ms
}

/*------------------------------------------------------------------------- */
/** overloaded function from stm32xxxx_hal_rtc.c */
void HAL_RTC_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
    rtcTimerMsValue += 10;             // increase ms counter by 10 as period is 10ms
}

/*------------------------------------------------------------------------- */
/** overloaded function from stm32xxxx_hal_uart.c, 
  * define LOG_HANDLE appropriately */
void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    // when this happens we can write again the 1st half of the dma buffer
    if(huart == &LOG_HANDLE){
    }
}

extern volatile bool uartTxInProgress;
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    // when this happens when can write again the 2nd half of the dma buffer
    if(huart == &LOG_HANDLE){
    }
    
    if(huart == &UART_HANDLE){
        uartTxInProgress = false;
    }    
}


/*------------------------------------------------------------------------- */
#if !JIGEN
/** overloaded function from stm32xxxx_hal_tim.c, 
  * define ADC_HANDLE appropriately */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc == &ADC_HANDLE)
    {
        adcIsr();
    }
}
#endif

/*------------------------------------------------------------------------- */
#if STUHFL_DEVICE_MODE
static void doInventoryCfg(bool singleTag, bool freqHopping, int antenna)
{
    STUHFL_T_ST25RU3993_TxRx_Cfg txRxCfg;
    GetTxRxCfg(&txRxCfg);
    txRxCfg.txOutputLevel = -2;
    txRxCfg.rxSensitivity = 3;
    txRxCfg.usedAntenna = antenna;
    txRxCfg.alternateAntennaInterval = 1;
    SetTxRxCfg(&txRxCfg);

    STUHFL_T_ST25RU3993_Gen2Inventory_Cfg invGen2Cfg;
    GetGen2InventoryCfg(&invGen2Cfg);
    invGen2Cfg.fastInv = TRUE;
    invGen2Cfg.autoAck = FALSE;
    invGen2Cfg.readTID = FALSE;
    invGen2Cfg.startQ = singleTag ? 0 : 4;
    invGen2Cfg.adaptiveQEnable = singleTag ? 0 : 1;
    invGen2Cfg.adjustOptions = 0;
    invGen2Cfg.adjustC2 = 35;
    invGen2Cfg.adjustC1 = 15;
    invGen2Cfg.autoTuningAlgo = TUNING_ALGO_FAST;
    invGen2Cfg.autoTuningInterval = 7;
    invGen2Cfg.autoTuningLevel = 20;
    invGen2Cfg.adaptiveSensitivityEnable = true;
    invGen2Cfg.adaptiveSensitivityInterval = 5;
    invGen2Cfg.sel = 0;
    invGen2Cfg.session = GEN2_SESSION_S0;
    invGen2Cfg.target = GEN2_TARGET_A;
    invGen2Cfg.toggleTarget = true;
    SetGen2InventoryCfg(&invGen2Cfg);

    //
    STUHFL_T_ST25RU3993_Gen2Protocol_Cfg gen2ProtocolCfg;
    GetGen2ProtocolCfg(&gen2ProtocolCfg);
    gen2ProtocolCfg.tari = GEN2_TARI_12_50;
    gen2ProtocolCfg.blf = GEN2_LF_320;
    gen2ProtocolCfg.coding = GEN2_COD_MILLER2;
    gen2ProtocolCfg.trext = true;
    SetGen2ProtocolCfg(&gen2ProtocolCfg);

    if (freqHopping) {
        // EU frequnecy profile with hopping
        STUHFL_T_ST25RU3993_Freq_Profile freqProfile;
        freqProfile.profile = PROFILE_EUROPE;
        SetFreqProfile(&freqProfile);
    } else {
        STUHFL_T_ST25RU3993_Freq_Profile newProfile;
        newProfile.profile = PROFILE_CUSTOM;
        SetFreqProfile(&newProfile);

        // EU single frequency without hopping
        STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom freqCustom;
        freqCustom.clearList = true;
        freqCustom.frequency = 865700;
        SetFreqProfileAdd2Custom(&freqCustom);

        // EU max sending time
        STUHFL_T_ST25RU3993_Freq_Hop freqHop;
        freqHop.maxSendingTime = 4000;
        SetFreqHop(&freqHop);
    }
}
static STUHFL_T_RET_CODE cbInventoryCycle(STUHFL_T_Inventory_Data *cycleData)
{
    inventoryCycleCbCnt++;    
    return ERR_NONE;
}
#endif

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
