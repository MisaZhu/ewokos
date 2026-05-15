#ifndef X86_IO_H
#define X86_IO_H

#include <stdint.h>

static inline void x86_io_wait(void) {
	__asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

static inline uint8_t x86_inb(uint16_t port) {
	uint8_t value;
	__asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

static inline void x86_outb(uint16_t port, uint8_t value) {
	__asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

#endif
