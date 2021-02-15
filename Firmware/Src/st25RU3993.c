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
 *  @brief Functions provided by the st25RU3993 series chips
 *
 *  Functions provided by the st25RU3993 series chips. All higher level
 *  and protocol work is contained in gen2.c
 */

/** @addtogroup ST25RU3993
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "st25RU3993.h"
#include "st25RU3993_public.h"
#include "logger.h"
#include "timer.h"
#include "gen2.h"
#include "tuner.h"
#include "spi_driver.h"
#include "gb29768.h"
#include "leds.h"
#include "crc.h"
/*
******************************************************************************
* DEFINES
******************************************************************************
*/
/** ADC Values are in sign magnitude representation -> convert */
#define CONVERT_ADC_TO_NAT(A)   (((A)&0x80)?((A)&0x7F):(0 - ((A)&0x7F)))

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
/** This variable is used as flag to signal an data reception.
  * It is a bit mask of the RESP_TXIRQ, RESP_RXIRQ, RESP_* values */
volatile uint16_t st25RU3993Response = 0;

extern uint8_t irqEnable;

extern uint8_t externalPA;

extern tuningTable_t tuningTable[NUM_SAVED_PROFILES];

extern uint8_t profile;

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
/** Restore registers 0x00 to 0x1D + 5 registers (0x22, 0x29, 0x35, 0x36, 0x3A)
  * after power up */
static uint8_t st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+6];

static uint8_t antennaOn = 0;

static uint8_t tuningRegs[3];

static uint8_t writeBuf[128];
static uint8_t readBuf[128];
static uint32_t baseFrequency = 0;
static int8_t lastSetSensitivity = 0;
/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
static uint8_t st25RU3993LockPLL(void);

/** This function talks with the ST25RU3993 chip.
  */
static void writeReadST25RU3993(uint8_t *wbuf, uint8_t wlen, uint8_t *rbuf, uint8_t rlen, uint8_t stopMode, uint8_t doStart);

static uint8_t isPllLocked(void);

static int8_t replyACK(uint8_t *rxbuf, uint16_t *rxbits, uint8_t followCmd);

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
static uint8_t isPllLocked(void)
{
    static const uint8_t lockAttempts = 200;
    uint8_t i;

    for(i=0 ; i<lockAttempts ; i++)
    {
        if(st25RU3993SingleRead(ST25RU3993_REG_AGCANDSTATUS) & 0x02)
        {
            return 1;
        }
        delay_us(10);
    }
    return 0;
}

/*------------------------------------------------------------------------- */
static uint8_t st25RU3993IsFrequencyChangeNecessary(uint32_t frequency)
{
    uint8_t buf[3];
    uint16_t ref=0, i, j, x, reg_A,reg_B;
    uint32_t divisor;

    st25RU3993ContinuousRead(ST25RU3993_REG_PLLMAIN1, 3, buf);
    switch(buf[0]& 0x70)
    {
        case 0x40:
        {
            ref=125;
            break;
        }
        case 0x50:
        {
            ref=100;
            break;
        }
        case 0x60:
        {
            ref=50;
            break;
        }
        case 0x70:
        {
            ref=25;
            break;
        }
        default:
        {
            ref=0;
            break;
        }
    }

    if(ref == 0)
    {
        return 0;
    }

    divisor=frequency/ref;

    i = 0x3FFF & (divisor >> 6); // division by 64
    x = (i<<6)+ i;
    if(divisor > x)
    {
        x += 65;
        i++;
    }
    x -= divisor;
    j = i;
    do
    {
        if(x >= 33)
        {
            i--;
            x -= 33;
        }
        if(x >= 32)
        {
            j--;
            x -= 32;
        }
    } while(x >= 32);
    if(x > 16)
    {            // subtract 32 from x if it is larger than 16
        x -= 32; // this yields more closely matched A and B values
        j--;
    }

    reg_A = i - x;
    reg_B = j + x;

    buf[0] = (buf[0] & 0xF0) | ((uint8_t)((reg_B >> 6) & 0x0F));
    buf[1] = (uint8_t)((reg_B << 2) & 0xFC) |  (uint8_t)((reg_A >> 8) & 0x03);
    buf[2] = (uint8_t)reg_A;

    if(tuningRegs[0]==buf[0] && tuningRegs[1]==buf[1] && tuningRegs[2]==buf[2])
    {
        return 1;
    }
    else
    {
        tuningRegs[0]=buf[0];
        tuningRegs[1]=buf[1];
        tuningRegs[2]=buf[2];
        return 1;
    }
}

/*------------------------------------------------------------------------- */
static uint8_t st25RU3993LockPLL(void)
{
    uint8_t buf;
    uint8_t vcoReg;
    uint8_t vcoSegment;
    uint8_t vcoVoltage;

    buf = st25RU3993SingleRead(ST25RU3993_REG_STATUSPAGE);
    buf &= ~0x30;
    buf |= 0x10;        // have vco_ri in aglstatus
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSPAGE, buf);

    vcoReg = st25RU3993SingleRead(ST25RU3993_REG_VCOCONTROL);
    vcoReg |= 0x80;     // set mvco bit
    st25RU3993SingleWrite(ST25RU3993_REG_VCOCONTROL, vcoReg);
    delay_us(600);

    vcoVoltage = st25RU3993SingleRead(ST25RU3993_REG_AGL) & 0x07;

    vcoReg &= ~0x80;    // reset mvco bit
    if(vcoVoltage <= 1)
    {
        // increase segment
        vcoSegment = vcoReg&0x0F;
        if(vcoSegment < 0x0F)
        {
            vcoSegment++;
        }
        vcoReg = (vcoReg&0xF0)|(vcoSegment&0x0F);
    }
    if(vcoVoltage >= 6)
    {
        // decrease segment
        vcoSegment = vcoReg&0x0F;
        if(vcoSegment > 0x00)
        {
            vcoSegment--;
        }
        vcoReg = (vcoReg&0xF0)|(vcoSegment&0x0F);
    }
    st25RU3993SingleWrite(ST25RU3993_REG_VCOCONTROL, vcoReg);
    //st25RU3993SingleCommand(ST25RU3993_CMD_VCO_MANUAL_RANGE);
    
    if(isPllLocked())
    {
        SET_PLL_LED_ON();
        return 1;
    }
    else
    {
        SET_PLL_LED_OFF();
        return 0;
    }
}

/*------------------------------------------------------------------------- */
static void writeReadST25RU3993(uint8_t *wbuf, uint8_t wlen, uint8_t *rbuf, uint8_t rlen, uint8_t stopMode, uint8_t doStart)
{
    memcpy(writeBuf,wbuf,wlen);
    memset(writeBuf+wlen,0x00,rlen);

    if(doStart) {
        spiSelect();
    }

    spiTxRx(writeBuf, readBuf, wlen+rlen);
    memcpy(rbuf,readBuf+wlen,rlen);

    if(stopMode != STOP_NONE) {
        spiUnselect();
    }
}

/*------------------------------------------------------------------------- */
void writeReadST25RU3993Isr(uint8_t *wbuf, uint8_t wlen, uint8_t *rbuf, uint8_t rlen)
{
    memcpy(writeBuf,wbuf,wlen);
    memset(writeBuf+wlen,0x00,rlen);

    spiSelect();
    spiTxRx(writeBuf, readBuf, wlen+rlen);
    memcpy(rbuf,readBuf+wlen,rlen);
    spiUnselect();
}

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
uint16_t st25RU3993Initialize(void)
{
    uint8_t myBuf[4];
    st25RU3993ResetDoNotPreserveRegisters();
    
#if ST25RU3993_DO_SELFTEST
    // check SPI communication
    myBuf[0] = 0x55;
    myBuf[1] = 0xAA;
    myBuf[2] = 0xFF;
    myBuf[3] = 0x00;
    st25RU3993ContinuousWrite(ST25RU3993_REG_MODULATORCONTROL1, myBuf, 4);
    memset(myBuf, 0x33, sizeof(myBuf));
    st25RU3993ContinuousRead(ST25RU3993_REG_MODULATORCONTROL1, 4,myBuf);
    if((myBuf[0]!=0x55) ||
        (myBuf[1]!=0xAA) ||
        (myBuf[2]!=0xFF) ||
        (myBuf[3]!=0x00))
    {
        LOG("%s(%d): data bus interface pins not working\n",__FUNCTION__,__LINE__);
        return 1; // data bus interface pins not working
    }

    // check EN pin + SPI communication
    st25RU3993ResetDoNotPreserveRegisters();
    st25RU3993ContinuousRead(ST25RU3993_REG_MODULATORCONTROL1, 4, myBuf);
    if((myBuf[0]==0x55) ||
        (myBuf[1]==0xAA) ||
        (myBuf[2]==0xFF) ||
        (myBuf[3]==0x00))
    {
        LOG("%s(%d): enable pin not working\n",__FUNCTION__,__LINE__);
        return 2; // enable pin not working
    }

    // check IRQ line
    delay_ms(1);
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK1, 0x20);
    // set up 48Byte transmission, but we supply less, therefore a fifo underflow IRQ is produced
    st25RU3993SingleWrite(ST25RU3993_REG_TXLENGTHUP, 0x03);
    st25RU3993SingleCommand(ST25RU3993_CMD_TRANSMCRC);
    st25RU3993ContinuousWrite(ST25RU3993_REG_FIFO,myBuf,4);
    st25RU3993ContinuousWrite(ST25RU3993_REG_FIFO,myBuf,4);
    st25RU3993ContinuousWrite(ST25RU3993_REG_FIFO,myBuf,4);
    st25RU3993ContinuousWrite(ST25RU3993_REG_FIFO,myBuf,4);
    st25RU3993ContinuousWrite(ST25RU3993_REG_FIFO,myBuf,4);
    st25RU3993ContinuousWrite(ST25RU3993_REG_FIFO,myBuf,4);

    st25RU3993WaitForResponse(RESP_FIFO);
    if(!(st25RU3993GetResponse() & RESP_FIFO))
    {
        LOG("%s(%d): IRQ line not working\n",__FUNCTION__,__LINE__);
        return 3;
    }

    st25RU3993ClrResponse();

    st25RU3993ResetDoNotPreserveRegisters();
    st25RU3993SingleCommand(ST25RU3993_CMD_HOP_TO_MAIN_FREQUENCY);
    
    LOG("st25RU3993Initialize(): Self tests OK\n");
#endif

    // Device Status Control Register 0x00
    // stby |   -   | agc_on | rec_on | rf_on
    //   0  |0 0 0 0|    0   |    0   |   0   = 0x00
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, 0x00);

    // Protocol Selection Register 0x01
    // RX_crc_n | dir_mode | AutoACK<1-0> | - | prot<2-0>
    //     0    |     0    |     0 0      | 0 |  0 0 0    = 0x0
    st25RU3993SingleWrite(ST25RU3993_REG_PROTOCOLCTRL, 0x0);

    // Tx Options Register 0x02
    //  - | TxOne<1-0> | - | Tari<2-0>
    // 0 0|     1 1    | 0 |   0 0 0   = 0x30
    st25RU3993SingleWrite(ST25RU3993_REG_TXOPTIONS, 0x30);

    // Rx Options Register 0x03 --> Set by Gen2 Configuration
    // TRcal High Register 0x04 --> Set by Gen2 Configuration
    // TRcal Low Register 0x05  --> Set by Gen2 Configuration

    // Modulation Ctrl2 0x14
    // del_len     | pr_ask | ook_ask
    // 1 1 1 0 1 1 |    1   |    1     = 0xEF
    st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL2, 0xEF);

    // Modulation Ctrl4 0x16
    //     1stTari
    // 1 0 0 0 1 0 0 1        = 0x89
    st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL4, 0x89);

    // PLL Main Register 0x17
    // - | RefFreq<2-0> | mB_val<9-6>
    // 0 |    1 1 0     |   0 0 1 1   = 0x63
    st25RU3993SingleWrite(ST25RU3993_REG_PLLMAIN1, 0x63);
    
    // PLL Main Register 0x18
    // mB_val<5-0> | mA_val<9-8>
    // 1 1 1 0 1 1 | 0 1         = 0xED
    st25RU3993SingleWrite(ST25RU3993_REG_PLLMAIN2, 0xED);
    
    // PLL Main Register 0x19
    // mA_val<7-0>
    // 0 0 0 1 1 0 1 0 = 0x1A
    st25RU3993SingleWrite(ST25RU3993_REG_PLLMAIN3, 0x1A);

#if ST25RU3993_DEBUG
    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, 0x80);     // enable debug signals on OAD
#else
    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, 0x00);
#endif

    // Miscellaneous Register 1 0x0D
    // hs_output | hs_oad | miso_pd2 | miso_pd1 | open_dr | s_mix | -
    //      0    |    0   |    1     |    1     |    0    |   0   |0 0 = 0x30
    st25RU3993SingleWrite(ST25RU3993_REG_MISC1, 0x30
#if ST25RU3993_DEBUG
            | 0x40
#endif
#ifdef SINGLEINP
            | 0x04
#endif
    );
    
    st25RU3993SingleWrite(ST25RU3993_REG_MISC2, 0x40);

    if(externalPA)
    {
        // REGULATOR and PA Bias Register 0x0B
        // pa_bias<1-0> | rvs_rf<2-1> | rvs<2-0>
        //     0 0      |    0 1 1    |  0 1 1   = 0x1B
        st25RU3993SingleWrite(ST25RU3993_REG_REGULATORCONTROL, 0x1B);

        // RF Output and LO Control Register 0x0C
        // eTX<7-5> | - | eTx<3-0>
        //   1 0 0  | 0 | 0 0 1 0  = 0x82
        st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, 0x82);

        // Modulator Control Register 1 0x13
        //  - | main_mod | aux_mod | - | e_lpf | ask_rate<1-0>
        //  0 |     0    |    1    |0 0|   0   |     0 0       = 0x20
        myBuf[0] = 0x20;

        // Modulator Control Register 2 0x14
        // ook_ask | pr_ask | del_len<5-0>
        //    1    |    1   | 0 1 1 1 0 1  = 0xDD
        myBuf[1] = 0xDD;

        // Modulator Control Register 3 0x15
        // trfon<1-0> | lin_mode | TX_lev<4-0>
        //     0 0    |     0    |  0 0 0 1 0  = 0x02 for USA/EU
        //     0 0    |     0    |  0 0 1 1 1  = 0x07 for JAPAN
        if(profile == PROFILE_JAPAN)
        {
            myBuf[2] = 0x07;
        }
        else
        {
            myBuf[2] = 0x02;
        }
        st25RU3993ContinuousWrite(ST25RU3993_REG_MODULATORCONTROL1, myBuf, 3);
    }
    else
    {
        // REGULATOR and PA Bias Register 0x0B
        // pa_bias<1-0> | rvs_rf<2-1> | rvs<2-0>
        //      1 0     |    0 1 1    |  0 1 1   = 0x1B
        st25RU3993SingleWrite(ST25RU3993_REG_REGULATORCONTROL, 0xBF);

        // RF Output and LO Control Register 0x0C
        // eTX<7-5> | - | eTx<3-0>
        //   0 1 1  | 0 | 1 0 0 0  = 0x68
        st25RU3993SingleWrite(ST25RU3993_REG_RFOUTPUTCONTROL, 0x68);

        // Modulator Control Register 1 0x13
        //  - | main_mod | aux_mod | - | e_lpf | ask_rate<1-0>
        //  0 |     1    |    0    |0 0|   0   |     0 0       = 0x40
        myBuf[0] = 0x40;

        // Modulator Control Register 2 0x14
        // ook_ask | pr_ask | del_len<5-0>
        //    1    |    1   | 0 1 1 1 0 1  = 0xDD
        myBuf[1] = 0xDD;

        // Modulator Control Register 3 0x15
        // trfon<1-0> | lin_mode | TX_lev<4-0>
        //     0 0    |    0     |  0 0 0 0 1  = 0x01
        myBuf[2] = 0x01;
        st25RU3993ContinuousWrite(ST25RU3993_REG_MODULATORCONTROL1, myBuf, 3);
    }

    // CP Control Register 0x12
    // LF_R3<7-6> | LF_C3<5-3> | cp<2-0>
    //     0 0    |   1 1 0    |  1 0 1  = 0x25
    //  30kOhm, 160pF, 1500uA
    st25RU3993SingleWrite(ST25RU3993_REG_CPCONTROL, 0x35);

    // Enable Interrupt Register Register 1 0x35
    // e_irq_TX | e_irq_RX | e_irq_fifo | e_irq_err | e_irq_header | - | e_irq_AutoAck | e_irq_noresp
    //     0    |    1     |      1     |     1     |      1       | 1 |       1       |      1       = 0x7F
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK1, 0x7F);

    // Enable Interrupt Register Register 2 0x35
    // e_irq_ana | e_irq_cmd |  -  | e_irq_err1 | e_irq_err2 | e_irq_err3
    //     0     |     0     |0 0 0|      1     |     1      |     1      = 0x07
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK2, 0x07);

    // RX Length Register 1 0x3A
    // RX_crc_n2 | fifo_dir_irq2 | rep_irg2 | auto_errcode_RXl | RXl11-RXl8
    //     0     |       0       |    0     |        1         |   0 0 0 0  = 0x10
    st25RU3993SingleWrite(ST25RU3993_REG_RXLENGTHUP, 0x10);

    //  Give base freq. a reasonable value
    if (!st25RU3993SetBaseFrequency(tuningTable[profile].freq[0])) {
        LOG("%s(%d): SetBaseFrequency not working\n",__FUNCTION__,__LINE__);
    }
    st25RU3993SetSensitivity(3);

#if ST25RU3993_DO_SELFTEST
    // Now that the chip is configured with correct ref frequency the PLL
    // should lock
    delay_ms(20);
    myBuf[0] = st25RU3993SingleRead(ST25RU3993_REG_AGCANDSTATUS);
    if(!(myBuf[0] & 0x01))
    {
        SET_OSC_LED_OFF();
        LOG("%s(%d): crystal not working\n",__FUNCTION__,__LINE__);
        return 4;   // Crystal not stable
    }
    else
    {
        SET_OSC_LED_ON();
    }
    if(!(myBuf[0] & 0x02))
    {
        SET_PLL_LED_OFF();
        LOG("%s(%d): PLL not working\n",__FUNCTION__,__LINE__);
        return 5;    // PLL not locked
    }
    else
    {
        SET_PLL_LED_ON();
    }
#endif

#if ELANCE
    // Configure LNA OFF and Bypass ON
    SET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin);
    SET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin);
#endif
    LOG("st25RU3993Initialize() OK\n");
    return 0;
}

/*------------------------------------------------------------------------- */
#define ISR_WATCHDOG_MAX_LOOPS  5000
void st25RU3993Isr(void)
{
    if(irqEnable)
    {
        uint32_t isrWatchdog = ISR_WATCHDOG_MAX_LOOPS;
        while(GET_GPIO(GPIO_IRQ_PORT,GPIO_IRQ_PIN))
        {
            static uint8_t regs[2];
            static uint8_t addr = READ | ST25RU3993_REG_IRQSTATUS1;  // 0x77

            writeReadST25RU3993Isr(&addr, 1, regs, 2);
            st25RU3993Response |= (regs[0] | (regs[1] << 8));
            
            if(GET_GPIO(GPIO_IRQ_PORT,GPIO_IRQ_PIN))
            {
                /* In rare cases it may happen that main level code calls BlockRX 
                 while reception is just terminating. This may lead to hanging 
                 interrupt logic. Resolve this situation */
                uint8_t command = ST25RU3993_CMD_DIRECT_MODE;
                writeReadST25RU3993(&command, 1, 0, 0, STOP_SGL, 1);
                
                command = ST25RU3993_CMD_ENABLERX;
                writeReadST25RU3993(&command, 1, 0, 0, STOP_SGL, 1);
                
                delay_us(100);
                command = ST25RU3993_CMD_BLOCKRX;
                writeReadST25RU3993(&command, 1, 0, 0, STOP_SGL, 1);

                if ((--isrWatchdog) == 0) {
                    // ISR reached max recovery loops number
                    return;
                }
            }
        }
    }
}

/*------------------------------------------------------------------------- */
uint8_t st25RU3993ReadChipVersion(void)
{
    uint8_t version;
    version = st25RU3993SingleRead(ST25RU3993_REG_DEVICEVERSION);
    return version;
}

/*------------------------------------------------------------------------- */
void st25RU3993SingleCommand(uint8_t command)
{
    DISEXTIRQ();
    writeReadST25RU3993(&command, 1, 0, 0, STOP_SGL, 1);
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
void st25RU3993ContinuousRead(uint8_t address, uint8_t len, uint8_t *readbuf)
{
    DISEXTIRQ();
    address |= READ;
    writeReadST25RU3993(&address, 1, readbuf, len, STOP_CONT, 1);
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
void st25RU3993FifoRead(uint8_t len, uint8_t *readbuf)
{
    static uint8_t address = ST25RU3993_REG_FIFO | READ ;
    DISEXTIRQ();
    writeReadST25RU3993(&address, 1, readbuf, len, STOP_CONT, 1);
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
/* Function is called from interrupt and normal level, therefore this
   function must be forced to be reentrant on Keil compiler. */
uint8_t st25RU3993SingleRead(uint8_t address)
{
    uint8_t readdata;

    DISEXTIRQ();
    address |= READ;
    writeReadST25RU3993(&address, 1, &readdata, 1, STOP_SGL, 1);

    ENEXTIRQ();
    return (readdata);
}

/*------------------------------------------------------------------------- */
void st25RU3993ContinuousWrite(uint8_t address, uint8_t *buf, uint8_t len)
{
    DISEXTIRQ();
    writeReadST25RU3993(&address, 1, 0, 0, STOP_NONE, 1);
    writeReadST25RU3993(buf, len, 0, 0, STOP_CONT, 0);
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
void st25RU3993SingleWrite(uint8_t address, uint8_t value)
{
    uint8_t buf[2];
    buf[0] = address;
    buf[1] = value;
    DISEXTIRQ();
    writeReadST25RU3993(buf, 2, 0, 0, STOP_SGL, 1);
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
void st25RU3993CommandContinuousAddress(uint8_t *command, uint8_t comLen,
                             uint8_t address, uint8_t *buf, uint8_t bufLen)
{
    DISEXTIRQ();
    writeReadST25RU3993(command, comLen, 0, 0, STOP_NONE, 1);
    writeReadST25RU3993(&address, 1, 0, 0, STOP_NONE, 0);
    writeReadST25RU3993(buf, bufLen, 0, 0, STOP_CONT, 0);
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
void st25RU3993WaitForResponseTimed(uint16_t waitMask, uint16_t counter)
{
    uint32_t counterUs = counter*100;
    while(((st25RU3993Response & waitMask) == 0) && (counterUs))
    {
        delay_us(10);
        counterUs--;
    }
    if(counterUs==0)
    {
        st25RU3993Response = RESP_NORESINTERRUPT;
    }
}

/*------------------------------------------------------------------------- */
void st25RU3993WaitForResponse(uint16_t waitMask)
{
    uint16_t counter;
    for(counter=0 ; counter<WAITFORRESPONSECOUNT ; counter++)
    {
        delay_us(WAITFORRESPONSEDELAY);
        if(st25RU3993Response & waitMask)
        {
            return;
        }
    }
    st25RU3993Response = RESP_NORESINTERRUPT;
}

/*------------------------------------------------------------------------- */
void st25RU3993EnterDirectMode(void)
{
    uint8_t command = ST25RU3993_CMD_DIRECT_MODE;
    DISEXTIRQ();
    st25RU3993ClrResponse();

    writeReadST25RU3993(&command, 1, 0, 0, STOP_SGL, 1);

    setPortDirect();
}

/*------------------------------------------------------------------------- */
void st25RU3993ExitDirectMode(void)
{
    setPortNormal();
    st25RU3993SingleCommand(ST25RU3993_CMD_BLOCKRX);
    st25RU3993ClrResponse();
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
uint8_t st25RU3993SetBaseFrequency(uint32_t frequency)
{
    uint8_t regStatusCtrl;
    uint8_t ret;
    uint8_t counter = 0;

    if(!st25RU3993IsFrequencyChangeNecessary(frequency))
    {
        return 1;   // okay
    }

    regStatusCtrl = st25RU3993SingleRead(ST25RU3993_REG_STATUSCTRL);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl & 0xFC); // field off

    // set new frequency
    st25RU3993ContinuousWrite(ST25RU3993_REG_PLLMAIN1, tuningRegs, 3);
    do
    {
        ret = st25RU3993LockPLL();
        counter++;
    } while(!ret && counter<15);

    // restore register setting
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl);
    // remember frequency
    baseFrequency = frequency;
    return ret;
}

/*------------------------------------------------------------------------- */
uint32_t st25RU3993GetBaseFrequency()
{
    return baseFrequency;
}

/*------------------------------------------------------------------------- */
void st25RU3993EnterPowerDownMode(void)
{
    uint8_t addr;
    uint16_t count;

    if(!GET_GPIO(GPIO_EN_PORT,GPIO_EN_PIN))
    {
        return;
    }

    st25RU3993AntennaPower(0); // disables also external PA etc.

    DISEXTIRQ();
    // Switch off antenna
    st25RU3993PowerDownRegs[0] = st25RU3993SingleRead(ST25RU3993_REG_STATUSCTRL);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, st25RU3993PowerDownRegs[0] & 0xFC);
    for(addr=0x01 ; addr<ST25RU3993_REG_ICD ; addr++)
    {
        if(addr!=0x0F)
        {
            st25RU3993PowerDownRegs[addr] = st25RU3993SingleRead(addr);
        }
    }
    st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+0] = st25RU3993SingleRead(ST25RU3993_REG_MIXOPTS);
    st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+1] = st25RU3993SingleRead(ST25RU3993_REG_STATUSPAGE);
    st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+2] = st25RU3993SingleRead(ST25RU3993_REG_IRQMASK1);
    st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+3] = st25RU3993SingleRead(ST25RU3993_REG_IRQMASK2);
    st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+4] = st25RU3993SingleRead(ST25RU3993_REG_TXSETTING);
    st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+5] = st25RU3993SingleRead(ST25RU3993_REG_RXLENGTHUP);
    // Wait for antenna being switched off
    count = 500;
    while(count-- && (st25RU3993SingleRead(ST25RU3993_REG_AGCANDSTATUS) & 0x04))
    {
        delay_ms(1);
    }

    RESET_GPIO(GPIO_EN_PORT,GPIO_EN_PIN);

    // Crystal not stable anymore
    SET_OSC_LED_OFF();
    // PLL not locked anymore
    SET_PLL_LED_OFF();
}

/*------------------------------------------------------------------------- */
void st25RU3993ExitPowerDownMode(void)
{
    uint8_t addr;
#if !NO_LEDS
    uint8_t buf[2];
#endif

    if(GET_GPIO(GPIO_EN_PORT,GPIO_EN_PIN))
    {
        return;
    }

    SET_GPIO(GPIO_EN_PORT,GPIO_EN_PIN);
    delay_us(10);
    st25RU3993WaitForStartup();

    // Do not switch on antenna before PLL is locked.
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, st25RU3993PowerDownRegs[0] & 0xFC);
    for(addr=0x01 ; addr<ST25RU3993_REG_ICD ; addr++)
    {
        if(addr!=0x0F)
        {
            st25RU3993SingleWrite(addr, st25RU3993PowerDownRegs[addr]);
        }
    }
    st25RU3993SingleWrite(ST25RU3993_REG_MIXOPTS,    st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+0]);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSPAGE, st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+1]);
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK1,   st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+2]);
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK2,   st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+3]);
    st25RU3993SingleWrite(ST25RU3993_REG_TXSETTING,  st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+4]);
    st25RU3993SingleWrite(ST25RU3993_REG_RXLENGTHUP, st25RU3993PowerDownRegs[ST25RU3993_REG_ICD+5] & 0xF0);
    delay_us(300);
    st25RU3993LockPLL();
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, st25RU3993PowerDownRegs[0]);

#if !NO_LEDS
    buf[0] = st25RU3993SingleRead(ST25RU3993_REG_AGCANDSTATUS);
#endif

#if !JIGEN && !NO_LEDS && !ELANCE
    if(!(buf[0] & 0x01))
    {
        // Crystal not stable
        SET_OSC_LED_OFF();
    }
    else
    {
        SET_OSC_LED_ON();
    }
#endif

#if !NO_LEDS
    if(!(buf[0] & 0x02))
    {
        // PLL not locked
        SET_PLL_LED_OFF();
    }
    else
    {
        SET_PLL_LED_ON();
    }
#endif
    ENEXTIRQ();
}

/*------------------------------------------------------------------------- */
void st25RU3993Reset(void)
{
    st25RU3993EnterPowerDownMode();
    delay_ms(1);
    st25RU3993ExitPowerDownMode();
}

/*------------------------------------------------------------------------- */
void st25RU3993ResetDoNotPreserveRegisters(void)
{
    RESET_GPIO(GPIO_EN_PORT,GPIO_EN_PIN);
    delay_ms(1);
    SET_GPIO(GPIO_EN_PORT,GPIO_EN_PIN);
    delay_us(10);
    st25RU3993WaitForStartup();
}

/*------------------------------------------------------------------------- */
void st25RU3993EnterPowerNormalMode(void)
{
    uint8_t regStatusCtrl;

    st25RU3993ExitPowerDownMode();      // ensure that EN is high
    regStatusCtrl = st25RU3993SingleRead(ST25RU3993_REG_STATUSCTRL);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl & 0x7F);
    st25RU3993AntennaPower(0);
}

/*------------------------------------------------------------------------- */
void st25RU3993ExitPowerNormalMode(void)
{
}

/*------------------------------------------------------------------------- */
void st25RU3993EnterPowerNormalRfMode(void)
{
    uint8_t regStatusCtrl;

    st25RU3993ExitPowerDownMode();      // ensure that EN is high
    regStatusCtrl = st25RU3993SingleRead(ST25RU3993_REG_STATUSCTRL);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl & 0x7F);
    st25RU3993AntennaPower(1);
}

/*------------------------------------------------------------------------- */
void st25RU3993ExitPowerNormalRfMode(void)
{
}

/*------------------------------------------------------------------------- */
void st25RU3993EnterPowerStandbyMode(void)
{
    uint8_t regStatusCtrl;

    st25RU3993ExitPowerDownMode();      // ensure that EN is high
    st25RU3993AntennaPower(0);
    regStatusCtrl = st25RU3993SingleRead(ST25RU3993_REG_STATUSCTRL);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl | 0x80);

    SET_PLL_LED_OFF(); // PLL not running in standby mode
}

/*------------------------------------------------------------------------- */
void st25RU3993ExitPowerStandbyMode(void)
{
    uint8_t regStatusCtrl;

    st25RU3993ExitPowerDownMode();      // ensure that EN is high
    regStatusCtrl = st25RU3993SingleRead(ST25RU3993_REG_STATUSCTRL);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl & 0x7F);
}

/*------------------------------------------------------------------------- */
void st25RU3993WaitForStartup(void)
{
    uint8_t osc_ok, version;
    uint8_t myBuf[2];
    // maximum trials
    uint16_t count = 250;

	while (count--){
		version = st25RU3993ReadChipVersion() & 0x60;   // ignore revision
		osc_ok = st25RU3993SingleRead(ST25RU3993_REG_AGCANDSTATUS) & 0x01;
		if (version == 0x60 && osc_ok) {
			// stop looping if version match and oscillator is already stable
            break;
		}
	} 

#if !JIGEN && !NO_LEDS && !ELANCE
    if(!osc_ok)
    {
        SET_OSC_LED_OFF();
    }
    else
    {
        SET_OSC_LED_ON();
    }
#endif
    delay_us(500);    
    st25RU3993ContinuousRead(ST25RU3993_REG_IRQSTATUS1, 2, &myBuf[0]);      // ensure that IRQ bits are reset
    st25RU3993ClrResponse();
    delay_ms(2);            // give ST25RU3993 some time to fully initialize
}

/*------------------------------------------------------------------------- */
void st25RU3993AntennaPower(uint8_t on)
{
    uint8_t regStatusCtrl;
    uint16_t count = 2000;
    
    if(on==antennaOn)
    {
        return;
    }
    antennaOn = on;

    regStatusCtrl = st25RU3993SingleRead(ST25RU3993_REG_STATUSCTRL);

    if(on)
    {
        if((regStatusCtrl & 0x03) == 0x03)
        {
            return;
        }

        if(externalPA)
        {
            SET_GPIO(GPIO_PA_LDO_PORT,GPIO_PA_LDO_PIN);
            //delay_us(300);  // Give PA LDO time to get to maximum power
        }

        st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl | 0x03);    // field on, receiver on
        while(count-- && ((st25RU3993SingleRead(ST25RU3993_REG_AGCANDSTATUS) & 0x06) != 0x06))
        {
           //RF field not stable & PLL not stable
           delay_us(10);
        }
        if(count == 0)
        {
            // still not stable
#if !NO_LEDS
            uint8_t regAgc = st25RU3993SingleRead(ST25RU3993_REG_AGCANDSTATUS);
            if(regAgc & 0x02)
            {
                // PLL stable
                SET_PLL_LED_ON();
            }
            else
            {
                SET_PLL_LED_OFF();
            }
#endif
            st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl & 0xFC); // field off
            if(externalPA)
            {
                RESET_GPIO(GPIO_PA_LDO_PORT,GPIO_PA_LDO_PIN);
            }
        }

        // Set RF LED on
        SET_RF_LED_ON();
    }
    else
    {
        if((regStatusCtrl & 0x03) == 0x00)
        {
            return;
        }

        st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl & 0xFC); // field off
        // Wait for antenna being switched off
        count = 5000;
        while(count-- && (st25RU3993SingleRead(ST25RU3993_REG_AGCANDSTATUS) & 0x04))
        {
            //RF field is stable
            delay_us(10);
        }

        if(externalPA)
        {
            RESET_GPIO(GPIO_PA_LDO_PORT,GPIO_PA_LDO_PIN);
        }

        // Set RF LED off
        SET_RF_LED_OFF();
    }

    if(on)
    {
        delay_ms(2);    // Settling Time, according to Gen2 standard we have to wait 1.5ms before issuing commands
    }
}

/*------------------------------------------------------------------------- */
void st25RU3993DecreaseSensitivity(void)
{
    int8_t sensitivity = st25RU3993GetLastSetSensitivity();//st25RU3993GetSensitivity();
    switch(sensitivity)
    {
        case -17: sensitivity = -17; break;
        case -14: sensitivity = -17; break;
        case -11: sensitivity = -14; break;
        case  -9: sensitivity = -11; break;
        case  -8: sensitivity =  -9; break;
        case  -6: sensitivity =  -8; break;
        case  -5: sensitivity =  -6; break;
        case  -3: sensitivity =  -5; break;
        case  -2: sensitivity =  -3; break;
        case   0: sensitivity =  -2; break;
        case   1: sensitivity =   0; break;
        case   3: sensitivity =   1; break;
        case   4: sensitivity =   3; break;
        case   6: sensitivity =   4; break;
        case   7: sensitivity =   6; break;
        case   9: sensitivity =   7; break;
        case  10: sensitivity =   9; break;
        case  13: sensitivity =  10; break;
        case  16: sensitivity =  13; break;
        case  19: sensitivity =  16; break;
        default: break;
    }
    st25RU3993SetSensitivity(sensitivity);
}

/*------------------------------------------------------------------------- */
void st25RU3993IncreaseSensitivity(void)
{
    int8_t sensitivity = st25RU3993GetLastSetSensitivity();//st25RU3993GetSensitivity();
    switch(sensitivity)
    {
        case -17: sensitivity = -14; break;
        case -14: sensitivity = -11; break;
        case -11: sensitivity =  -9; break;
        case  -9: sensitivity =  -8; break;
        case  -8: sensitivity =  -6; break;
        case  -6: sensitivity =  -5; break;
        case  -5: sensitivity =  -3; break;
        case  -3: sensitivity =  -2; break;
        case  -2: sensitivity =   0; break;
        case   0: sensitivity =   1; break;
        case   1: sensitivity =   3; break;
        case   3: sensitivity =   4; break;
        case   4: sensitivity =   6; break;
        case   6: sensitivity =   7; break;
        case   7: sensitivity =   9; break;
        case   9: sensitivity =  10; break;
        case  10: sensitivity =  13; break;
        case  13: sensitivity =  16; break;
        case  16: sensitivity =  19; break;
        case  19: sensitivity =  19; break;
        default: break;
    }
    st25RU3993SetSensitivity(sensitivity);
}
/*------------------------------------------------------------------------- */
int8_t st25RU3993GetLastSetSensitivity(void)
{
    return lastSetSensitivity;
}
/*------------------------------------------------------------------------- */
int8_t st25RU3993GetSensitivity(void)
{
    int8_t sensitivity = 0;
    uint8_t reg0a, reg0d;

    reg0d = st25RU3993SingleRead(ST25RU3993_REG_MISC1);
    reg0a = st25RU3993SingleRead(ST25RU3993_REG_RXMIXERGAIN);
    if(reg0d & 0x04)
    {
        // single ended input mixer
        if((reg0a & 0x03) == 0x03)      // 6dB gain increase
        {
            sensitivity -= 6;
        }
        else if((reg0a & 0x03) == 0x00) // 6dB gain decrease
        {
            sensitivity += 6;
        }
    }
    else
    {
        // differential input mixer
        if(reg0a & 0x01)  // -8 dB attenuation
        {
            sensitivity -= 8;
        }
        if(reg0a & 0x02)  // +10 dB mixer gain increase
        {
            sensitivity += 10;
        }

        if(reg0a & 0x20)
        {
            // RX Gain direction: increase
            sensitivity += ((reg0a>>6)*3);
        }
        else
        {
            sensitivity -= ((reg0a>>6)*3);
        }
    }
    return sensitivity;
}

/*------------------------------------------------------------------------- */
int8_t st25RU3993SetSensitivity(int8_t minimumSignal)
{
    uint8_t reg0d, reg0a;
    
    // remember sensitivity ..
    lastSetSensitivity = minimumSignal;
    
    reg0a = (st25RU3993SingleRead(ST25RU3993_REG_RXMIXERGAIN)&0x1C);
    reg0d = st25RU3993SingleRead(ST25RU3993_REG_MISC1);
    if(reg0d & 0x04)
    {
        // single ended input mixer
        if(minimumSignal >= 6)
        {
            minimumSignal -= 6; // 6dB gain decrease
            reg0a |= 0;
        }
        else if(minimumSignal <= -6)
        {
            minimumSignal += 6; // 6dB gain increase
            reg0a |= 3;
        }
        else
        {
            reg0a |= 1;         // nominal gain
        }
    }
    else
    {
        // differential input mixer
        if(minimumSignal%3 == 0)
        {
            if(minimumSignal<=0)
                minimumSignal = -minimumSignal;
            else
                reg0a |= 0x20;
            reg0a |= (minimumSignal/3)<<6;
        }
        else
        {
            if(minimumSignal<0)
            {
                reg0a |= 0x01;  // -8 dB attenuation
                minimumSignal+=8;
                
                if(minimumSignal<=0)
                    minimumSignal = -minimumSignal;
                else
                    reg0a |= 0x20;
                reg0a |= (minimumSignal/3)<<6;
            }
            else
            {
                reg0a |= 0x02;  // 10 dB mixer gain increase
                minimumSignal-=10;
                
                if(minimumSignal<=0)
                    minimumSignal = -minimumSignal;
                else
                    reg0a |= 0x20;
                reg0a |= (minimumSignal/3)<<6;
            }
        }
    }
    st25RU3993SingleWrite(ST25RU3993_REG_RXMIXERGAIN, reg0a);
    
    if(reg0d & 0x04){
        // apply mixer input voltage range for single ended mixer ..
        uint8_t reg22;
        reg22 = st25RU3993SingleRead(ST25RU3993_REG_MIXOPTS);
        switch (reg0a & 0x03) {
            case 0x00: reg22 = 0x17; break;
            case 0x01: reg22 = 0x14; break;
            case 0x02: reg22 = 0x0A; break;
            case 0x03: reg22 = 0x00; break;
        }
        st25RU3993SingleWrite(ST25RU3993_REG_MIXOPTS, reg22);
    }   
    
    return minimumSignal;
}

/*------------------------------------------------------------------------- */
int8_t st25RU3993GetTxOutputLevel(void)
{
    int8_t txOut = 0;
    uint8_t tmp = (st25RU3993SingleRead(ST25RU3993_REG_MODULATORCONTROL3) & 0x1F);
    switch((tmp >> 3) & 0x03){
        case 0: txOut = 0; break;
        case 1: txOut = -8; break;
        case 2: txOut = -12; break;
        case 3: // RFU
        default:txOut = 0; 
            break;
    }
    txOut -= (tmp & 0x07);
    return txOut;
}

/*------------------------------------------------------------------------- */
void st25RU3993SetTxOutputLevel(int8_t txOutputLevel)
{
    uint8_t tmp = abs(txOutputLevel);    
    uint8_t txOut = 0;
    uint8_t regMod3;
    
    if(tmp >= 12) {
        txOut |= (0x02 << 3);
        tmp -= 12;
    } 
    else if(tmp >= 8) {
        txOut |= (0x01 << 3);
        tmp -= 8;
    }
    txOut |= (tmp & 0x07);

    regMod3 = st25RU3993SingleRead(ST25RU3993_REG_MODULATORCONTROL3);
    regMod3 &= 0xE0;
    regMod3 |= txOut;
    st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL3, regMod3); 
}

/*------------------------------------------------------------------------- */
void st25RU3993GetRSSIMaxSensitive(uint16_t num_of_ms_to_scan, uint8_t *rssiLogI, uint8_t *rssiLogQ)
{
    uint8_t regValRXFilter, regValRXMixerGain, regValPower = 0;
    uint8_t regMixOpt = 0;
    
    //RX Filters to match
    regValRXFilter = st25RU3993SingleRead(ST25RU3993_REG_RXFILTER);
    st25RU3993SingleWrite(ST25RU3993_REG_RXFILTER, 0xFF);
    //RX GAIN maximum
    regValRXMixerGain = st25RU3993SingleRead(ST25RU3993_REG_RXMIXERGAIN);
    st25RU3993SetSensitivity(19);
    // Power to maximum
    regValPower = st25RU3993SingleRead(ST25RU3993_REG_MODULATORCONTROL3);
    st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL3, 0x00);
    //Change Mixer Options
    regMixOpt = st25RU3993SingleRead(ST25RU3993_REG_MIXOPTS);
    st25RU3993SingleWrite(ST25RU3993_REG_MIXOPTS, 0x39);

    st25RU3993GetRSSI(num_of_ms_to_scan, rssiLogI, rssiLogQ);

    //Restore Settings
    st25RU3993SingleWrite(ST25RU3993_REG_RXFILTER, regValRXFilter);
    st25RU3993SingleWrite(ST25RU3993_REG_RXMIXERGAIN, regValRXMixerGain);
    st25RU3993SingleWrite(ST25RU3993_REG_MIXOPTS, regMixOpt);
    st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL3, regValPower);
}

/*------------------------------------------------------------------------- */
void st25RU3993GetRSSI(uint16_t num_of_ms_to_scan, uint8_t *rssiLogI, uint8_t *rssiLogQ)
{
    uint8_t valRssiLog, valRssiLogI, valRssiLogQ = 0;
    uint8_t regStatus, regStatusCtrl, regProt = 0;
    uint8_t regIRQ1Mask = 0;
    uint8_t sumRssiLogMax = 0;
    uint8_t sumRssiLog = 0;
    uint16_t num_of_reads = num_of_ms_to_scan * 2 + 1; // 500us delay
    uint16_t count = 2000;

    // Receiver ON and Transmitter  ON
    regStatusCtrl = st25RU3993SingleRead(ST25RU3993_REG_STATUSCTRL);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl | 0x03); // field on, receiver on
    if((regStatusCtrl & 0x03) != 0x03)
    {
        delay_ms(3); // According Datasheet
    }
    // dir_mode = 1
    regProt = st25RU3993SingleRead(ST25RU3993_REG_PROTOCOLCTRL);
    st25RU3993SingleWrite(ST25RU3993_REG_PROTOCOLCTRL, regProt | 0x40);
    // e_irq_noresp  = 0
    regIRQ1Mask = st25RU3993SingleRead(ST25RU3993_REG_IRQMASK1);
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK1, regIRQ1Mask & 0xFE);
    // Real RSSI
    regStatus = st25RU3993SingleRead(ST25RU3993_REG_STATUSPAGE);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSPAGE, regStatus & 0xF0);

    // PLL check and RF POWER check
    while(count-- && !(st25RU3993SingleRead(ST25RU3993_REG_AGCANDSTATUS) & 0x04))
    {
        // RF field not stable
        delay_us(10);
    }

    // Send Direct Command
    st25RU3993SingleCommand(ST25RU3993_CMD_ENABLERX);
    // Wait and read raw data
    while(num_of_reads--)
    {
        delay_us(500);
        valRssiLog = st25RU3993SingleRead(ST25RU3993_REG_RSSILEVELS);
        valRssiLogI = valRssiLog&0x0F;
        valRssiLogQ = ((valRssiLog >> 4) & 0x0F);
        sumRssiLog = valRssiLogI + valRssiLogQ;

        // take maximum value
        if(sumRssiLog >= sumRssiLogMax)
        {
            sumRssiLogMax = sumRssiLog;
            *rssiLogI = valRssiLogI;
            *rssiLogQ = valRssiLogQ;
        }
    }

    // Restore Settings
    st25RU3993SingleCommand(ST25RU3993_CMD_BLOCKRX);
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK1, regIRQ1Mask);
    st25RU3993SingleWrite(ST25RU3993_REG_PROTOCOLCTRL, regProt);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatusCtrl);
}

/*------------------------------------------------------------------------- */
int8_t st25RU3993GetADC(void)
{
    int8_t val;
    st25RU3993SingleCommand(ST25RU3993_CMD_TRIGGERADCCON);
    delay_us(20);   // according to spec
    val = st25RU3993SingleRead(ST25RU3993_REG_ADC);
    val = CONVERT_ADC_TO_NAT(val);
    return val;
}

/*------------------------------------------------------------------------- */
uint16_t st25RU3993GetReflectedPower(void)
{
    uint16_t valIQ;
    int8_t valI, valQ;
    uint8_t regMeas, regIRQMask1, regIRQMask2,regValProtoCtrl;

    regIRQMask1 = st25RU3993SingleRead(ST25RU3993_REG_IRQMASK1);
    regIRQMask2 = st25RU3993SingleRead(ST25RU3993_REG_IRQMASK2);
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK1, regIRQMask1 & ~0x01); // disable noresponse irq
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK2, regIRQMask2 & ~0x40); // disable cmd irq

    regValProtoCtrl = st25RU3993SingleRead(ST25RU3993_REG_PROTOCOLCTRL);
    st25RU3993SingleWrite(ST25RU3993_REG_PROTOCOLCTRL, regValProtoCtrl | 0x40);

    regMeas = st25RU3993SingleRead(ST25RU3993_REG_MEASUREMENTCONTROL);

    st25RU3993SingleCommand(ST25RU3993_CMD_BLOCKRX);    // Reset the receiver - otherwise the I values seem to oscillate
    st25RU3993SingleCommand(ST25RU3993_CMD_ENABLERX);

    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, (regMeas & 0xF0) | 0xC1); // set msel bits -> Mixer DC level I-channel
    //delay_us(900); // settling time
    valI = st25RU3993GetADC();

    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, (regMeas & 0xF0) | 0xC2); // set msel bits -> Mixer DC level Q-channel
    //delay_us(900); // settling time
    valQ = st25RU3993GetADC();

    st25RU3993SingleCommand(ST25RU3993_CMD_BLOCKRX);    // Disable the receiver since we enabled it before

    // save together
    valIQ = valQ;
    valIQ = (valIQ<<8)&0xFF00;
    valIQ = (valIQ&0xFF00)|(valI&0x00FF);

    // restore previous register values
    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, regMeas);
    st25RU3993SingleWrite(ST25RU3993_REG_PROTOCOLCTRL, regValProtoCtrl);
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK1, regIRQMask1);
    st25RU3993SingleWrite(ST25RU3993_REG_IRQMASK2, regIRQMask2);

    return valIQ;
}

/*------------------------------------------------------------------------- */
uint16_t st25RU3993GetReflectedPowerNoiseLevel(void)
{
    uint16_t valIQ;
    int8_t valI, valQ;
    uint8_t regMeas, regStatus;
    regStatus = st25RU3993SingleRead(ST25RU3993_REG_STATUSCTRL);
    regMeas = st25RU3993SingleRead(ST25RU3993_REG_MEASUREMENTCONTROL);

    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, regMeas & 0x0F);  // disable the OAD pin outputs

    // First measure the offset which might appear with disabled antenna
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, (regStatus & 0xFC) | 0x02);   // field off, receiver on
    st25RU3993SingleCommand(ST25RU3993_CMD_BLOCKRX); // Reset the receiver - otherwise the I values seem to oscillate
    st25RU3993SingleCommand(ST25RU3993_CMD_ENABLERX);

st25RU3993SetSensitivity(0);

    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, (regMeas & 0xF0) | 0x01); // set msel bits -> Mixer DC level I-channel
    delay_us(300); // settling time
    valI = st25RU3993GetADC();

    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, (regMeas & 0xF0) | 0x02); // set msel bits -> Mixer DC level Q-channel
    delay_us(300); // settling time
    valQ = st25RU3993GetADC();

    // save together
    valIQ = valQ;
    valIQ = (valIQ<<8)&0xFF00;
    valIQ = (valIQ&0xFF00)|(valI&0x00FF);

    // restore previous register values
    st25RU3993SingleWrite(ST25RU3993_REG_MEASUREMENTCONTROL, regMeas);
    st25RU3993SingleWrite(ST25RU3993_REG_STATUSCTRL, regStatus);
    return valIQ;
}

/*------------------------------------------------------------------------- */
static void st25RU3993TxBytes(uint8_t *cmd, uint8_t *txbuf, uint16_t txbits)
{
    uint8_t txbytes = (txbits + 7)/8;
    uint8_t totx = txbytes > 24 ? 24 : txbytes;
    uint8_t buf[2];

    // set up two bytes tx length register
    buf[1] = (txbits % 8) << 1;
    buf[1] |= (txbits / 8) << 4;
    buf[0] = txbits / 128;
    if(*cmd != ST25RU3993_CMD_QUERY) {
      st25RU3993ContinuousWrite(ST25RU3993_REG_TXLENGTHUP,buf,2);
    }

    st25RU3993CommandContinuousAddress(cmd, 1, ST25RU3993_REG_FIFO, txbuf, totx);
    txbytes -= totx;
    txbuf += totx;
    while(txbytes) {
        totx = txbytes > 16 ? 18 : txbytes;
        st25RU3993WaitForResponse(RESP_FIFO);
        st25RU3993ClrResponseMask(RESP_FIFO);
        st25RU3993ContinuousWrite(ST25RU3993_REG_FIFO,txbuf,totx);
        txbytes -= totx;
        txbuf += totx;
    }
}

/*------------------------------------------------------------------------- */
int8_t st25RU3993TxRxGen2Bytes(uint8_t cmd, uint8_t *txbuf, uint16_t txbits,
                               uint8_t *rxbuf, uint16_t *rxbits,
                               uint8_t norestime, uint8_t followCmd,
                               uint8_t waitTxIrq)
{
    static uint8_t currCmd;
    uint8_t buf[2];
    uint16_t awaitedBytes = 0;
    uint16_t awaitedBits = 0;
    
    if(rxbits) {
        awaitedBytes = (*rxbits + 7) / 8;
        awaitedBits = *rxbits;
        *rxbits = 0;
    }
    if(rxbuf || awaitedBits) {
        if((cmd != ST25RU3993_CMD_QUERYREP) && (cmd != ST25RU3993_CMD_QUERYADJUSTUP)&& (cmd != ST25RU3993_CMD_QUERYADJUSTNIC)&& (cmd != ST25RU3993_CMD_QUERYADJUSTDOWN))
        {
            st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, norestime);
        }
    }
    if(awaitedBits) {
        if( (cmd != 0) &&
            (cmd != ST25RU3993_CMD_QUERY) &&
            (cmd != ST25RU3993_CMD_QUERYREP) && 
            (cmd != ST25RU3993_CMD_QUERYADJUSTUP)&& 
            (cmd != ST25RU3993_CMD_QUERYADJUSTNIC)&& 
            (cmd != ST25RU3993_CMD_QUERYADJUSTDOWN) && 
            (cmd != ST25RU3993_CMD_REQRN)) {
            buf[1] = awaitedBits & 0xFF;
            buf[0] = ((awaitedBits>>8) & 0x0F) | 0x10;    // set auto_errcode_rxl
            st25RU3993ContinuousWrite(ST25RU3993_REG_RXLENGTHUP,buf,2);
        }
    }



    /****************************************************************/
    /*  Handle TX                                                   */

#if ELANCE    
    // switch LNA + BYP off during transmit
    uint8_t vsd = GET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin);
    uint8_t byd = GET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin);    
    SET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin);
    RESET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin);
#endif

    if(txbits) {
        st25RU3993TxBytes(&cmd, txbuf, txbits);
        currCmd = cmd;
    }
    else if(cmd) {
        st25RU3993SingleCommand(cmd);
        currCmd = cmd;
    }
    if(currCmd && waitTxIrq) {
        st25RU3993WaitForResponse(RESP_TXIRQ);
        st25RU3993ClrResponseMask(RESP_TXIRQ | RESP_FIFO);
    }

#if ELANCE
    // revert LNA + BYP after transmit
    vsd ? SET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin) : RESET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin);
    byd ? SET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin) : RESET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin);
#endif



    /****************************************************************/
    /*  Handle RX                                                   */
    if(rxbits && !awaitedBits && !rxbuf) {
        // we do not want to receive any data
        st25RU3993SingleCommand(ST25RU3993_CMD_BLOCKRX);
        buf[1] = 0;
        buf[0] = 0;
        st25RU3993ContinuousWrite(ST25RU3993_REG_RXLENGTHUP,buf,2);
    }
    if(rxbuf) {
        uint16_t receivedBytes = 0;
        uint8_t count;
        uint16_t resp;

        /***************************************/
        /* Receive bytes from previous command */
        if(0xFF == norestime) {
            uint16_t responseTimout;
            switch(getGen2IntConfig()->blf)
            {
                case GEN2_LF_40:        responseTimout = 120;   break;
                case GEN2_LF_160:       responseTimout = 50;    break;
                case GEN2_LF_213:       responseTimout = 40;    break;
                default:                responseTimout = 30;    break;
            }
            st25RU3993WaitForResponseTimed(RESP_RXDONE_OR_ERROR | RESP_FIFO, responseTimout);
        }
        else {
            st25RU3993WaitForResponse(RESP_RXDONE_OR_ERROR | RESP_FIFO);
        }
        resp = st25RU3993GetResponse();
        while((resp & RESP_FIFO) && !(resp & RESP_RXIRQ)) {     // While data in fifo and not receive RXcompletion
            count = 18;
            if(rxbits && count > awaitedBytes) {
                count = awaitedBytes;
            }
            st25RU3993FifoRead(count, rxbuf);
            rxbuf += count;
            awaitedBytes -= count;
            receivedBytes += count;
            st25RU3993ClrResponseMask(RESP_FIFO);
            if(rxbits && awaitedBytes == 0) {   // we do not want to receive more data
                return ST25RU3993_ERR_RXCOUNT;
            }
            st25RU3993WaitForResponse(RESP_RXDONE_OR_ERROR | RESP_FIFO);
            resp = st25RU3993GetResponse();
        }
        
        /*************************************************/
        /* Wait for previous command completion or error */
        st25RU3993WaitForResponse(RESP_RXDONE_OR_ERROR);
        resp = st25RU3993GetResponse();
        st25RU3993ClrResponse();

        /******************************/
        /* Send following command ... */
        if(followCmd && !(resp & (RESP_NORESINTERRUPT | RESP_RXERR))) {
            switch(getGen2IntConfig()->blf)// prevent violate T2 min
            {
                case GEN2_LF_40:    delay_us(200);  break;
                case GEN2_LF_160:   delay_us(30);   break;
                case GEN2_LF_213:   delay_us(15);   break;
                case GEN2_LF_256:   delay_us(6);    break;
                default:            break;
            }

            st25RU3993SingleCommand(followCmd);
            currCmd = followCmd;
        }

        /***********************************/
        /* ... and receive remaining bytes */
        count = st25RU3993SingleRead(ST25RU3993_REG_FIFOSTATUS) & 0x1F; // get the number of bytes
        if(rxbits && count > awaitedBytes) {
            count = awaitedBytes;
            resp |= RESP_RXCOUNTERROR;
        }
        if(count) {
            st25RU3993FifoRead(count, rxbuf);
        }

        receivedBytes += count;
        if(rxbits) {
            *rxbits = 8 * receivedBytes;
        }

        if(getGen2IntConfig()->blf == GEN2_LF_40) {
            delay_us(100);
        }

        if(resp & (RESP_NORESINTERRUPT | RESP_RXERR | RESP_HEADERBIT | RESP_RXCOUNTERROR)) {
            if(resp & RESP_NORESINTERRUPT) {
				return ST25RU3993_ERR_NORES;
            }
            if((resp & RESP_HEADERBIT) && (currCmd == ST25RU3993_CMD_TRANSMCRCEHEAD)) {
                return ST25RU3993_ERR_HEADER;
            }
            if(resp & RESP_RXCOUNTERROR) {
                return ST25RU3993_ERR_RXCOUNT;
            }
            if(resp & RESP_PREAMBLEERROR) {
                return ST25RU3993_ERR_PREAMBLE;
            }
            if(resp & RESP_CRCERROR) { 
                return ST25RU3993_ERR_CRC;                
            }
            return -1;
        }
    }

    return ERR_NONE;
}


/*------------------------------------------------------------------------- */
int8_t st25RU3993RxGen2EPC(uint8_t *rxbuf, uint16_t *rxbits,
                           uint8_t norestime, uint8_t followCmd,
                           uint8_t waitTxIrq, uint8_t *maxAckReplyCnt)
{
    static uint8_t currCmd2;
    uint8_t buf[2];
    uint16_t awaitedBytes = 0;
    uint16_t awaitedBits = 0;
    
    if(rxbits) {
        awaitedBytes = (*rxbits + 7) / 8;
        awaitedBits = *rxbits;
        *rxbits = 0;
    }

    // Is already setup with the upfront QUERY command
    //st25RU3993SingleWrite(ST25RU3993_REG_RXNORESPONSEWAITTIME, norestime);

    /****************************************************************/
    /*  Handle RX                                                   */
    if(rxbits && !awaitedBits && !rxbuf) {
        // we do not want to receive any data
        st25RU3993SingleCommand(ST25RU3993_CMD_BLOCKRX);
        buf[1] = 0;
        buf[0] = 0;
        st25RU3993ContinuousWrite(ST25RU3993_REG_RXLENGTHUP,buf,2);
    }
    if(rxbuf) {
        uint16_t receivedBytes = 0;
        uint8_t count;
        uint16_t resp;

        /***************************************/
        /* Receive bytes from previous command */
        if(0xFF == norestime) {
            uint16_t responseTimout;
            switch(getGen2IntConfig()->blf)
            {
                case GEN2_LF_40:        responseTimout = 120;   break;
                case GEN2_LF_160:       responseTimout = 50;    break;
                case GEN2_LF_213:       responseTimout = 40;    break;
                default:                responseTimout = 30;    break;
            }
            st25RU3993WaitForResponseTimed(RESP_RXDONE_OR_ERROR | RESP_FIFO, responseTimout);
        }
        else {
            st25RU3993WaitForResponse(RESP_RXDONE_OR_ERROR | RESP_FIFO);
        }
        resp = st25RU3993GetResponse();
        
#if MANUAL_PC_L_BITCORRECTION
        uint16_t resp0 = resp;
#endif       
 
        while((resp & RESP_FIFO) && !(resp & RESP_RXIRQ)) {     // While data in fifo and not receive RXcompletion
            count = 18;
            if(rxbits && count > awaitedBytes) {
                count = awaitedBytes;
            }
            st25RU3993FifoRead(count, rxbuf);
            rxbuf += count;
            awaitedBytes -= count;
            receivedBytes += count;
            st25RU3993ClrResponseMask(RESP_FIFO);
            if(rxbits && awaitedBytes == 0) {   // we do not want to receive more data
                return ST25RU3993_ERR_RXCOUNT;
            }
            st25RU3993WaitForResponse(RESP_RXDONE_OR_ERROR | RESP_FIFO);
            resp = st25RU3993GetResponse();
        }
        
        /*************************************************/
        /* Wait for previous command completion or error */
        st25RU3993WaitForResponse(RESP_RXDONE_OR_ERROR);
        resp = st25RU3993GetResponse();
        st25RU3993ClrResponse();

#if MANUAL_PC_L_BITCORRECTION
    /* manunal bit correction of L in PC */
    if(resp0 == 0x40A0){    //  0x40 .. Irq_cmd; 0xA0 .. Irq_TX | Irq_fifo 
        //if((resp & RESP_CRCERROR) && (resp & RESP_RXCOUNTERROR)) {
        if(resp == 0x0650) {    // 0x06 .. Irq_err1 | Irq_err2; 0x50 .. Irq_err | Irq_Rx
            // correct L to 6 words and try to verify the CRC manual
            uint8_t rxbuf_tmp[14];
            memcpy(rxbuf_tmp, rxbuf, 14);
            rxbuf_tmp[0] &= 0x07;
            rxbuf_tmp[0] |= 0x30;   
            uint16_t crc = crc16Bytewise(rxbuf_tmp, 14);
            uint16_t crc_recv = (rxbuf[14]<<8) | rxbuf[15];
            if(crc == crc_recv){
                // looks that the assumption to correct L to 6 words generates a valid EPC
                // we can correct now the data 
                rxbuf[0] =  rxbuf_tmp[0];   // corrected PC
                resp = 0x0040;              // no eror
                *rxbits = 8 * (2+12);           // 2x PC | 12x EPC
            }
        }
    }
#endif

        /******************************/
        /* Send following command ... */
        if(followCmd && !(resp & (RESP_NORESINTERRUPT | RESP_RXERR))) {
            switch(getGen2IntConfig()->blf)// prevent violate T2 min
            {
                case GEN2_LF_40:    delay_us(200);  break;
                case GEN2_LF_160:   delay_us(30);   break;
                case GEN2_LF_213:   delay_us(15);   break;
                case GEN2_LF_256:   delay_us(6);    break;
                default:            break;
            }

            st25RU3993SingleCommand(followCmd);
            currCmd2 = followCmd;
        }

        /***********************************/
        /* ... and receive remaining bytes */
        count = st25RU3993SingleRead(ST25RU3993_REG_FIFOSTATUS) & 0x1F; // get the number of bytes
        if(rxbits && count > awaitedBytes) {
            count = awaitedBytes;
            resp |= RESP_RXCOUNTERROR;
        }
        if(count) {
            st25RU3993FifoRead(count, rxbuf);
        }

        receivedBytes += count;
        if(rxbits) {
            *rxbits = 8 * receivedBytes;
        }

        if(getGen2IntConfig()->blf == GEN2_LF_40) {
            delay_us(100);
        }

        if(resp & (RESP_NORESINTERRUPT | RESP_RXERR | RESP_HEADERBIT | RESP_RXCOUNTERROR)) {
            if(resp & RESP_NORESINTERRUPT) {
                return ST25RU3993_ERR_NORES;
            }
            if((resp & RESP_HEADERBIT) && (currCmd2 == ST25RU3993_CMD_TRANSMCRCEHEAD)) {
                return ST25RU3993_ERR_HEADER;
            }
            if(resp & RESP_RXCOUNTERROR) {
                    return ST25RU3993_ERR_RXCOUNT;
            }
            if(resp & RESP_PREAMBLEERROR) {
                return ST25RU3993_ERR_PREAMBLE;
            }
            if(resp & RESP_CRCERROR) {                
                while((*maxAckReplyCnt)-- > 0){ 
                    resp = replyACK(rxbuf, rxbits, followCmd);
                    if(resp == ERR_NONE){
                        break;
                    }
                }
                return resp;                                
            }
            return -1;
        }
    }

    return ERR_NONE;
}

static int8_t replyACK(uint8_t *rxbuf, uint16_t *rxbits, uint8_t followCmd)
{
    static uint8_t currCmd3;
    uint16_t awaitedBytes = 0;
    if(rxbits) {
        awaitedBytes = (*rxbits + 7) / 8;
//        awaitedBits = *rxbits;
        *rxbits = 0;
    }

    /****************************************************************/
    /*  Handle TX                                                   */

#if ELANCE    
    // switch LNA + BYP off during transmit
    uint8_t vsd = GET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin);
    uint8_t byd = GET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin);    
    SET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin);
    RESET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin);
#endif

    st25RU3993SingleCommand(ST25RU3993_CMD_ACK);
        
#if ELANCE
    // revert LNA + BYP after transmit
    vsd ? SET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin) : RESET_GPIO(VLNA_VSD_GPIO_Port,VLNA_VSD_Pin);
    byd ? SET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin) : RESET_GPIO(VLNA_BYP_GPIO_Port,VLNA_BYP_Pin);
#endif



    /****************************************************************/
    /*  Handle RX                                                   */
     if(rxbuf) {
        uint16_t receivedBytes = 0;
        uint8_t count;
        uint16_t resp;

        /***************************************/
        /* Receive bytes from previous command */
        st25RU3993WaitForResponse(RESP_RXDONE_OR_ERROR | RESP_FIFO);
        resp = st25RU3993GetResponse();
        while((resp & RESP_FIFO) && !(resp & RESP_RXIRQ)) {     // While data in fifo and not receive RXcompletion
            count = 18;
            if(rxbits && (count > awaitedBytes)) {
                count = awaitedBytes;
            }
            st25RU3993FifoRead(count, rxbuf);
            rxbuf += count;
            awaitedBytes -= count;
            receivedBytes += count;
            st25RU3993ClrResponseMask(RESP_FIFO);
            if(rxbits && (awaitedBytes == 0)) {   // we do not want to receive more data
                return ST25RU3993_ERR_RXCOUNT;
            }
            st25RU3993WaitForResponse(RESP_RXDONE_OR_ERROR | RESP_FIFO);
            resp = st25RU3993GetResponse();
        }
        
        /*************************************************/
        /* Wait for previous command completion or error */
        st25RU3993WaitForResponse(RESP_RXDONE_OR_ERROR);
        resp = st25RU3993GetResponse();
        st25RU3993ClrResponse();

        /******************************/
        /* Send following command ... */
        if(followCmd && !(resp & (RESP_NORESINTERRUPT | RESP_RXERR))) {
            switch(getGen2IntConfig()->blf)// prevent violate T2 min
            {
                case GEN2_LF_40:    delay_us(200);  break;
                case GEN2_LF_160:   delay_us(30);   break;
                case GEN2_LF_213:   delay_us(15);   break;
                case GEN2_LF_256:   delay_us(6);    break;
                default:            break;
            }

            st25RU3993SingleCommand(followCmd);
            currCmd3 = followCmd;
        }

        /***********************************/
        /* ... and receive remaining bytes */
        count = st25RU3993SingleRead(ST25RU3993_REG_FIFOSTATUS) & 0x1F; // get the number of bytes
        if(rxbits && count > awaitedBytes) {
            count = awaitedBytes;
            resp |= RESP_RXCOUNTERROR;
        }
        if(count) {
            st25RU3993FifoRead(count, rxbuf);
        }

        receivedBytes += count;
        if(rxbits) {
            *rxbits = 8 * receivedBytes;
        }

        if(getGen2IntConfig()->blf == GEN2_LF_40) {
            delay_us(100);
        }

        if(resp & (RESP_NORESINTERRUPT | RESP_RXERR | RESP_HEADERBIT | RESP_RXCOUNTERROR)) {
            if(resp & RESP_NORESINTERRUPT) {
				return ST25RU3993_ERR_NORES;
            }
            if((resp & RESP_HEADERBIT) && (currCmd3 == ST25RU3993_CMD_TRANSMCRCEHEAD)) {
                return ST25RU3993_ERR_HEADER;
            }
            if(resp & RESP_RXCOUNTERROR) {
                return ST25RU3993_ERR_RXCOUNT;
            }
            if(resp & RESP_PREAMBLEERROR) {
                return ST25RU3993_ERR_PREAMBLE;
            }
            if(resp & RESP_CRCERROR) { 
                return ST25RU3993_ERR_CRC;                
            }
            return -1;
        }
    }

    return ERR_NONE;
}


/**
  * @}
  */
