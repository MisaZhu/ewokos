#ifndef TYPES_H
#define TYPES_H


#ifndef __ASSEMBLER__

//#include <stdint.h>
#ifndef NULL
#define NULL ((void*) 0)
#endif

/* boolean */
#ifndef bool
typedef int bool;
#endif

/* integer types */

typedef __signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;

typedef __signed char i8_t;
typedef unsigned char u8_t;
typedef short i16_t;
typedef unsigned short u16_t;
typedef int i32_t;
typedef unsigned int u32_t;

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

#define CMD_MAX 128
#define FULL_NAME_MAX 256
#define SHORT_NAME_MAX 32
#define DEV_NAME_MAX 64

#define ALIGN_DOWN(x, alignment) ((x) & ~(alignment - 1))
#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

typedef void (*free_func_t) (void* p);
typedef void* (*malloc_func_t) (uint32_t size);

#endif

#endif
