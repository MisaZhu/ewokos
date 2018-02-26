#ifndef TYPES_H
#define TYPES_H

#ifndef __ASSEMBLER__

#ifndef NULL
#define NULL ((void*) 0)
#endif

/* boolean */
typedef int bool;

/* integer types */
typedef __signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef uint32_t uintptr_t;

/* size type */
typedef unsigned int size_t;

/* boolean constants */
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#define KB 1024
#define MB (1024*KB)
#define GB (1024*MB)

#endif

#endif
