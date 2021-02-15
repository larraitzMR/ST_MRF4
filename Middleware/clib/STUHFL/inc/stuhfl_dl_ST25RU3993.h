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
#if !defined __STUHFL_DL_ST25RU3993_EVAL_H
#define __STUHFL_DL_ST25RU3993_EVAL_H

#include "stuhfl.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#pragma pack(push, 1)

/** parameters for command TUNER_TABLE_DEFAULT */
#define RESET_DEFAULT_ALL_FREQS 0

/** define to identify Cin cap of tuner. */
#define TUNER_CIN           0x01
/** define to identify Clen cap of tuner. */
#define TUNER_CLEN          0x02
/** define to identify Cout cap of tuner. */
#define TUNER_COUT          0x04

/** Definition of the frequencies maximum value */
#define FREQUENCY_MAX_VALUE 999000U
#define DEFAULT_FREQUENCY   865700U

/* Set/Get */
typedef struct {
    uint8_t                             addr;   /**< I Param: Register address */
    uint8_t                             data;   /**< I/O Param: Register data */
} STUHFL_T_ST25RU3993_Register;
#define STUHFL_O_ST25RU3993_REGISTER_INIT(...) ((STUHFL_T_ST25RU3993_Register) { .addr = 0, .data = 0, ##__VA_ARGS__ })

/* Set/Get */
#define POWER_DOWN              0       /** value for #readerPowerDownMode. Activates power down mode. (EN low)*/
#define POWER_NORMAL            1       /** value for #readerPowerDownMode. Activates normal mode. (EN high, RF off, stdby 0)*/
#define POWER_NORMAL_RF         2       /** value for #readerPowerDownMode. Activates normal mode with rf field on. (EN high, RF off, stdby 0)*/
#define POWER_STANDBY           3       /** value for #readerPowerDownMode. Activates standby mode. (EN high, RF off, stdby 1)*/

#define STUHFL_RWD_CFG_ID_PWR_DOWN_MODE     0x00    /**< Reader configuration ID for Power down mode */
#define STUHFL_RWD_CFG_ID_EXTVCO            0x01    /**< Reader configuration ID for external voltage controlled oscilator */
#define STUHFL_RWD_CFG_ID_PA                0x02    /**< Reader configuration ID for power amplifier */
#define STUHFL_RWD_CFG_ID_INP               0x03    /**< Reader configuration ID for input */
#define STUHFL_RWD_CFG_ID_ANTENNA_SWITCH    0x04    /**< Reader configuration ID for antenna switch */
#define STUHFL_RWD_CFG_ID_TUNER             0x05    /**< Reader configuration ID for tuner */
#define STUHFL_RWD_CFG_ID_HARDWARE_ID_NUM   0x06    /**< Reader configuration ID for hardware ID */
typedef struct {
    uint8_t                             id;     /**< I Param: Reader configuration ID */
    uint8_t                             value;  /**< I/O Param: Value of corresponding configuration */
} STUHFL_T_ST25RU3993_RwdConfig;
#define STUHFL_O_ST25RU3993_RWDCONFIG_INIT(...) ((STUHFL_T_ST25RU3993_RwdConfig) { .id = STUHFL_RWD_CFG_ID_PWR_DOWN_MODE, .value = POWER_NORMAL, ##__VA_ARGS__ })

/* Set */
#define ANTENNA_POWER_MODE_ON               ((uint8_t)0x00)
#define ANTENNA_POWER_MODE_OFF              ((uint8_t)0xFF)
typedef struct {
    uint8_t                             mode;       /**< I Param: Antenna Power mode */
    uint16_t                            timeout;    /**< I Param: Timeout before settings will be applied */
    uint32_t                            frequency;  /**< I Param: Frequency to be used */
} STUHFL_T_ST25RU3993_Antenna_Power;
#define STUHFL_O_ST25RU3993_ANTENNA_POWER_INIT(...) ((STUHFL_T_ST25RU3993_Antenna_Power) { .mode = ANTENNA_POWER_MODE_OFF, .timeout = 0, .frequency = DEFAULT_FREQUENCY, ##__VA_ARGS__ })

/* Get */
typedef struct {
    uint32_t                            frequency;  /**< I Param: Frequency for RSSI */
    uint8_t                             rssiLogI;   /**< O param: I parameter of logarithmic scaled RSSI */
    uint8_t                             rssiLogQ;   /**< O param: Q parameter of logarithmic scaled RSSI */
} STUHFL_T_ST25RU3993_Freq_Rssi;
#define STUHFL_O_ST25RU3993_FREQ_RSSI_INIT(...) ((STUHFL_T_ST25RU3993_Freq_Rssi) { .frequency = DEFAULT_FREQUENCY, .rssiLogI = 0, .rssiLogQ = 0, ##__VA_ARGS__ })

/* Get */
typedef struct {
    uint32_t                            frequency;              /**< I Param: Frequency to be used for the measurement */
    bool                                applyTunerSetting;      /**< I/O Param: flag to apply tuner settings for frequency */
    int8_t                              reflectedI;             /**< O Param: Reflected I */
    int8_t                              reflectedQ;             /**< O Param: Reflected Q */
} STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info;
#define STUHFL_O_ST25RU3993_FREQ_REFLECTEDPOWER_INFO_INIT(...) ((STUHFL_T_ST25RU3993_Freq_ReflectedPower_Info) { .frequency = DEFAULT_FREQUENCY, .applyTunerSetting = false, .reflectedI = 0, .reflectedQ = 0, ##__VA_ARGS__ })

/* Set */
#define PROFILE_CUSTOM              0
#define PROFILE_EUROPE              1
#define PROFILE_USA                 2
#define PROFILE_JAPAN               3
#define PROFILE_CHINA               4
#define PROFILE_CHINA2              5
#define NUM_SAVED_PROFILES          6

#define PROFILE_NEWTUNING           0xFF

typedef struct STUHFL_DEPRECATED STUHFL_S_ST25RU3993_Freq_Profile {
    uint8_t                             profile;    /**< I Param: Frequency profile */
} STUHFL_T_ST25RU3993_Freq_Profile;
#define STUHFL_O_ST25RU3993_FREQ_PROFILE_INIT(...) ((STUHFL_T_ST25RU3993_Freq_Profile) { .profile = PROFILE_EUROPE, ##__VA_ARGS__ })

typedef struct STUHFL_DEPRECATED STUHFL_S_ST25RU3993_Freq_Profile_Add2Custom {
    bool                                clearList;  /**< I Param: Flag to clear all tuning entries */
    uint32_t                            frequency;  /**< I Param: Frequency to append to custom profile */
} STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom;
#define STUHFL_O_ST25RU3993_FREQ_PROFILE_ADD2CUSTOM_INIT(...) ((STUHFL_T_ST25RU3993_Freq_Profile_Add2Custom) { .clearList = true, .frequency = DEFAULT_FREQUENCY, ##__VA_ARGS__ })

/* Get */
typedef struct STUHFL_DEPRECATED STUHFL_S_ST25RU3993_Freq_Profile_Info {
    uint8_t                             profile;        /**< O Param: Current used profile index */
    uint32_t                            minFrequency;   /**< O Param: Minimum frequency for profile */
    uint32_t                            maxFrequency;   /**< O Param: Maximum frequency for profile */
    uint8_t                             numFrequencies; /**< O Param: Number of frequency entries for profile */
} STUHFL_T_ST25RU3993_Freq_Profile_Info;
#define STUHFL_O_ST25RU3993_FREQ_PROFILE_INFO_INIT(...) ((STUHFL_T_ST25RU3993_Freq_Profile_Info) { .profile = PROFILE_EUROPE, .minFrequency = 865700, .maxFrequency = 867500, .numFrequencies = 4, ##__VA_ARGS__ })

#define MAX_ANTENNA                     4U
#define MAX_FREQUENCY                   53U
/* Set/Get */
typedef struct {
    uint8_t cin ;           /**< I/O Param: IN capacitance of tuning PI network */
    uint8_t clen;           /**< I/O Param: LEN capacitance of tuning PI network */
    uint8_t cout;           /**< I/O Param: OUT capacitance of tuning PI network */
} STUHFL_T_ST25RU3993_Caps;
#define STUHFL_O_ST25RU3993_CAPS_INIT(...) ((STUHFL_T_ST25RU3993_Caps) { .cin = 0, .clen = 0, .cout = 0, ##__VA_ARGS__ })

/* Set/Get */
typedef struct {
    uint32_t                            frequency;                  /**< I/O Param: List of frequencies to be used for hopping */
    STUHFL_T_ST25RU3993_Caps            caps;                       /**< I/O Param: Tuning capacitors for each frequncies */
    uint8_t                             rfu1;                       /** rfu */
    uint8_t                             rfu2;                       /** rfu */
} STUHFL_T_ST25RU3993_ChannelItem;
#define STUHFL_O_ST25RU3993_CHANNELITEM_INIT(...) ((STUHFL_T_ST25RU3993_ChannelItem) { .frequency = DEFAULT_FREQUENCY, .caps = STUHFL_O_ST25RU3993_CAPS_INIT(), .rfu1 = 0, .rfu2 = 0, ##__VA_ARGS__ })

typedef struct {
    uint8_t                             antenna;                    /**< I Param: Antenna to which the ChannelList belongs */
    bool                                persistent;                 /**< I Param: Set/Get channel list to/from persistent flash memory */
    uint8_t                             nFrequencies;               /**< I/O Param: Number valid frequencies in the frequency data array*/
    uint8_t                             currentChannelListIdx;      /**< I/O Param: Current item index in channel list */
    STUHFL_T_ST25RU3993_ChannelItem     item[MAX_FREQUENCY];        /**< I/O Param: Tuning capacitors for each frequncies */
} STUHFL_T_ST25RU3993_ChannelList;
#define STUHFL_O_ST25RU3993_CHANNELLIST_INIT(...) ((STUHFL_T_ST25RU3993_ChannelList) { .antenna = ANTENNA_1, .persistent = false, .nFrequencies = 1, .currentChannelListIdx = 0,    \
                                                    .item = {       \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT()  \
                                                    }, ##__VA_ARGS__ })

#define STUHFL_O_ST25RU3993_CHANNELLIST_EUROPE_INIT(...) ((STUHFL_T_ST25RU3993_ChannelList) { .antenna = ANTENNA_1, .persistent = false, .nFrequencies = 4, .currentChannelListIdx = 0,    \
                                                    .item = {       \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=866900), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=865700), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=866300), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=867500), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT()  \
                                                    }, ##__VA_ARGS__ })

#define STUHFL_O_ST25RU3993_CHANNELLIST_USA_INIT(...) ((STUHFL_T_ST25RU3993_ChannelList) { .antenna = ANTENNA_1, .persistent = false, .nFrequencies = 50, .currentChannelListIdx = 0,    \
                                                    .item = {       \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=902750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=915250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=903250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=915750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=903750), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=916250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=904250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=916750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=904750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=917250), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=905250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=917750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=905750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=918250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=906250), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=918750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=906750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=919250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=907250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=919750), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=907750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=920250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=908250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=920750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=908750), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=909250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=909750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=922250), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=910250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=922750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=910750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=923250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=911250), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=923750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=911750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=924250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=912250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=924750), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=912750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=925250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=913250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=925750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=913750), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=926250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=914250), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=926750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=914750), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=927250), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT()  \
                                                    }, ##__VA_ARGS__ })

#define STUHFL_O_ST25RU3993_CHANNELLIST_JAPAN_INIT(...) ((STUHFL_T_ST25RU3993_ChannelList) { .antenna = ANTENNA_1, .persistent = false, .nFrequencies = 9, .currentChannelListIdx = 0,    \
                                                    .item = {       \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=920500), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=920700), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=920900), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921100), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921300), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921500), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921700), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921900), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=922100), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), \
                                                    }, ##__VA_ARGS__ })

#define STUHFL_O_ST25RU3993_CHANNELLIST_CHINA_INIT(...) ((STUHFL_T_ST25RU3993_ChannelList) { .antenna = ANTENNA_1, .persistent = false, .nFrequencies = 16, .currentChannelListIdx = 0,    \
                                                    .item = {       \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=840625), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=840875), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=841125), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=841375), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=841625), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=841875), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=842125), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=842375), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=842625), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=842875), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=843125), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=843375), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=843625), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=843875), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=844125), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=844375), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                     \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT()                                                                                                                                                                       \
                                                    }, ##__VA_ARGS__ })

#define STUHFL_O_ST25RU3993_CHANNELLIST_CHINA2_INIT(...) ((STUHFL_T_ST25RU3993_ChannelList) { .antenna = ANTENNA_1, .persistent = false, .nFrequencies = 16, .currentChannelListIdx = 0,    \
                                                    .item = {       \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=920625), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=920875), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921125), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921375), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921625), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=921875), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=922125), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=922375), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=922625), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=922875), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=923125), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=923375), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=923625), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=923875), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=924125), \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(.frequency=924375), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                     \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(),                                                                                      \
                                                    STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT(), STUHFL_O_ST25RU3993_CHANNELITEM_INIT()                                                                                                                                                                       \
                                                    }, ##__VA_ARGS__ })

/* Set/Get */
typedef struct {
    uint16_t                            maxSendingTime;     /**< I/O Param: Maximum sending time frequency hopping */
} STUHFL_T_ST25RU3993_Freq_Hop;
#define STUHFL_O_ST25RU3993_FREQ_HOP_INIT(...) ((STUHFL_T_ST25RU3993_Freq_Hop) { .maxSendingTime = 400, ##__VA_ARGS__ })

/* Set/Get */
typedef struct {
    uint16_t                            listeningTime;      /**< I/O Param: Time for listing periode */
    uint16_t                            idleTime;           /**< I/O Param: Idle time for LBT */
    uint8_t                             rssiLogThreshold;   /**< I/O Param: RSSI threshold value */
    uint8_t                             skipLBTcheck;       /**< I/O Param: Flag to wheter LBT check shall be skipped at all */
} STUHFL_T_ST25RU3993_Freq_LBT;
#define STUHFL_O_ST25RU3993_FREQ_LBT_INIT(...) ((STUHFL_T_ST25RU3993_Freq_LBT) { .listeningTime = 1, .idleTime = 0, .rssiLogThreshold = 31, .skipLBTcheck = true, ##__VA_ARGS__ })

/* Set */
#define CONTINUOUS_MODULATION_MODE_STATIC          0
#define CONTINUOUS_MODULATION_MODE_PSEUDO_RANDOM   1
#define CONTINUOUS_MODULATION_MODE_ETSI            2
#define CONTINUOUS_MODULATION_MODE_OFF             3
typedef struct {
    uint32_t                            frequency;      /**< I Param: Frequency to be used for continuouse modulation */
    bool                                enableContMod;  /**< I Param: Flag to enable or disable modulation */
    uint16_t                            maxSendingTime; /**< I Param: Maximum modulation time in ms. If zero the modulation will not stopped automatically */
    uint8_t                             modulationMode; /**< I Param: Modulation mode */
} STUHFL_T_ST25RU3993_Freq_ContMod;
#define STUHFL_O_ST25RU3993_FREQ_CONTMOD_INIT(...) ((STUHFL_T_ST25RU3993_Freq_ContMod) { .frequency = DEFAULT_FREQUENCY, .enableContMod = false, .maxSendingTime = 0, .modulationMode = CONTINUOUS_MODULATION_MODE_OFF, ##__VA_ARGS__ })

/* Set/Get */
/* TrExt defines */
#define TREXT_LEAD_OFF      false   /**< do not send lead code */
#define TREXT_LEAD_ON       true    /**< send lead code  */
/* Gen2 Tari */
#define GEN2_TARI_6_25          0 /**< set tari to  6.25 us */
#define GEN2_TARI_12_50         1 /**< set tari to 12.5  us */
#define GEN2_TARI_25_00         2 /**< set tari to 25    us */
/* Gen2 Backlink frequencies */
#define GEN2_LF_40              0   /** <link frequency  40 kHz */
#define GEN2_LF_160             6   /** <link frequency 160 kHz */
#define GEN2_LF_213             8   /** <link frequency 213 kHz */
#define GEN2_LF_256             9   /** <link frequency 256 kHz */
#define GEN2_LF_320             12  /** <link frequency 320 kHz */
#define GEN2_LF_640             15  /** <link frequency 640 kHz */
/* Gen2 Rx coding */
#define GEN2_COD_FM0            0 /** <FM0 coding for rx      */
#define GEN2_COD_MILLER2        1 /** <MILLER2 coding for rx */
#define GEN2_COD_MILLER4        2 /** <MILLER4 coding for rx */
#define GEN2_COD_MILLER8        3 /** <MILLER8 coding for rx */
typedef struct {
    uint8_t tari;       /**< I/O Param: Tari setting */
    uint8_t blf;        /**< I/O Param: GEN2_LF_40, ... */
    uint8_t coding;     /**< I/O Param: GEN2_COD_FM0, ... */
    bool    trext;      /**< I/O Param: 1 if the preamble is long, i.e. with pilot tone */
} STUHFL_T_ST25RU3993_Gen2Protocol_Cfg;
#define STUHFL_O_ST25RU3993_GEN2PROTOCOL_CFG_INIT(...) ((STUHFL_T_ST25RU3993_Gen2Protocol_Cfg) { .tari = GEN2_TARI_25_00, .blf = GEN2_LF_256, .coding = GEN2_COD_MILLER8, .trext = TREXT_LEAD_ON, ##__VA_ARGS__ })

/* Set/Get */
#define GB29768_BLF_64      0   /**< link frequency 64 kHz */
#define GB29768_BLF_137     1   /**< link frequency 137 kHz */
#define GB29768_BLF_174     2   /**< link frequency 174 kHz */
#define GB29768_BLF_320     3   /**< link frequency 320 kHz */
#define GB29768_BLF_128     4   /**< link frequency 128 kHz */
#define GB29768_BLF_274     5   /**< link frequency 274 kHz */
#define GB29768_BLF_349     6   /**< link frequency 349 kHz */
#define GB29768_BLF_640     7   /**< link frequency 640 kHz */

/* Rx coding values */
#define GB29768_COD_FM0     0   /**< FM coding for rx      */
#define GB29768_COD_MILLER2 1   /**< MILLER2 coding for rx */
#define GB29768_COD_MILLER4 2   /**< MILLER4 coding for rx */
#define GB29768_COD_MILLER8 3   /**< MILLER8 coding for rx */

/* Tc (TPP coding interval) */
#define GB29768_TC_6_25     0
#define GB29768_TC_12_5     1

typedef struct {
    bool    trext;       /**< I/O Param: 1 if the lead code is sent, 0 otherwise */
    uint8_t blf;         /**< I/O Param: backscatter link frequency factor */
    uint8_t coding;      /**< I/O Param: GB29768_COD_FM0, GB29768_COD_MILLER2, ... */
    uint8_t tc;          /**< I/O Param: GB29768_TC_12_5, GB29768_TC_6_25 */
} STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg;
#define STUHFL_O_ST25RU3993_GB29768PROTOCOL_CFG_INIT(...) ((STUHFL_T_ST25RU3993_Gb29768Protocol_Cfg) { .trext = TREXT_LEAD_ON, .blf = GB29768_BLF_320, .coding = GB29768_COD_MILLER2, .tc = GB29768_TC_12_5, ##__VA_ARGS__ })

/* Set/Get */
#if ELANCE
#define ANTENNA_1   0
#define ANTENNA_2   1
#define ANTENNA_3   2
#define ANTENNA_4   3
#define ANTENNA_ALT 0xFF
#else
#define ANTENNA_1   0
#define ANTENNA_2   1
#define ANTENNA_ALT 2
#endif
typedef struct {
    int8_t      txOutputLevel;                  /**< I/O Param: Tx output level. See Modulator control register 3 for further info. Range [0db..-19db] .. -0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15,-16,-17,-18,-19 */
    int8_t      rxSensitivity;                  /**< I/O Param: Rx sensitivity level. Range [-17dB..+19dB] .. -17,-14,-11,-9,-8,-6,-5,-3,-2,0,1,3,4,6,7,9,10,13,16,19 */
    uint8_t     usedAntenna;                    /**< I/O Param: Antenna to be used */
    uint16_t    alternateAntennaInterval;       /**< I/O Param: Time in ms for alternating the antennas when alternating mode is used */
    uint8_t     vlna;                           /**< I/O Param: VLNA setting. This is only used with ELANCE boards, all other board will ignore this setting */
} STUHFL_T_ST25RU3993_TxRx_Cfg;
#define STUHFL_O_ST25RU3993_TXRX_CFG_INIT(...) ((STUHFL_T_ST25RU3993_TxRx_Cfg) { .txOutputLevel = -2, .rxSensitivity = 3, .usedAntenna = ANTENNA_1, .alternateAntennaInterval = 1, .vlna = 3, ##__VA_ARGS__ })

/* Set/Get */
typedef struct {
    uint8_t useExternal;     /* I/O */
} STUHFL_T_ST25RU3993_PA_Cfg;
#define STUHFL_O_ST25RU3993_PA_CFG_INIT(...) ((STUHFL_T_ST25RU3993_PA_Cfg) { .useExternal = 1, ##__VA_ARGS__ })

/* Set/Get */
/* Gen2 Session */
#define GEN2_SESSION_S0         0x00
#define GEN2_SESSION_S1         0x01
#define GEN2_SESSION_S2         0x02
#define GEN2_SESSION_S3         0x03

/* Gen2 Target */
#define GEN2_TARGET_A           0
#define GEN2_TARGET_B           1

#define NUM_C_VALUES            16U

/** Definition of the maximum number of the gen2 Q Value, standard allows up to 15 */
#define MAXGEN2Q                15U

typedef struct {
    /* Generic inventory info */
    bool        fastInv;                        /**< I Param: Fast Inventory enabling. If set to 0 normal inventory round will be performed, if set to 1 fast inventory rounds will be performed.*/
    bool        autoAck;                        /**< I Param: Automatic Ack enabling. If set to 0 inventory round commands will be triggered by the FW, otherwise the autoACK feature of the reader will be used which sends the required Gen2 commands automatically.*/
    bool        readTID;                        /**< I Param: read TID enabling.  If set to 1 an read of the TID will be performed in inventory rounds.*/

    /* Anticollision */
    uint8_t     startQ;                         /**< I/O Param: Q starting value */
    uint8_t     adaptiveQEnable;                /**< I/O Param: Flag to enable automatic Q adaption */
    uint8_t     minQ;                           /**< I/O Param: Minimum value that Q could reach */
    uint8_t     maxQ;                           /**< I/O Param: Maximum value that Q could reach. If value exceeds 15 it is truncated to 15. */
    uint8_t     adjustOptions;                  /**< I/O Param: Q algorithm options */
    uint8_t     C2[NUM_C_VALUES];               /**< I/O Param: Q algorithm C2 values for each Q */
    uint8_t     C1[NUM_C_VALUES];               /**< I/O Param: Q algorithm C1 values for each Q */

    /* Automatic tuning */
    uint16_t    autoTuningInterval;             /**< I/O Param: Auto tuning check interval (in inventory rounds)*/
    uint8_t     autoTuningLevel;                /**< I/O Param: Deviation of (I^2+Q^2) to trigger retuning */
    uint8_t     autoTuningAlgo;                 /**< I/O Param: Algorithm used for automatic (re)tuning. See  */

    /* Query params */
    uint8_t     sel;                            /**< I/O Param: For QUERY Sel field */
    uint8_t     session;                        /**< I/O Param: GEN2_SESSION_S0, ... */
    uint8_t     target;                         /**< I/O Param: For QUERY Target field */
    bool        toggleTarget;                   /**< I/O Param: Toggle between Target A and B */
    bool        targetDepletionMode;            /**< I/O Param: If set to 1 and the target shall be toggled in inventory an additional inventory round before the target is toggled will be executed. This gives "weak" transponders an additional chance to reply. */

    /* Adaptive sensitivity */
    uint16_t    adaptiveSensitivityInterval;   /**< I/O Param: Adaptive sensitivity interval (in inventory rounds)*/
    uint8_t     adaptiveSensitivityEnable;      /**< I/O Param: Flag to enable automatic sensitivity adjustment */
} STUHFL_T_ST25RU3993_Gen2Inventory_Cfg;
#define STUHFL_O_ST25RU3993_GEN2INVENTORY_CFG_INIT(...) ((STUHFL_T_ST25RU3993_Gen2Inventory_Cfg) { .fastInv = false, .autoAck = true, .readTID = false, \
                                                                                              .startQ = 4, .adaptiveQEnable = true, .minQ = 0, .maxQ = MAXGEN2Q, .adjustOptions = 0, \
                                                                                              .C2 = {35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35}, .C1 = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15}, \
                                                                                              .autoTuningInterval = 7, .autoTuningLevel = 20, .autoTuningAlgo = TUNING_ALGO_FAST, \
                                                                                              .sel = 0, .session = GEN2_SESSION_S0, .target = GEN2_TARGET_A, .toggleTarget = false, .targetDepletionMode = false, \
                                                                                              .adaptiveSensitivityInterval = 5, .adaptiveSensitivityEnable = false, \
                                                                                              ##__VA_ARGS__ })

/* condition field */
#define GB29768_CONDITION_ALL   0x00    /**< all tags participate */
#define GB29768_CONDITION_FLAG1 0x01    /**< all tags with matching flag 1 */
#define GB29768_CONDITION_FLAG0 0x02    /**< all tags with matching flag 0 */

/* Session field */
#define GB29768_SESSION_S0  0x00
#define GB29768_SESSION_S1  0x01
#define GB29768_SESSION_S2  0x02
#define GB29768_SESSION_S3  0x03

/* Target */
#define GB29768_TARGET_0    0
#define GB29768_TARGET_1    1

typedef struct {
    /* Automatic tuning */
    uint16_t    autoTuningInterval;             /**< I/O Param: Auto tuning check interval (in inventory rounds)*/
    uint8_t     autoTuningLevel;                /**< I/O Param: Deviation of (I^2+Q^2) to trigger retuning */
    uint8_t     autoTuningAlgo;                 /**< I/O Param: Algorithm used for automatic (re)tuning. See  */

    /* Adaptive sensitivity */
    uint16_t    adaptiveSensitivityInterval;    /**< I/O Param: Adaptive sensitivity interval (in inventory rounds)*/
    uint8_t     adaptiveSensitivityEnable;      /**< I/O Param: Flag to enable automatic sensitivity adjustment */

    /* Query params */
    uint8_t     condition;                      /**< I/O Param: Condition setting */
    uint8_t     session;                        /**< I/O Param: GB29768_SESSION_S0, ... */
    uint8_t     target;                         /**< I/O Param: For QUERY Target field */
    bool        toggleTarget;                   /**< I/O Param: Target A/B toggle */
    bool        targetDepletionMode;            /**< I/O Param: If TRUE and the target shall be toggled in inventory an additional inventory round before the target is toggled will be executed. This gives "weak" transponders an additional chance to reply. */

    /* Anticollision */
    uint16_t    endThreshold;                   /**< I Param: GBT anticollision end threshold parameter.*/
    uint16_t    ccnThreshold;                   /**< I Param: GBT anticollision CCN threshold parameter.*/
    uint16_t    cinThreshold;                   /**< I Param: GBT anticollision CIN threshold parameter.*/

    /* Generic inventory info */
    bool        readTID;                        /**< I Param: read TID enabling.  If set to 1 an read of the TID will be performed in inventory rounds.*/
} STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg;
#define STUHFL_O_ST25RU3993_GB29768INVENTORY_CFG_INIT(...) ((STUHFL_T_ST25RU3993_Gb29768Inventory_Cfg) { \
                                                                                              .autoTuningInterval = 7, .autoTuningLevel = 20, .autoTuningAlgo = TUNING_ALGO_FAST, \
                                                                                              .adaptiveSensitivityInterval = 5, .adaptiveSensitivityEnable = false, \
                                                                                              .condition = GB29768_CONDITION_ALL, .session = GB29768_SESSION_S0, .target = GB29768_TARGET_0, .toggleTarget = true, .targetDepletionMode = false, \
                                                                                              .endThreshold = 2, .ccnThreshold = 3, .cinThreshold = 4, \
                                                                                              .readTID = false, \
                                                                                              ##__VA_ARGS__ })

// --------------------------------------------------------------------------
/*  */
typedef struct STUHFL_DEPRECATED STUHFL_S_ST25RU3993_TuningTableEntry {
    uint8_t     entry;                  /**< I Param: table entry, when set to 0xFF the entry will be appended to the end of the table */
    uint32_t    freq;                   /**< I/O Param: Frequency for when this entry shall be applied */
    uint8_t     applyCapValues[MAX_ANTENNA]; /**< I Param: flag to specify if cap values should be applied for antenna */
    uint8_t     cin[MAX_ANTENNA];       /**< I/O Param: IN capacitance of tuning PI network */
    uint8_t     clen[MAX_ANTENNA];      /**< I/O Param: LEN capacitance of tuning PI network */
    uint8_t     cout[MAX_ANTENNA];      /**< I/O Param: OUT capacitance of tuning PI network */
    uint16_t    IQ[MAX_ANTENNA];        /**< I/O Param: IQ value for this entry */
} STUHFL_T_ST25RU3993_TuningTableEntry;
#define STUHFL_O_ST25RU3993_TUNINGTABLEENTRY_INIT(...) ((STUHFL_T_ST25RU3993_TuningTableEntry) { .entry = 0, .freq = DEFAULT_FREQUENCY, .applyCapValues = {false, false, false, false}, .cin = {0,0,0,0}, .clen = {0,0,0,0}, .cout = {0,0,0,0}, .IQ = {0xFFFF,0xFFFF,0xFFFF,0xFFFF}, ##__VA_ARGS__ })

/*  */
typedef struct STUHFL_DEPRECATED STUHFL_S_ST25RU3993_TunerTableSet {
    uint8_t     profile;    /**< I Param: Profile identifier */
    uint32_t    freq;       /**< I Param: Frequency */
} STUHFL_T_ST25RU3993_TunerTableSet;
#define STUHFL_O_ST25RU3993_TUNERTABLESET_INIT(...) ((STUHFL_T_ST25RU3993_TunerTableSet) { .profile = PROFILE_EUROPE, .freq = RESET_DEFAULT_ALL_FREQS, ##__VA_ARGS__ })

/* Get/Set */
typedef struct STUHFL_DEPRECATED STUHFL_S_ST25RU3993_Tuning {
    uint8_t antenna;    /**< I Param: Antenna on which tuning settings apply */
    uint8_t cin;        /**< I/O Param: Measured IN capacitance of tuning PI network */
    uint8_t clen;       /**< I/O Param: Measured LEN capacitance of tuning PI network */
    uint8_t cout;       /**< I/O Param: Measured OUT capacitance of tuning PI network */
} STUHFL_T_ST25RU3993_Tuning;
#define STUHFL_O_ST25RU3993_TUNING_INIT(...) ((STUHFL_T_ST25RU3993_Tuning) { .antenna = ANTENNA_1, .cin = 0, .clen = 0, .cout = 0, ##__VA_ARGS__ })

/*  */
typedef struct STUHFL_DEPRECATED STUHFL_S_ST25RU3993_TuningTableInfo {
    uint8_t profile;                    /**< I Param: profile ID where number of entries should be replied */
    uint8_t numEntries;                 /**< O Param: number of entries for requested profile */
} STUHFL_T_ST25RU3993_TuningTableInfo;
#define STUHFL_O_ST25RU3993_TUNINGTABLEINFO_INIT(...) ((STUHFL_T_ST25RU3993_TuningTableInfo) { .profile = PROFILE_EUROPE, .numEntries = 4, ##__VA_ARGS__ })

/* Get/Set */
typedef struct {
    uint8_t                     antenna;        /**< I Param: Antenna on which tuning settings apply */
    uint8_t                     channelListIdx; /**< I Param: Related channel list entry */
    STUHFL_T_ST25RU3993_Caps    caps;           /**< I/O Param: Tuning caps at antenna */
} STUHFL_T_ST25RU3993_TuningCaps;
#define STUHFL_O_ST25RU3993_TUNINGCAPS_INIT(...) ((STUHFL_T_ST25RU3993_TuningCaps) { .antenna = ANTENNA_1, .channelListIdx = 0, .caps = STUHFL_O_ST25RU3993_CAPS_INIT(), ##__VA_ARGS__ })

/**< Tune algorithm definitions */
#define TUNING_ALGO_NONE        0
#define TUNING_ALGO_FAST        1   /**< Simple automatic tuning function.This function tries to find an optimized tuner setting(minimal reflected power). The function starts at the current tuner setting and modifies the tuner caps until a setting with a minimum of reflected power is found.When changing the tuner further leads to an increase of reflected power the algorithm stops. Note that, although the algorithm has been optimized to not immediately stop at local minima of reflected power, it still might not find the tuner setting with the lowest reflected power.The algorithm of tunerMultiHillClimb() is probably producing better results, but it is slower. */
#define TUNING_ALGO_SLOW        2   /**< Sophisticated automatic tuning function.This function tries to find an optimized tuner setting(minimal reflected power). The function splits the 3 - dimensional tuner - setting - space(axis are Cin, Clen and Cout) into segments and searches in each segment(by using tunerOneHillClimb()) for its local minimum of reflected power. The tuner setting(point in tuner - setting - space) which has the lowest reflected power is returned in parameter res. This function has a much higher probability to find the tuner setting with the lowest reflected power than tunerOneHillClimb() but on the other hand takes much longer. */
#define TUNING_ALGO_MEDIUM      3   /**< Enhanced Sophisticated automatic tuning function.This function tries to find an optimized tuner setting(minimal reflected power). The function splits the 3 - dimensional tuner - setting - space(axis are Cin, Clen and Cout) into segments and get reflected power for each of them.A tunerOneHillClimb() is then run on the 3 segments with minimum of reflected power. The tuner setting(point in tuner - setting - space) which has the lowest reflected power is then returned in parameter res. This function has a much higher probability to find the tuner setting with the lowest reflected power than tunerOneHillClimb() and is faster than tunerMultiHillClimb(). */

#define TUNING_ALGO_ENABLE_FPD  0x80    /**<STUHFL_DEPRECATED: Enable False Positive Detection during tuning algorithm.*/

/* */
typedef struct STUHFL_DEPRECATED STUHFL_S_ST25RU3993_Tune {
    uint8_t algo;                   /**< I Param: Used algorithm for tuning. */
} STUHFL_T_ST25RU3993_Tune;
#define STUHFL_O_ST25RU3993_TUNE_INIT(...) ((STUHFL_T_ST25RU3993_Tune) { .algo = TUNING_ALGO_MEDIUM, ##__VA_ARGS__ })

typedef struct {
    bool                            enableFPD;                  /**< I Param: Do false positive check */
    bool                            save2Flash;                 /**< I Param: Save tuning results to flash */
    uint8_t                         channelListIdx;             /**< I Param: Index of channel list frequency aimed to be tuned */
    uint8_t                         antenna;                    /**< I Param: Antenna for which the channel list shall be used */
    uint8_t                         algorithm;                  /**< I Param: Used algorithm for tuning. */
    uint8_t                         rfu;
} STUHFL_T_ST25RU3993_TuneCfg;
#define STUHFL_O_ST25RU3993_TUNECFG_INIT(...) ((STUHFL_T_ST25RU3993_TuneCfg) { .enableFPD = true, .save2Flash = false, .channelListIdx = 0, .antenna = ANTENNA_1, .algorithm = TUNING_ALGO_MEDIUM, .rfu = 0, ##__VA_ARGS__ })

#pragma pack(pop)

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __STUHFL_DL_ST25RU3993_EVAL_H
