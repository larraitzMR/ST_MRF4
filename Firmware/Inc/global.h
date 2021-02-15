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
 *  @brief This file provides declarations for global helper functions.
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup Global
  * @{
  */

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

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
/**
 * Converts uint32_t number into EBV format. The EBV is stored in parameter ebv.
 * Parameter len will be set to the length of data in ebv.
 * @param addr uint32_t address to convert into EBV
 * @param ebvAddr array to store the EBV
 * @param ebvLen number of resulting EBV bytes
 */
void uint32ToEbv(uint32_t addr, uint8_t *ebvAddr, uint8_t *ebvLen);

/**
 * Inserts an array of uint8_t values into an uint8_t buffer at a specific bitpos
 * @param dest Destination buffer
 * @param source Source buffer
 * @param len Number of bytes in source buffer
 * @param bitpos bitposition (1-8) on destination buffer where source values should be put.
 */
void insertBitStream(uint8_t *dest, uint8_t const *source, uint8_t len, uint8_t bitpos);

/**
 * Extract an array of uint8_t values from an uint8_t buffer from a given bitpos
 * @param dest Dest buffer
 * @param source Source buffer
 * @param bitLen Number of bit in source buffer to extract
 * @param bitpos absolute bit position on source buffer where extraction should start.
 */
void extractBitStream(uint8_t * dest, uint8_t const *source, uint16_t bitLen, uint16_t bitpos);

uint32_t readU32FromLittleEndianBuffer(uint8_t const *buffer);

#endif

/**
  * @}
  */
/**
  * @}
  */
