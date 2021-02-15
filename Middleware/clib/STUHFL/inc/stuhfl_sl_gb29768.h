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

//
#if !defined __STUHFL_SL_GB29768_H
#define __STUHFL_SL_GB29768_H

#include "stuhfl.h"
#include "stuhfl_sl.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#pragma pack(push, 1)

#define GB29768_SELECT_MODE_CLEAR_LIST      0
#define GB29768_SELECT_MODE_ADD2LIST        1
#define GB29768_SELECT_MODE_CLEAR_AND_ADD   2

#define GB29768_TARGET_S0                   0x00
#define GB29768_TARGET_S1                   0x01
#define GB29768_TARGET_S2                   0x02
#define GB29768_TARGET_S3                   0x03
#define GB29768_TARGET_MATCHINGFLAG         0x04

#define GB29768_AREA_TAGINFO                0x00
#define GB29768_AREA_CODING                 0x10
#define GB29768_AREA_SECURITY               0x20
#define GB29768_AREA_USER                   0x30

#define GB29768_RULE_MATCH1_ELSE_0          0x00
#define GB29768_RULE_MATCHX_ELSE_0          0x01
#define GB29768_RULE_MATCH1_ELSE_X          0x02
#define GB29768_RULE_MATCH0_ELSE_1          0x03

typedef struct {
    uint8_t                             mode;           /**< I Param: Select mode to be applied (CLEAR_LIST, ADD2LIST, CLEAR_AND_ADD). */
    uint8_t                             target;         /**< I Param: indicates whether the select modifies a tag's match flag or its inventoried flag. */
    uint8_t                             rule;           /**< I Param: Elicit the tag behavior according to Gb29768 Sort specification. Nota: the rule GB29768_RULE_MATCH0_ELSE_1 (0x03) is to be used to invert selection. */
    uint8_t                             storageArea;    /**< I Param: Bank (TagInfo, Coding, Security, User) on which apply the sort. */
    uint8_t                             mask[32];       /**< I Param: Selection mask. */
    uint32_t                            bitPointer;     /**< I Param: Bit starting address to which mask is applied (bit address). */
    uint8_t                             bitLength;      /**< I Param: Mask length (in bits). */
} STUHFL_T_Gb29768_Sort;
#define STUHFL_O_GB29768_SORT_INIT(...) ((STUHFL_T_Gb29768_Sort) { .mode = GB29768_SELECT_MODE_CLEAR_LIST, .target = GB29768_TARGET_S0, .rule = 0, .storageArea = GB29768_AREA_TAGINFO, .mask = {0}, .bitPointer = 0, .bitLength = 0, ##__VA_ARGS__ })

#define GB29768_CONFIGURATION_ATTRIBUTE         0x00
#define GB29768_CONFIGURATION_SECURITYMODE      0x01

#define GB29768_ACTION_ATTRIBUTE_READWRITE      0x00
#define GB29768_ACTION_ATTRIBUTE_READUNWRITE    0x01
#define GB29768_ACTION_ATTRIBUTE_UNREADWRITE    0x02
#define GB29768_ACTION_ATTRIBUTE_UNREADUNWRITE  0x03

#define GB29768_ACTION_SECMODE_RESERVED         0x00
#define GB29768_ACTION_SECMODE_NOAUTH           0x01
#define GB29768_ACTION_SECMODE_AUTH_NOSECCOMM   0x02
#define GB29768_ACTION_SECMODE_AUTH_SECCOMM     0x03

typedef struct {
    uint8_t                 storageArea;            /**< I Param: Bank (TagInfo, Coding, Security, User) on which apply the lock. */
    uint8_t                 configuration;          /**< I Param: Configure attribute and security mode of storage area. */
    uint8_t                 action;                 /**< I Param: Define how the lock operation is performed. */
    uint8_t                 pwd[PASSWORD_LEN];      /**< I Param: Password. */
} STUHFL_T_Gb29768_Lock;
#define STUHFL_O_GB29768_LOCK_INIT(...) ((STUHFL_T_Gb29768_Lock) { .storageArea = GB29768_AREA_TAGINFO, .configuration = GB29768_CONFIGURATION_ATTRIBUTE, .action = GB29768_ACTION_ATTRIBUTE_READWRITE, .pwd = {0}, ##__VA_ARGS__ })

typedef struct {
    uint8_t                             storageArea;                /**< I Param: Bank (TagInfo, Coding, Security, User) on which apply the erase. */
    uint8_t                             bytes2Erase;                /**< I Param: Number of bytes to erase. */
    uint32_t                            bytePtr;                    /**< I Param: Byte address to which erase data. */
    uint8_t                             pwd[PASSWORD_LEN];          /**< I Param: Password. */
} STUHFL_T_Gb29768_Erase;
#define STUHFL_O_GB29768_ERASE_INIT(...) ((STUHFL_T_Gb29768_Erase) { .storageArea = GB29768_AREA_TAGINFO, .bytes2Erase = 0, .bytePtr = 0, .pwd = {0}, ##__VA_ARGS__ })

#pragma pack(pop)

/**
 * Perform Gb29768 Inventory depending on the current inventory and Gb29768 configuration
 * @param invOption: See STUHFL_T_Inventory_Option struct for further info
 * @param invData: See STUHFL_T_Inventory_Data struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gb29768_Inventory(STUHFL_T_Inventory_Option *invOption, STUHFL_T_Inventory_Data *invData);

/**
 * Perform Gb29768 Select command to select or filter transponders
 * @param sortData: See STUHFL_T_Gb29768_Sort struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gb29768_Sort(STUHFL_T_Gb29768_Sort *sortData);

/**
 * Perform Gb29768 Read command to read from the Gb29768 transponders
 * @param readData: See STUHFL_T_Read struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gb29768_Read(STUHFL_T_Read *readData);
/**
 * Perform Gb29768 Write command to write data to Gb29768 transponders
 * @param writeData: See STUHFL_T_Write struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gb29768_Write(STUHFL_T_Write *writeData);

/**
 * Perform Gb29768 Lock command to lock transponders
 * @param lockData: See STUHFL_T_Gb29768_Lock struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gb29768_Lock(STUHFL_T_Gb29768_Lock *lockData);

/**
 * Perform Gb29768 Kill command to kill transponders.
 * NOTE: After this command your transponders will not be functional anymore
 * @param killData: See STUHFL_T_Gb29768_Kill struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gb29768_Kill(STUHFL_T_Kill *killData);

/**
 * Perform Gb29768 erase command to erase data in transponders.
 * @param eraseData: See STUHFL_T_Gb29768_Erase struct for further info
 *
 * @return error code
*/
STUHFL_DLL_API STUHFL_T_RET_CODE CALL_CONV STUHFL_F_Gb29768_Erase(STUHFL_T_Gb29768_Erase *eraseData);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __STUHFL_SL_GB29768_H
