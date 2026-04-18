/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _SDL_config_ewokos_h
#define _SDL_config_ewokos_h

/* General platform specific identifiers */
#include "SDL_platform.h"

/* Comment this if you want to build without any C library requirements */
#define HAVE_LIBC 1
#if HAVE_LIBC

/* Useful headers */
/* #undef HAVE_ALLOCA_H */
#define HAVE_SYS_TYPES_H 1
#define HAVE_STDIO_H 1
#define STDC_HEADERS 1
#define HAVE_STDLIB_H 1
#define HAVE_STDARG_H 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_CTYPE_H 1
#define HAVE_MATH_H 1
/* #undef HAVE_ICONV_H */
/* #undef HAVE_SIGNAL_H */
/* #undef HAVE_ALTIVEC_H */
/* #undef HAVE_PTHREAD_NP_H */
/* #undef HAVE_LIBUDEV_H */
/* #undef HAVE_DBUS_DBUS_H */

/* C library functions */
#define HAVE_MALLOC 1
#define HAVE_CALLOC 1
#define HAVE_REALLOC 1
#define HAVE_FREE 1
/* #undef HAVE_ALLOCA */
/* #undef HAVE_GETENV */
/* #undef HAVE_SETENV */
/* #undef HAVE_PUTENV */
/* #undef HAVE_UNSETENV */
#define HAVE_QSORT 1
#define HAVE_ABS 1
#define HAVE_BCOPY 1
#define HAVE_MEMSET 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMCMP 1
#define HAVE_STRLEN 1
/* #undef HAVE_STRLCPY */
/* #undef HAVE_STRLCAT */
/* #undef HAVE_STRDUP */
/* #undef HAVE__STRREV */
/* #undef HAVE__STRUPR */
/* #undef HAVE__STRLWR */
/* #undef HAVE_INDEX */
/* #undef HAVE_RINDEX */
#define HAVE_STRCHR 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
/* #undef HAVE_ITOA */
/* #undef HAVE__LTOA */
/* #undef HAVE__UITOA */
/* #undef HAVE__ULTOA */
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
/* #undef HAVE__I64TOA */
/* #undef HAVE__UI64TOA */
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
//#define HAVE_STRTOD 1
#define HAVE_ATOI 1
#define HAVE_ATOF 1
#define HAVE_STRCMP 1
#define HAVE_STRNCMP 1
/* #undef HAVE__STRICMP */
#define HAVE_STRCASECMP 1
/* #undef HAVE__STRNICMP */
#define HAVE_STRNCASECMP 1
/* #undef HAVE_SSCANF */
#define HAVE_VSSCANF 1
/* #undef HAVE_SNPRINTF */
#define HAVE_VSNPRINTF 1
/* #undef HAVE_M_PI */
#define HAVE_ATAN 1
#define HAVE_ATAN2 1
#define HAVE_ACOS 1
#define HAVE_ASIN 1
/*#define HAVE_CEIL 1*/
#define HAVE_COPYSIGN 1
#define HAVE_COS 1
#define HAVE_COSF 1
#define HAVE_FABS 1
#define HAVE_FLOOR 1
#define HAVE_LOG 1
#define HAVE_POW 1
#define HAVE_SCALBN 1
#define HAVE_SIN 1
#define HAVE_SINF 1
#define HAVE_SQRT 1
/* #undef HAVE_FSEEKO */
/* #undef HAVE_FSEEKO64 */
/* #undef HAVE_SIGACTION */
/* #undef HAVE_SA_SIGACTION */
/* #undef HAVE_SETJMP */
/* #undef HAVE_NANOSLEEP */
/* #undef HAVE_SYSCONF */
/* #undef HAVE_SYSCTLBYNAME */
/* #undef HAVE_CLOCK_GETTIME */
/* #undef HAVE_GETPAGESIZE */
/* #undef HAVE_MPROTECT */
/* #undef HAVE_ICONV */
/* #undef HAVE_PTHREAD_SETNAME_NP */
/* #undef HAVE_PTHREAD_SET_NAME_NP */
/* #undef HAVE_SEM_TIMEDWAIT */

#else
#define HAVE_STDARG_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#endif /* HAVE_LIBC */

#ifdef __GNUC__
#define HAVE_GCC_SYNC_LOCK_TEST_AND_SET 1
#endif

#define HAVE_NES

/* SDL internal assertion support */
/* #undef SDL_DEFAULT_ASSERT_LEVEL */

/* Allow disabling of core subsystems */
#define SDL_ATOMIC_DISABLED 1
/* #undef SDL_AUDIO_DISABLED */
#define SDL_JOYSTICK_DISABLED 1
#define SDL_CPUINFO_DISABLED 1
/* #undef SDL_EVENTS_DISABLED */
/* #undef SDL_FILE_DISABLED */
#define SDL_HAPTIC_DISABLED 1
#define SDL_LOADSO_DISABLED 1
/* #undef SDL_RENDER_DISABLED */

#define SDL_THREADS_DISABLED 1
//#define SDL_THREAD_EWOKOS 1 

/*#define SDL_TIMERS_DISABLED 1*/
#define SDL_TIMER_EWOKOS  1

/* #undef SDL_VIDEO_DISABLED */
/* #undef SDL_POWER_DISABLED */
/* #undef SDL_FILESYSTEM_DISABLED */

/* Disable the joystick driver for ewokos */
/* #undef SDL_JOYSTICK_EWOKOS */

/* Enable various audio drivers */
#define SDL_AUDIO_DRIVER_EWOKOS  1

/* Enable various video drivers */
#define SDL_VIDEO_DRIVER_EWOKOS  1

/* Enable system filesystem support */
#define SDL_FILESYSTEM_DUMMY  1

#define SDL_VIDEO_RENDER_EWOKOS 1

#if defined(__cplusplus)
extern "C" {
#endif

/* Interrupt handler, to be called from IRQ handler function */
void SDL_Interrupt_Handler();

#if defined(__cplusplus)
}
#endif

#endif /* _SDL_config_ewokos_h */
