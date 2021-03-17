/**
  ******************************************************************************
  * @file           STUHFL_demo.c
  * @brief          Main program body
  ******************************************************************************
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

#include "stuhfl.h"
#include "stuhfl_sl.h"
#include "stuhfl_sl_gen2.h"
#include "stuhfl_sl_gb29768.h"
#include "stuhfl_al.h"
#include "stuhfl_dl.h"
#include "stuhfl_pl.h"
#include "stuhfl_evalAPI.h"
#include "stuhfl_err.h"
#include "stuhfl_platform.h"
#include "stuhfl_log.h"

#include <WinSock2.h>
#include "main.h"
#include "network.h"

#if defined(WIN32) || defined(WIN64)
#include <conio.h>
#elif defined(POSIX)
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include <stdlib.h>
#include <stdio.h>

//
#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#define CLEAR_SCREEN()          system("cls");
#elif defined(POSIX)
#define USE_VS2017_LINUX_CONSOLE    1
#if defined(USE_VS2017_LINUX_CONSOLE)
#define CLEAR_SCREEN()          for(int i=0; i<100; i++)printf("\n");   // when using the VS2017 linux console there is no standardized way to clean the output.
// The easiest way is to scroll out the old content.
#else
#define CLEAR_SCREEN()          system("clear");
#endif
bool _kbhit();
char _getch();
#define  PLAYGROUND             1
#define COM_PORT                "/dev/ttyUSB0"
#else
#endif

#ifdef POSIX
extern int usleep (__useconds_t __useconds);
#endif

// globals
#define LOG_BUF_SIZE    0xFFFF
static char logBuf[LOG_BUF_SIZE];
static int logBufPos = 0;

static bool useNewTuningMechanism = false;

#define BUFFER_SIZE 70
#define SND_BUFFER_SIZE         (UART_RX_BUFFER_SIZE)   /* HOST SND buffer based on FW RCV buffer */
#define RCV_BUFFER_SIZE         (UART_TX_BUFFER_SIZE)   /* HOST RCV buffer based on FW SND buffer */

/**
  * @brief      Configuration for Inventory Runner + Gen2 Inventory.
  *
  * @param[in]  singleTag:      true: single tag -> no adaptive Q +  Q = 0<br>
  *                             false: multiple tags -> adaptive Q + Q = 4<br>
  * @param[in]  freqHopping:    true: EU frequency profile with hopping<br>
  *                             false: EU frequency profile without hopping<br>
  * @param[in]  antenna:        ANTENNA_1: Using Antenna 1<br>
  *                             ANTENNA_2: Using Antenna 2<br>
  *
  * @retval     None
  */
void setupGen2Config(bool singleTag, bool freqHopping, int antenna)
{
    STUHFL_T_ST25RU3993_TxRx_Cfg TxRxCfg = STUHFL_O_ST25RU3993_TXRX_CFG_INIT();          // Set to FW default values
    TxRxCfg.usedAntenna = (uint8_t)antenna;
    SetTxRxCfg(&TxRxCfg);

    STUHFL_T_ST25RU3993_Gen2Inventory_Cfg invGen2Cfg = STUHFL_O_ST25RU3993_GEN2INVENTORY_CFG_INIT();     // Set to FW default values
    invGen2Cfg.fastInv = true;
    invGen2Cfg.autoAck = false;
    invGen2Cfg.startQ = singleTag ? 0 : 4;
    invGen2Cfg.adaptiveQEnable = !singleTag;
    invGen2Cfg.adaptiveSensitivityEnable = true;
    invGen2Cfg.toggleTarget = true;
    invGen2Cfg.targetDepletionMode = true;
    SetGen2InventoryCfg(&invGen2Cfg);

    //
    STUHFL_T_ST25RU3993_Gen2Protocol_Cfg gen2ProtocolCfg = STUHFL_O_ST25RU3993_GEN2PROTOCOL_CFG_INIT();  // Set to FW default values
    SetGen2ProtocolCfg(&gen2ProtocolCfg);

    STUHFL_T_ST25RU3993_Freq_LBT freqLBT = STUHFL_O_ST25RU3993_FREQ_LBT_INIT();                          // Set to FW default values
    freqLBT.listeningTime = 0;
    SetFreqLBT(&freqLBT);

    STUHFL_T_ST25RU3993_Freq_Profile freqProfile = STUHFL_O_ST25RU3993_FREQ_PROFILE_INIT();          // Set to FW default values

    freqProfile.profile = PROFILE_EUROPE;
    SetFreqProfile(&freqProfile);

    STUHFL_T_ST25RU3993_Freq_Hop freqHop = STUHFL_O_ST25RU3993_FREQ_HOP_INIT();              // Set to FW default values
    SetFreqHop(&freqHop);

    STUHFL_T_Gen2_Select    Gen2Select = STUHFL_O_GEN2_SELECT_INIT();                        // Set to FW default values
    Gen2Select.mode = GEN2_SELECT_MODE_CLEAR_LIST;  // Clear all Select filters
    Gen2_Select(&Gen2Select);

    printf("Tuning Profile frequencies: algo: TUNING_ALGO_SLOW\n");

    // Get freq profile + number of frequencies
    STUHFL_T_ST25RU3993_Freq_Profile_Info   freqProfileInfo = STUHFL_O_ST25RU3993_FREQ_PROFILE_INFO_INIT();
    GetFreqProfileInfo(&freqProfileInfo);

    // Tune for each freq
    for (uint8_t i = 0; i < freqProfileInfo.numFrequencies; i++) {
        STUHFL_T_ST25RU3993_TuningTableEntry    tuningTableEntry = STUHFL_O_ST25RU3993_TUNINGTABLEENTRY_INIT();
        STUHFL_T_ST25RU3993_Antenna_Power       antPwr = STUHFL_O_ST25RU3993_ANTENNA_POWER_INIT();
        STUHFL_T_ST25RU3993_Tune                tune = STUHFL_O_ST25RU3993_TUNE_INIT();

        tuningTableEntry.entry = i;
        GetTuningTableEntry(&tuningTableEntry);               // Retrieve frequency related to this entry

        tuningTableEntry.entry = i;
        memset(tuningTableEntry.applyCapValues, false, MAX_ANTENNA);    // Do not apply caps, only set entry
        SetTuningTableEntry(&tuningTableEntry);

        antPwr.mode = ANTENNA_POWER_MODE_ON;
        antPwr.timeout = 0;
        antPwr.frequency = tuningTableEntry.freq;
        SetAntennaPower(&antPwr);

        tune.algo = TUNING_ALGO_SLOW;
        Tune(&tune);

        antPwr.mode = ANTENNA_POWER_MODE_OFF;
        antPwr.timeout = 0;
        antPwr.frequency = tuningTableEntry.freq;
        SetAntennaPower(&antPwr);
    }
}


/**
  * @brief      Search for valid com ports
  *
  * @param[out] szPort: Found COM port.
  *
  * @retval     true: Found
  * @retval     false: otherwise
  */
static bool getComPort(char* szPort)
{
#if defined(WIN32) || defined(WIN64)
    int port = 0;

    int ports[256];
    int nPorts = 0;
    char szComPort[32];
    HANDLE hCom = NULL;

    nPorts = 0;
    for (int i = 1; i <= 255; i++) {
        // test possible com boards
        snprintf(szComPort, sizeof(szComPort), "\\\\.\\COM%d", i);
        hCom = CreateFileA(szComPort,
            GENERIC_READ | GENERIC_WRITE,   // desired access should be read&write
            0,                              // COM port must be opened in non-sharing mode
            NULL,                           // don't care about the security
            OPEN_EXISTING,                  // IMPORTANT: must use OPEN_EXISTING for a COM port
            0,                              // usually overlapped but non-overlapped for existance test
            NULL);                          // always NULL for a general purpose COM port

        if (INVALID_HANDLE_VALUE != hCom) {
            // COM port exist
            ports[nPorts] = i;
            nPorts++;
            CloseHandle(hCom);
        }
    }

    if (nPorts == 0) {
        printf("Sorry, no serial port detected. press any key to terminate ..\n");
        while (!_kbhit());
        return false;
    }
    else if (nPorts > 1) {
        printf("Found the following com ports: ");
        for (int i = 0; i < nPorts; i++) {
            printf("%d, ", ports[i]);
        }
        printf("\nPlease specify the port to use: ");
        port = _getch();
        port -= '0';
    }
    else {
        // exact one port found.
        port = ports[0];
    }
    snprintf(szPort, 32, "\\\\.\\COM%d", port);

#elif defined(POSIX)
    // NOTE: currently search for valid com ports on posix devices is not implemented
    snprintf(szPort, 32, "%s", COM_PORT);
#else
#endif
    //
    return true;
}

/**
  * @brief      print log message to user screen.
  *
  * @param[in]  log: Pointer to log buffer
  * @param[in]  logBufSize: Log buffer size
  * @param[in]  clearScreen: Clear screen before message output
  *
  * @retval     None
  */
void log2Screen(bool clearScreen, bool flush, const char* format, ...)
{
    if (clearScreen) {
        CLEAR_SCREEN();
    }

    // update buf
    va_list args;
    va_start(args, format);
    if ((LOG_BUF_SIZE - logBufPos) > 0) {
        int writtenLen = vsnprintf(&logBuf[logBufPos], (size_t)(LOG_BUF_SIZE - logBufPos), format, args);
        if ((writtenLen > 0) && (writtenLen < (LOG_BUF_SIZE - logBufPos))) {
            logBufPos += writtenLen;
        }
    }
    va_end(args);

    // and log to screen ..
    if (flush) {
        printf(logBuf);
        logBufPos = 0;
    }
}



int main (int argc, char * argv[])
{

    //Servidor socket
    WSADATA WSAData;
    SOCKET server;
    SOCKET clientRead, client;
    //SOCKADDR_IN  clientAddr;
    struct sockaddr_in clientAddr, clientAddrRead;
    int clientAddrSize = sizeof(clientAddr);
    int clientAddrSizeRead = sizeof(clientAddrRead);
    int retval;
    int conectado = 0;

    char msg[BUFFER_SIZE + 1];


    STUHFL_T_DEVICE_CTX device = 0;
    uint8_t sndData[SND_BUFFER_SIZE];
    uint8_t rcvData[RCV_BUFFER_SIZE];

    char comPort[64];
   // char* comPort;
    getComPort(comPort);

    STUHFL_T_RET_CODE ret = STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_PORT, (STUHFL_T_PARAM_VALUE)comPort);
    ret |= STUHFL_F_Connect(&device, sndData, SND_BUFFER_SIZE, rcvData, RCV_BUFFER_SIZE);

    // enable data line
    uint8_t on = TRUE;
    ret |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_DTR, (STUHFL_T_PARAM_VALUE)&on);
    // toogle reset line
    on = TRUE;
    ret |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RTS, (STUHFL_T_PARAM_VALUE)&on);
    on = FALSE;
    ret |= STUHFL_F_SetParam(STUHFL_PARAM_TYPE_CONNECTION | STUHFL_PARAM_KEY_RTS, (STUHFL_T_PARAM_VALUE)&on);

    // give board time to boot
    usleep(600000);     // (600 msec is on the safe side)

     /* COMUNICACIÓN SOCKET CON EL SOFTWARE MYRUNS */
    server = configure_tcp_socket(5557);

    if ((client = accept(server, (struct sockaddr*)&clientAddr, &clientAddrSize)) != INVALID_SOCKET)
    {
        conectado = 1;
        printf("CONECTADO AL READER\n");
    }

    server = configure_tcp_socket(5556);
    if ((clientRead = accept(server, (struct sockaddr*)&clientAddrRead, &clientAddrSizeRead)) != INVALID_SOCKET)
    {
        printf("Conectado para enviar tags!\n");
    }
    

    while (conectado == 1) {

        memset(msg, '\0', BUFFER_SIZE + 1);
        retval = recv(client, msg, (BUFFER_SIZE), MSG_WAITALL);
        char* mens = strtok(msg, "#");
        sprintf(msg, "%s", mens);

        //printf("msg WHILE: %s\n", msg);
        if (strncmp(msg, "DISCONNECT", 10) == 0) {
            printf("msg: %s\n", msg);
            conectado = 0;
            send(client, "OK#", 3, 0);
        }
        if (strncmp(msg, "POWER_MINMAX", 12) == 0)
        {
            printf("msg: %s\n", msg);
        }
        else if (strncmp(msg, "GET_POWER", 9) == 0) {
            printf("msg: %s\n", msg);

            STUHFL_T_ST25RU3993_Antenna_Power pwr = STUHFL_O_ST25RU3993_ANTENNA_POWER_INIT();

            // get current freq
            GetAntennaPower(&pwr);
            printf("msg: %s\n", pwr);
           
            
        }
        else if (strncmp(msg, "SET_POWER", 9) == 0) {
            printf("msg: %s\n", msg);
            char* mens = strtok(msg, " ");
            char* pow = strtok(NULL, " ");
            double value = atof(pow);
            STUHFL_T_ST25RU3993_Antenna_Power pwr = STUHFL_O_ST25RU3993_ANTENNA_POWER_INIT();
            SetAntennaPower(&pwr);

            printf("RECIBIDO POWER: %.1f\n", value);
          
            send(client, "OK#", 3, 0);
        }
        else if (strcmp(msg, "ANT_PORTS") == 0) {
            //printf("msg: %s\n", msg);
        }
        else if (strncmp(msg, "CON_ANT_PORTS", 13) == 0) {
            //que antenas estan enabled
            printf("msg: %s\n", msg);
            char selAnt[4];
            char selAntSend[5];
             
            send(client, selAntSend, 4, 0);
        }
        else if (strncmp(msg, "GET_SEL_ANT", 11) == 0) {
            printf("msg: %s\n", msg);
            //int* selAnt[4];
            char selAnt[5] = { 0 };
            char selAntSend[5];
            char p[2] = { '1','#' };
            char a[2] = "1#";

            strcat(selAnt, "#");
            printf(selAnt);
            send(client, selAnt, strlen(selAnt), 0);
        }
        else if (strncmp(msg, "SET_SEL_ANT", 11) == 0) {
            printf("msg: %s\n", msg);
            char* mens = strtok(msg, " ");
            char* ant = strtok(NULL, "");
            printf("CONECTADAS: %s\n", ant);

            send(client, "OK#", 3, 0);
        }
        else if (strncmp(msg, "GET_INFO", 8) == 0) {
            printf("msg: %s\n", msg);
            char info[70];
            //getReaderInfo(handle, info);
            STUHFL_T_Version        swVer = STUHFL_O_VERSION_INIT();
            STUHFL_T_Version        hwVer = STUHFL_O_VERSION_INIT();
            STUHFL_T_Version_Info   swInfo = STUHFL_O_VERSION_INFO_INIT();
            STUHFL_T_Version_Info   hwInfo = STUHFL_O_VERSION_INFO_INIT();

            STUHFL_T_RET_CODE ret = GetBoardVersion(&swVer, &hwVer);
            ret |= GetBoardInfo(&swInfo, &hwInfo);
            sprintf(info, "%d.%d,%d.%d,%s,%s#", swVer.major, swVer.minor, hwVer.major, hwVer.minor, swInfo.info, hwInfo.info);

            printf("info: %s", info);
            send(client, info, sizeof(info), 0);
            memset(info, 0, sizeof(info));
        }
        else if (strncmp(msg, "GET_ADV_OPT", 11) == 0) {
            printf("msg: %s\n", msg);
            char* mens = strtok(msg, " ");
            char* advopt = strtok(NULL, "");
            char optionSend[8];
            memset(optionSend, 0, sizeof(optionSend));
            STUHFL_T_ST25RU3993_Gen2Inventory_Cfg invGen2Cfg = STUHFL_O_ST25RU3993_GEN2INVENTORY_CFG_INIT();
            GetGen2InventoryCfg(&invGen2Cfg);
            STUHFL_T_ST25RU3993_Gen2Protocol_Cfg gen2ProtocolCfg = STUHFL_O_ST25RU3993_GEN2PROTOCOL_CFG_INIT();
            GetGen2ProtocolCfg(&gen2ProtocolCfg);
           // GetFreqProfileInfo(&freqProfile);
            if (strncmp(advopt, "SESSION", 7) == 0) {
                sprintf(optionSend, "%d#", invGen2Cfg.session);
            } else if (strncmp(advopt, "TARGET", 6) == 0) {
                sprintf(optionSend, "%d#", invGen2Cfg.target);
            }
            else if (strncmp(advopt, "TARI", 4) == 0) {
                sprintf(optionSend, "%d#", gen2ProtocolCfg.tari);
            }
            else if (strncmp(advopt, "BLF", 3) == 0) {
                sprintf(optionSend, "%d#", gen2ProtocolCfg.blf);
            }
            printf("Option: %s", optionSend);
            send(client, optionSend, sizeof(optionSend), 0);
            memset(optionSend, 0, sizeof(optionSend));
        }
        else if (strncmp(msg, "SET_REGION", 10) == 0) {
            printf("msg: %s\n", msg);
            char* mens = strtok(msg, " ");
            char* reg = strtok(NULL, "");
            printf("REGION: %s\n", reg);
            fflush(stdout);
            int region = reg - '0';
           
            STUHFL_T_ST25RU3993_Freq_Profile freqProfile = STUHFL_O_ST25RU3993_FREQ_PROFILE_INIT();
            switch (region)
            {
            case 0:
                freqProfile.profile = PROFILE_CUSTOM;
                break;
            case 1:
                freqProfile.profile = PROFILE_EUROPE;
                break;
            case 2:
                freqProfile.profile = PROFILE_USA;
                break;
            case 3:
                freqProfile.profile = PROFILE_JAPAN;
                break;
            case 4:
                freqProfile.profile = PROFILE_CHINA;
                break; 
            case 5:
                freqProfile.profile = PROFILE_CHINA2;
                break;
            default:
                break;
            }
          
            SetFreqProfile(&freqProfile); 
            send(client, "OK#", 3, 0);
        }
        else if (strncmp(msg, "SET_TARI", 8) == 0) {
            printf("msg: %s\n", msg);
            char* mens = strtok(msg, " ");
            char* tari = strtok(NULL, "");
            printf("SET TARI: %s\n", tari);
            fflush(stdout);

            send(client, "OK#", 3, 0);

        }
        else if (strncmp(msg, "SET_BLF", 7) == 0) {
            printf("msg: %s\n", msg);
            char* mens = strtok(msg, " ");
            char* blf = strtok(NULL, "");
            printf("SET BLF: %s\n", blf);
            fflush(stdout);
  
            send(client, "OK#", 3, 0);
        }
        else if (strncmp(msg, "SET_M", 5) == 0) {
            printf("msg: %s\n", msg);
            char* mens = strtok(msg, " ");
            char* m = strtok(NULL, "");
            printf("SET M: %s\n", m);
            fflush(stdout);

            send(client, "OK#", 3, 0);
        }
        else if (strncmp(msg, "SET_Q", 5) == 0) {
            printf("msg: %s\n", msg);
            //setAdvancedOptions(handle, "SET_Q", nuevo);
            send(client, "OK#", 3, 0);

        }
        else if (strncmp(msg, "SET_SESSION", 11) == 0) {
            printf("msg: %s\n", msg);
            char* mens = strtok(msg, " ");
            char* session = strtok(NULL, "");
            printf("SESION: %s\n", session);
            fflush(stdout);
            STUHFL_T_ST25RU3993_Gen2Inventory_Cfg invGen2Cfg = STUHFL_O_ST25RU3993_GEN2INVENTORY_CFG_INIT();
            invGen2Cfg.session = session;
            SetGen2InventoryCfg(&invGen2Cfg);

            send(client, "OK#", 3, 0);
        }
        else if (strncmp(msg, "SET_TARGET", 10) == 0) {
            printf("msg: %s\n", msg);
            char* mens = strtok(msg, " ");
            char* target = strtok(NULL, "");
            printf("SET TARGET: %s\n", target);
            STUHFL_T_ST25RU3993_Gen2Inventory_Cfg invGen2Cfg = STUHFL_O_ST25RU3993_GEN2INVENTORY_CFG_INIT();
            invGen2Cfg.session = target;
            SetGen2InventoryCfg(&invGen2Cfg);
            fflush(stdout);
           
            send(client, "OK#", 3, 0);
        }
        else if (strncmp(msg, "START_READING", 13) == 0) {
       
            printf("msg: %s\n", msg);

            char mensaje[50];
            char epcbin[4];
            char epc[24];

            memset(epc, 0, sizeof(epc));
            memset(mensaje, 0, sizeof(mensaje));

            setupGen2Config(false, true, ANTENNA_1);
            // apply data storage location, where the found TAGs shall be stored
            STUHFL_T_Inventory_Tag tagData[MAX_TAGS_PER_ROUND];

            // Set inventory data and print all found tags
            STUHFL_T_Inventory_Data invData = STUHFL_O_INVENTORY_DATA_INIT();
            invData.tagList = tagData;
            invData.tagListSizeMax = MAX_TAGS_PER_ROUND;
            STUHFL_T_Inventory_Option invOption = STUHFL_O_INVENTORY_OPTION_INIT();

            Gen2_Inventory(&invOption, &invData);
           // printTagList(&invOption, &invData);

            // print transponder information for TagList
            if (invData.tagListSize > 0) { 
                printf("TagListSize: %d\n",invData.tagListSize);
                for (int tagIdx = 0; tagIdx < invData.tagListSize; tagIdx++) {
                  //printf("tagIdx: %d len: %d\n", tagIdx, invData.tagList[tagIdx].epc.len);              
                    for (int i = 0; i < invData.tagList[tagIdx].epc.len; i++) {
                        sprintf(epcbin, "%02x", invData.tagList[tagIdx].epc.data[i]);
                        //printf(epcbin);
                        strcat(epc, epcbin);
                    }
                    //printf("epc: %s\n",epc);
                    sprintf(mensaje, "$%s#", epc);
                    printf("tag para enviar: %s\n",mensaje);
                    memset(epc, 0, sizeof(epc));
                    send(clientRead, mensaje, strlen(mensaje), 0);
                }
            }
            else {
                send(clientRead, "$#", 2, 0);
            }
          
            send(client, "OK#", 3, 0);
        }
        else if (strncmp(msg, "STOP_READING", 13) == 0) {
            printf("msg: %s\n", msg);
           
            send(client, "OK#", 3, 0);
            //startReading = 0;
            //CloseHandle(hilo);
        }
        else if (strncmp(msg, "READ_INFO", 9) == 0) {
            printf("msg: %s\n", msg);
            STUHFL_T_Read readData = STUHFL_O_READ_INIT();
            Gen2_Read(&readData);
            send(client, "OK#", 3, 0);
        }
        else if (strncmp(msg, "WRITE_EPC", 9) == 0) {
            printf("msg: %s\n", msg);
            char EPCBuf[24];
           
            send(client, "OK#", 3, 0);
        }
    }
}


/**
  * @brief          Inventory demo.<br>
  *                 Launch a Gen2 inventory and outputs all detected tags
  *
  * @param[in]      invOption: Gen2 inventory options
  * @param[in]      invData: Gen2 inventory data
  * @param[out]     None
  *
  * @retval         None
  */

void printTagList(STUHFL_T_Inventory_Option* invOption, STUHFL_T_Inventory_Data* invData)
{
    char mensaje[50];
    char epcbin[24];
    char epc[24];

    memset(epc, 0, sizeof(epc));
    memset(epcbin, 0, sizeof(epcbin));
    memset(mensaje, 0, sizeof(mensaje));
    //
    log2Screen(false, false, "\n\n--- Gen2_Inventory Option ---\n");
    log2Screen(false, false, "rssiMode    : %d\n", invOption->rssiMode);
    log2Screen(false, false, "reportMode  : %d\n", invOption->reportOptions);
    log2Screen(false, false, "\n");

    log2Screen(false, false, "--- Round Info ---\n");
    log2Screen(false, false, "tuningStatus: %s\n", invData->statistics.tuningStatus == TUNING_STATUS_UNTUNED ? "UNTUNED" : (invData->statistics.tuningStatus == TUNING_STATUS_TUNING ? "TUNING" : "TUNED"));
    log2Screen(false, false, "roundCnt    : %d\n", invData->statistics.roundCnt);
    log2Screen(false, false, "sensitivity : %d\n", invData->statistics.sensitivity);
    log2Screen(false, false, "Q           : %d\n", invData->statistics.Q);
    log2Screen(false, false, "adc         : %d\n", invData->statistics.adc);
    log2Screen(false, false, "frequency   : %d\n", invData->statistics.frequency);
    log2Screen(false, false, "tagCnt      : %d\n", invData->statistics.tagCnt);
    log2Screen(false, false, "empty Slots : %d\n", invData->statistics.emptySlotCnt);
    log2Screen(false, false, "collisions  : %d\n", invData->statistics.collisionCnt);
    log2Screen(false, false, "preampleErr : %d\n", invData->statistics.preambleErrCnt);
    log2Screen(false, false, "crcErr      : %d\n\n", invData->statistics.crcErrCnt);

    printf("tagListSize: %d\n", invData->tagListSize);
    // print transponder information for TagList
    for (int tagIdx = 0; tagIdx < invData->tagListSize; tagIdx++) {
        log2Screen(false, false, "\n\n--- %03d ---\n", tagIdx + 1);
        log2Screen(false, false, "agc         : %d\n", invData->tagList[tagIdx].agc);
        log2Screen(false, false, "rssiLogI    : %d\n", invData->tagList[tagIdx].rssiLogI);
        log2Screen(false, false, "rssiLogQ    : %d\n", invData->tagList[tagIdx].rssiLogQ);
        log2Screen(false, false, "rssiLinI    : %d\n", invData->tagList[tagIdx].rssiLinI);
        log2Screen(false, false, "rssiLinQ    : %d\n", invData->tagList[tagIdx].rssiLinQ);
        log2Screen(false, false, "pc          : ");
        for (int i = 0; i < MAX_PC_LENGTH; i++) {
            log2Screen(false, false, "%02x ", invData->tagList[tagIdx].pc[i]); 
        }
        log2Screen(false, false, "\nepcLen      : %d\n", invData->tagList[tagIdx].epc.len);
        log2Screen(false, false, "epc         : ");
        //printf("tagIdx: %d\n", tagIdx);
        for (int i = 0; i < invData->tagList[tagIdx].epc.len; i++) {
            printf("tagIdx: %d i: %d\n", tagIdx, i);
            log2Screen(false, false, "%02x ", invData->tagList[tagIdx].epc.data[i]);
            printf("%02x ", invData->tagList[tagIdx].epc.data[i]);
            sprintf(epcbin, "%02x", invData->tagList[tagIdx].epc.data[i]);
            //printf(epcbin);
            strcat(epc, epcbin);
            /*printf(epc);*/
        }
        //printf(epc);
        log2Screen(false, false, "\ntidLen      : %d\n", invData->tagList[tagIdx].tid.len);
        log2Screen(false, false, "tid         : ");
        for (int i = 0; i < invData->tagList[tagIdx].tid.len; i++) {
            log2Screen(false, false, "%02x ", invData->tagList[tagIdx].tid.data[i]);
        }
        sprintf(mensaje, "$%s#", epc);
        memset(epc, 0, sizeof(epc));
        //send(clientRead, mensaje, strlen(mensaje), 0);
    }
    log2Screen(false, true, "\n");
}
