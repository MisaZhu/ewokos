/******************************************************************************
*	configuration.h
*	 by Alex Chadwick
*
*	A light weight implementation of the USB protocol stack fit for a simple
*	driver.
*
*	configuration.h contains definitions for all optional components
*	The makefile defines three main categories of definitions:
*		CONFIG: Whether or not this is a DEBUG driver
*		TARGET: The target system
*		TYPE:	What sort of driver to compile (e.g. standalone)
*****************************************************************************/

// Check we have a CONFIG. Valid choices are DEBUG and FINAL. If neither of
// of these are specified, CONFIG_ERROR will be. If not, the haven't used the
// makefile.
#if defined DEBUG
#elif defined FINAL
#elif defined CONFIG_ERROR
#	error Please specify the CONFIG as either DEBUG or FINAL (default)
#else
#	error Please ensure you compile the driver with the makefile provided
#endif

// Check we have a target. This should either be RPI1|RPI2 or NONE.
// If neither of these is specified, TARGET_ERROR will be. 
// If not, the haven't used the makefile.
#if defined TARGET_RPI1
	// Compiling for the Raspberry Pi (model B). 
	// This is an ARM1176JZF-S, running ARMv6.
	// The chip is a Broadcom 2835 with a Designware OTG Core, mapped to 
	// physical address 0x20980000
#	define ARM
#	define ARM_V6
#	define ENDIAN_LITTLE
#	define BROADCOM_2835
#	define HCD_DESIGNWARE_20
//#	define HCD_DESIGNWARE_BASE ((void*)0x20980000)
//#	define _RASPI_MMIO_BASE 0xc0000000
#	define HCD_DESIGNWARE_BASE ((void*)(_v_mmio_base + 0x0980000))

#elif defined TARGET_RPI2
	// Compiling for the Raspberry Pi 2 (model B). 
	// This is an Cortex-A7, running ARMv7.
	// The chip is a Broadcom 2835 with a Designware OTG Core, mapped to 
	// physical address 0x3F980000
#	define ARM
#	define ARM_V6
#	define ENDIAN_LITTLE
#	define BROADCOM_2835
#	define HCD_DESIGNWARE_20
//#	define _RASPI_MMIO_BASE 0xc0000000
#	define HCD_DESIGNWARE_BASE ((void*)(_v_mmio_base + 0x0980000))
#elif defined TARGET_NONE
	// Compiling for no target architecture. This will rapidly run into errors.
#elif defined TARGET_ERROR
#	error Please specify the TARGET as either RPI1 or RPI2 or NONE (default)
#else
#	error Please ensure you compile the driver with the makefile provided
#endif


#if defined TYPE_STANDALONE
	// Disables all logging
#	define NO_LOG
	// Disables external memory management
#	define MEM_INTERNAL_MANAGER
	// Disables external memory reservation
#	define MEM_NO_RESERVE
#elif defined TYPE_LOWLEVEL
	// Disables external memory management
#	define MEM_INTERNAL_MANAGER
#	define MEM_NO_RESERVE
#elif defined TYPE_DRIVER
#elif defined TYPE_ERROR
#	error Please specify the TYPE as either STANDALONE, LOWLEVEL (default) or DRIVER
#else
#	error Please ensure you compile the driver with the makefile provided
#endif
