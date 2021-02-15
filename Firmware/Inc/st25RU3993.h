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
 *  @brief Declaration of low level functions provided by the st25RU3993 series chips
 *
 *  Declares low level functions provided by the st25RU3993 series chips. All
 *  higher level functions are provided by st25RU3993_public.h and protocol
 *  work is contained in gen2.h
 */

/** @addtogroup ST25RU3993
  * @{
  */

#ifndef _ST25RU3993_H_
#define _ST25RU3993_H_

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include "st25RU3993_config.h"
#include "st25RU3993_registers.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
/** Definition protocol read bit */
#define READ                    0x40

/* Reader Commands ************************************************************/
#define ST25RU3993_CMD_IDLE                  0x80   // unused
#define ST25RU3993_CMD_DIRECT_MODE           0x81

#define ST25RU3993_CMD_SOFT_INIT             0x83   // unused
#define ST25RU3993_CMD_HOP_TO_MAIN_FREQUENCY 0x84   // used in self test
#define ST25RU3993_CMD_HOP_TO_AUX_FREQUENCY  0x85   // unused

#define ST25RU3993_CMD_TRIGGERADCCON         0x87
#define ST25RU3993_CMD_TRIG_RX_FILTER_CAL    0x88   // unused
#define ST25RU3993_CMD_INC_RX_FILTER_CAL     0x89   // unused
#define ST25RU3993_CMD_DEC_RX_FILTER_CAL     0x8A   // unused

#define ST25RU3993_CMD_TRANSMCRC             0x90
#define ST25RU3993_CMD_TRANSMCRCEHEAD        0x91
#define ST25RU3993_CMD_TRANSMNOCRC           0x92   // unused

#define ST25RU3993_CMD_BLOCKRX               0x96
#define ST25RU3993_CMD_ENABLERX              0x97
#define ST25RU3993_CMD_QUERY                 0x98
#define ST25RU3993_CMD_QUERYREP              0x99
#define ST25RU3993_CMD_QUERYADJUSTUP         0x9A
#define ST25RU3993_CMD_QUERYADJUSTNIC        0x9B
#define ST25RU3993_CMD_QUERYADJUSTDOWN       0x9C
#define ST25RU3993_CMD_ACK                   0x9D
#define ST25RU3993_CMD_NAK                   0x9E
#define ST25RU3993_CMD_REQRN                 0x9F

#define ST25RU3993_CMD_SUPPLY_AUTO_LEVEL     0xA2   // unused
#define ST25RU3993_CMD_SUPPLY_MANUAL_LEVEL   0xA3   // unused
#define ST25RU3993_CMD_VCO_AUTO_RANGE        0xA4   // unused
#define ST25RU3993_CMD_VCO_MANUAL_RANGE      0xA5
#define ST25RU3993_CMD_AGL_ON                0xA6   // unused
#define ST25RU3993_CMD_AGL_OFF               0xA7   // unused
#define ST25RU3993_CMD_STORE_RSSI            0xA8   // unused
#define ST25RU3993_CMD_CLEAR_RSSI            0xA9   // unused
#define ST25RU3993_CMD_ANTI_COLL_ON          0xAA
#define ST25RU3993_CMD_ANTI_COLL_OFF         0xAB

/* IRQ Mask register **********************************************************/
/** ST25RU3993 interrupt: ana bit: change of OSC, PLL, RF field */
#define ST25RU3993_IRQ2_END_ANA              0x80
/** ST25RU3993 interrupt: cmd bit: end of command */
#define ST25RU3993_IRQ2_END_CMD              0x40
/** ST25RU3993 interrupt: error1 bit: CRC/autohop */
#define ST25RU3993_IRQ2_CRCERROR             0x04
/** ST25RU3993 interrupt: error 2 bit: rxcount */
#define ST25RU3993_IRQ2_RXCOUNT              0x02
/** ST25RU3993 interrupt: error 3 bit: preamble/fifo overflow */
#define ST25RU3993_IRQ2_PREAMBLE             0x01

#define ST25RU3993_IRQ2_MASK_ALL             0xC7

/** ST25RU3993 IRQ status register: transmit complete */
#define ST25RU3993_IRQ1_TX                   0x80
/** ST25RU3993 IRQ status register: receive complete */
#define ST25RU3993_IRQ1_RX                   0x40
/** ST25RU3993 interrupt: fifo bit */
#define ST25RU3993_IRQ1_FIFO                 0x20
/** ST25RU3993 IRQ status register: receive error */
#define ST25RU3993_IRQ1_RXERR                0x10
/** ST25RU3993 interrupt: header bit / 2bytes */
#define ST25RU3993_IRQ1_HEADER               0x08
/** ST25RU3993 interrupt: Auto ACK finished */
#define ST25RU3993_IRQ1_AUTOACK              0x02
/** ST25RU3993 interrupt: no response bit */
#define ST25RU3993_IRQ1_NORESP               0x01

#define ST25RU3993_IRQ1_MASK_ALL             0xFB

/******************************************************************************/
/** ST25RU3993 ST25RU3993_REG_FIFO status register: FIFO overflow */
#define ST25RU3993_FIFOSTAT_OVERFLOW         0x20

#define RESP_NORESINTERRUPT ST25RU3993_IRQ1_NORESP
#define RESP_AUTOACK        ST25RU3993_IRQ1_AUTOACK
#define RESP_HEADERBIT      ST25RU3993_IRQ1_HEADER
#define RESP_RXERR          ST25RU3993_IRQ1_RXERR
#define RESP_FIFO           ST25RU3993_IRQ1_FIFO
#define RESP_RXIRQ          ST25RU3993_IRQ1_RX
#define RESP_TXIRQ          ST25RU3993_IRQ1_TX
#define RESP_PREAMBLEERROR (ST25RU3993_IRQ2_PREAMBLE << 8)
#define RESP_RXCOUNTERROR  (ST25RU3993_IRQ2_RXCOUNT  << 8)
#define RESP_CRCERROR      (ST25RU3993_IRQ2_CRCERROR << 8)
#define RESP_END_CMD       (ST25RU3993_IRQ2_END_CMD  << 8)
#define RESP_END_ANA       (ST25RU3993_IRQ2_END_ANA  << 8)

#define RESP_RXDONE_OR_ERROR    (RESP_RXIRQ | RESP_AUTOACK | RESP_RXERR | RESP_NORESINTERRUPT)
#define RESP_ANY                (ST25RU3993_IRQ1_MASK_ALL | (ST25RU3993_IRQ2_MASK_ALL << 8))

#define RSSI_MODE_REALTIME                  0x00
#define RSSI_MODE_PILOT                     0x04
#define RSSI_MODE_2NDBYTE                   0x06
#define RSSI_MODE_PEAK                      0x08

/** Frequency offset for RSSI measurement. \n
  * If the measured signal has the same frequency as the pll, I + Q is zero
  * after mixing the signal.\n
  * If the measured signal will differ with the pll frequency setting, the
  * filtered mixed signal will be seen.\n
  * This filtered mixed signal should have a maximum if pll frequency and
  * signal frequency differ with this offset.\n
  * Therefore, the offset has a dependency on the filter settings. */
#define ST25RU3993_RSSI_FREQUENCY_OFFSET    50

/** delay in us which will be used st25RU3993WaitForResponse() loop */
#define WAITFORRESPONSEDELAY            1

/** max delay in us which we will wait for an response from ST25RU3993 \n
  * at 40kHz BLF one gen2 slot takes ~40ms, we are going to wait
  * 45ms (in st25RU3993WaitForResponse()) to be on the safe side. */
#define WAITFORRESPONSETIMEOUT          45000
#define WAITFORRESPONSECOUNT            (WAITFORRESPONSETIMEOUT / WAITFORRESPONSEDELAY)

#define st25RU3993GetResponse()         st25RU3993Response
#define st25RU3993ClrResponseMask(mask) st25RU3993Response&=~(mask)
#define st25RU3993ClrResponse()         st25RU3993Response=0

/*
******************************************************************************
* FUNCTION PROTOTYPES
******************************************************************************
*/
/**
  * Sends only one command to the ST25RU3993.
  * @param command: The command which is sent to the ST25RU3993.
  */
void st25RU3993SingleCommand(uint8_t command);
void st25RU3993SingleCommandFast(void);

/** This function talks with the ST25RU3993 chip from ISR. Is only called from
  * the ISR (no reentrant function necessary).
  */
void writeReadST25RU3993Isr(uint8_t *wbuf, uint8_t wlen, uint8_t *rbuf, uint8_t rlen);

/** Reads data from a address and some following addresses from the
  * ST25RU3993. The len parameter defines the number of address read.
  * @param address addressbyte
  * @param len Length of the buffer.
  * @param *readbuf Pointer to the first byte of the array where the data has
  * to be stored in.
  */
void st25RU3993ContinuousRead(uint8_t address, uint8_t len, uint8_t *readbuf);

/** Reads data from the fifo and some following addresses from the
  * ST25RU3993. The len parameter defines the number of address read.
  * @param len Length of the buffer.
  * @param *readbuf Pointer to the first byte of the array where the data has
  * to be stored in.
  */
void st25RU3993FifoRead(uint8_t len, uint8_t *readbuf);

/** Single Read -> reads one byte from one address of the ST25RU3993.
  * @param address The addressbyte
  * @return The databyte read from the ST25RU3993
  */
uint8_t st25RU3993SingleRead(uint8_t address);

/** Continuous Write -> writes several bytes to subsequent addresses of the
  * ST25RU3993.
  * @param address addressbyte
  * @param *buf Pointer to the first byte of the array.
  * @param len Length of the buffer.
  */
void st25RU3993ContinuousWrite(uint8_t address, uint8_t *buf, uint8_t len);

/** Single Write -> writes one byte to one address of the ST25RU3993.
  * @param address The addressbyte
  * @param value The databyte
  */
void st25RU3993SingleWrite(uint8_t address, uint8_t value);

/** Sends first some commands to the ST25RU3993. The number of
  * commands is specified with the parameter comLen. Then it sets
  * the address where the first byte has to be written and after that
  * every byte from the buffer is written to the ST25RU3993.
  * @param *command Pointer to the first byte of the command buffer
  * @param comLen Length of the command buffer.
  * @param address addressbyte
  * @param *buf Pointer to the first byte of the data array.
  * @param bufLen Length of the buffer.
  */
void st25RU3993CommandContinuousAddress(uint8_t *command, uint8_t comLen,
                             uint8_t address, uint8_t *buf, uint8_t bufLen);

/** This function waits for the specified response(IRQ).
  */
void st25RU3993WaitForResponse(uint16_t waitMask);

/** This function waits for the specified response(IRQ).
  */
void st25RU3993WaitForResponseTimed(uint16_t waitMask, uint16_t ms);

/** @brief Enter the direct mode
  *
  * The direct mode is needed since the ST25RU3993 doesn't support ISO18000-6B
  * directly. Direct mode means that the MCU directly issues the commands.
  * The ST25RU3993 modulates the serial stream from the MCU and sends it out.
  */
void st25RU3993EnterDirectMode(void);

/** @brief Leave the direct mode
  *
  * After calling this function the ST25RU3993 is in normal mode again.
  *
  */
void st25RU3993ExitDirectMode(void);

/** Enter the power down mode by setting EN pin to low, saving
  * registers beforehand
  */
void st25RU3993EnterPowerDownMode(void);

/** Exit the power down mode by setting EN pin to high, restoring
  * registers afterwards.
  */
void st25RU3993ExitPowerDownMode(void);

/** Enter the normal power mode: EN is high, stby and rf_on bits are low
  */
void st25RU3993EnterPowerNormalMode(void);

/** Exit the normal power mode.
  */
void st25RU3993ExitPowerNormalMode(void);

/** @brief Enter the normal power mode with rf on.
  * EN is high, stby bit is low and rf_on bit is high.
  */
void st25RU3993EnterPowerNormalRfMode(void);

/** Exit the normal power mode with rf on.
  */
void st25RU3993ExitPowerNormalRfMode(void);

/** @brief Enter the standby power down mode
  * EN is high, stby is high and rf_on bit is low.
  */
void st25RU3993EnterPowerStandbyMode(void);

/** Exit the standby power down mode.
  */
void st25RU3993ExitPowerStandbyMode(void);

/** after EN goes high the IRQ register has to be read after osc_ok
  * goes high.
  */
void st25RU3993WaitForStartup(void);

/** @brief External Interrupt Function
  * The ST25RU3993 uses the external interrupt to signal the uC that
  * something happened. The interrupt function reads the ST25RU3993
  * IRQ status registers.\n
  * The interrupt function sets the flags if an event occurs.
  */
void st25RU3993Isr(void);

#endif /* _ST25RU3993_H_ */

/**
  * @}
  */
