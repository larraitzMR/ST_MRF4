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
 *  @brief This file includes some useful functions.
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup Global
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <string.h>

#include "global.h"

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
void uint32ToEbv(uint32_t addr, uint8_t *ebvAddr, uint8_t *ebvLen)
{
    uint8_t ebvTemp[5];
    ebvTemp[4] = (uint8_t)((addr&      0x7F));
    ebvTemp[3] = (uint8_t)((addr&    0x3F80)>>7)  | 0x80;
    ebvTemp[2] = (uint8_t)((addr&  0x1FC000)>>14) | 0x80;
    ebvTemp[1] = (uint8_t)((addr& 0xFE00000)>>21) | 0x80;
    ebvTemp[0] = (uint8_t)((addr&0xF0000000)>>28) | 0x80; // would be 0x7F0000000, but we end here since we convert only 32 bit (uint32_t)

    if(addr>=0x10000000) {
        *ebvLen = 5;
    }
    else if(addr>=0x200000) {
        *ebvLen = 4;
    }
    else if(addr>=0x4000) {
        *ebvLen = 3;
    }
    else if(addr>=0x80) {
        *ebvLen = 2;
    }
    else {
        *ebvLen = 1;
    }
    memcpy(ebvAddr, &ebvTemp[5-*ebvLen], *ebvLen);
}

/*------------------------------------------------------------------------- */
void insertBitStream(uint8_t *dest, uint8_t const *source, uint8_t len, uint8_t bitpos)
{
    uint8_t i;
    uint8_t mask0 = (1<<bitpos)-1;
    uint8_t mask1 = (1<<(8-bitpos))-1;

    for(i=0 ; i<len ; i++)
    {
        dest[i+0] &= (~mask0);
        dest[i+0] |= ((source[i]>>(8-bitpos)) & mask0);
        dest[i+1] &= ((~mask1) << bitpos);
        dest[i+1] |= ((source[i] & mask1) << bitpos);
    }
}

/*------------------------------------------------------------------------- */
void extractBitStream(uint8_t * dest, uint8_t const *source, uint16_t bitLen, uint16_t bitpos)
{
    uint16_t len = 0;
    
    if (bitpos%8 == 0) {
        memcpy(dest, &source[bitpos/8], (bitLen+7)/8);     // Copies all bits (even some extra ones)
        len = bitLen/8;
    }
    else {
        uint16_t maxParsedBits = bitpos + bitLen + 7;
        uint8_t shift = 8 - (bitpos%8);

        uint16_t tmp = source[bitpos/8];  // Initiate shift buffer with first byte
        for (uint16_t bitIndex=bitpos+8 ; bitIndex<maxParsedBits ; bitIndex += 8) {
            tmp <<= 8;                  // shift irrelevant bits
            if ((bitIndex/8) < (maxParsedBits/8)) {    // Parsed bits do not exceed input buffer
                tmp |= source[bitIndex/8];                  // Add next input buffer byte
            }
            dest[len++] = (uint8_t)(tmp >> shift);       // Use shift to take only relevant bits
        }            
    }

    if (bitLen%8 != 0) {
        // Eventually pads useless bits to 0
        uint8_t paddedbits = 8-(bitLen%8);
        dest[len] = (dest[len] >> paddedbits) << paddedbits;    // Double shift ensures pad with 0
        len++;
    }
}

/*------------------------------------------------------------------------- */
uint32_t readU32FromLittleEndianBuffer(uint8_t const *buffer)
{
    return (uint32_t)(buffer[0] | ((uint32_t)buffer[1] << 8) |
            ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24));
}

/**
  * @}
  */
/**
  * @}
  */
