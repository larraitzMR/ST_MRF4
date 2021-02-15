/******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2020 STMicroelectronics</center></h2>
  *
  * Licensed under ST MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/myliberty
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
 *  @brief Todo
 *
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "rffe.h"
#include "stm32l4xx_hal.h"
#include <string.h>

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
GPIO_TypeDef *DATA_PORT;
GPIO_TypeDef *CLK_PORT;
GPIO_TypeDef *GPIO_PORT;
uint16_t DATA_PIN;
uint16_t CLK_PIN;
uint16_t GPIO_PIN;

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
static uint8_t data_out[22];

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void Delay_RFFE()
{
    //HAL_Delay(1);
}

/*------------------------------------------------------------------------- */
uint8_t calcParity(uint8_t *data, uint8_t dataLen) 
{    
    uint8_t p = 0;
    for(int i = 0; i < dataLen; i++)
    {
        if(data[i])
            p++;
    }
    
    return !(p%2);
}

// Init
/*------------------------------------------------------------------------- */
void RFFE_Init(GPIO_TypeDef *DataPort, uint16_t DataPin, GPIO_TypeDef *ClkPort, uint16_t ClkPin, GPIO_TypeDef *GpioPort, uint16_t GpioPin)
{
    DATA_PORT = DataPort;
    DATA_PIN = DataPin;
    
    CLK_PORT = ClkPort;
    CLK_PIN = ClkPin;

    GPIO_PORT = GpioPort;
    GPIO_PIN = GpioPin;
    
    // reset clk and data lines
    HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
    if(GpioPort != NULL){
    HAL_GPIO_WritePin(GPIO_PORT, GPIO_PIN, GPIO_PIN_RESET);
    }
}

// Send Frame to HVDAC RFFE
/*------------------------------------------------------------------------- */
void RFFE_WriteRegister(uint8_t USID, uint8_t RegAddr, uint8_t Data)
{
    uint8_t i; 
    
    // build send frame
    memset(data_out, 0, 22);
    
    // USID
    data_out[0] = USID & 0x08 ? 1 : 0;
    data_out[1] = USID & 0x04 ? 1 : 0;
    data_out[2] = USID & 0x02 ? 1 : 0;
    data_out[3] = USID & 0x01 ? 1 : 0;
    // WD
    data_out[4] = 0;
    data_out[5] = 1;
    data_out[6] = 0;
    // RegAddr
    data_out[7] = RegAddr & 0x10 ? 1 : 0;
    data_out[8] = RegAddr & 0x08 ? 1 : 0;
    data_out[9] = RegAddr & 0x04 ? 1 : 0;
    data_out[10] = RegAddr & 0x02 ? 1 : 0;
    data_out[11] = RegAddr & 0x01 ? 1 : 0;
    // Parity
    data_out[12] = calcParity(data_out, 12);
    // Data
    data_out[13] = Data & 0x80 ? 1 : 0;
    data_out[14] = Data & 0x40 ? 1 : 0;
    data_out[15] = Data & 0x20 ? 1 : 0;
    data_out[16] = Data & 0x10 ? 1 : 0;
    data_out[17] = Data & 0x08 ? 1 : 0;
    data_out[18] = Data & 0x04 ? 1 : 0;
    data_out[19] = Data & 0x02 ? 1 : 0;
    data_out[20] = Data & 0x01 ? 1 : 0;
    // Parity
    data_out[21] = calcParity(&data_out[13], 8);
    
    //Start Code 010
    HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_RESET);
    Delay_RFFE();
    HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_SET);
    Delay_RFFE();
    HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_RESET);
    Delay_RFFE();

    // Data (send USID, Cmd, RegAddr)
    for(i=0; i<=21; i++)
    {
        if(data_out[i])
            HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_RESET);
        
        Delay_RFFE();
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
        Delay_RFFE();
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
        Delay_RFFE();
    }
  
    // BusPark
    HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
    Delay_RFFE();
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
    Delay_RFFE();
} 

// Read from RFFE
/*------------------------------------------------------------------------- */
void RFFE_ReadRegister(uint8_t USID, uint8_t RegAddr, uint8_t *Data)
{
    uint8_t i;
    
    // build send frame
    memset(data_out, 0, 22);
    
    // USID
    data_out[0] = USID & 0x08 ? 1 : 0;
    data_out[1] = USID & 0x04 ? 1 : 0;
    data_out[2] = USID & 0x02 ? 1 : 0;
    data_out[3] = USID & 0x01 ? 1 : 0;
    // RD
    data_out[4] = 0;
    data_out[5] = 1;
    data_out[6] = 1;
    // RegAddr
    data_out[7] = RegAddr & 0x10 ? 1 : 0;
    data_out[8] = RegAddr & 0x08 ? 1 : 0;
    data_out[9] = RegAddr & 0x04 ? 1 : 0;
    data_out[10] = RegAddr & 0x02 ? 1 : 0;
    data_out[11] = RegAddr & 0x01 ? 1 : 0;
    // Parity
    data_out[12] = calcParity(data_out, 12);
    
    //Start Code 010
    HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_RESET);
    Delay_RFFE();
    HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_SET);
    Delay_RFFE();
    HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_RESET);
    Delay_RFFE();
    
    // Data (send USID, Cmd, RegAddr)
    for(i=0; i<=12; i++)
    {
        if(data_out[i])
            HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_RESET);
        
        Delay_RFFE();
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
        Delay_RFFE();
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
        Delay_RFFE();
    }

    // BusPark
    HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
    Delay_RFFE();
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
    Delay_RFFE();

    // set IO pin set to input mode
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pin = DATA_PIN;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(DATA_PORT, &GPIO_InitStructure);
    
    // Data (received Data)
    for(i=13; i<=21; i++)
    {
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
        Delay_RFFE();
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
        Delay_RFFE();
        
        
        if(HAL_GPIO_ReadPin(DATA_PORT, DATA_PIN) != (uint32_t)GPIO_PIN_RESET)
            data_out[i] = SET;
        else
            data_out[i] = RESET;
        Delay_RFFE();
    }
    
    // IO back to output mode
    GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(DATA_PORT, &GPIO_InitStructure);
    
    // BusPark
    HAL_GPIO_WritePin(DATA_PORT, DATA_PIN, GPIO_PIN_RESET);
    Delay_RFFE();
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
    Delay_RFFE();
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
    Delay_RFFE();

    // decode bits
    *Data = 0;
    for(i=0; i<8; i++)
    {
        *Data |= data_out[i+13] << (7-i);
    }
}

