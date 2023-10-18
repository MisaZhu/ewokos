/* src/config.h.  Generated from config.h.in by configure.  */
/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/config.h.in                                              *
 * Created:     2002-12-16 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2002-2019 Hampa Hug <hampa@hampa.ch>                     *
 *****************************************************************************/

/*****************************************************************************
 * This program is free software. You can redistribute it and / or modify it *
 * under the terms of the GNU General Public License version 2 as  published *
 * by the Free Software Foundation.                                          *
 *                                                                           *
 * This program is distributed in the hope  that  it  will  be  useful,  but *
 * WITHOUT  ANY   WARRANTY,   without   even   the   implied   warranty   of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  General *
 * Public License for more details.                                          *
 *****************************************************************************/


#ifndef PCE_CONFIG_H
#define PCE_CONFIG_H 1


#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif


#ifndef _NETBSD_SOURCE
#define _NETBSD_SOURCE 1
#endif


//#define PCE_LARGE_FILE 1

#ifdef PCE_LARGE_FILE
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#endif


#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDINT_H 1
//#define HAVE_TERMIOS_H 1
#define HAVE_UNISTD_H 1
/* #undef HAVE_LINUX_IF_TUN_H */
/* #undef HAVE_LINUX_TCP_H */
#define HAVE_SYS_IOCTL_H 1
//#define HAVE_SYS_POLL_H 1
//#define HAVE_SYS_SOCKET_H 1
/* #undef HAVE_SYS_SOUNDCARD_H */
#define HAVE_SYS_STAT_H 1
//#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1

//#define HAVE_FSEEKO 1
//#define HAVE_FTRUNCATE 1
//#define HAVE_FUTIMES 1
//#define HAVE_USLEEP 1
//#define HAVE_NANOSLEEP 1
#define HAVE_SLEEP 1
//#define HAVE_GETTIMEOFDAY 1

#define PCE_VERSION_MAJ 0
#define PCE_VERSION_MIN 2
#define PCE_VERSION_MIC 2
#define PCE_VERSION_STR "20210526-16d1dc72"

/* #undef PCE_HOST_LINUX */
/* #undef PCE_HOST_WINDOWS */
/* #undef PCE_HOST_SUNOS */
/* #undef PCE_HOST_NETBSD */

/* #undef PCE_HOST_IA32 */
/* #undef PCE_HOST_PPC */
/* #undef PCE_HOST_SPARC */

#define PCE_DIR_ETC "NONE/etc"

#define PCE_BUILD_IBMPC 1

/* #undef PCE_ENABLE_X11 */

//#define PCE_ENABLE_SDL 0
/* #undef PCE_ENABLE_SDL1 */
//#define PCE_ENABLE_SDL2 0

/* #undef PCE_ENABLE_READLINE */

/* #undef PCE_ENABLE_TUN */

//#define PCE_ENABLE_CHAR_POSIX 0
/* #undef PCE_ENABLE_CHAR_PPP */
//#define PCE_ENABLE_CHAR_PTY 0
/* #undef PCE_ENABLE_CHAR_SLIP */
//#define PCE_ENABLE_CHAR_TCP 0
//#define PCE_ENABLE_CHAR_TIOS 0
/* #undef PCE_ENABLE_CHAR_WINCOM */

/* #undef PCE_ENABLE_SOUND_OSS */

/* directory separator */
#define PCE_DIR_SEP '/'

#define stderr stdout
#endif
