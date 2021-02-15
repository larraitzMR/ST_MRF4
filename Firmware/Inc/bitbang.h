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
 *  @brief Bitbang module header file
 *
 *  The bitbang module is needed to support serial protocols via bitbang technique.
 */

/** @addtogroup Protocol
  * @{
  */

#ifndef BITBANG_H
#define BITBANG_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "platform.h"
#include <stdint.h>

#if GB29768
#include "gb29768.h"
#endif


/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#if GB29768
/* DEBUG options */
//#define GB29768PULSES_DEBUG

#define MAX_RESPONSE_BYTES      64
#define MAX_RESPONSE_BITS       (MAX_RESPONSE_BYTES*8)

#define GB29768_MILLER_BIT_MAX_PULSES   (8*2)       /* MILLER8 needs 8*2 pulses for 1 bit */
#define MAX_RESPONSE_PULSES             (GB29768_MILLER_BIT_MAX_PULSES + 4)

#ifdef GB29768PULSES_DEBUG
#define MAX_RESPONSE_PULSES_DEBUG       1024        /* Store all pulses whatever selected Miller (may fail if > MILLER2) */
#endif
#endif  // GB29768

/*
******************************************************************************
* STRUCTS
******************************************************************************
*/

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
#if GB29768
gb29768_error_t gb29768GetMillerBits(uint8_t *bits, uint32_t *nbBits, gb29768Config_t *gbCfg, uint8_t lastCommand);
gb29768_error_t gb29768GetFM0Bits(uint8_t *bits, uint32_t *nbBits, gb29768Config_t *gbCfg, uint8_t lastCommand);
gb29768_error_t gb29768RxFlush(uint32_t blf);
#endif

#endif /* BITBANG_H */

/**
  * @}
  */
/**
  * @}
  */
