/******************************************************************************
*	platform/none/byteorder.h
*	 by Alex Chadwick
*
*	A light weight implementation of the USB protocol stack fit for a simple
*	driver.
*
*	platform/none/byteorder.h contains generic definitions for changing the 
*	byte order of halfs to match the processor.
******************************************************************************/
#include <configuration.h>
#include <types.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined ENDIAN_BIG
/**
	Converts a number from the machine's byte order to big endian or back.
*/
#define EndianBigWord(x) x
/**
	Converts a number from the machine's byte order to little endian or back.
*/
#define EndianLittleWord(x) ({ u32 t = x; (t >> 24) & 0xff | (t >> 8) & 0xff00 | (t << 8) & 0xff00 | (t << 24) & 0xff000000; })
/**
	Converts a number from the machine's byte order to big endian or back.
*/
#define EndianBigHalf(x) x
/**
	Converts a number from the machine's byte order to little endian or back.
*/
#define EndianLittleHalf(x) ({ u16 t = x; (t >> 8) & 0xff | (t << 8) & 0xff00; })
#elif defined ENDIAN_LITTLE
/**
	Converts a number from the machine's byte order to big endian or back.
*/
#define EndianBigWord(x) ({ u32 t = x; (t >> 24) & 0xff | (t >> 8) & 0xff00 | (t << 8) & 0xff0000 | (t << 24) & 0xff000000; })
/**
	Converts a number from the machine's byte order to little endian or back.
*/
#define EndianLittleWord(x) x
/**
	Converts a number from the machine's byte order to big endian or back.
*/
#define EndianBigHalf(x) ({ u16 t = x; (t >> 8) & 0xff | (t << 8) & 0xff00; })
/**
	Converts a number from the machine's byte order to little endian or back.
*/
#define EndianLittleHalf(x) x
#else
#error Endianness not specified.
#endif

#ifdef __cplusplus
}
#endif
