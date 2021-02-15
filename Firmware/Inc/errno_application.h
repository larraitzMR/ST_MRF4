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
 *  @brief This file provides defines for error codes reported by the FW to the host.
 *
 *  This file contains error codes for Gen2 protocol handling, Gen2 tag access functions
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup PC_Communication
  * @{
  */

#ifndef ERRNO_ST25RU3993_H
#define ERRNO_ST25RU3993_H

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#if defined(POSIX)
#define APPLI_ERROR_TYPE   (unsigned)    
#else
#define APPLI_ERROR_TYPE
#endif

/* Error codes to be used within the application.
 * They are represented by an int8_t */
#define ERR_NONE            APPLI_ERROR_TYPE ( 0) /**< no error occured */
#define ERR_NOMEM           APPLI_ERROR_TYPE (-1) /**< not enough memory to perform the requested operation */
#define ERR_BUSY            APPLI_ERROR_TYPE (-2) /**< device or resource busy */
#define ERR_IO              APPLI_ERROR_TYPE (-3) /**< generic IO error */
#define ERR_TIMEOUT         APPLI_ERROR_TYPE (-4) /**< error due to timeout */
#define ERR_REQUEST         APPLI_ERROR_TYPE (-5) /**< invalid request or requested function can't be executed at the moment */
#define ERR_NOMSG           APPLI_ERROR_TYPE (-6) /**< No message of desired type */
#define ERR_PARAM           APPLI_ERROR_TYPE (-7) /**< Parameter error */
#define ERR_PROTO           APPLI_ERROR_TYPE (-8) /**< Protocol error */

#define ERR_LAST_ERROR              (-32)  /**< last error number reserved for general errors */
#define ERR_FIRST_ST25RU3993_ERROR  (ERR_LAST_ERROR-1)   /**< first error number reserved for specific chip errors */

/* Errors primarily raised by ST25RU3993 itself, however codes like ERR_CHIP_CRCERROR can be rereused by other protocols */
#define ERR_CHIP_NORESP             APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 0)    /* -33 */
#define ERR_CHIP_HEADER             APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 1)
#define ERR_CHIP_PREAMBLE           APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 2)
#define ERR_CHIP_RXCOUNT            APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 3)
#define ERR_CHIP_CRCERROR           APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 4)
#define ERR_CHIP_FIFO               APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 5)
#define ERR_CHIP_COLL               APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 6)

#define ERR_REFLECTED_POWER         APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 16)

/* Upper level protocol errors for GEN2, e.g. a Write can fail when access command has failed... */
#define ERR_GEN2_SELECT             APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 32)
#define ERR_GEN2_ACCESS             APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 33)
#define ERR_GEN2_REQRN              APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 34)
#define ERR_GEN2_CHANNEL_TIMEOUT    APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 35)

/* Gen2V2 error codes */
#define ERR_GEN2_ERRORCODE_OTHER            APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 36)
#define ERR_GEN2_ERRORCODE_NOTSUPPORTED     APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 37)
#define ERR_GEN2_ERRORCODE_PRIVILEGES       APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 38)
#define ERR_GEN2_ERRORCODE_MEMOVERRUN       APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 39)
#define ERR_GEN2_ERRORCODE_MEMLOCKED        APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 40)
#define ERR_GEN2_ERRORCODE_CRYPTO           APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 41)
#define ERR_GEN2_ERRORCODE_ENCAPSULATION    APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 42)
#define ERR_GEN2_ERRORCODE_RESPBUFOVERFLOW  APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 43)
#define ERR_GEN2_ERRORCODE_SECURITYTIMEOUT  APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 44)
#define ERR_GEN2_ERRORCODE_POWER_SHORTAGE   APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 45)
#define ERR_GEN2_ERRORCODE_NONSPECIFIC      APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 46)

/* Upper level protocol errors for GB29768 */
#define ERR_GB29768_POWER_SHORTAGE          APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 48)  /*!< Not enough power from reader */
#define ERR_GB29768_PERMISSION_ERROR        APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 49)  /*!< Permission error */
#define ERR_GB29768_STORAGE_OVERFLOW        APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 50)  /*!< Tag memory too short */
#define ERR_GB29768_STORAGE_LOCKED          APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 51)  /*!< Tag memory locked */
#define ERR_GB29768_PASSWORD_ERROR          APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 52)  /*!< Password error */
#define ERR_GB29768_AUTH_ERROR              APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 53)  /*!< Authentication error */
#define ERR_GB29768_ACCESS_ERROR            APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 54)  /*!< Tag access error */
#define ERR_GB29768_ACCESS_TIMEOUT_ERROR    APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 55)  /*!< Tag access timeout */
#define ERR_GB29768_OTHER                   APPLI_ERROR_TYPE (ERR_FIRST_ST25RU3993_ERROR - 56)  /*!< Other error occurred */



#endif /* ERRNO_ST25RU3993_H */

/**
  * @}
  */
/**
  * @}
  */
