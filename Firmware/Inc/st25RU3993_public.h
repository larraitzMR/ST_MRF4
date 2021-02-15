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
 *  @brief Declaration of public functions provided by the ST25RU3993 series chips
 *
 *  Declares functions provided by the ST25RU3993 series chips. All higher level
 *  and protocol work is contained in gen2.h. Register access is 
 *  performed using st25RU3993.h.
 */

/** @addtogroup ST25RU3993
  * @{
  */

#ifndef _ST25RU3993_PUBLIC_H_
#define _ST25RU3993_PUBLIC_H_

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include "stuhfl_dl_ST25RU3993.h"
#include "stuhfl_sl.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/

/*
******************************************************************************
* STRUCTS
******************************************************************************
*/
/** @struct tag_t
  * This struct stores the whole information of one tag.
  */
typedef struct
{
    /** RN16 number */
    uint8_t rn16[2];
    /** PC value */
    uint8_t pc[2];
    /** EPC array */
    uint8_t epc[MAX_EPC_LENGTH]; /* epc must immediately follow pc */
    /** EPC length, in bytes */
    uint8_t epclen;
    /** Handle for write and read communication with the Tag */
    uint8_t handle[2];
    /** I part of logarithmic rssi which has been measured when reading this Tag. */
    uint8_t rssiLogI;
    /** Q part of logarithmic rssi which has been measured when reading this Tag. */
    uint8_t rssiLogQ;
    /** linear rssi which has been measured when reading this Tag. */
    int8_t rssiLinI;
    /** linear rssi which has been measured when reading this Tag. */
    int8_t rssiLinQ;
    /** content of AGC and Internal Status Display Register 0x2A after reading a tag. */
    uint8_t agc;
    /** TID length, in bytes */
    uint8_t tidlen;
    /** TID array */
    uint8_t tid[MAX_TID_LENGTH];
    /** XPC length, in bytes */
    uint8_t xpclen;
    /** XPC array */
    uint8_t xpc[MAX_XPC_LENGTH];

    /** Antenna at which Tag was seen (appended with v1.9.3)  */
    uint8_t antenna;    

    /** Timestamp as system tick count (appended with v1.6.7) */
    uint32_t timeStamp;
} tag_t;

/** @struct freq_t
  * This struct stores the list of frequencies which are used for hopping.
  * For tuning enabled boards the struct also stores the tuning settings for
  * each frequency.
  */
typedef struct
{
    /** Number of frequencies in freq. */
    uint8_t numFreqs;

    /** List of frequencies which are used for hopping. */
    uint32_t freq[MAX_FREQUENCY];
} freq_t;

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
/** This function initialises the ST25RU3993. A return value greater 0 indicates an error.\n
 *
 * @return       error code
 * @return 0     : if everything went o.k.
 * @return 1     : writing + reading SPI failed.
 * @return 2     : Reset via EN low + high failed.
 * @return 3     : IRQ line failed.
 * @return 4     : Crystal not stable (Bit0 in AGC and Internal Status register 0x2A)
 * @return 5     : PLL not locked (Bits1 in AGC and Internal Status register 0x2A)
 */
uint16_t st25RU3993Initialize(void);

/** This function reads the version register of the ST25RU3993 and
  * returns the value. \n
  *
  * @return Returns the value of the ST25RU3993 version register.
  */
uint8_t st25RU3993ReadChipVersion(void);

/** This function sets the frequency in the appropriate register
  * WARNING, only implemented for PLL Main Registers!!!
  *
  * @param frequency:   frequency in kHz
  *
  * @return 0 : pll lock was not successful
  * @return 1 : pll lock was successful
  */
uint8_t st25RU3993SetBaseFrequency(uint32_t frequency);

/** This function return the last frequency that was set with st25RU3993SetBaseFrequency(..).
  */
uint32_t st25RU3993GetBaseFrequency(void);

/** This function turns the antenna power on or off.
  *
  * @param on boolean to value to turn it on (1) or off (0).
  */
void st25RU3993AntennaPower(uint8_t on);

/** This function gets the RSSI (ReceivedSignalStrengthIndicator)
  * Register Values I and Q of the environment.
  * For measuring the antenna will be switched off.
  * The st25RU3993 will change the register settings to max. sensitivity.
  * Samples will be taken every 500us for the time num_of_ms_to_scan.
  *
  * The returned value is the highest RSSI value.
  *
  * Use this function to implement LBT (Listen Before Talk).
  * Only available for boards with external PA.
  */
void st25RU3993GetRSSIMaxSensitive(uint16_t num_of_ms_to_scan, uint8_t *rssiLogI, uint8_t *rssiLogQ);

/** This function gets the RSSI (ReceivedSignalStrengthIndicator)
  * Register Values I and Q of the environment.
  * Samples will be taken every 500us for the time num_of_ms_to_scan.
  *
  * The returned value is the highest RSSI value.
  *
  * Use this function to implement LBT (Listen Before Talk).
  * Only available for boards with external PA.
  */
void st25RU3993GetRSSI(uint16_t num_of_ms_to_scan, uint8_t *rssiLogI, uint8_t *rssiLogQ);

/** This function tries to set the sensitivity to a level to allow detection
  * of signals using st25RU3993GetRSSI() with a level higher than miniumSignal(dBm).
  *
  * @return the level offset. Sensitivity was set to minimumSignal - (returned_val).
  * negative values mean the sensitivity could not be reached.
  */
int8_t st25RU3993SetSensitivity(int8_t minimumSignal);

/** This function gets the current rx sensitivity level.
  */
int8_t st25RU3993GetSensitivity(void);
int8_t st25RU3993GetLastSetSensitivity(void);
void st25RU3993IncreaseSensitivity(void);
void st25RU3993DecreaseSensitivity(void);
/** This function tries to set the tx output level.
  */
void st25RU3993SetTxOutputLevel(int8_t txOutputLevel);

/** This function gets the current tx output level.
  */
int8_t st25RU3993GetTxOutputLevel(void);

/** This function gets the values for the reflected power. 
  * The two measured 8-bit values are returned as
  * one 16-bit value, the lower byte containing receiver-A (I) value, the
  * higher byte containing receiver-B (Q) value. The two values are signed
  * values and will be converted from sign magnitude representation to natural.
  *
  * @note Antenna has to be switched on manually before
  *
  * @note: To get a more accurate result the I and Q values of
  * st25RU3993GetReflectedPowerNoiseLevel() should be subtracted from the result of
  * st25RU3993GetReflectedPower()
  */
uint16_t st25RU3993GetReflectedPower(void);

/** This function measures and returns the noise level of the reflected power
  * measurement of the active antenna. The returned value is of type uint16_t and
  * contains the 8 bit I and Q values of the reflected power. The I value is
  * located in the lower 8 bits, the Q value in the upper 8 bits.
  * See st25RU3993GetReflectedPower() fore more info.
  */
uint16_t st25RU3993GetReflectedPowerNoiseLevel(void);

/** This function performs a reset by driving the enable line first low,
  * then high.
  */
void st25RU3993ResetDoNotPreserveRegisters(void);

/** This function is for generic sending and receiving gen2 commands through
    the FIFO.
    The parameters are:
    @param cmd: One of the ST25RU3993_CMD_* commands wich transmits something.
    @param txbuf: buffer to be tranmitted.
    @param txbits: the size of txbuf in bits
    @param rxbuf: a buffer getting the response of the tag
    @param rxbits: the maximum number of expected rx bits (size of rxbuf in bits)
                      The value will be written to rxlength register. If 0 rxlength is
                      automatically calculated by the reader and no boundary check for rxbuf is done.
    @param norestime: value for register ST25RU3993_REG_RXNORESPONSEWAITTIME
                      0xFF means 26 ms. Other values are much shorter.
    @param followCmd: Command to be sent after rxdone interrupt but before
           emptying FIFO. This is needed for not violating T2 timing requirement.
    @param waitTxIrq: If 0 tx irq will not be handled after transmitting command,
           otherwise FW will wait for Tx irq after transmitting command.

    Certain values may be set to 0/NULL thus changing the behavior. Examples:

    - st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_TRANSMCRCEHEAD, buf_, 50, buf_, &rxbits, 0xFF, 0) with rxbits=33
      can be used to send WRITE command and retrieve after max 20ms a reply expecting a header.
    - st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_TRANSMCRC, buf_, len, NULL, &rxbits, 0, 0) with rxbits=0
      is a transmit-only used for sending SELECT. With *rxbits==0 and rxbuf==NULL RX will be turned off immediately
      after TX-done interrupt
    - st25RU3993TxRxGen2Bytes(ST25RU3993_CMD_QUERY,buf_,16,tag->rn16,&rxlen,gen2IntConfig.noRespTime,ST25RU3993_CMD_ACK) with rxlen==16
      sends QUERY command, waits for tx-done, then waits for rx-done, then
      sends ACK command and only as the last step reads RN16 from FIFO
    - st25RU3993TxRxGen2Bytes(0,buf_,0,buf_,&rxlen,gen2IntConfig.noRespTime,fast?0:ST25RU3993_CMD_REQRN) with rxlen==sizeof(buf_)*8
      does not send anything (ACK was executed via followCmd as previous example),
      waits for tx-done, waits for rx-done, sends REQRN, reads EPC from FIFO

  */
int8_t st25RU3993TxRxGen2Bytes(uint8_t cmd, uint8_t *txbuf, uint16_t txbits,
                               uint8_t *rxbuf, uint16_t *rxbits,
                               uint8_t norestime, uint8_t followCmd,
                               uint8_t waitTxIrq);
                               
int8_t st25RU3993RxGen2EPC(uint8_t *rxbuf, uint16_t *rxbits,
                           uint8_t norestime, uint8_t followCmd,
                           uint8_t waitTxIrq, uint8_t *maxAckReplyCnt);
                           
/**
  * This functions performs an ADC conversion and returns the signed result in machine coding */
int8_t st25RU3993GetADC(void);

#endif /* _ST25RU3993_PUBLIC_H_ */

/**
  * @}
  */
