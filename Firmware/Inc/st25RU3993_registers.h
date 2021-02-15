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
 *  @brief ST25RU3993 Registers
 */

/** @addtogroup ST25RU3993
  * @{
  */

#ifndef _ST25RU3993_REGS_H_
#define _ST25RU3993_REGS_H_

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#define ST25RU3993_REG_STATUSCTRL           0x00
#define ST25RU3993_REG_PROTOCOLCTRL         0x01
#define ST25RU3993_REG_TXOPTIONS            0x02
#define ST25RU3993_REG_RXOPTIONS            0x03
#define ST25RU3993_REG_TRCALHIGH            0x04
#define ST25RU3993_REG_TRCALLOW             0x05
#define ST25RU3993_REG_AUTOACKTIMER         0x06
#define ST25RU3993_REG_RXNORESPONSEWAITTIME 0x07
#define ST25RU3993_REG_RXWAITTIME           0x08
#define ST25RU3993_REG_RXFILTER             0x09
#define ST25RU3993_REG_RXMIXERGAIN          0x0A
#define ST25RU3993_REG_REGULATORCONTROL     0x0B
#define ST25RU3993_REG_RFOUTPUTCONTROL      0x0C
#define ST25RU3993_REG_MISC1                0x0D
#define ST25RU3993_REG_MISC2                0x0E

#define ST25RU3993_REG_MEASUREMENTCONTROL   0x10
#define ST25RU3993_REG_VCOCONTROL           0x11
#define ST25RU3993_REG_CPCONTROL            0x12
#define ST25RU3993_REG_MODULATORCONTROL1    0x13
#define ST25RU3993_REG_MODULATORCONTROL2    0x14
#define ST25RU3993_REG_MODULATORCONTROL3    0x15
#define ST25RU3993_REG_MODULATORCONTROL4    0x16   // unused
#define ST25RU3993_REG_PLLMAIN1             0x17
#define ST25RU3993_REG_PLLMAIN2             0x18
#define ST25RU3993_REG_PLLMAIN3             0x19
#define ST25RU3993_REG_PLLAUX1              0x1A   // unused
#define ST25RU3993_REG_PLLAUX2              0x1B   // unused
#define ST25RU3993_REG_PLLAUX3              0x1C   // unused
#define ST25RU3993_REG_ICD                  0x1D

#define ST25RU3993_REG_MIXOPTS              0x22

#define ST25RU3993_REG_TEST1                0x23   // unused
#define ST25RU3993_REG_TEST2                0x24   // unused
#define ST25RU3993_REG_TEST3                0x25   // unused
#define ST25RU3993_REG_TEST4                0x26   // unused

#define ST25RU3993_REG_STATUSPAGE           0x29
#define ST25RU3993_REG_AGCANDSTATUS         0x2A
#define ST25RU3993_REG_RSSILEVELS           0x2B
#define ST25RU3993_REG_AGL                  0x2C
#define ST25RU3993_REG_ADC                  0x2D
#define ST25RU3993_REG_COMMANDSTATUS        0x2E   // unused

#define ST25RU3993_REG_DEVICEVERSION        0x33

#define ST25RU3993_REG_IRQMASK1             0x35
#define ST25RU3993_REG_IRQMASK2             0x36
#define ST25RU3993_REG_IRQSTATUS1           0x37
#define ST25RU3993_REG_IRQSTATUS2           0x38   // unused
#define ST25RU3993_REG_FIFOSTATUS           0x39
#define ST25RU3993_REG_RXLENGTHUP           0x3A
#define ST25RU3993_REG_RXLENGTHLOW          0x3B
#define ST25RU3993_REG_TXSETTING            0x3C
#define ST25RU3993_REG_TXLENGTHUP           0x3D
#define ST25RU3993_REG_TXLENGTHLOW          0x3E   // unused
#define ST25RU3993_REG_FIFO                 0x3F

#endif

/**
  * @}
  */
