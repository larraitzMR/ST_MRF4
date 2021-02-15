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
 *  @brief CRC calculation module header file
 *
 *  This module calculates the CRC-16 checksum according to the CCITT standard.
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup CRC
  * @{
  */

#ifndef CRC_H
#define CRC_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
uint8_t crc5Bitwise(const uint8_t *data, uint16_t bits);
uint16_t crc16Bitwise(const uint8_t *data, uint16_t bits);
uint16_t crc16Bytewise(const uint8_t *data, uint16_t len);

#endif /* CRC_H */

/**
  * @}
  */
/**
  * @}
  */
