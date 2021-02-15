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

#if !defined __STUHFL_HELPERS_H
#define __STUHFL_HELPERS_H

//
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#include "stuhfl.h"

// --------------------------------------------------------------------------
// to type from array
#define WORD2ARRAY(w, a)    memcpy((a), &(w), sizeof(uint16_t))
#define DWORD2ARRAY(dw, a)  memcpy((a), &(dw), sizeof(uint32_t))
// to array from type
#define ARRAY2WORD(a)       *((uint16_t*) (a))
#define ARRAY2DWORD(a)      *((uint32_t*) (a))

// --------------------------------------------------------------------------
// tlv encoding
uint16_t addTlv8(uint8_t *data, uint8_t tag, uint8_t value);
uint16_t addTlv16(uint8_t *data, uint8_t tag, uint16_t value);
uint16_t addTlv32(uint8_t *data, uint8_t tag, uint32_t value);
uint16_t addTlvExt(uint8_t *data, uint8_t tag, uint16_t len, void *value);
uint16_t addTlvExt2(uint8_t *data, uint8_t tag, uint8_t cnt, uint16_t *len, void **value);
// tlv decoding
uint16_t getTlv8(uint8_t *data, uint8_t *tag, uint8_t *value);
uint16_t getTlv16(uint8_t *data, uint8_t *tag, uint16_t *value);
uint16_t getTlv32(uint8_t *data, uint8_t *tag, uint32_t *value);
uint16_t getTlvExt(uint8_t *data, uint8_t *tag, uint16_t *len, void *value);

uint8_t getTlvTag(uint8_t *data);
uint16_t getTlvLen(uint8_t *data);

// --------------------------------------------------------------------------
char* byteArray2HexString(char* retBuf, uint16_t retBufSize, uint8_t* data, uint16_t dataLen);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__STUHFL_HELPERS_H
