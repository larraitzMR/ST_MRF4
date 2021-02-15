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
#if !defined __STUHFL_PLATFORM_H
#define __STUHFL_PLATFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

// --------------------------------------------------------------------------
#if defined(__GNUC__) || defined(__clang__)
// --------------------------------------------------------------------------
#define CALL_CONV
#define CALL_CONV_STD

//#define STUHFL_DLL_API
#define STUHFL_DLL_API_EXPORT       1
#if defined(STUHFL_DLL_API_EXPORT)
#define STUHFL_DLL_API          __attribute__((visibility("default")))
#else
#define STUHFL_DLL_API          __attribute__((visibility("hidden")))
#endif
#define STUHFL_DEPRECATED       __attribute__((deprecated))
// --------------------------------------------------------------------------
#elif defined(_MSC_VER)
// --------------------------------------------------------------------------
#define CALL_CONV                   __cdecl
#define CALL_CONV_STD               __stdcall

//#define STUHFL_DLL_API
#define STUHFL_DLL_API_EXPORT       1
#if defined(STUHFL_DLL_API_EXPORT)
#define STUHFL_DLL_API          __declspec(dllexport)
#else
#define STUHFL_DLL_API          __declspec(dllimport)
#endif
#define STUHFL_DEPRECATED       __declspec(deprecated)
// --------------------------------------------------------------------------
#else
// --------------------------------------------------------------------------
#define CALL_CONV
#define STUHFL_DLL_API
#define STUHFL_DEPRECATED
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------

#ifdef IGNORE_DEPRECATED_WARNING
#undef STUHFL_DEPRECATED
#define STUHFL_DEPRECATED
#endif


// --------------------------------------------------------------------------
#if defined(WIN32) || defined(WIN64)
// --------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers   
#include <windows.h>

STUHFL_DLL_API void CALL_CONV usleep(__int64 usec);
// --------------------------------------------------------------------------
#elif defined(POSIX)
// --------------------------------------------------------------------------
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

#define TRUE                        1
#define FALSE                       0
#define INVALID_HANDLE_VALUE        ((unsigned)(-1))
#define HANDLE                      int
// --------------------------------------------------------------------------
#else
// --------------------------------------------------------------------------
#endif

//
STUHFL_DLL_API uint32_t CALL_CONV getMilliCount(void);
STUHFL_DLL_API uint32_t CALL_CONV getMilliSpan(uint32_t firstTime);



#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __STUHFL_PLATFORM_H
