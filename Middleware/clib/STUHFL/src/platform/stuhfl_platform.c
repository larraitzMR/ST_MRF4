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

//
#include "stuhfl_platform.h"

// - WINDOWS ----------------------------------------------------------------
#if defined(WIN32) || defined(WIN64)

void CALL_CONV usleep(__int64 usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}

STUHFL_DLL_API uint32_t CALL_CONV getMilliCount()
{
    return GetTickCount();
}

// - POSIX ------------------------------------------------------------------
#elif defined(POSIX)

STUHFL_DLL_API uint32_t CALL_CONV getMilliCount()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return (uint32_t)((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

// - OTHER PLATFORMS --------------------------------------------------------
#else

#endif


STUHFL_DLL_API uint32_t CALL_CONV getMilliSpan(uint32_t firstTime)
{
    uint32_t m = getMilliCount();
    uint32_t span = 0;
    if (m > firstTime) {
        span = m - firstTime;
    } else {
        span = (0xFFFFFFFF - firstTime) + m;
    }
    return span;
}

/**
  * @}
  */
/**
  * @}
  */
