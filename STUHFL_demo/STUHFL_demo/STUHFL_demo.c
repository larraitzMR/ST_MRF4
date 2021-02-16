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

#include "STUHFL_demo.h"

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

//
bool test4CompatibleVersion();

//
#define SND_BUFFER_SIZE         (UART_RX_BUFFER_SIZE)   /* HOST SND buffer based on FW RCV buffer */
#define RCV_BUFFER_SIZE         (UART_TX_BUFFER_SIZE)   /* HOST RCV buffer based on FW SND buffer */

// globals
#define LOG_BUF_SIZE    0xFFFF
static char logBuf[LOG_BUF_SIZE];
static int logBufPos = 0;

// local functions
static bool getComPort(char* szPort);

/**
  * @brief  The application entry point.<br><br>
  *         Adequate define needs to be set up to run related demo <br>
  *         INTERACTIVE_DEMO:               demo_Interactive(), <b>enabled by default</b><br>
  *         PLAYGROUND:                     demo_Playground() <br>
  *         DUMP_DEMO:                      demo_DumpRegisters() + demo_DumpRwdCfg() <br>
  *         GEN2_SINGLE_TAG_RD_WR_DEMO:     demo_evalAPI_Gen2RdWr() <br>
  *         MULTI_TAG_INVENTORY_DEMO:       demo_InventoryRunner() <br>
  *
  * @retval 0: Ended without error
  */
int main (int argc, char * argv[])
{
    //
    uint32_t v = STUHFL_F_GetLibVersion();
    printf("Welcome to the ST-UHF-L demo V%d.%d.%d.%d\n", ((v & 0xFF000000) >> 24), ((v & 0x00FF0000) >> 16), ((v & 0x0000FF00) >> 8), ((v & 0x000000FF) >> 0));

    //
    STUHFL_T_DEVICE_CTX device = 0;

    uint8_t sndData[SND_BUFFER_SIZE];
    uint8_t rcvData[RCV_BUFFER_SIZE];

    char comPort[64];
    getComPort(comPort);

    // For windows this will print into the "Output" window of Visual Studio
    // For Linux this will print to "syslog".
    // NOTE: The syslog file could be become rather big, please take care and limit the size on your system if needed.
    //       eg: use "tail -f /var/log/syslog"
    demo_LogLowLevel(true);

    //
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

    // test if used board is compatible
    if(!test4CompatibleVersion()) {
        printf("\nNo valid FW version detected.\n\nPlease check your board, the connection and\nverify that your FW is up to date.\n\nPress any key to terminate ..\n");
        while (!_kbhit());
        return 0;
    }

    // Showcase basic functionality
    demo_GetVersion();

#if DEBUG_OAD
    // This demonstrates the usage of the OAD pins of the ST25RU3993.
    // Attach a digital tracer to the OAD pins and you could see the Tx modulation and Rx subcarrier outputs on OAD and OAD2
    STUHFL_T_ST25RU3993_Register reg = STUHFL_T_ST25RU3993_REGISTER();
    reg.addr = 0x10;
    reg.data = 0;
    GetRegister(&reg);
    reg.data |= 0x80;
    SetRegister(&reg);
#endif

#if defined(POSIX) || PLAYGROUND
    printf("Launching Inventory Runner for 2000 rounds..\n");
    demo_Playground();
#endif

#if DUMP_DEMO
    demo_DumpRegisters();
    demo_DumpRwdCfg();
#endif

#if GEN2_SINGLE_TAG_RD_WR_DEMO
    demo_evalAPI_Gen2RdWr();
#endif

#if MULTI_TAG_INVENTORY_DEMO
    printf("Launching Inventory Runner with infinite loop ...\n");
    demo_InventoryRunner(0, false);             // run inventory with infinite loop for multiple tags
#endif

#if defined(INTERACTIVE_DEMO)
    printf("\nEnter interactive menu (y/n) ?\n");
    int key = _getch();
    if (key != 'n' && key != 'N' && key != 27 /*ESC*/) {
        demo_Interactive();
    }
#endif

    //
    printf("\n\npress any key to terminate ..\n");
    while (!_kbhit());

    //
    demo_LogLowLevel(false);
    return 0;
}

/**
  * @brief      Try to read board FW and HW version information and compare if compatible with current lib version
  *
  * @retval     true: Compatible
  * @retval     false: Not compatible
  */
bool test4CompatibleVersion()
{
    uint32_t v = STUHFL_F_GetLibVersion();
    //STUHFL_T_Version lowestSwVer = { MAJOR, MINOR, MICRO, BUILD };
    STUHFL_T_Version lowestSwVer = { (uint8_t)((v & 0xFF000000) >> 24), (uint8_t)((v & 0x00FF0000) >> 16), (uint8_t)((v & 0x0000FF00) >> 8), (uint8_t)((v & 0x000000FF) >> 0) };
    STUHFL_T_Version lowestHwVer = { 1,1,0,0 };

    bool compatible = false;
    STUHFL_T_Version swVer, hwVer;
    if( ERR_NONE == GetBoardVersion(&swVer, &hwVer)) {
        // test versions
        if (   (memcmp(&lowestSwVer, &swVer, sizeof(STUHFL_T_Version)) <= 0)
                && (memcmp(&lowestHwVer, &hwVer, sizeof(STUHFL_T_Version)) <= 0)) {
            compatible = true;
        }
    }
    return compatible;
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
    } else if (nPorts > 1) {
        printf("Found the following com ports: ");
        for (int i = 0; i < nPorts; i++) {
            printf("%d, ", ports[i]);
        }
        printf("\nPlease specify the port to use: ");
        port = _getch();
        port -= '0';
    } else {
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

#if defined(POSIX)
/**
  * @brief      POSIX key press implementation.
  *
  * @retval     true: key has been pressed
  * @retval     false: otherwise
  */
bool _kbhit()
{
    struct termios term;
    tcgetattr(0, &term);

    struct termios term2 = term;
    term2.c_lflag &= (tcflag_t)~ICANON;
    tcsetattr(0, TCSANOW, &term2);

    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);

    tcsetattr(0, TCSANOW, &term);

    return byteswaiting > 0;
}

/**
  * @brief      POSIX implementation for getting terminal input
  *
  * @retval     true: key has been pressed
  * @retval     false: otherwise
  */
char _getch(void)
{
    char buf = 0;
    struct termios old = { 0 };
    fflush(stdout);
    if (tcgetattr(0, &old) < 0) {
        perror("tcsetattr()");
    }
    old.c_lflag &= (tcflag_t)~ICANON;
    old.c_lflag &= (tcflag_t)~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0) {
        perror("tcsetattr ICANON");
    }
    if (read(0, &buf, 1) < 0) {
        perror("read()");
    }
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0) {
        perror("tcsetattr ~ICANON");
    }
    printf("%c\n", buf);
    return buf;
}
#endif
