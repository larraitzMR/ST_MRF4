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

#include "main.h"

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
  //conexion con el soft

   //while principal enviar y recibir mensajes
}
