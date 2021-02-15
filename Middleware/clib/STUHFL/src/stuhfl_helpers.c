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
 *  @brief 
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup PC_Communication
  * @{
  */

#include "stuhfl_helpers.h"
#include "stuhfl_platform.h"


uint16_t getTlv8(uint8_t *data, uint8_t *tag, uint8_t *value)
{
    uint16_t i = 0;
    *tag = data[i++];
    i++;    //*len = data[1];
    if (value != NULL) {
        *value = data[i++];
    }
    i = (uint16_t)(i + (uint16_t)sizeof(uint8_t));

    return i;
}
uint16_t getTlv16(uint8_t *data, uint8_t *tag, uint16_t *value)
{
    uint16_t i = 0;
    *tag = data[i];
    i++;    //*len = data[1];
    if (value != NULL) {
        *value = ARRAY2WORD(&data[i]);
    }
    i = (uint16_t)(i + (uint16_t)sizeof(uint16_t));

    return i;
}
uint16_t getTlv32(uint8_t *data, uint8_t *tag, uint32_t *value)
{
    uint16_t i = 0;
    *tag = data[i++];
    i++;    //*len = data[1];
    if (value != NULL) {
        *value = ARRAY2DWORD(&data[i]);
    }
    i = (uint16_t)(i + (uint16_t)sizeof(uint32_t));

    return i;
}
uint16_t getTlvExt(uint8_t *data, uint8_t *tag, uint16_t *len, void *value)
{
    uint16_t i = 0;
    *tag = data[i++];
    if (data[i] & 0x80) {
        data[i] &= 0x7F;
        *len = (uint16_t)(data[i++] << 8);
        *len = (uint16_t)(*len | (uint16_t)data[i++]);
    } else {
        *len = data[i++];
    }
    if (value != NULL) {
        memcpy(value, &data[i], *len);
    }
    i = (uint16_t)(i + (uint16_t)*len);

    return i;
}
uint8_t getTlvTag(uint8_t *data)
{
    return data[0];
}
uint16_t getTlvLen(uint8_t *data)
{
    uint16_t i = 1;
    uint16_t len = 0;
    if (data[i] & 0x80) {
        data[i] &= 0x7F;
        len = (uint16_t)(data[i++] << 8);
        len = (uint16_t)(len | (uint16_t)data[i++]);
    } else {
        len = data[i++];
    }
    return len;
}
uint16_t addTlv8(uint8_t *data, uint8_t tag, uint8_t value)
{
    uint16_t i = 0;
    data[i++] = tag;
    data[i++] = sizeof(uint8_t);
    data[i++] = value;

    return i;
}
uint16_t addTlv16(uint8_t *data, uint8_t tag, uint16_t value)
{
    uint16_t i = 0;
    data[i++] = tag;
    data[i++] = sizeof(uint16_t);
    WORD2ARRAY(value, &data[i]);
    i = (uint16_t)(i + (uint16_t)sizeof(uint16_t));

    return i;
}
uint16_t addTlv32(uint8_t *data, uint8_t tag, uint32_t value)
{
    uint16_t i = 0;
    data[i++] = tag;
    data[i++] = sizeof(uint32_t);
    DWORD2ARRAY(value, &data[i]);
    i = (uint16_t)(i + (uint16_t)sizeof(uint32_t));

    return i;
}
uint16_t addTlvExt(uint8_t *data, uint8_t tag, uint16_t len, void *value)
{
    uint16_t i = 0;
    data[i++] = tag;
    if (len > 0x7F) {
        data[i++] = (uint8_t)(0x80U | (len >> 8));
        data[i++] = (uint8_t)(len & 0x00FF);
    } else {
        data[i++] = (uint8_t)len;
    }
    memcpy(&data[i], value, len);
    i = (uint16_t)(i + (uint16_t)len);

    return i;
}
uint16_t addTlvExt2(uint8_t *data, uint8_t tag, uint8_t cnt, uint16_t *len, void **value)
{
    uint8_t x;
    uint16_t i = 0;
    data[i++] = tag;

    uint16_t l = 0;
    for (x = 0; x < cnt; x++) {
        l = (uint16_t)(l + (uint16_t)len[x]);
    }

    if (l > 0x7F) {
        data[i++] = (uint8_t)(0x80 | (l >> 8));
        data[i++] = (uint8_t)(l & 0x00FF);
    } else {
        data[i++] = (uint8_t)l;
    }
    for (x = 0; x < cnt; x++) {
        memcpy(&data[i], value[x], len[x]);
        i = (uint16_t)(i + (uint16_t)len[x]);
    }

    return i;
}


char* byteArray2HexString(char* retBuf, uint16_t retBufSize, uint8_t* data, uint16_t dataLen)
{
    int j = 0;
    for (int i = 0; i < dataLen; i++) {
        j += snprintf(retBuf + j, (uint32_t)(retBufSize - j), "%02X", (uint8_t)data[i]);
        if (j >= (retBufSize - 3)) {    // Enough room for the 2 next upcoming bytes ?
            break;
        }
    }
    retBuf[j] = 0;
    return retBuf;
}


/**
  * @}
  */
/**
  * @}
  */
