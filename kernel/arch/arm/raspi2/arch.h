#ifndef ARCH_H
#define ARCH_H

#include <types.h>

typedef struct {
	uint32_t irq_basic_pending;
	uint32_t irq_gpu_pending1;
	uint32_t irq_gpu_pending2;
	uint32_t fiq_control;
	uint32_t irq_gpu_enable1;
	uint32_t irq_gpu_enable2;
	uint32_t irq_basic_enable;
	uint32_t irq_gpu_disable1;
	uint32_t irq_gpu_disable2;
	uint32_t irq_basic_disable;
} pic_regs_t;

#endif
