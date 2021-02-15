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
 *  @brief Implementation of tuner functionality.
 *
 *  This file includes the whole functionality to adjust the PI tuner network
 *  on tuner enabled boards.
 *  The communication pins for the tuner network are defined in platform.h.
 *  The tuner network consists of 3 variable capacitors: Cin, Clen and Cout
 *  and looks like this:
 \code
                         .--- L ---.
                         |         |
  in ----.----.----- L --.--C_len--.--.-----.-----.----- out
         |    |                       |     |     |
        C_in  L                     C_out   L     R
         |    |                       |     |     |
        ___  ___                     ___   ___   ___
         -    -                       -     -     -
         '    '                       '     '     '
 \endcode
 */

/** @addtogroup ST25RU3993
  * @{
  */
/** @addtogroup Tuner
  * @{
  */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "spi.h"
#include "tuner.h"
#include "flash_access.h"
#include "st25RU3993_public.h"
#include "platform.h"
#include "logger.h"
#include "timer.h"
#include "spi_driver.h"
#if ELANCE
#include "rffe.h"
#endif

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#if VERBOSE_LOG && (USE_LOGGER == LOGGER_ON)
static uint8_t curCin, curCout, curClen;
#endif

#define GET_SPI_CPHA            hspi1.Init.CLKPhase
#define SET_SPI_CPHA(cpha)      do { hspi1.Init.CLKPhase = cpha; HAL_SPI_Init(&hspi1); } while ( 0 )
  
#ifdef TUNER_CONFIG_CIN
#define TUNER_SETCAP_CIN tunerSetCapCin
#else
#define TUNER_SETCAP_CIN(...)
#endif
#ifdef TUNER_CONFIG_CLEN
#define TUNER_SETCAP_CLEN tunerSetCapClen
#else
#define TUNER_SETCAP_CLEN(...)
#endif
#ifdef TUNER_CONFIG_COUT
#define TUNER_SETCAP_COUT tunerSetCapCout
#else
#define TUNER_SETCAP_COUT(...)
#endif

#define FALSE_POS_MIN_DIFF      5   // minimum reflected power value change of x
#define FALSE_POS_REFL_SMALL    20  // check false positive if reflected power < x
#define FALSE_POS_REFL_DELTA_LOWER_TX   5   // delta reflected power gap for comparision after lowering the TX power

// 0x080FE000 - 0x080FFF00 reserved for tuning table data
#if ELANCE
#define FLASH_STORE_START_TUNING_TABLE_EUROPE       0x0807F800
#define FLASH_STORE_START_TUNING_TABLE_USA          0x0807F000
#define FLASH_STORE_START_TUNING_TABLE_JAPAN        0x0807E800
#define FLASH_STORE_START_TUNING_TABLE_CHINA        0x0807E000
#define FLASH_STORE_START_TUNING_TABLE_CHINA2       0x0807D800
#define FLASH_STORE_START_TUNING_TABLE_CUSTOM       0x0807C800
#define FLASH_STORE_START_TUNING_TABLE_NEWTUNING    0x0807C000

#define FLASH_STORE_NEWTUNING_ANTENNA_BLOCK_SIZE    (0x800/FW_NB_ANTENNA)
#elif JIGEN && defined(STM32WB55xx)
#define FLASH_STORE_START_TUNING_TABLE_EUROPE       0x0807F000
#define FLASH_STORE_START_TUNING_TABLE_USA          0x0807E000
#define FLASH_STORE_START_TUNING_TABLE_JAPAN        0x0807D000
#define FLASH_STORE_START_TUNING_TABLE_CHINA        0x0807C000
#define FLASH_STORE_START_TUNING_TABLE_CHINA2       0x0807B000
#define FLASH_STORE_START_TUNING_TABLE_CUSTOM       0x0807A000
#define FLASH_STORE_START_TUNING_TABLE_NEWTUNING    0x08079000

#define FLASH_STORE_NEWTUNING_ANTENNA_BLOCK_SIZE    (0x1000/FW_NB_ANTENNA)
#else
#define FLASH_STORE_START_TUNING_TABLE_EUROPE       0x080FF800
#define FLASH_STORE_START_TUNING_TABLE_USA          0x080FF000
#define FLASH_STORE_START_TUNING_TABLE_JAPAN        0x080FE800
#define FLASH_STORE_START_TUNING_TABLE_CHINA        0x080FE000
#define FLASH_STORE_START_TUNING_TABLE_CHINA2       0x080FD800
#define FLASH_STORE_START_TUNING_TABLE_CUSTOM       0x080FD000
#define FLASH_STORE_START_TUNING_TABLE_NEWTUNING    0x080FC800

#define FLASH_STORE_NEWTUNING_ANTENNA_BLOCK_SIZE    (0x800/FW_NB_ANTENNA)
#endif

// If next struct generates compilation error, this reveals the flash block antenna size is below STUHFL_T_ST25RU3993_ChannelList structure size which should not be !!
typedef char assertion_on_FlashBlockAntennaSize[(sizeof(STUHFL_T_ST25RU3993_ChannelList)<=FLASH_STORE_NEWTUNING_ANTENNA_BLOCK_SIZE)*2-1 ];


#define MAX_MULTI_HILLCLIMB_SUMMITS     3


/*
******************************************************************************
* EXTERNAL FUNCTIONS
******************************************************************************
*/

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
/** The tuningTable contains a list of frequencies and values for the tuner for every frequency.
 * When frequency hopping is performed (hopFrequencies()) the closest frequency in the list is looked up
 * and the corresponding tuner values are applied to the DTCs of the tuner.
 * The tuningTable can be modified via callTunerTable().  */
tuningTable_t tuningTable[NUM_SAVED_PROFILES];

extern uint8_t usedAntenna;

extern freq_t frequencies[NUM_SAVED_PROFILES];

/*
******************************************************************************
* LOCAL STRUCTS
******************************************************************************
*/
typedef struct {
    uint16_t    reflectedPower;
    uint8_t     cin;
    uint8_t     cout;
    uint8_t     clen;
} tuning_data_t;

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
#if defined(TUNER_CONFIG_CIN) || defined(TUNER_CONFIG_CLEN) || defined(TUNER_CONFIG_COUT)
    /** equidistant startint points for hill climb (value space 0-31, so starting 
      * values are 5, 11, 16, 21 and 27 to each be in the center of a fifth) */
    //static const uint8_t tuneStartPoints[5] = {21,42,64,85,107};  // ELANCE
    //static const uint8_t tuneStartPoints[5] = {5,11,16,21,27};    // !ELANCE
#define START_POINTS 3

static const uint8_t tuneStartPoints[START_POINTS] = {
         (((MAX_DAC_VALUE+1)*1)/(START_POINTS+1))
#if (START_POINTS > 1)
        ,(((MAX_DAC_VALUE+1)*2)/(START_POINTS+1))
#endif          
#if (START_POINTS > 2)
        ,(((MAX_DAC_VALUE+1)*3)/(START_POINTS+1))
#endif          
#if (START_POINTS > 3)
        ,(((MAX_DAC_VALUE+1)*4)/(START_POINTS+1))
#endif          
#if (START_POINTS > 4)
        ,(((MAX_DAC_VALUE+1)*5)/(START_POINTS+1))
#endif          
#if (START_POINTS > 5)
        ,(((MAX_DAC_VALUE+1)*6)/(START_POINTS+1))
#endif          
#if (START_POINTS > 6)
        ,(((MAX_DAC_VALUE+1)*7)/(START_POINTS+1))
#endif          
#if (START_POINTS > 7)
        ,(((MAX_DAC_VALUE+1)*8)/(START_POINTS+1))
#endif          
#if (START_POINTS > 8)
        ,(((MAX_DAC_VALUE+1)*9)/(START_POINTS+1))
#endif          
};

#define START_POINTS_ENHANCED 4
static const uint8_t tuneStartPoints_Enhanced[START_POINTS_ENHANCED] = {
         (((MAX_DAC_VALUE+1)*1)/(START_POINTS_ENHANCED+1))
#if (START_POINTS_ENHANCED > 1)
        ,(((MAX_DAC_VALUE+1)*2)/(START_POINTS_ENHANCED+1))
#endif          
#if (START_POINTS_ENHANCED > 2)
        ,(((MAX_DAC_VALUE+1)*3)/(START_POINTS_ENHANCED+1))
#endif          
#if (START_POINTS_ENHANCED > 3)
        ,(((MAX_DAC_VALUE+1)*4)/(START_POINTS_ENHANCED+1))
#endif          
#if (START_POINTS_ENHANCED > 4)
        ,(((MAX_DAC_VALUE+1)*5)/(START_POINTS_ENHANCED+1))
#endif          
#if (START_POINTS_ENHANCED > 5)
        ,(((MAX_DAC_VALUE+1)*6)/(START_POINTS_ENHANCED+1))
#endif          
#if (START_POINTS_ENHANCED > 6)
        ,(((MAX_DAC_VALUE+1)*7)/(START_POINTS_ENHANCED+1))
#endif          
#if (START_POINTS_ENHANCED > 7)
        ,(((MAX_DAC_VALUE+1)*8)/(START_POINTS_ENHANCED+1))
#endif          
#if (START_POINTS_ENHANCED > 8)
        ,(((MAX_DAC_VALUE+1)*9)/(START_POINTS_ENHANCED+1))
#endif          
};
    
#endif


#if ELANCE
static uint8_t USID = 0x07;
#endif

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
static void tunerSetCaps(uint8_t cin, uint8_t clen, uint8_t cout);
#ifdef TUNER_CONFIG_CIN
static void tunerSetCapCin(uint8_t val);
#endif
#ifdef TUNER_CONFIG_CLEN
static void tunerSetCapClen(uint8_t val);
#endif
#ifdef TUNER_CONFIG_COUT
static void tunerSetCapCout(uint8_t val);
#endif
static void replaceTuningTable(tuningTable_t *tableOut, uint32_t freq, uint8_t iTarget, tuningTable_t *tableIn);
static uint8_t tunerClimbOneParam(STUHFL_T_ST25RU3993_Caps *caps, bool doFalsePositiveDetection, const uint8_t el, uint8_t *elval, uint16_t *reflectedPower, uint8_t *maxSteps);
static uint8_t detectFalsePositive(STUHFL_T_ST25RU3993_Caps *caps, const uint8_t el, const uint8_t elval, const uint16_t reflectedPower, const int8_t dir);
static uint8_t detectReflectedPowerChange(const uint8_t cin, const uint8_t clen, const uint8_t cout, const uint16_t reflectedPower);
static uint8_t reduceTxWhenFalsePositive(uint16_t reflectedPower);

#if (USE_LOGGER == LOGGER_ON)
#if ELANCE

void sweepCaps(void)
{
    uint16_t iqTogetherReflected;
    int8_t i, q;

    st25RU3993AntennaPower(1);
    LOG("-- cin, clen, cout, i, q -- sweep start\n");
    for(uint8_t cin = 0; cin < 128; cin += 4){
        for(uint8_t clen = 0; clen < 128; clen += 4){
            for(uint8_t cout = 0; cout < 128; cout += 4){
                //
                tunerSetCaps(cin, clen, cout);

                //delay_us(5000);
                iqTogetherReflected = st25RU3993GetReflectedPower();
                i = (iqTogetherReflected & 0xFF);
                //i = (i<0?-i:i);
                q = ((iqTogetherReflected >> 8)&0xFF);
                //q = (q<0?-q:q);

                LOG("%d,%d,%d,%d,%d\n", cin, clen, cout, i, q);
            }
        }
    }
    LOG("-- sweep stop\n");
    st25RU3993AntennaPower(0);

}
#endif

#if EVAL
void sweepCaps(void)
{
    uint16_t iqTogetherReflected;
    int8_t i, q;

    st25RU3993AntennaPower(1);
    LOG("-- cin, clen, cout, i, q -- sweep start\n");
    for(uint8_t cin = 0; cin < 32; cin ++){
        for(uint8_t clen = 0; clen < 32; clen ++){
            for(uint8_t cout = 0; cout < 32; cout++){
                //
                tunerSetCaps(cin, clen, cout);

                //delay_us(5000);
                iqTogetherReflected = st25RU3993GetReflectedPower();
                i = (iqTogetherReflected & 0xFF);
                //i = (i<0?-i:i);
                q = ((iqTogetherReflected >> 8)&0xFF);
                //q = (q<0?-q:q);

                LOG("%d,%d,%d,%d,%d\n", cin, clen, cout, i, q);
            }
        }
    }
    LOG("-- sweep stop\n");
    st25RU3993AntennaPower(0);

}
#endif
#endif

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
static void tunerSetCaps(uint8_t cin, uint8_t clen, uint8_t cout)
{
    TUNER_SETCAP_CIN(cin);
    TUNER_SETCAP_CLEN(clen);
    TUNER_SETCAP_COUT(cout);
}

/*------------------------------------------------------------------------- */
static uint8_t detectReflectedPowerChange(const uint8_t cin, const uint8_t clen, const uint8_t cout, const uint16_t reflectedPower)
{
    uint16_t diff;
    uint16_t reflectedPowerNew;
    
    tunerSetCaps(cin,clen,cout);
    reflectedPowerNew = tunerGetReflected();
    
    diff = (reflectedPowerNew > reflectedPower ? reflectedPowerNew - reflectedPower : reflectedPower-reflectedPowerNew);
    if(diff < FALSE_POS_MIN_DIFF)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static uint8_t reduceTxWhenFalsePositive(uint16_t reflectedPower)
{
    uint16_t reflPwr = 0;
    uint8_t tx_power_original = st25RU3993SingleRead(ST25RU3993_REG_MODULATORCONTROL3);
    uint8_t tx_power = (tx_power_original&0x1F);
    uint8_t errors = 0;
    
    if(tx_power == 23){  // Already tried with minimum Tx power
        return 0;
    }

    tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power);
    tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power);
    tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power); // TX power -3
    st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL3,((tx_power_original&0xE0) | tx_power));
    reflPwr = tunerGetReflected();
    errors = (reflPwr > (reflectedPower + FALSE_POS_REFL_DELTA_LOWER_TX) ? errors+1:errors);
    
    if(errors == 0){
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power);
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power);
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power); // TX power -6
        st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL3,((tx_power_original&0xE0) | tx_power));
        reflPwr = tunerGetReflected();
        errors = (reflPwr > (reflectedPower + FALSE_POS_REFL_DELTA_LOWER_TX) ? errors+1:errors);
    }

    if(errors == 0){
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power);
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power);
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power); // TX power -9
        st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL3,((tx_power_original&0xE0) | tx_power));
        reflPwr = tunerGetReflected();
        errors = (reflPwr > (reflectedPower + FALSE_POS_REFL_DELTA_LOWER_TX) ? errors+1:errors);
    }
    if(errors == 0){
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power);
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power);
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power); // TX power -12
        st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL3,((tx_power_original&0xE0) | tx_power));
        reflPwr = tunerGetReflected();
        errors = (reflPwr > (reflectedPower + FALSE_POS_REFL_DELTA_LOWER_TX) ? errors+1:errors);
    }
    if(errors == 0){
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power);
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power);
        tx_power = (tx_power < 23 ? (tx_power == 15 ? tx_power+5 : tx_power+1) : tx_power); // TX power -15
        st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL3,((tx_power_original&0xE0) | tx_power));
        reflPwr = tunerGetReflected();
        errors = (reflPwr > (reflectedPower + FALSE_POS_REFL_DELTA_LOWER_TX) ? errors+1:errors);
    }
    
    if(errors){
        // error condition, reduce tx power
        return 1;
    } 
    else{
        st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL3,tx_power_original);
    }
    
    return 0;
}

/*------------------------------------------------------------------------- */
static uint8_t detectFalsePositive(STUHFL_T_ST25RU3993_Caps *caps, const uint8_t el, const uint8_t elval, const uint16_t reflectedPower, const int8_t dir)
{
    STUHFL_T_ST25RU3993_Caps    origCaps;

    origCaps.cin = (el == TUNER_CIN) ? elval : caps->cin;
    origCaps.clen = (el == TUNER_CLEN) ? elval : caps->clen;
    origCaps.cout = (el == TUNER_COUT) ? elval : caps->cout;
    
    // reflected power extremely small (< -30 dBm)
    if(reflectedPower < FALSE_POS_REFL_SMALL)
    {
        uint8_t errors = 0;
        uint8_t i;
        
        if(el == TUNER_CIN) {
            for(i = 1; i < 4; i++){
                int8_t c = origCaps.cin + (dir * i);
                if(c < 0) { c = 0; }
                if(c > MAX_DAC_VALUE) { c =  MAX_DAC_VALUE; }
                errors += detectReflectedPowerChange(c, origCaps.clen, origCaps.cout, reflectedPower);
           }
            
        }
        if(el == TUNER_CLEN) {
            for(i = 1; i < 4; i++){
                int8_t c = origCaps.clen + (dir * i);
                if(c < 0) { c = 0; }
                if(c > MAX_DAC_VALUE) { c =  MAX_DAC_VALUE; }
                errors += detectReflectedPowerChange(origCaps.cin, c, origCaps.cout, reflectedPower); 
            }
        }
        if(el == TUNER_COUT) {
            for(i = 1; i < 4; i++){
                int8_t c = origCaps.cout + (dir * i);
                if(c < 0) { c = 0; }
                if(c > MAX_DAC_VALUE) { c =  MAX_DAC_VALUE; }
                errors += detectReflectedPowerChange(origCaps.cin, origCaps.clen, c, reflectedPower);                
            }
        }        
        
        tunerSetCaps(origCaps.cin, origCaps.clen, origCaps.cout);
        
        // at least x times the change of 1/2/3 C values had no significant effect on the reflected power
        if(errors)  // 0-7 (3 - 3 - 1)
        {
            reduceTxWhenFalsePositive(reflectedPower);
        }
    }
    return 0;
}

/*------------------------------------------------------------------------- */
#ifdef TUNER_CONFIG_CIN
static void tunerSetCapCin(uint8_t val)
{
#if ELANCE
    RFFE_WriteRegister(USID, 0x02, val);
    delay_us(20);
#else
uint32_t cpha = GET_SPI_CPHA;
SET_SPI_CPHA(SPI_PHASE_1EDGE);

    SET_GPIO(GPIO_CIN_PORT,GPIO_CIN_PIN);
    spiTxRx(&val, 0, 1);
    RESET_GPIO(GPIO_CIN_PORT,GPIO_CIN_PIN);
    delay_us(20);

SET_SPI_CPHA(cpha);
#endif
#if VERBOSE_LOG && (USE_LOGGER == LOGGER_ON)
curCin = val;    
#endif
}
#endif

/*------------------------------------------------------------------------- */
#ifdef TUNER_CONFIG_CLEN
static void tunerSetCapClen(uint8_t val)
{
#if ELANCE
    RFFE_WriteRegister(USID, 0x03, val);
    delay_us(20);
#else
uint32_t cpha = GET_SPI_CPHA;
SET_SPI_CPHA(SPI_PHASE_1EDGE);

    SET_GPIO(GPIO_CLEN_PORT,GPIO_CLEN_PIN);
    spiTxRx(&val, 0, 1);
    RESET_GPIO(GPIO_CLEN_PORT,GPIO_CLEN_PIN);
    delay_us(20);

SET_SPI_CPHA(cpha);
#endif
#if VERBOSE_LOG && (USE_LOGGER == LOGGER_ON)
curClen = val;
#endif
}
#endif

/*------------------------------------------------------------------------- */
#ifdef TUNER_CONFIG_COUT
static void tunerSetCapCout(uint8_t val)
{
#if ELANCE
    RFFE_WriteRegister(USID, 0x04, val);
    delay_us(20);
#else
uint32_t cpha = GET_SPI_CPHA;
SET_SPI_CPHA(SPI_PHASE_1EDGE);
  
    SET_GPIO(GPIO_COUT_PORT,GPIO_COUT_PIN);
    spiTxRx(&val, 0, 1);
    RESET_GPIO(GPIO_COUT_PORT,GPIO_COUT_PIN);
    delay_us(20);
    
SET_SPI_CPHA(cpha);
#endif
#if VERBOSE_LOG && (USE_LOGGER == LOGGER_ON)
curCout = val;
#endif
}
#endif

/*------------------------------------------------------------------------- */
static void replaceTuningTable(tuningTable_t *tableOut, uint32_t freq, uint8_t iTarget, tuningTable_t *tableIn)
{
    uint8_t iSource;
    if(freq==RESET_DEFAULT_ALL_FREQS)
    {
        // whole tuning table is replaced
        memcpy(tableOut, tableIn, sizeof(tuningTable_t));
    }
    else
    {
        // only one frequency entry in the table is replaced
        for(iSource=0;iSource<tableIn->tableSize;iSource++)
        {
            // find entry of default table relating to current tuning table
            if(tableIn->freq[iSource]==freq)
                break;
        }
        if (iSource >= tableIn->tableSize) {
            return;
        }
        
        for (uint8_t ant=0 ; ant<FW_NB_ANTENNA ; ant++) {
            tableOut->cin[ant][iTarget]  = tableIn->cin[ant][iSource];
            tableOut->clen[ant][iTarget]  = tableIn->clen[ant][iSource];
            tableOut->cout[ant][iTarget]  = tableIn->cout[ant][iSource];
            tableOut->tunedIQ[ant][iTarget]  = tableIn->tunedIQ[ant][iSource];
        }
    }
}

/*------------------------------------------------------------------------- */
// el       .. Pi circuite ELement (el = [CIN, CLEN, COUT])
// elval    .. cap value of element. Start from current value and reply new
static uint8_t tunerClimbOneParam(STUHFL_T_ST25RU3993_Caps *caps, bool doFalsePositiveDetection, const uint8_t el, uint8_t *elval, uint16_t *reflectedPower, uint8_t *maxSteps)
{
    uint8_t maxSteps_start = *maxSteps;
    uint16_t reflectedPowerStart = *reflectedPower;
    uint8_t tx_power_original = st25RU3993SingleRead(ST25RU3993_REG_MODULATORCONTROL3);
    uint16_t refl;
    int8_t dir = 0;
    int8_t add = 0;
    uint8_t improvement = 3;
    uint8_t bestelval;

    if(doFalsePositiveDetection){
        // check if we start off from falsePositive and if so reduce TX power..
        if(*reflectedPower < FALSE_POS_REFL_SMALL){
            reduceTxWhenFalsePositive(*reflectedPower);
        }
    }    

    do
    {
        dir = 0;
        add = 0;
        improvement = 3;

        // test cap value - 1
        *maxSteps = maxSteps_start;
        if(*elval > 0){
            tunerSetCap(el, *elval-1);
            refl = tunerGetReflected();
            if(refl < *reflectedPower){
                *reflectedPower = refl;
                dir = -1;
            }
        }
        // test cap value + 1
        if(*elval < MAX_DAC_VALUE){
            tunerSetCap(el, *elval+1);
            refl = tunerGetReflected();
            if(refl < *reflectedPower){
                *reflectedPower = refl;
                dir = 1;
            }
        }

        /* if it's possible we try to change the value by 2 and check what direction is better.
         * This improves tuning in the case of a local minima. */

        // test cap value - 2
        if(*elval > 1){
            tunerSetCap(el, *elval-2);
            refl = tunerGetReflected();
            if(refl < *reflectedPower){
                *reflectedPower = refl;
                dir = -1;
                add = -1;
            }
        }

        // test cap value + 2
        if(*elval < (MAX_DAC_VALUE-1)){
            tunerSetCap(el, *elval+2);
            refl = tunerGetReflected();
            if(refl < *reflectedPower){
                *reflectedPower = refl;
                dir = 1;
                add = 1;
            }
        }

        // apply values
        *elval += add;
        *elval += dir;
        *elval = (*elval > MAX_DAC_VALUE ? MAX_DAC_VALUE : *elval);
        bestelval = *elval;
        tunerSetCap(el, *elval);

        if(dir!=0)
        {
            // continue until improvements or maxSteps
            (*maxSteps)--;
            while(improvement && *maxSteps)
            {
                if(*elval == 0){
                    break;
                }
                if(*elval == MAX_DAC_VALUE){
                    break;
                }
                tunerSetCap(el, *elval + dir);
                refl = tunerGetReflected();
                
                if(refl < *reflectedPower){
                    (*maxSteps)--;
                    *reflectedPower = refl;
                    *elval += dir;
                    *elval = (*elval > MAX_DAC_VALUE ? MAX_DAC_VALUE : *elval);
                    bestelval = *elval;
                    improvement = 3; /* we don't want to stop when a local minima is hit, so we
                    continue tuning for 3 more steps, even if they are worse than the one before.
                    If it does not improve in this 3 steps, we will go back to the best value. */
                }
                else{
                    improvement--;
                    //break;
                    
                }
            }
        }
        *elval = bestelval;
        tunerSetCap(el, *elval);

        if(!doFalsePositiveDetection){
            // if no false positive detection abort loop in any case ..
            break;
        }
    } while(detectFalsePositive(caps, el, *elval, *reflectedPower, dir));
    
    // false positive condition failed
    st25RU3993SingleWrite(ST25RU3993_REG_MODULATORCONTROL3, tx_power_original);

     // improvement ?
	return (reflectedPowerStart > (*reflectedPower)) ? 1 : 0; 
}

/*------------------------------------------------------------------------- */
static uint32_t getFlashStoreAddr(uint8_t p)
{
    switch (p) {
        case PROFILE_EUROPE:    return FLASH_STORE_START_TUNING_TABLE_EUROPE;   
        case PROFILE_USA:       return FLASH_STORE_START_TUNING_TABLE_USA;      
        case PROFILE_JAPAN:     return FLASH_STORE_START_TUNING_TABLE_JAPAN;    
        case PROFILE_CHINA:     return FLASH_STORE_START_TUNING_TABLE_CHINA;    
        case PROFILE_CHINA2:    return FLASH_STORE_START_TUNING_TABLE_CHINA2;   
        case PROFILE_CUSTOM:    return FLASH_STORE_START_TUNING_TABLE_CUSTOM;   
        case PROFILE_NEWTUNING: return FLASH_STORE_START_TUNING_TABLE_NEWTUNING;
        default:                return 0;
    }
}

/*------------------------------------------------------------------------- */
static bool loadFromFlash64Bit(uint32_t addr, uint16_t dataLen, uint8_t *data)
{
    uint8_t tmp = (uint8_t) *(uint32_t*)(addr);

    // check the first byte of the tuning table if it is already programmed (!= 0xFF) 
    if (tmp != 0xFF) {
        for(uint32_t i=0 ; i<dataLen ; i++) {
            data[i] = (uint8_t) *(uint32_t*)(addr + i);
        }
        return true;
    }
    return false;
}

/*------------------------------------------------------------------------- */
static void saveToFlash64Bit(uint32_t addr, uint16_t dataLen, uint8_t *data)
{
    // correct length to be aligned with 64 bits
    dataLen -= (dataLen % 8);
    dataLen += 8;
    // IMPORTANT: because we can only write 64 bits aligned we also need to
    // align the tuningTable sizes to it or have some margin at the end of the
    // table do not overwrite the data behind
    uint64_t page = 0;
    for(uint32_t i=0 ; i<dataLen ; i+=8)
    {
        page = *(uint64_t*)(&data[i]);
        flashProgram64Bit(&addr, page);
    }
}


/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
/*------------------------------------------------------------------------- */
uint16_t tunerGetReflected(void)
{
    uint16_t iqTogetherReflected;
    int8_t i, q;

    iqTogetherReflected = st25RU3993GetReflectedPower();
    i = (iqTogetherReflected & 0xFF);
    q = ((iqTogetherReflected >> 8)&0xFF);    
    return ((i*i) + (q*q));
}

/*------------------------------------------------------------------------- */
void tunerSetCap(uint8_t component, uint8_t val)
{
    if(val > MAX_DAC_VALUE)
        return;
    
    if(component == TUNER_CIN){
        TUNER_SETCAP_CIN(val);
    }
    else if(component == TUNER_CLEN)
    {
        TUNER_SETCAP_CLEN(val);
    }
    else if(component == TUNER_COUT)
    {
        TUNER_SETCAP_COUT(val);
    }
}




/*------------------------------------------------------------------------- */
void tunerOneHillClimb(STUHFL_T_ST25RU3993_Caps *caps, uint16_t *tunedIQ, bool doFalsePositiveDetection, uint8_t maxSteps)
{
    bool improvement;
    uint16_t reflectedPower;
    uint8_t valCap;

    // set initial tuning
    tunerSetTuning(caps->cin, caps->clen, caps->cout);
    *tunedIQ = tunerGetReflected();

    do
    {
        improvement = false;

        // Stores parameters before processing with tunerClimbOneParam() as it modifies them even if not improved
        valCap = caps->cin;
        reflectedPower = *tunedIQ;
        if(tunerClimbOneParam(caps, doFalsePositiveDetection, TUNER_CIN,&valCap,&reflectedPower,&maxSteps))
        {
            // Update parameters ONLY if tunerClimbOneParam() improved them
            caps->cin = valCap;
            *tunedIQ = reflectedPower;
            improvement = true;
        }
        else
        {
            tunerSetCap(TUNER_CIN,caps->cin);
        }

        // Stores parameters before processing with tunerClimbOneParam() as it modifies them even if not improved
        valCap = caps->clen;
        reflectedPower = *tunedIQ;
        if(tunerClimbOneParam(caps, doFalsePositiveDetection, TUNER_CLEN,&valCap,&reflectedPower,&maxSteps))
        {
            // Update parameters ONLY if tunerClimbOneParam() improved them
            caps->clen = valCap;
            *tunedIQ = reflectedPower;
            improvement = true;
        }
        else
        {
            tunerSetCap(TUNER_CLEN,caps->clen);
        }

        // Stores parameters before processing with tunerClimbOneParam() as it modifies them even if not improved
        valCap = caps->cout;
        reflectedPower = *tunedIQ;
        if(tunerClimbOneParam(caps, doFalsePositiveDetection, TUNER_COUT,&valCap,&reflectedPower,&maxSteps))
        {
            // Update parameters ONLY if tunerClimbOneParam() improved them
            caps->cout = valCap;
            *tunedIQ = reflectedPower;
            improvement = true;
        }
        else
        {
            tunerSetCap(TUNER_COUT,caps->cout);
        }

        if(maxSteps == 1)
            break;
    } while(improvement);
}

/*------------------------------------------------------------------------- */
void tunerMultiHillClimb(STUHFL_T_ST25RU3993_Caps *caps, uint16_t *tunedIQ, bool doFalsePositiveDetection)
{
    uint8_t i;
#if defined(TUNER_CONFIG_CIN) || defined(TUNER_CONFIG_CLEN) || defined(TUNER_CONFIG_COUT)
    uint8_t j;
#endif
    uint16_t reflectedPower;
    uint8_t cin, cout, clen;

    // save initial values
    cin     = caps->cin;
    clen    = caps->clen;
    cout    = caps->cout;

    // set initial tuning
    tunerSetTuning(cin, clen, cout);
    
    // get initial reflected power
    *tunedIQ = tunerGetReflected();
    reflectedPower = *tunedIQ;

    // go through all permutations
    for(i=0 ; i < (START_POINTS*START_POINTS*START_POINTS) ; i++)  // START_POINTS ^ 3 steps (3 capacitors, START_POINTS starting values)
    {
#if defined(TUNER_CONFIG_CIN) || defined(TUNER_CONFIG_CLEN) || defined(TUNER_CONFIG_COUT)
        j=i;
#endif
#ifdef TUNER_CONFIG_CIN                                                             // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 == i
        caps->cin  = tuneStartPoints[j % START_POINTS];   // 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2
        j /= START_POINTS;
#endif
#ifdef TUNER_CONFIG_CLEN
        caps->clen = tuneStartPoints[j % START_POINTS];   // 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2
        j /= START_POINTS;
#endif
#ifdef TUNER_CONFIG_COUT
        caps->cout = tuneStartPoints[j % START_POINTS];   // 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2
        j /= START_POINTS;
#endif
        tunerOneHillClimb(caps, tunedIQ, doFalsePositiveDetection, 30);

        // tuning became better
        if(*tunedIQ < reflectedPower)
        {
            // save best found cin/clen/cout values
            cin     = caps->cin;
            clen    = caps->clen;
            cout    = caps->cout;
            reflectedPower = *tunedIQ;
        }
    }

    // Set and use best combination
    caps->cin  = cin; 
    caps->clen = clen; 
    caps->cout = cout; 
    tunerSetTuning(cin, clen, cout);

    *tunedIQ = tunerGetReflected();
    reflectedPower = *tunedIQ;
}

/*------------------------------------------------------------------------- */
void tunerEnhancedMultiHillClimb(STUHFL_T_ST25RU3993_Caps *caps, uint16_t *tunedIQ, bool doFalsePositiveDetection)
{
    uint8_t i;
#if defined(TUNER_CONFIG_CIN) || defined(TUNER_CONFIG_CLEN) || defined(TUNER_CONFIG_COUT)
    uint8_t j;
#endif
    uint16_t reflectedPower;
    uint8_t cin, cout, clen;
    tuning_data_t   hillClimbSummits[MAX_MULTI_HILLCLIMB_SUMMITS];

    // save initial values
    cin     = caps->cin;
    clen    = caps->clen;
    cout    = caps->cout;

    // set initial tuning
    tunerSetTuning(cin, clen, cout);
    
    // get initial reflected power
    *tunedIQ = tunerGetReflected();
    reflectedPower = *tunedIQ;

    memset((uint8_t *)hillClimbSummits, 0xFF, sizeof(tuning_data_t)*MAX_MULTI_HILLCLIMB_SUMMITS);
    
    // go through all permutations
    for(i=0 ; i < (START_POINTS_ENHANCED*START_POINTS_ENHANCED*START_POINTS_ENHANCED) ; i++)  // START_POINTS_ENHANCED ^ 3 steps (3 capacitors, START_POINTS_ENHANCED starting values)
    {
#if defined(TUNER_CONFIG_CIN) || defined(TUNER_CONFIG_CLEN) || defined(TUNER_CONFIG_COUT)
        j=i;
#endif
#ifdef TUNER_CONFIG_CIN                                                             // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 == i
        caps->cin  = tuneStartPoints_Enhanced[j % START_POINTS_ENHANCED];   // 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2
        j /= START_POINTS_ENHANCED;
#endif
#ifdef TUNER_CONFIG_CLEN
        caps->clen = tuneStartPoints_Enhanced[j % START_POINTS_ENHANCED];   // 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2
        j /= START_POINTS_ENHANCED;
#endif
#ifdef TUNER_CONFIG_COUT
        caps->cout = tuneStartPoints_Enhanced[j % START_POINTS_ENHANCED];   // 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2
        j /= START_POINTS_ENHANCED;
#endif
        // set tuning and get reflected power
        tunerSetTuning(caps->cin, caps->clen, caps->cout);
        *tunedIQ = tunerGetReflected();

        
        uint8_t k=0;
        uint8_t shiftDone = 0;
        while ((k<MAX_MULTI_HILLCLIMB_SUMMITS) && (! shiftDone)) {
            if (*tunedIQ < hillClimbSummits[k].reflectedPower) {
                // shift in array all summits below reflected
                if (k < (MAX_MULTI_HILLCLIMB_SUMMITS-1)) {
                    for (uint8_t m=0 ; m < (MAX_MULTI_HILLCLIMB_SUMMITS-1) - k ; m++) {
                        memcpy((uint8_t *)&hillClimbSummits[(MAX_MULTI_HILLCLIMB_SUMMITS-1)-m], (uint8_t *)&hillClimbSummits[(MAX_MULTI_HILLCLIMB_SUMMITS-1)-(m+1)], sizeof(tuning_data_t));
                    }
                }

                // store current summit
                hillClimbSummits[k].reflectedPower = *tunedIQ;
                hillClimbSummits[k].cin = caps->cin;
                hillClimbSummits[k].cout = caps->cout;
                hillClimbSummits[k].clen = caps->clen;
                shiftDone = 1;
            }
            k++;
        }
    }

    // restore initial values and get reflected power
    caps->cin  = cin; 
    caps->clen = clen; 
    caps->cout = cout; 
    tunerSetTuning(cin, clen, cout);
    *tunedIQ = tunerGetReflected();
    reflectedPower = *tunedIQ;
    

    // Run Hill Climb with the best combinations of cin/clen/cout
    for(i=0 ; i<MAX_MULTI_HILLCLIMB_SUMMITS ; i++)
    {
        // Set tuning values to current tested combination
        caps->cin  = hillClimbSummits[i].cin; 
        caps->clen = hillClimbSummits[i].clen; 
        caps->cout = hillClimbSummits[i].cout; 
        tunerOneHillClimb(caps, tunedIQ, doFalsePositiveDetection, 30);

        // tuning became better
        if(*tunedIQ < reflectedPower)
        {
            // save best found cin/clen/cout values
            cin     = caps->cin;
            clen    = caps->clen;
            cout    = caps->cout;
            reflectedPower = *tunedIQ;
        }
    }

    // Set and use best combination
    caps->cin  = cin; 
    caps->clen = clen; 
    caps->cout = cout; 
    tunerSetTuning(cin, clen, cout);
    *tunedIQ = tunerGetReflected();
}




/*------------------------------------------------------------------------- */
void tunerSetTuning(uint8_t cin, uint8_t clen, uint8_t cout)
{
    static uint8_t cinOld, clenOld, coutOld;
    
    if(cinOld == cin && clenOld == clen && coutOld == cout) {
        return;
    }
    cinOld = cin;
    clenOld = clen;
    coutOld = cout;
    
    tunerSetCaps(cin, clen, cout);
}

/*------------------------------------------------------------------------- */
void loadChannelListFromFlash(uint8_t antenna, STUHFL_T_ST25RU3993_ChannelList *channelList)
{
    channelList->antenna = antenna;
    channelList->persistent = true;
    channelList->nFrequencies = 0;
    channelList->currentChannelListIdx = 0;

    if (antenna >= FW_NB_ANTENNA) {
        return;
    }

    uint32_t addr = FLASH_STORE_START_TUNING_TABLE_NEWTUNING + antenna*FLASH_STORE_NEWTUNING_ANTENNA_BLOCK_SIZE;
    loadFromFlash64Bit(addr, sizeof(STUHFL_T_ST25RU3993_ChannelList), (uint8_t *)channelList);
}

/*------------------------------------------------------------------------- */
void saveChannelListToFlash(uint8_t antenna, STUHFL_T_ST25RU3993_ChannelList *channelList)
{
    uint32_t addr;
    
    if (antenna >= FW_NB_ANTENNA) {
        return;
    }

#if FW_NB_ANTENNA > 1
    STUHFL_T_ST25RU3993_ChannelList otherChannelLists[FW_NB_ANTENNA-1];
    uint8_t                         otherChannelListsIdx[FW_NB_ANTENNA-1];
    for (uint32_t i=0,j=0 ; i<FW_NB_ANTENNA ; i++) {
        if (i != antenna) {
            // Save all other antennas data content
            loadChannelListFromFlash(i, &otherChannelLists[j]);
            otherChannelListsIdx[j] = i;
            j++;
        }
    }
#endif

    // before writing new data all needs to be erased 
    // flash write can only clear 1 bit (1->0), 
    // if 1 bit should be set (0->1) it needs to be erased
    clearFlashPage(PROFILE_NEWTUNING);

    addr = FLASH_STORE_START_TUNING_TABLE_NEWTUNING + antenna*FLASH_STORE_NEWTUNING_ANTENNA_BLOCK_SIZE;
    saveToFlash64Bit(addr, sizeof(STUHFL_T_ST25RU3993_ChannelList), (uint8_t *)channelList);

#if FW_NB_ANTENNA > 1
    for (uint32_t i=0 ; i<FW_NB_ANTENNA-1 ; i++) {
        // Restore all other antennas data content
        addr = FLASH_STORE_START_TUNING_TABLE_NEWTUNING + otherChannelListsIdx[i]*FLASH_STORE_NEWTUNING_ANTENNA_BLOCK_SIZE;
        saveToFlash64Bit(addr, sizeof(STUHFL_T_ST25RU3993_ChannelList), (uint8_t *)&otherChannelLists[i]);
    }
#endif
}

/*------------------------------------------------------------------------- */
void loadTuningTablesFromFlashAll(void)
{
    loadTuningTablesFromFlash(PROFILE_EUROPE);
    loadTuningTablesFromFlash(PROFILE_USA);
    loadTuningTablesFromFlash(PROFILE_JAPAN);
    loadTuningTablesFromFlash(PROFILE_CHINA);
    loadTuningTablesFromFlash(PROFILE_CHINA2);
    loadTuningTablesFromFlash(PROFILE_CUSTOM);
}

/*------------------------------------------------------------------------- */
void loadTuningTablesFromFlash(uint8_t p)
{
    if (p == PROFILE_NEWTUNING) {
        // PROFILE_NEWTUNING is ignored and not processed
        return;
    }

    uint32_t addr = getFlashStoreAddr(p);
    if (addr != 0) {
        bool loadIsOk = loadFromFlash64Bit(addr, sizeof(tuningTable_t), (uint8_t *)&tuningTable[p]);
        if ((!loadIsOk) || (tuningTable[p].tableSize == 0)) {
            setDefaultTuningTable(p, &tuningTable[p], RESET_DEFAULT_ALL_FREQS);
        }
    }
}

/*------------------------------------------------------------------------- */
void saveTuningTableToFlash(uint8_t p)
{
    if (p == PROFILE_NEWTUNING) {
        // PROFILE_NEWTUNING is ignored and not processed
        return;
    }

    uint32_t addr = getFlashStoreAddr(p);
    if (addr != 0) {
        // before writing new data all needs to be erased 
        // flash write can only clear 1 bit (1->0), 
        // if 1 bit should be set (0->1) it needs to be erased
        clearFlashPage(p);

        saveToFlash64Bit(addr, sizeof(tuningTable_t), (uint8_t *)&tuningTable[p]);
    }
}

/*------------------------------------------------------------------------- */
void clearFlashPage(uint8_t p)
{
    // clear flash (limitation of flash technology, only transitions to 0 possible)
    uint32_t addr = getFlashStoreAddr(p);
    if (addr != 0) {
        flashErasePage(addr, 1);
    }
}

/*------------------------------------------------------------------------- */
void setDefaultTuningTable(uint8_t profileNew, tuningTable_t *table, uint32_t freq)
{
    uint8_t i;
    uint8_t iTarget=0;
    
    if (profileNew >= NUM_SAVED_PROFILES) {
        // PROFILE_NEWTUNING is ignored and not processed
        return;
    }    
    
    if(freq != RESET_DEFAULT_ALL_FREQS)
    {
        // find best matching entry in currently saved tuning table to the selected frequency
        uint32_t diff = 999000;
        for(i=0;(i<table->tableSize) && (diff>0);i++)
        {
            uint32_t diffA = (table->freq[i] > freq) ? table->freq[i]-freq : freq-table->freq[i];
            if(diffA<diff)
            {
                diff = diffA;
                iTarget = i;
            }
        }
        // set frequency to the nearest one in the tuning table
        freq = table->freq[iTarget];
    }

    if(profileNew == PROFILE_EUROPE)
    {
#ifdef ANTENNA_SWITCH
#if ELANCE
        tuningTable_t t = {4,0,
        /* freq */
        {865700,866300,866900,867500},
        /* cin */
        {
        {63,63,63,63},
        {63,63,63,63},
        {63,63,63,63},
        {63,63,63,63}
        },
        /* clen */
        {
        {63,63,63,63},
        {63,63,63,63},
        {63,63,63,63},
        {63,63,63,63}
        },
        /* cout */
        {
        {63,63,63,63},
        {63,63,63,63},
        {63,63,63,63},
        {63,63,63,63}
        },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#else
        tuningTable_t t = {4,0,
        /* freq */
        {865700,866300,866900,867500},
        /* cin */
        {
        {12,12,11,11},
        {12,11,11,11}
        },
        /* clen */
        {
        {12,12,12,12},
        {9,9,9,9}
        },
        /* cout */
        {
        {14,14,14,14},
        {16,16,16,16}
        },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#endif
#else
        tuningTable_t t = {4,0,
        /* freq */
        {865700,866300,866900,867500},
        /* cin */
        {
        {15,15,15,15}
        },
        /* clen */
        {
        {15,15,15,15}
        },
        /* cout */
        {
        {15,15,15,15}
        },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#endif
        replaceTuningTable(table, freq, iTarget, &t);
    }
    else if(profileNew == PROFILE_USA)
    {
#ifdef ANTENNA_SWITCH
#if ELANCE
        tuningTable_t t = {50,0,
        /* freq */
        {902750,903250,903750,904250,904750,905250,905750,906250,906750,907250,907750,908250,908750,909250,909750,910250,910750,911250,911750,912250,912750,913250,913750,914250,914750,915250,915750,916250,916750,917250,917750,918250,918750,919250,919750,920250,920750,921250,921750,922250,922750,923250,923750,924250,924750,925250,925750,926250,926750,927250},
        /* cin */
        {
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63}
         },
        /* clen */
        {
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63}
         },
        /* cout */
        {
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63}
         },
        /* tunedIQ */
        {
        {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
        {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
        {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
        {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}
        }};
#else
        tuningTable_t t = {50,0,
        /* freq */
        {902750,903250,903750,904250,904750,905250,905750,906250,906750,907250,907750,908250,908750,909250,909750,910250,910750,911250,911750,912250,912750,913250,913750,914250,914750,915250,915750,916250,916750,917250,917750,918250,918750,919250,919750,920250,920750,921250,921750,922250,922750,923250,923750,924250,924750,925250,925750,926250,926750,927250},
        /* cin */
        {
        {13,13,13,13,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,12,12,12,12,12,12,12,10,10,10,10,10,10,10,10,8,8,8},
        {9,9,9,9,9,9,10,10,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9                  }
         },
        /* clen */
        {
        {24,24,24,24,8,8,19,8,8,8,8,8,23,23,23,23,23,23,23,23,23,23,8,8,8,8,8,8,8,8,10,24,24,24,24,24,24,24,24,16,16,16,16,16,16,16,16,6,6,6			   },
        {19,19,19,19,19,19,25,25,21,21,21,21,21,21,22,22,22,22,22,22,8,8,8,25,25,25,25,14,14,14,14,27,27,29,29,29,29,29,29,29,30,30,30,30,30,30,30,30,30,30}
         },
        /* cout */
        {
        {12,12,12,12,12,12,14,12,12,12,12,12,14,14,14,14,14,14,14,14,14,14,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12},
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,13,13,13,15,15,15,15,14,14,14,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
         },
        /* tunedIQ */
        {
        {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
        {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}
        }};
#endif
#else
        tuningTable_t t = {50,0,
        /* freq */
        {902750,903250,903750,904250,904750,905250,905750,906250,906750,907250,907750,908250,908750,909250,909750,910250,910750,911250,911750,912250,912750,913250,913750,914250,914750,915250,915750,916250,916750,917250,917750,918250,918750,919250,919750,920250,920750,921250,921750,922250,922750,923250,923750,924250,924750,925250,925750,926250,926750,927250},
        /* cin */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
         },
        /* clen */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
         },
        /* cout */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
         },
        /* tunedIQ */
        {
        {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}
        }};
#endif        
        replaceTuningTable(table, freq, iTarget, &t);
    }
    else if(profileNew == PROFILE_JAPAN)
    {
#ifdef ANTENNA_SWITCH
#if ELANCE
        tuningTable_t t = {9,0,
        /* freq */
        {920500,920700,920900,921100,921300,921500,921700,921900,922100},
        /* cin */
        {
        {63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63}
        },
        /* clen */
        {
        {63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63}
        },
        /* cout */
        {
        {63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63}
        },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#else
        tuningTable_t t = {9,0,
        /* freq */
        {920500,920700,920900,921100,921300,921500,921700,921900,922100},
        /* cin */
        {
        {10,10,10,10,10,10,10,10,10},
        {9,9,9,9,9,9,9,9,9         }
        },
        /* clen */
        {
        {16,16,16,16,16,16,16,16,16},
        {29,29,29,29,29,29,29,29,30}
        },
        /* cout */
        {
        {12,12,12,12,12,12,12,12,12},
        {15,15,15,15,15,15,15,15,15}
        },

        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#endif        
#else
        tuningTable_t t = {9,0,
        /* freq */
        {920500,920700,920900,921100,921300,921500,921700,921900,922100},
        /* cin */
        {
        {15,15,15,15,15,15,15,15,15}
        },
        /* clen */
        {
        {15,15,15,15,15,15,15,15,15}
        },
        /* cout */
        {
        {15,15,15,15,15,15,15,15,15}
        },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#endif
        replaceTuningTable(table, freq, iTarget, &t);
    }
    else if(profileNew == PROFILE_CHINA)
    {
#ifdef ANTENNA_SWITCH
#if ELANCE
        tuningTable_t t = {16,0,
        /* freq */
        {840625,840875,841125,841375,841625,841875,842125,842375,842625,842875,843125,843375,843625,843875,844125,844375},
        /* cin */
        {
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63}
        },
        /* clen */
        {
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63}
        },
        /* cout */
        {
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63}
        },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#else
        tuningTable_t t = {16,0,
        /* freq */
        {840625,840875,841125,841375,841625,841875,842125,842375,842625,842875,843125,843375,843625,843875,844125,844375},
        /* cin */
        {
        {15,15,15,15,15,15,13,13,13,13,15,15,15,15,15,15},
        {13,13,13,13,13,13,13,13,16,16,16,16,16,16,16,16}
        },
        /* clen */
        {
        {21,21,21,21,21,21,14,14,14,14,23,23,23,23,23,23},
        {15,15,15,15,15,15,15,15,20,20,20,20,20,20,20,20}
        },
        /* cout */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
        {18,18,18,18,18,18,18,18,17,17,17,17,17,17,17,17}
        },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#endif
#else
        tuningTable_t t = {16,0,
        /* freq */
        {840625,840875,841125,841375,841625,841875,842125,842375,842625,842875,843125,843375,843625,843875,844125,844375},
        /* cin */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
        },
        /* clen */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
        },
        /* cout */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
        },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#endif
        replaceTuningTable(table, freq, iTarget, &t);
    }
    else if(profileNew == PROFILE_CHINA2)
    {
#ifdef ANTENNA_SWITCH
#if ELANCE
        tuningTable_t t = {16,0,
        /* freq */
        {920625,920875,921125,921375,921625,921875,922125,922375,922625,922875,923125,923375,923625,923875,924125,924375},
        /* cin */
        {
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},         
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},         
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},         
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63}         
        },
        /* clen */
        {
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},         
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},         
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},         
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63}        
        },
        /* cout */
        {
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},         
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},         
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63},         
        {63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63}         
        },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#else
        tuningTable_t t = {16,0,
        /* freq */
        {920625,920875,921125,921375,921625,921875,922125,922375,922625,922875,923125,923375,923625,923875,924125,924375},
        /* cin */
        {
        {8,8,8,8,8,8,8,8,7,7,7,7,7,7,7,7},
        {9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9}
         },
        /* clen */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
        {30,30,29,29,30,30,30,30,30,30,30,30,30,30,30,30}
         },
        /* cout */
        {
        {13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13},
        {16,16,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
         },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#endif
#else
        tuningTable_t t = {16,0,
        /* freq */
        {920625,920875,921125,921375,921625,921875,922125,922375,922625,922875,923125,923375,923625,923875,924125,924375},
        /* cin */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
        },
        /* clen */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}         
        },
        /* cout */
        {
        {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}         
        },
        /* tunedIQ */
        {
        {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}
        }};
#endif
        replaceTuningTable(table, freq, iTarget, &t);
    }
    else
    {
        table->tableSize = MAX_FREQUENCY;
        memset((uint8_t *)table->cin, 15, sizeof(uint8_t)*FW_NB_ANTENNA*MAX_FREQUENCY);
        memset((uint8_t *)table->clen, 15, sizeof(uint8_t)*FW_NB_ANTENNA*MAX_FREQUENCY);
        memset((uint8_t *)table->cout, 15, sizeof(uint8_t)*FW_NB_ANTENNA*MAX_FREQUENCY);
        memset((uint8_t *)table->tunedIQ, 0xFF, sizeof(uint16_t)*FW_NB_ANTENNA*MAX_FREQUENCY);  // tunedIQ is a uint16_t array
    }
}

/*------------------------------------------------------------------------- */
void initializeFrequencies(void)
{
    frequencies[PROFILE_CUSTOM].numFreqs = 0;
    frequencies[PROFILE_EUROPE].numFreqs = 4;
    frequencies[PROFILE_USA].numFreqs = 50;
    frequencies[PROFILE_JAPAN].numFreqs = 9;
    frequencies[PROFILE_CHINA].numFreqs = 16;
    frequencies[PROFILE_CHINA2].numFreqs = 16;

    frequencies[PROFILE_EUROPE].freq[0] = 866900;
    frequencies[PROFILE_EUROPE].freq[1] = 865700;
    frequencies[PROFILE_EUROPE].freq[2] = 866300;
    frequencies[PROFILE_EUROPE].freq[3] = 867500;
        
    frequencies[PROFILE_USA].freq[0]  = 902750;//923250;
    frequencies[PROFILE_USA].freq[1]  = 915250;//907250;
    frequencies[PROFILE_USA].freq[2]  = 903250;//915750;
    frequencies[PROFILE_USA].freq[3]  = 915750;//903750;
    frequencies[PROFILE_USA].freq[4]  = 903750;//903250;
    frequencies[PROFILE_USA].freq[5]  = 916250;//925250;
    frequencies[PROFILE_USA].freq[6]  = 904250;//909750;
    frequencies[PROFILE_USA].freq[7]  = 916750;//926250;
    frequencies[PROFILE_USA].freq[8]  = 904750;//924250;
    frequencies[PROFILE_USA].freq[9]  = 917250;//910750;
    frequencies[PROFILE_USA].freq[10] = 905250;//904750;
    frequencies[PROFILE_USA].freq[11] = 917750;//908250;
    frequencies[PROFILE_USA].freq[12] = 905750;//917250;
    frequencies[PROFILE_USA].freq[13] = 918250;//914250;
    frequencies[PROFILE_USA].freq[14] = 906250;//916750;
    frequencies[PROFILE_USA].freq[15] = 918750;//910250;
    frequencies[PROFILE_USA].freq[16] = 906750;//908750;
    frequencies[PROFILE_USA].freq[17] = 919250;//914750;
    frequencies[PROFILE_USA].freq[18] = 907250;//904250;
    frequencies[PROFILE_USA].freq[19] = 919750;//905250;
    frequencies[PROFILE_USA].freq[20] = 907750;//922750;
    frequencies[PROFILE_USA].freq[21] = 920250;//911750;
    frequencies[PROFILE_USA].freq[22] = 908250;//911250;
    frequencies[PROFILE_USA].freq[23] = 920750;//912750;
    frequencies[PROFILE_USA].freq[24] = 908750;//924750;
    frequencies[PROFILE_USA].freq[25] = 921250;//913250;
    frequencies[PROFILE_USA].freq[26] = 909250;//923750;
    frequencies[PROFILE_USA].freq[27] = 921750;//906750;
    frequencies[PROFILE_USA].freq[28] = 909750;//920750;
    frequencies[PROFILE_USA].freq[29] = 922250;//922250;
    frequencies[PROFILE_USA].freq[30] = 910250;//927250;
    frequencies[PROFILE_USA].freq[31] = 922750;//912250;
    frequencies[PROFILE_USA].freq[32] = 910750;//907750;
    frequencies[PROFILE_USA].freq[33] = 923250;//918750;
    frequencies[PROFILE_USA].freq[34] = 911250;//905750;
    frequencies[PROFILE_USA].freq[35] = 923750;//913750;
    frequencies[PROFILE_USA].freq[36] = 911750;//919250;
    frequencies[PROFILE_USA].freq[37] = 924250;//916250;
    frequencies[PROFILE_USA].freq[38] = 912250;//906250;
    frequencies[PROFILE_USA].freq[39] = 924750;//919750;
    frequencies[PROFILE_USA].freq[40] = 912750;//921750;
    frequencies[PROFILE_USA].freq[41] = 925250;//909250;
    frequencies[PROFILE_USA].freq[42] = 913250;//915250;
    frequencies[PROFILE_USA].freq[43] = 925750;//920250;
    frequencies[PROFILE_USA].freq[44] = 913750;//921250;
    frequencies[PROFILE_USA].freq[45] = 926250;//902750;
    frequencies[PROFILE_USA].freq[46] = 914250;//925750;
    frequencies[PROFILE_USA].freq[47] = 926750;//917750;
    frequencies[PROFILE_USA].freq[48] = 914750;//926750;
    frequencies[PROFILE_USA].freq[49] = 927250;//918250;
    
    frequencies[PROFILE_JAPAN].freq[0] = 920500;
    frequencies[PROFILE_JAPAN].freq[1] = 920700;
    frequencies[PROFILE_JAPAN].freq[2] = 920900;
    frequencies[PROFILE_JAPAN].freq[3] = 921100;
    frequencies[PROFILE_JAPAN].freq[4] = 921300;
    frequencies[PROFILE_JAPAN].freq[5] = 921500;
    frequencies[PROFILE_JAPAN].freq[6] = 921700;
    frequencies[PROFILE_JAPAN].freq[7] = 921900;
    frequencies[PROFILE_JAPAN].freq[8] = 922100;
    

    frequencies[PROFILE_CHINA].freq[0] = 840625;
    frequencies[PROFILE_CHINA].freq[1] = 840875;
    frequencies[PROFILE_CHINA].freq[2] = 841125;
    frequencies[PROFILE_CHINA].freq[3] = 841375;
    frequencies[PROFILE_CHINA].freq[4] = 841625;
    frequencies[PROFILE_CHINA].freq[5] = 841875;
    frequencies[PROFILE_CHINA].freq[6] = 842125;
    frequencies[PROFILE_CHINA].freq[7] = 842375;
    frequencies[PROFILE_CHINA].freq[8] = 842625;
    frequencies[PROFILE_CHINA].freq[9] = 842875;
    frequencies[PROFILE_CHINA].freq[10] = 843125;
    frequencies[PROFILE_CHINA].freq[11] = 843375;
    frequencies[PROFILE_CHINA].freq[12] = 843625;
    frequencies[PROFILE_CHINA].freq[13] = 843875;
    frequencies[PROFILE_CHINA].freq[14] = 844125;
    frequencies[PROFILE_CHINA].freq[15] = 844375;

    frequencies[PROFILE_CHINA2].freq[0] = 920625;
    frequencies[PROFILE_CHINA2].freq[1] = 920875;
    frequencies[PROFILE_CHINA2].freq[2] = 921125;
    frequencies[PROFILE_CHINA2].freq[3] = 921375;
    frequencies[PROFILE_CHINA2].freq[4] = 921625;
    frequencies[PROFILE_CHINA2].freq[5] = 921875;
    frequencies[PROFILE_CHINA2].freq[6] = 922125;
    frequencies[PROFILE_CHINA2].freq[7] = 922375;
    frequencies[PROFILE_CHINA2].freq[8] = 922625;
    frequencies[PROFILE_CHINA2].freq[9] = 922875;
    frequencies[PROFILE_CHINA2].freq[10] = 923125;
    frequencies[PROFILE_CHINA2].freq[11] = 923375;
    frequencies[PROFILE_CHINA2].freq[12] = 923625;
    frequencies[PROFILE_CHINA2].freq[13] = 923875;
    frequencies[PROFILE_CHINA2].freq[14] = 924125;
    frequencies[PROFILE_CHINA2].freq[15] = 924375;
}

/*------------------------------------------------------------------------- */
void randomizeFrequencies(uint8_t p)
{
    freq_t frequenciesTemp;
    uint8_t i;

    if (p >= NUM_SAVED_PROFILES) {  // No randomization for NEWTUNING
        return;
    }    

    if (frequencies[p].numFreqs > MAX_FREQUENCY) {
        // Something went wrong with frequencies, reinitialize all
        initializeFrequencies();
    }

    bool alreadyUsed[MAX_FREQUENCY];
    for(i=0 ; i<frequencies[p].numFreqs ; i++)
    {
        frequenciesTemp.freq[i] = frequencies[p].freq[i];
        alreadyUsed[i] = false;
    }

    for(i=0 ; i<frequencies[p].numFreqs ; i++)
    {
        uint8_t index;
        do
        {
            index = rand()%(frequencies[p].numFreqs);
            index = (index >= frequencies[p].numFreqs ? 0 : index); 
        } while(alreadyUsed[index]);
        alreadyUsed[index] = true;
        
        frequencies[p].freq[i] = frequenciesTemp.freq[index];
    }
}

/**
  * @}
  */
/**
  * @}
  */
