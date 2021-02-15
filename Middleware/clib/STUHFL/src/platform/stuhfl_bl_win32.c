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
 *  @brief
 *
 */

/** @addtogroup Application
  * @{
  */
/** @addtogroup PC_Communication
  * @{
  */

#if defined(WIN32) || defined(WIN64)

//
#include <stdio.h>

#include "stuhfl_dl.h"
#include "stuhfl_bl_win32.h"
#include "stuhfl_err.h"

static STUHFL_T_ParamTypeConnectionRdTimeout rdComTimeout = 4000;
static STUHFL_T_ParamTypeConnectionWrTimeout wrComTimeout = 1000;

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_Connect_Win32(STUHFL_T_DEVICE_CTX *device, char* port, uint32_t br)
{
    HANDLE h;
    DCB dcb;

    STUHFL_T_RET_CODE ret = ERR_GENERIC;

    h = CreateFileA(
            port,
            GENERIC_READ | GENERIC_WRITE, 0, 0,
            OPEN_EXISTING,
            0, /*FILE_FLAG_OVERLAPPED, */
            0);

    // Return error if invalid handle
    if (h == INVALID_HANDLE_VALUE) {
        return ERR_IO;
    }

    // Get default serial port parameters
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        return ERR_IO;
    }

    //
    dcb.DCBlength = sizeof(dcb);                                    // sizeof(DCB)
    dcb.BaudRate = br;                                              // baud rate
    dcb.fBinary = TRUE;                                             // binary mode, no EOF check
    dcb.fParity = FALSE;                                            // enable parity checking
    dcb.fOutxCtsFlow = FALSE;                                       // CTS output flow control
    dcb.fOutxDsrFlow = FALSE;                                       // DSR output flow control
    dcb.fDtrControl = DTR_CONTROL_ENABLE;                           // DTR flow control type
    dcb.fDsrSensitivity = FALSE;                                    // DSR sensitivity
    //DWORD fTXContinueOnXoff:1;                                    // XOFF continues Tx
    //DWORD fOutX: 1;                                               // XON/XOFF out flow control
    //DWORD fInX: 1;                                                // XON/XOFF in flow control
    //DWORD fErrorChar: 1;                                          // enable error replacement
    //DWORD fNull: 1;                                               // enable null stripping
    dcb.fRtsControl = RTS_CONTROL_ENABLE;                           // RTS flow control
    //DWORD fAbortOnError:1;                                        // abort reads/writes on error
    //DWORD fDummy2:17;                                             // reserved
    //WORD wReserved;                                               // not currently used
    //WORD XonLim;                                                  // transmit XON threshold
    //WORD XoffLim;                                                 // transmit XOFF threshold
    dcb.ByteSize = 8;                                               // number of bits/byte, 4-8
    dcb.Parity = NOPARITY;                                          // 0-4=no,odd,even,mark,space
    dcb.StopBits = ONESTOPBIT;                                      // 0,1,2 = 1, 1.5, 2
    //char XonChar;                                                 // Tx and Rx XON character
    //char XoffChar;                                                // Tx and Rx XOFF character
    //char ErrorChar;                                               // error replacement character
    //char EofChar;                                                 // end of input character
    //char EvtChar;                                                 // received event character
    //WORD wReserved1;                                              // reserved; do not use

    // setup comm state
    if (!SetCommState(h, &dcb)) {
        return ERR_IO;
    }

    COMMTIMEOUTS ct;
    if (!GetCommTimeouts(h, &ct)) {
        return ERR_IO;
    }
    // define ReadTimeOuts
    ct.ReadTotalTimeoutConstant = rdComTimeout;
    ct.ReadTotalTimeoutMultiplier = 0;
    ct.ReadIntervalTimeout = 0;
    // define WriteTimeOuts
    ct.WriteTotalTimeoutMultiplier = 0;
    ct.WriteTotalTimeoutConstant = wrComTimeout;
    //set Timeouts
    if (!SetCommTimeouts(h, &ct)) {
        return ERR_IO;
    }

    // Set internal driver buffer to max for Rx and 4k for Tx
    SetupComm(h, 0xFFFFFFFF, 0x1000);

    // finally assign handle as device context
    *device = (STUHFL_T_DEVICE_CTX)h;
    return ERR_NONE;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_Reset_Win32(STUHFL_T_DEVICE_CTX *device, STUHFL_T_RESET resetType)
{
    if (resetType == STUHFL_RESET_TYPE_CLEAR_COMM) {
        // Abort all outstandig r/w operations and clear buffers
        if (!PurgeComm(*device, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
            return ERR_IO;
        }

        // Clear Port
        COMSTAT comStat;
        DWORD   dwErrors;
        // Get and clear current errors on the port.
        if (!ClearCommError(*device, &dwErrors, &comStat)) {
            return ERR_IO;
        }

        return ERR_NONE;
    }
    return ERR_PARAM;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_Disconnect_Win32(STUHFL_T_DEVICE_CTX *device)
{
    //
    if ((*device == INVALID_HANDLE_VALUE) || (*device == NULL)) {
        return ERR_PARAM;
    }

    if (!CloseHandle(*device)) {
        return ERR_IO;
    }

    *device = NULL;

    return ERR_NONE;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_SndRaw_Win32(STUHFL_T_DEVICE_CTX *device, uint8_t *data, uint16_t dataLen)
{
    //
    if ((*device == INVALID_HANDLE_VALUE) || (*device == NULL)) {
        return ERR_PARAM;
    }

    //
    STUHFL_T_RET_CODE ret = STUHFL_F_Reset_Win32(device, STUHFL_RESET_TYPE_CLEAR_COMM);
    if (ret != ERR_NONE) {
        return ret;
    }

    //
    DWORD       dwWritten = 0;
    bool        success = false;

#if 0
    // Asynchron Write operation. NOTE: Use FILE_FLAG_OVERLAPPED with create handle when using this
    OVERLAPPED  osWrite = { 0 };
    // Create this write operation's OVERLAPPED structure's hEvent.
    osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osWrite.hEvent == NULL) {
        // error creating overlapped event handle (Issue write)
    } else {
        // Do overlapped write operation
        if (!WriteFile(*device, data, dataLen, &dwWritten, &osWrite)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                // WriteFile failed, but isn't delayed. Report error and abort.
            } else {
                // write is pending.
                switch (WaitForSingleObject(osWrite.hEvent, wrComTimeout)) {
                // OVERLAPPED structure's event has been signaled.
                case WAIT_OBJECT_0:
                    if (GetOverlappedResult(*device, &osWrite, &dwWritten, FALSE)) {
                        // write operation completed successfully.
                        success = true;
                    }
                    break;
                default:
                    // An error has occurred in WaitForSingleObject.
                    // This usually indicates a problem with the
                    // OVERLAPPED structure's event handle.
                    break;
                }
            }
        } else {
            // write operation completed successfully.
            success = true;
        }
        CloseHandle(osWrite.hEvent);
    }
#else
    // write operation completed successfully.
    if (WriteFile(*device, data, dataLen, &dwWritten, NULL)) {
        if(dataLen == dwWritten);
        success = true;
    }
#endif

    if (!success) {
        ret = ERR_IO;
    }

    return ret;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_RcvRaw_Win32(STUHFL_T_DEVICE_CTX *device, uint8_t *data, uint16_t *dataLen)
{
    //
    bool            success = false;
    DWORD           dwBytesRead = 0;
    DWORD           nNumberOfBytesToRead = *dataLen;
    *dataLen = 0;

    if ((*device == INVALID_HANDLE_VALUE) || (*device == NULL)) {
        return ERR_PARAM;
    }

#if 0
    // Asynchron Read operation. NOTE: Use FILE_FLAG_OVERLAPPED with create handle when using this
    OVERLAPPED      osReader = { 0 };
    // Create the overlapped event. Must be closed before exiting
    // to avoid a handle leak.
    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osReader.hEvent == NULL) {
        // error creating overlapped event handle
    } else {
        //
        // Do overlapped read operation
        //
        if (!ReadFile(*device, data, nNumberOfBytesToRead, &dwBytesRead, &osReader)) {
            // read not delayed?
            if (GetLastError() != ERROR_IO_PENDING) {
                // Error in communications; report it.
            } else {
                // wait for completion of read
                switch (WaitForSingleObject(osReader.hEvent, rdComTimeout)) {
                // Read completed.
                case WAIT_OBJECT_0:
                    if (!GetOverlappedResult(*device, &osReader, &dwBytesRead, FALSE)) {
                        break;
                    } else {
                        //read successfull
                        success = true;
                        break;
                    }
                case WAIT_TIMEOUT: //timeout occourred
                    break;
                default:
                    // Error in the WaitForSingleObject; abort.
                    // This indicates a problem with the OVERLAPPED structure's event handle.
                    break;
                }
            }
        } else {
            success = true;
        }
        CloseHandle(osReader.hEvent);
    }
#else
    success = ReadFile(*device, data, nNumberOfBytesToRead, &dwBytesRead, NULL);
#endif

    //
    if (success) {
        *dataLen = (uint16_t)dwBytesRead;
    }

    return success ? ERR_NONE : ERR_IO;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_SetDTR_Win32(STUHFL_T_DEVICE_CTX *device, uint8_t dtrValue)
{
    //
    if ((*device == INVALID_HANDLE_VALUE) || (*device == NULL)) {
        return ERR_PARAM;
    }

    //
    return EscapeCommFunction(*device, dtrValue ? SETDTR : CLRDTR) ? ERR_NONE : ERR_IO;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_GetDTR_Win32(STUHFL_T_DEVICE_CTX *device, uint8_t *dtrValue)
{
    //
    if ((*device == INVALID_HANDLE_VALUE) || (*device == NULL)) {
        return ERR_PARAM;
    }

    DCB dcb;

    // Get default serial port parameters
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(*device, &dcb)) {
        return ERR_IO;
    }

    //
    *dtrValue = (uint8_t)dcb.fDtrControl;
    return ERR_NONE;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_SetRTS_Win32(STUHFL_T_DEVICE_CTX *device, uint8_t rtsValue)
{
    //
    if ((*device == INVALID_HANDLE_VALUE) || (*device == NULL)) {
        return ERR_PARAM;
    }

    //
    return EscapeCommFunction(*device, rtsValue ? SETRTS: CLRRTS) ? ERR_NONE : ERR_IO;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_GetRTS_Win32(STUHFL_T_DEVICE_CTX *device, uint8_t *rtsValue)
{
    //
    if ((*device == INVALID_HANDLE_VALUE) || (*device == NULL)) {
        return ERR_PARAM;
    }

    DCB dcb;

    // Get default serial port parameters
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(*device, &dcb)) {
        return ERR_IO;
    }

    //
    *rtsValue = (uint8_t)dcb.fRtsControl;
    return ERR_NONE;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_SetTimeouts_Win32(STUHFL_T_DEVICE_CTX *device, uint32_t rdTimeout, uint32_t wrTimeout)
{
    //
    if ((*device == INVALID_HANDLE_VALUE) || (*device == NULL)) {
        return ERR_PARAM;
    }

    COMMTIMEOUTS ct;

    // define ReadTimeOuts
    ct.ReadTotalTimeoutConstant = rdTimeout;
    ct.ReadTotalTimeoutMultiplier = 0;
    ct.ReadIntervalTimeout = 0;
    // define WriteTimeOuts
    ct.WriteTotalTimeoutMultiplier = 0;
    ct.WriteTotalTimeoutConstant = wrTimeout;

    //set Timeouts
    if (!SetCommTimeouts(*device, &ct)) {
        return ERR_IO;
    }

    rdComTimeout = rdTimeout;
    wrComTimeout = wrTimeout;

    return ERR_NONE;
}

// --------------------------------------------------------------------------
STUHFL_T_RET_CODE STUHFL_F_GetTimeouts_Win32(STUHFL_T_DEVICE_CTX *device, uint32_t *rdTimeout, uint32_t *wrTimeout)
{
    *rdTimeout = rdComTimeout;
    *wrTimeout = wrComTimeout;
    return ERR_NONE;
}

#endif

/**
  * @}
  */
/**
  * @}
  */
