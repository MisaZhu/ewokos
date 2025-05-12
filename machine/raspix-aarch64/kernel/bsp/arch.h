#ifndef ARCH_H
#define ARCH_H

typedef struct {
	uint32_t status;	
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t enable;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
} pic_regs_t;

typedef struct {
	uint32_t status;	
	uint32_t r1;
	uint32_t enable;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
} sic_regs_t;

#endif
