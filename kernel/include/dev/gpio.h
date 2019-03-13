/*

	Part of the Raspberry-Pi Bare Metal Tutorials
	Copyright (c) 2013-2015, Brian Sidebotham
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice,
		this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
		this list of conditions and the following disclaimer in the
		documentation and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef GPIO_H
#define GPIO_H

#include <types.h>

typedef enum {
	GPIO0 = 0,
	GPIO1,
	GPIO2,
	GPIO3,
	GPIO4,
	GPIO5,
	GPIO6,
	GPIO7,
	GPIO8,
	GPIO9,
	GPIO10 = 10,
	GPIO11,
	GPIO12,
	GPIO13,
	GPIO14,
	GPIO15,
	GPIO16,
	GPIO17,
	GPIO18,
	GPIO19,
	GPIO20 = 20,
	GPIO21,
	GPIO22,
	GPIO23,
	GPIO24,
	GPIO25,
	GPIO26,
	GPIO27,
	GPIO28,
	GPIO29,
	GPIO30 = 30,
	GPIO31,
	GPIO32,
	GPIO33,
	GPIO34,
	GPIO35,
	GPIO36,
	GPIO37,
	GPIO38,
	GPIO39,
	GPIO40 = 40,
	GPIO41,
	GPIO42,
	GPIO43,
	GPIO44,
	GPIO45,
	GPIO46,
	GPIO47,
	GPIO48,
	GPIO49,
	GPIO50 = 50,
	GPIO51,
	GPIO52,
	GPIO53
} GPIOPinT;

/** The GPIO Peripheral is described in section 6 of the BCM2835 Peripherals
	documentation.

	There are 54 general-purpose I/O (GPIO) lines split into two banks. All
	GPIO pins have at least two alternative functions within BCM. The
	alternate functions are usually peripheral IO and a single peripheral
	may appear in each bank to allow flexibility on the choice of IO voltage.
	Details of alternative functions are given in section 6.2. Alternative
	Function Assignments.

	The GPIO peripheral has three dedicated interrupt lines. These lines are
	triggered by the setting of bits in the event detect status register. Each
	bank has its’ own interrupt line with the third line shared between all
	bits.

	The Alternate function table also has the pull state (pull-up/pull-down)
	which is applied after a power down. */

typedef struct {
	uint32_t    GPFSEL0;
	uint32_t    GPFSEL1;
	uint32_t    GPFSEL2;
	uint32_t    GPFSEL3;
	uint32_t    GPFSEL4;
	uint32_t    GPFSEL5;
	const uint32_t    Reserved0;
	uint32_t    GPSET0;
	uint32_t    GPSET1;
	const uint32_t    Reserved1;
	uint32_t    GPCLR0;
	uint32_t    GPCLR1;
	const uint32_t    Reserved2;
	uint32_t    GPLEV0;
	uint32_t    GPLEV1;
	const uint32_t    Reserved3;
	uint32_t    GPEDS0;
	uint32_t    GPEDS1;
	const uint32_t    Reserved4;
	uint32_t    GPREN0;
	uint32_t    GPREN1;
	const uint32_t    Reserved5;
	uint32_t    GPFEN0;
	uint32_t    GPFEN1;
	const uint32_t    Reserved6;
	uint32_t    GPHEN0;
	uint32_t    GPHEN1;
	const uint32_t    Reserved7;
	uint32_t    GPLEN0;
	uint32_t    GPLEN1;
	const uint32_t    Reserved8;
	uint32_t    GPAREN0;
	uint32_t    GPAREN1;
	const uint32_t    Reserved9;
	uint32_t    GPAFEN0;
	uint32_t    GPAFEN1;
	const uint32_t    Reserved10;
	uint32_t    GPPUD;
	uint32_t    GPPUDCLK0;
	uint32_t    GPPUDCLK1;
	const uint32_t    Reserved11;
} GPIOT;


typedef enum {
	IO_LO = 0,
	IO_HI,
	IO_ON,
	IO_OFF,
	IO_UNKNOWN,
} GPIOValueT;

extern void gpioInit();
extern GPIOT* getGPIO(void);
extern GPIOValueT gpioGet(GPIOPinT pin);
extern void gpioHi(GPIOPinT pin);
extern void gpioLo(GPIOPinT pin);
extern void gpioSet(GPIOPinT pin, GPIOValueT value);
extern void gpioToggle(GPIOPinT pin);

#endif
