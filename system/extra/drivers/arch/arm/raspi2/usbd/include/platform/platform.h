/******************************************************************************
*	platform/platform.h
*	 by Alex Chadwick
*
*	A light weight implementation of the USB protocol stack fit for a simple
*	driver.
*
*	platform/platform.h contains definitions pertaining to the platform that
*	the system will run on.
******************************************************************************/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <configuration.h>
#include <types.h>

/**
	\brief Allocates memory of specified length. 

	Allocates memory of length at least length to the driver and returns the 
	address. If MEM_INTERNAL_MANAGER is defined, then all platforms must
	provide an implementation which calls MemoryReserve. Can return NULL on
	error.
*/
void* MemoryAllocate(u32 length);
/**
	\brief Deallocates memory of specified address, previously allocated by
	MemoryAllocate. 

	Deallocates memory of the specified address that was previously allocated
	by MemoryAllocate. If MEM_INTERNAL_MANAGER is defined, then all platforms
	must provide an implementation. Calling with an address not received from
	MemoryAllocate produces undefined results.
*/
void MemoryDeallocate(void* address);
/**
	\brief Notifies the system of memory usage. 

	Notifies the parent system of an unavoidable memory usage. This is 
	typically used for memory mapped IO systems, in which certain addresses
	have special meaning. It is up to the parent system to implement whatever
	must be done. The return value should be a virtual address that maps to 
	the requested physical address, or NULL on error. If MEM_NO_RESERVE is 
	defined, a dummy implementation is created.
*/
void* MemoryReserve(u32 length, void* physicalAddress);
/**
	\brief Copies chunks of memory. 

	Copies length bytes from source to destinatoin. If either source or 
	destination are null, should not copy anything.
*/
void MemoryCopy(void* destination, void* source, u32 length);
void MemorySet(void* destination, u8 v, u32 length);

#ifdef NO_LOG
#define LOG(x)
#define LOGL(x, len) 
#define LOGF(x, ...) 
#define LOGFL(x, len, ...) 
#else
/**
	\brief Notifies the user of progress. 

	Notifies the parent system of progress loading the driver. Messages may be 
	displayed to a semi-technically competant user.
*/
void LogPrint(const char* message, u32 messageLength);
/**
	\brief Notifies the user of progress. 

	Prints our a formatted string. Uses all the formtting options in printf. 
	Implemented in platform.c, by calling LogPrint. Messages truncated to 160
	characters.
*/
void LogPrintF(char* format, u32 formatLength, ...);

#define LOG(x) (LogPrint(x, sizeof(x)))
#define LOGL(x, len) (LogPrint(x, len))
#define LOGF(x, ...) (LogPrintF(x, sizeof(x), __VA_ARGS__))
#define LOGFL(x, len, ...) (LogPrintF(x, len, __VA_ARGS__))
#endif
#ifdef DEBUG
#define LOG_DEBUG(x) LOG(x)
#define LOG_DEBUGL(x, len) LOGL(x, len)
#define LOG_DEBUGF(x, ...) LOGF(x, __VA_ARGS__)
#define LOG_DEBUGFL(x, len, ...) LOGFL(x, len, __VA_ARGS__)
#else
#define LOG_DEBUG(x) 
#define LOG_DEBUGL(x, len) 
#define LOG_DEBUGF(x, ...) 
#define LOG_DEBUGFL(x, len, ...) 
#endif

/**
	\brief Turns on the USB host controller.

	Notifies the parent system that the USB contorller now requires power. 
*/
Result PowerOnUsb();
/**
	\brief Turns on the USB host controller.

	Notifies the parent system that the USB contorller no longer requires power. 
*/
void PowerOffUsb();

/**
	\brief Delays for delay microseconds.

	Delays for a number of microseconds. 	
*/
void MicroDelay(u32 delay);


#ifdef ARM
#	ifdef ARM_V6
#		include "arm/armv6.h"
#		ifdef BROADCOM_2835
#		include "arm/broadcom2835.h"
#		endif // BROADCOM_2835
#	else
#	error Unrecognised ARM Version
#	endif // ARM_V6
#else
#error Unrecognised Processor Family
#endif // ARM

#ifdef __cplusplus
}
#endif

#endif
