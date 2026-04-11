/* jconfig.h for ewokos - generated for embedded system */
#ifndef JCONFIG_H
#define JCONFIG_H

/* errno values for embedded systems */
#ifndef EINVAL
#define EINVAL 22
#endif
#ifndef ENOMEM
#define ENOMEM 12
#endif

#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_UNISTD_H 1

#define NEED_SHORT_EXIF_NAMES 1
#define NEED_SYS_TYPES_H 1

#define JPEG_LIB_VERSION 70
#define JPEG_LIB_VERSION_MAJOR 7
#define JPEG_LIB_VERSION_MINOR 0

#define INPUT_BUF_SIZE 4096
#define OUTPUT_BUF_SIZE 4096

#define MAX_ALLOC_CHUNK 65536L

#define D_MAXJSAMPLE 255
#define CENTERJSAMPLE 128
#define RANGEJSAMPLE 128

#define BITS_IN_JSAMPLE 8
#define JSAMPLE_T unsigned char
#define UINT8_T unsigned char
#define UINT16_T unsigned short
#define INT16_T short
#define INT32_T int

/* Word size - required by jchuff.c */
#ifdef __aarch64__
#define JPEG_INT16 0
#define JPEG_INT32 1
typedef unsigned char JOCTET;
typedef unsigned short I16;
typedef unsigned int I32;
typedef unsigned char JUINT8;
typedef unsigned short JUINT16;
typedef unsigned int JUINT32;
#define SIZEOF(size) 1
#define SIZEOF_SIZE_T 8
#else
#define JPEG_INT16 1
#define JPEG_INT32 0
typedef unsigned char JOCTET;
typedef unsigned short I16;
typedef unsigned int I32;
typedef unsigned char JUINT8;
typedef unsigned short JUINT16;
typedef unsigned int JUINT32;
#define SIZEOF(size) 1
#endif

#ifdef __aarch64__
#define ALIGNMENT 8
#else
#define ALIGNMENT 4
#endif

/* Use inline keyword */
#ifndef INLINE
#define INLINE inline
#endif

/* FALLTHROUGH macro for switch statements */
#ifndef FALLTHROUGH
#define FALLTHROUGH ;
#endif

/* Package info for version string */
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "libjpeg"
#endif
#ifndef VERSION
#define VERSION "6.2"
#endif
#ifndef BUILD
#define BUILD "embedded"
#endif

#endif
