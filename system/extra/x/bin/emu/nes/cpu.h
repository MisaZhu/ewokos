/*
 *   This file is part of nes_emu.
 *   Copyright (c) 2019 Franz Flasch.
 *
 *   nes_emu is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   nes_emu is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with nes_emu.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include <memory_controller.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cpu_registers_s
{
	/* Accumulator 1 byte wide */
	uint8_t A;

	/* X register 1 byte wide */
	uint8_t X;

	/* Y register 1 byte wide */
	uint8_t Y;

	/* Program Counter 2 byte wide */
	uint16_t PC;

	/* Stack Pointer 1 byte wide */
	uint8_t S;

	/* Status Register 1 byte wide */
	uint8_t P;

} cpu_registers_t;

typedef struct nes_cpu_s 
{

	uint64_t num_cycles;
	uint16_t op_address;
	uint8_t  op_value;
	uint8_t  additional_cycles;

	/* CPU Registers */
	cpu_registers_t regs;

	/* New interface */
	nes_mem_td *nes_mem;

} nes_cpu_t;

void nes_cpu_reset(nes_cpu_t *nes_cpu);
void nes_cpu_init(nes_cpu_t *nes_cpu, nes_mem_td *nes_mem);
uint32_t nes_cpu_run(nes_cpu_t *nes_cpu);
uint32_t nes_cpu_nmi(nes_cpu_t *nes_cpu);
void nes_cpu_print_state(nes_cpu_t *nes_cpu, uint8_t opcode);
#ifdef __cplusplus
}
#endif
#endif
