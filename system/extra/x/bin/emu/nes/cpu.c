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

#include <stdio.h>
#include <string.h>

#include <nes.h>
#include <cpu.h>

#if DEBUG_CPU 
#define debug_print(...) printf(__VA_ARGS__) 
#else
#define debug_print(...) {}
#endif

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_UNUSED    0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_NEGATIVE  0x80

inline static uint8_t memory_read_byte(nes_cpu_t *nes_cpu, uint16_t addr) 
{
    return (uint8_t)cpu_memory_access(nes_cpu->nes_mem, addr, 0, ACCESS_READ_BYTE);
}

inline static uint16_t memory_read_word(nes_cpu_t *nes_cpu, uint16_t addr) {
    return cpu_memory_access(nes_cpu->nes_mem, addr, 0, ACCESS_READ_WORD);
}

inline static void memory_write_byte(nes_cpu_t *nes_cpu, uint16_t addr, uint8_t data) 
{
    cpu_memory_access(nes_cpu->nes_mem, addr, data, ACCESS_WRITE_BYTE);
}

inline static void memory_write_word(nes_cpu_t *nes_cpu, uint16_t addr, uint16_t data) {
    cpu_memory_access(nes_cpu->nes_mem, addr, data, ACCESS_WRITE_WORD);
}

inline static void check_page_cross_x(nes_cpu_t *nes_cpu)
{
    if((nes_cpu->op_address & 0xff) < nes_cpu->regs.X)
    {
        nes_cpu->additional_cycles += 1;
    }
}

inline static void check_page_cross_y(nes_cpu_t *nes_cpu)
{
    if((nes_cpu->op_address & 0xff) < nes_cpu->regs.Y)
    {
        nes_cpu->additional_cycles += 1;
    }
}

inline static void check_new_page(nes_cpu_t *nes_cpu)
{
    if ((nes_cpu->op_address >> 8) != (nes_cpu->regs.PC >> 8)) {
        nes_cpu->additional_cycles += 1;
    }  
}

inline static void cpu_checknz(nes_cpu_t *nes_cpu, uint8_t n)
{
    if((n >> 7) & 1) { nes_cpu->regs.P |= FLAG_NEGATIVE; } else { nes_cpu->regs.P &= ~FLAG_NEGATIVE; }
    if(n == 0)       { nes_cpu->regs.P |= FLAG_ZERO; }     else { nes_cpu->regs.P &= ~FLAG_ZERO; }
}

inline static void cpu_modify_flag(nes_cpu_t *nes_cpu, uint8_t flag, int value) {
    if(value) { nes_cpu->regs.P |= flag; }
    else      { nes_cpu->regs.P &= ~flag; }
}

inline static void cpu_stack_push_byte(nes_cpu_t *nes_cpu, uint8_t data) 
{ 
    memory_write_byte(nes_cpu, 0x100 + nes_cpu->regs.S, data); 
    nes_cpu->regs.S -= 1; 
}

inline static void cpu_stack_push_word(nes_cpu_t *nes_cpu, uint16_t data)    
{ 
    memory_write_word(nes_cpu, 0x0ff + nes_cpu->regs.S, data); 
    nes_cpu->regs.S -= 2; 
}

inline static uint8_t cpu_stack_pop_byte(nes_cpu_t *nes_cpu)              
{ 
    nes_cpu->regs.S += 1; 
    return memory_read_byte(nes_cpu, 0x100 + nes_cpu->regs.S); 
}

inline static uint16_t cpu_stack_pop_word(nes_cpu_t *nes_cpu)              
{
    nes_cpu->regs.S += 2; 
    return memory_read_word(nes_cpu, 0x0ff + nes_cpu->regs.S); 
}

inline static void cpu_addressing_implied(nes_cpu_t *nes_cpu) 
{ 
}

inline static void cpu_addressing_accumulator(nes_cpu_t *nes_cpu) 
{ 
}

inline static void cpu_addressing_immediate(nes_cpu_t *nes_cpu) 
{
    nes_cpu->op_value = memory_read_byte(nes_cpu, nes_cpu->regs.PC);
    nes_cpu->regs.PC++;
}

inline static void cpu_addressing_zeropage(nes_cpu_t *nes_cpu) 
{
    nes_cpu->op_address = memory_read_byte(nes_cpu, nes_cpu->regs.PC);
    nes_cpu->op_value = memory_read_byte(nes_cpu, nes_cpu->op_address);
    nes_cpu->regs.PC++;
}

inline static void cpu_addressing_zeropage_x(nes_cpu_t *nes_cpu) 
{
    nes_cpu->op_address = (memory_read_byte(nes_cpu, nes_cpu->regs.PC) + nes_cpu->regs.X) & 0xff;
    nes_cpu->op_value = memory_read_byte(nes_cpu, nes_cpu->op_address);
    nes_cpu->regs.PC++;
}

inline static void cpu_addressing_zeropage_y(nes_cpu_t *nes_cpu) 
{
    nes_cpu->op_address = (memory_read_byte(nes_cpu, nes_cpu->regs.PC) + nes_cpu->regs.Y) & 0xff;
    nes_cpu->op_value = memory_read_byte(nes_cpu, nes_cpu->op_address);
    nes_cpu->regs.PC++;
}

inline static void cpu_addressing_absolute(nes_cpu_t *nes_cpu) 
{
    nes_cpu->op_address = memory_read_word(nes_cpu, nes_cpu->regs.PC);
    nes_cpu->op_value = memory_read_byte(nes_cpu, nes_cpu->op_address);
    nes_cpu->regs.PC += 2;
}

inline static void cpu_addressing_absolute_x(nes_cpu_t *nes_cpu) 
{
    nes_cpu->op_address = memory_read_word(nes_cpu, nes_cpu->regs.PC) + nes_cpu->regs.X;
    nes_cpu->op_value = memory_read_byte(nes_cpu, nes_cpu->op_address);
    nes_cpu->regs.PC += 2;
}

inline static void cpu_addressing_absolute_y(nes_cpu_t *nes_cpu) 
{
    nes_cpu->op_address = (memory_read_word(nes_cpu, nes_cpu->regs.PC) + nes_cpu->regs.Y) & 0xffff;
    nes_cpu->op_value = memory_read_byte(nes_cpu, nes_cpu->op_address);
    nes_cpu->regs.PC += 2;
}

inline static void cpu_addressing_relative(nes_cpu_t *nes_cpu) 
{
    nes_cpu->op_address = memory_read_byte(nes_cpu, nes_cpu->regs.PC);
    nes_cpu->regs.PC++;
    if(nes_cpu->op_address & 0x80) { nes_cpu->op_address -= 0x100; }
    nes_cpu->op_address += nes_cpu->regs.PC;
}

inline static void cpu_addressing_indirect(nes_cpu_t *nes_cpu) 
{
    uint16_t arg_addr = memory_read_word(nes_cpu, nes_cpu->regs.PC);

    /* 6502 Bug */
    if((arg_addr & 0xff) == 0xff) {
        /* 'Simulate' Bug in 6502 */
        nes_cpu->op_address = (memory_read_byte(nes_cpu, arg_addr & 0xff00) << 8) + memory_read_byte(nes_cpu, arg_addr);
    } else {
        nes_cpu->op_address = memory_read_word(nes_cpu, arg_addr);
    }
    nes_cpu->regs.PC += 2;
}

inline static void cpu_addressing_indirect_x(nes_cpu_t *nes_cpu) 
{
    uint8_t arg_addr = memory_read_byte(nes_cpu, nes_cpu->regs.PC);
    nes_cpu->op_address = (memory_read_byte(nes_cpu, (arg_addr + nes_cpu->regs.X + 1) & 0xff) << 8) | memory_read_byte(nes_cpu, (arg_addr + nes_cpu->regs.X) & 0xff);
    nes_cpu->op_value = memory_read_byte(nes_cpu, nes_cpu->op_address);
    nes_cpu->regs.PC++;
}

inline static void cpu_addressing_indirect_y(nes_cpu_t *nes_cpu) 
{
    uint8_t arg_addr = memory_read_byte(nes_cpu, nes_cpu->regs.PC);
    nes_cpu->op_address = (((memory_read_byte(nes_cpu, (arg_addr + 1) & 0xff) << 8) | memory_read_byte(nes_cpu, arg_addr)) + nes_cpu->regs.Y) & 0xffff;
    nes_cpu->op_value = memory_read_byte(nes_cpu, nes_cpu->op_address);
    nes_cpu->regs.PC++;
}

/* ALU instructions */
inline static void cpu_ora(nes_cpu_t *nes_cpu) 
{
    nes_cpu->regs.A |= nes_cpu->op_value;
    cpu_checknz(nes_cpu, nes_cpu->regs.A);
}

inline static void cpu_and(nes_cpu_t *nes_cpu)
{
    nes_cpu->regs.A &= nes_cpu->op_value;
    cpu_checknz(nes_cpu, nes_cpu->regs.A);
}

inline static void cpu_eor(nes_cpu_t *nes_cpu) 
{
    nes_cpu->regs.A ^= nes_cpu->op_value;
    cpu_checknz(nes_cpu, nes_cpu->regs.A);
}

inline static void cpu_asl(nes_cpu_t *nes_cpu) 
{
    cpu_modify_flag(nes_cpu, FLAG_CARRY, nes_cpu->op_value & 0x80);
    nes_cpu->op_value <<= 1;
    cpu_checknz(nes_cpu, nes_cpu->op_value);
    memory_write_byte(nes_cpu, nes_cpu->op_address, nes_cpu->op_value);
}

inline static void cpu_asla(nes_cpu_t *nes_cpu) 
{
    cpu_modify_flag(nes_cpu, FLAG_CARRY, nes_cpu->regs.A & 0x80);
    nes_cpu->regs.A <<= 1;
    cpu_checknz(nes_cpu, nes_cpu->regs.A);
}

inline static void cpu_rol(nes_cpu_t *nes_cpu) 
{
    uint8_t tmp = nes_cpu->regs.P & FLAG_CARRY;
    cpu_modify_flag(nes_cpu, FLAG_CARRY, nes_cpu->op_value & 0x80);
    nes_cpu->op_value <<= 1;
    nes_cpu->op_value |= tmp ? 1 : 0;
    memory_write_byte(nes_cpu, nes_cpu->op_address, nes_cpu->op_value);
    cpu_checknz(nes_cpu, nes_cpu->op_value);
}

inline static void cpu_rola(nes_cpu_t *nes_cpu) 
{
    uint8_t tmp = nes_cpu->regs.P & FLAG_CARRY;
    cpu_modify_flag(nes_cpu, FLAG_CARRY, nes_cpu->regs.A & 0x80);
    nes_cpu->regs.A <<= 1;
    nes_cpu->regs.A |= tmp ? 1 : 0;
    cpu_checknz(nes_cpu, nes_cpu->regs.A);
}

inline static void cpu_ror(nes_cpu_t *nes_cpu) 
{
    uint8_t tmp = nes_cpu->regs.P & FLAG_CARRY;
    cpu_modify_flag(nes_cpu, FLAG_CARRY, nes_cpu->op_value & 0x01);
    nes_cpu->op_value >>= 1;
    nes_cpu->op_value |= (tmp ? 1 : 0) << 7;
    memory_write_byte(nes_cpu, nes_cpu->op_address, nes_cpu->op_value);
    cpu_checknz(nes_cpu, nes_cpu->op_value);
}

inline static void cpu_rora(nes_cpu_t *nes_cpu) 
{
    uint8_t tmp = nes_cpu->regs.P & FLAG_CARRY;
    cpu_modify_flag(nes_cpu, FLAG_CARRY, nes_cpu->regs.A & 0x01);
    nes_cpu->regs.A >>= 1;
    nes_cpu->regs.A |= (tmp ? 1 : 0) << 7;
    cpu_checknz(nes_cpu, nes_cpu->regs.A);
}

inline static void cpu_lsr(nes_cpu_t *nes_cpu) 
{
    cpu_modify_flag(nes_cpu, FLAG_CARRY, nes_cpu->op_value & 0x01);
    nes_cpu->op_value >>= 1;
    memory_write_byte(nes_cpu, nes_cpu->op_address, nes_cpu->op_value);
    cpu_checknz(nes_cpu, nes_cpu->op_value);
}

inline static void cpu_lsra(nes_cpu_t *nes_cpu) 
{
    cpu_modify_flag(nes_cpu, FLAG_CARRY, nes_cpu->regs.A & 0x01);
    nes_cpu->regs.A >>= 1;
    cpu_checknz(nes_cpu, nes_cpu->regs.A);
}

inline static void cpu_adc(nes_cpu_t *nes_cpu) 
{
    uint16_t tmp;
    tmp = nes_cpu->op_value + nes_cpu->regs.A + ((nes_cpu->regs.P & FLAG_CARRY) ? 1 : 0);
    cpu_modify_flag(nes_cpu, FLAG_CARRY, tmp & 0xff00);
    cpu_modify_flag(nes_cpu, FLAG_OVERFLOW, ((nes_cpu->op_value ^ tmp) & (nes_cpu->regs.A ^ tmp)) & 0x80);
    nes_cpu->regs.A = (uint8_t)(tmp & 0xff);
    cpu_checknz(nes_cpu, nes_cpu->regs.A);
}

inline static void cpu_sbc(nes_cpu_t *nes_cpu) 
{
    uint16_t tmp;
    tmp = nes_cpu->regs.A - nes_cpu->op_value - (1 - ((nes_cpu->regs.P & FLAG_CARRY) ? 1 : 0));
    cpu_modify_flag(nes_cpu, FLAG_CARRY, (tmp & 0xff00) == 0);
    cpu_modify_flag(nes_cpu, FLAG_OVERFLOW, ((nes_cpu->regs.A ^ nes_cpu->op_value) & (nes_cpu->regs.A ^ tmp)) & 0x80);
    nes_cpu->regs.A = (uint8_t)(tmp & 0xff);
    cpu_checknz(nes_cpu, nes_cpu->regs.A);
}

/* Branching */
inline static void cpu_bmi(nes_cpu_t *nes_cpu) 
{ 
    if(nes_cpu->regs.P & FLAG_NEGATIVE) 
    { 
        check_new_page(nes_cpu);
        nes_cpu->regs.PC = nes_cpu->op_address; 
        nes_cpu->additional_cycles += 1;
    }
}

inline static void cpu_bcs(nes_cpu_t *nes_cpu) 
{ 
    if(nes_cpu->regs.P & FLAG_CARRY) 
    {
        check_new_page(nes_cpu);
        nes_cpu->regs.PC = nes_cpu->op_address; 
        nes_cpu->additional_cycles += 1;
    }
}

inline static void cpu_beq(nes_cpu_t *nes_cpu) 
{
    if(nes_cpu->regs.P & FLAG_ZERO) 
    {
        check_new_page(nes_cpu);
        nes_cpu->regs.PC = nes_cpu->op_address; 
        nes_cpu->additional_cycles += 1;
    }
}

inline static void cpu_bvs(nes_cpu_t *nes_cpu) 
{
    if(nes_cpu->regs.P & FLAG_OVERFLOW) 
    {
        check_new_page(nes_cpu);
        nes_cpu->regs.PC = nes_cpu->op_address; 
        nes_cpu->additional_cycles += 1;
    }
}

inline static void cpu_bpl(nes_cpu_t *nes_cpu)
{
    if(!(nes_cpu->regs.P & FLAG_NEGATIVE)) 
    {
        check_new_page(nes_cpu);
        nes_cpu->regs.PC = nes_cpu->op_address; 
        nes_cpu->additional_cycles += 1;
    }
}

inline static void cpu_bcc(nes_cpu_t *nes_cpu)
{
    if(!(nes_cpu->regs.P & FLAG_CARRY))
    {
        check_new_page(nes_cpu);
        nes_cpu->regs.PC = nes_cpu->op_address; 
        nes_cpu->additional_cycles += 1;
    }
}

inline static void cpu_bne(nes_cpu_t *nes_cpu)
{
    if(!(nes_cpu->regs.P & FLAG_ZERO)) 
    {
        check_new_page(nes_cpu);
        nes_cpu->regs.PC = nes_cpu->op_address; 
        nes_cpu->additional_cycles += 1;
    }
}

inline static void cpu_bvc(nes_cpu_t *nes_cpu)
{
    if(!(nes_cpu->regs.P & FLAG_OVERFLOW)) 
    {
        check_new_page(nes_cpu);
        nes_cpu->regs.PC = nes_cpu->op_address; 
        nes_cpu->additional_cycles += 1;
    }
}

/* Compare */
inline static void cpu_bit(nes_cpu_t *nes_cpu) 
{
    cpu_modify_flag(nes_cpu, FLAG_OVERFLOW, nes_cpu->op_value & 0x40);
    cpu_modify_flag(nes_cpu, FLAG_NEGATIVE, nes_cpu->op_value & 0x80);
    cpu_modify_flag(nes_cpu, FLAG_ZERO, !(nes_cpu->op_value & nes_cpu->regs.A));
}

inline static void cpu_cmp(nes_cpu_t *nes_cpu) 
{
    int tmpc = nes_cpu->regs.A - nes_cpu->op_value;
    cpu_modify_flag(nes_cpu, FLAG_CARRY, tmpc >= 0);
    cpu_checknz(nes_cpu, (uint8_t)tmpc);
}

inline static void cpu_cpx(nes_cpu_t *nes_cpu) 
{
    int tmpc = nes_cpu->regs.X - nes_cpu->op_value;
    cpu_modify_flag(nes_cpu, FLAG_CARRY, tmpc >= 0);
    cpu_checknz(nes_cpu, (uint8_t)tmpc);
}

inline static void cpu_cpy(nes_cpu_t *nes_cpu) 
{
    int tmpc = nes_cpu->regs.Y - nes_cpu->op_value;
    cpu_modify_flag(nes_cpu, FLAG_CARRY, tmpc >= 0);
    cpu_checknz(nes_cpu, (uint8_t)tmpc);
}

inline static void cpu_clc(nes_cpu_t *nes_cpu) { cpu_modify_flag(nes_cpu, FLAG_CARRY, 0); }
inline static void cpu_cli(nes_cpu_t *nes_cpu) { cpu_modify_flag(nes_cpu, FLAG_INTERRUPT, 0); }
inline static void cpu_cld(nes_cpu_t *nes_cpu) { cpu_modify_flag(nes_cpu, FLAG_DECIMAL, 0); }
inline static void cpu_clv(nes_cpu_t *nes_cpu) { cpu_modify_flag(nes_cpu, FLAG_OVERFLOW, 0); }
inline static void cpu_sec(nes_cpu_t *nes_cpu) { cpu_modify_flag(nes_cpu, FLAG_CARRY, 1); }
inline static void cpu_sei(nes_cpu_t *nes_cpu) { cpu_modify_flag(nes_cpu, FLAG_INTERRUPT, 1); }
inline static void cpu_sed(nes_cpu_t *nes_cpu) { cpu_modify_flag(nes_cpu, FLAG_DECIMAL, 1); }

inline static void cpu_dec(nes_cpu_t *nes_cpu) {
    nes_cpu->op_value -= 1;
    memory_write_byte(nes_cpu, nes_cpu->op_address, nes_cpu->op_value);
    cpu_checknz(nes_cpu, nes_cpu->op_value);
}

inline static void cpu_dex(nes_cpu_t *nes_cpu) {
    nes_cpu->regs.X--;
    cpu_checknz(nes_cpu, nes_cpu->regs.X);
}

inline static void cpu_dey(nes_cpu_t *nes_cpu) {
    nes_cpu->regs.Y--;
    cpu_checknz(nes_cpu, nes_cpu->regs.Y);
}

inline static void cpu_inc(nes_cpu_t *nes_cpu) {
    nes_cpu->op_value += 1;
    memory_write_byte(nes_cpu, nes_cpu->op_address, nes_cpu->op_value);
    cpu_checknz(nes_cpu, nes_cpu->op_value);
}

inline static void cpu_inx(nes_cpu_t *nes_cpu) {
    nes_cpu->regs.X++;
    cpu_checknz(nes_cpu, nes_cpu->regs.X);
}

inline static void cpu_iny(nes_cpu_t *nes_cpu) {
    nes_cpu->regs.Y++;
    cpu_checknz(nes_cpu, nes_cpu->regs.Y);
}

inline static void cpu_lda(nes_cpu_t *nes_cpu) 
{
    nes_cpu->regs.A = nes_cpu->op_value; 
    cpu_checknz(nes_cpu, nes_cpu->regs.A); 
}

inline static void cpu_ldx(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.X = nes_cpu->op_value; 
    cpu_checknz(nes_cpu, nes_cpu->regs.X); 
}
inline static void cpu_ldy(nes_cpu_t *nes_cpu) 
{
    nes_cpu->regs.Y = nes_cpu->op_value; 
    cpu_checknz(nes_cpu, nes_cpu->regs.Y); 
}

inline static void cpu_axs(nes_cpu_t *nes_cpu) 
{ 
    memory_write_byte(nes_cpu, nes_cpu->op_address, nes_cpu->regs.A & nes_cpu->regs.X); 
}

inline static void cpu_sta(nes_cpu_t *nes_cpu) 
{ 
    memory_write_byte(nes_cpu, nes_cpu->op_address, nes_cpu->regs.A); 
}

inline static void cpu_stx(nes_cpu_t *nes_cpu) 
{ 
    memory_write_byte(nes_cpu, nes_cpu->op_address, nes_cpu->regs.X); 
}

inline static void cpu_sty(nes_cpu_t *nes_cpu) 
{ 
    memory_write_byte(nes_cpu, nes_cpu->op_address, nes_cpu->regs.Y); 
}

inline static void cpu_nop(nes_cpu_t *nes_cpu) {}

inline static void cpu_pha(nes_cpu_t *nes_cpu) 
{ 
    cpu_stack_push_byte(nes_cpu, nes_cpu->regs.A); 
}

inline static void cpu_php(nes_cpu_t *nes_cpu) 
{ 
    cpu_stack_push_byte(nes_cpu, nes_cpu->regs.P | 0x30); 
}

inline static void cpu_pla(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.A = cpu_stack_pop_byte(nes_cpu); 
    cpu_checknz(nes_cpu, nes_cpu->regs.A); 
}

inline static void cpu_plp(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.P = (cpu_stack_pop_byte(nes_cpu) & 0xef) | 0x20; 
}

inline static void cpu_rts(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.PC = cpu_stack_pop_word(nes_cpu) + 1; 
}
inline static void cpu_rti(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.P = cpu_stack_pop_byte(nes_cpu) | FLAG_UNUSED; 
    nes_cpu->regs.PC = cpu_stack_pop_word(nes_cpu); 
}

inline static void cpu_jmp(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.PC = nes_cpu->op_address; 
}

inline static void cpu_jsr(nes_cpu_t *nes_cpu) 
{ 
    cpu_stack_push_word(nes_cpu, nes_cpu->regs.PC - 1); 
    nes_cpu->regs.PC = nes_cpu->op_address; 
}

inline static void cpu_brk(nes_cpu_t *nes_cpu) 
{
    cpu_stack_push_word(nes_cpu, nes_cpu->regs.PC - 1);
    cpu_stack_push_byte(nes_cpu, nes_cpu->regs.P);
    nes_cpu->regs.P |= FLAG_UNUSED | FLAG_BREAK;
    nes_cpu->regs.PC = memory_read_word(nes_cpu, 0xfffa);
}

inline static void cpu_tax(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.X = nes_cpu->regs.A; 
    cpu_checknz(nes_cpu, nes_cpu->regs.X); 
}

inline static void cpu_tay(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.Y = nes_cpu->regs.A; 
    cpu_checknz(nes_cpu, nes_cpu->regs.Y); 
}

inline static void cpu_txa(nes_cpu_t *nes_cpu) 
{
    nes_cpu->regs.A = nes_cpu->regs.X; 
    cpu_checknz(nes_cpu, nes_cpu->regs.A); 
}

inline static void cpu_tya(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.A = nes_cpu->regs.Y; 
    cpu_checknz(nes_cpu, nes_cpu->regs.A); 
}

inline static void cpu_tsx(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.X = nes_cpu->regs.S; cpu_checknz(nes_cpu, nes_cpu->regs.X); 
}

inline static void cpu_txs(nes_cpu_t *nes_cpu) 
{ 
    nes_cpu->regs.S = nes_cpu->regs.X; 
}

inline uint64_t cpu_clock(nes_cpu_t *nes_cpu) 
{
    return nes_cpu->num_cycles;
}

void nes_cpu_print_state(nes_cpu_t *nes_cpu, uint8_t opcode)
{
    debug_print("PC:%04X opcode:%02X A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lld\n", 
            nes_cpu->regs.PC, 
            opcode, 
            nes_cpu->regs.A, 
            nes_cpu->regs.X, 
            nes_cpu->regs.Y, 
            nes_cpu->regs.P, 
            nes_cpu->regs.S, 
            nes_cpu->num_cycles);
}

uint32_t nes_cpu_run(nes_cpu_t *nes_cpu) 
{
    uint8_t opcode = 0;
    uint32_t cycles = 0;
    nes_cpu->additional_cycles = 0;

    opcode = memory_read_byte(nes_cpu, nes_cpu->regs.PC);

    nes_cpu_print_state(nes_cpu, opcode);

    nes_cpu->regs.PC++;

    switch(opcode) {
        case 0x00: cpu_addressing_implied(nes_cpu);     cpu_brk(nes_cpu);  cycles += 7; break;
        case 0x01: cpu_addressing_indirect_x(nes_cpu);  cpu_ora(nes_cpu);  cycles += 6; break;
        /* 0x02 HLT */
        case 0x03: cpu_addressing_indirect_x(nes_cpu);  cpu_asl(nes_cpu);  cpu_ora(nes_cpu);  cycles += 8; break;
        case 0x04: cpu_addressing_zeropage(nes_cpu);    cpu_nop(nes_cpu);  cycles += 3; break;
        case 0x05: cpu_addressing_zeropage(nes_cpu);    cpu_ora(nes_cpu);  cycles += 3; break;
        case 0x06: cpu_addressing_zeropage(nes_cpu);    cpu_asl(nes_cpu);  cycles += 5; break;
        case 0x07: cpu_addressing_zeropage(nes_cpu);    cpu_asl(nes_cpu);  cpu_ora(nes_cpu);  cycles += 5; break;
        case 0x08: cpu_addressing_implied(nes_cpu);     cpu_php(nes_cpu);  cycles += 3; break;
        case 0x09: cpu_addressing_immediate(nes_cpu);   cpu_ora(nes_cpu);  cycles += 2; break;
        case 0x0A: cpu_addressing_accumulator(nes_cpu); cpu_asla(nes_cpu); cycles += 2; break;

        case 0x0C: cpu_addressing_absolute(nes_cpu);    cpu_nop(nes_cpu);  cycles += 4; break;
        case 0x0D: cpu_addressing_absolute(nes_cpu);    cpu_ora(nes_cpu);  cycles += 4; break;
        case 0x0E: cpu_addressing_absolute(nes_cpu);    cpu_asl(nes_cpu);  cycles += 6; break;
        case 0x0F: cpu_addressing_absolute(nes_cpu);    cpu_asl(nes_cpu);  cpu_ora(nes_cpu);  cycles += 6; break;
        case 0x10: cpu_addressing_relative(nes_cpu);    cpu_bpl(nes_cpu);  cycles += 2; break;
        case 0x11: cpu_addressing_indirect_y(nes_cpu);  cpu_ora(nes_cpu);  cycles += 5; break;
        /* 0x12 HLT */
        case 0x13: cpu_addressing_indirect_y(nes_cpu);  cpu_asl(nes_cpu);  cpu_ora(nes_cpu);  cycles += 8; break;
        case 0x14: cpu_addressing_zeropage_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0x15: cpu_addressing_zeropage_x(nes_cpu);  cpu_ora(nes_cpu);  cycles += 4; break;
        case 0x16: cpu_addressing_zeropage_x(nes_cpu);  cpu_asl(nes_cpu);  cycles += 6; break;
        case 0x17: cpu_addressing_zeropage_x(nes_cpu);  cpu_asl(nes_cpu);  cpu_ora(nes_cpu);  cycles += 6; break;
        case 0x18: cpu_addressing_implied(nes_cpu);     cpu_clc(nes_cpu);  cycles += 2; break;
        case 0x19: cpu_addressing_absolute_y(nes_cpu);  cpu_ora(nes_cpu);  cycles += 4; break;
        case 0x1A: cpu_addressing_implied(nes_cpu);     cpu_nop(nes_cpu);  cycles += 2; break;
        case 0x1B: cpu_addressing_absolute_y(nes_cpu);  cpu_asl(nes_cpu);  cpu_ora(nes_cpu);  cycles += 7; break;
        case 0x1C: cpu_addressing_absolute_x(nes_cpu);  check_page_cross_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0x1D: cpu_addressing_absolute_x(nes_cpu);  cpu_ora(nes_cpu);  cycles += 4; break;
        case 0x1E: cpu_addressing_absolute_x(nes_cpu);  cpu_asl(nes_cpu);  cycles += 7; break;
        case 0x1F: cpu_addressing_absolute_x(nes_cpu);  cpu_asl(nes_cpu);  cpu_ora(nes_cpu);  cycles += 7; break;
        case 0x20: cpu_addressing_absolute(nes_cpu);    cpu_jsr(nes_cpu);  cycles += 6; break;
        case 0x21: cpu_addressing_indirect_x(nes_cpu);  cpu_and(nes_cpu);  cycles += 6; break;
        /* 0x22 HLT */
        case 0x23: cpu_addressing_indirect_x(nes_cpu);  cpu_rol(nes_cpu);  cpu_and(nes_cpu);  cycles += 8; break;
        case 0x24: cpu_addressing_zeropage(nes_cpu);    cpu_bit(nes_cpu);  cycles += 3; break;
        case 0x25: cpu_addressing_zeropage(nes_cpu);    cpu_and(nes_cpu);  cycles += 3; break;
        case 0x26: cpu_addressing_zeropage(nes_cpu);    cpu_rol(nes_cpu);  cycles += 5; break;
        case 0x27: cpu_addressing_zeropage(nes_cpu);    cpu_rol(nes_cpu);  cpu_and(nes_cpu);  cycles += 5; break;
        case 0x28: cpu_addressing_implied(nes_cpu);     cpu_plp(nes_cpu);  cycles += 4; break;
        case 0x29: cpu_addressing_immediate(nes_cpu);   cpu_and(nes_cpu);  cycles += 2; break;
        case 0x2A: cpu_addressing_accumulator(nes_cpu); cpu_rola(nes_cpu); cycles += 2; break;

        case 0x2C: cpu_addressing_absolute(nes_cpu);    cpu_bit(nes_cpu);  cycles += 4; break;
        case 0x2D: cpu_addressing_absolute(nes_cpu);    cpu_and(nes_cpu);  cycles += 4; break;
        case 0x2E: cpu_addressing_absolute(nes_cpu);    cpu_rol(nes_cpu);  cycles += 6; break;
        case 0x2F: cpu_addressing_absolute(nes_cpu);    cpu_rol(nes_cpu);  cpu_and(nes_cpu); cycles += 6; break;
        case 0x30: cpu_addressing_relative(nes_cpu);    cpu_bmi(nes_cpu);  cycles += 2; break;
        case 0x31: cpu_addressing_indirect_y(nes_cpu);  cpu_and(nes_cpu);  cycles += 5; break;
        /* 0x32 HLT */
        case 0x33: cpu_addressing_indirect_y(nes_cpu);  cpu_rol(nes_cpu);  cpu_and(nes_cpu);  cycles += 8; break;
        case 0x34: cpu_addressing_zeropage_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0x35: cpu_addressing_zeropage_x(nes_cpu);  cpu_and(nes_cpu);  cycles += 4; break;
        case 0x36: cpu_addressing_zeropage_x(nes_cpu);  cpu_rol(nes_cpu);  cycles += 6; break;
        case 0x37: cpu_addressing_zeropage_x(nes_cpu);  cpu_rol(nes_cpu);  cpu_and(nes_cpu);  cycles += 6; break;
        case 0x38: cpu_addressing_implied(nes_cpu);     cpu_sec(nes_cpu);  cycles += 2; break;
        case 0x39: cpu_addressing_absolute_y(nes_cpu);  cpu_and(nes_cpu);  cycles += 4; break;
        case 0x3A: cpu_addressing_implied(nes_cpu);     cpu_nop(nes_cpu);  cycles += 2; break;
        case 0x3B: cpu_addressing_absolute_y(nes_cpu);  cpu_rol(nes_cpu); cpu_and(nes_cpu);  cycles += 7; break;
        case 0x3C: cpu_addressing_absolute_x(nes_cpu);  check_page_cross_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0x3D: cpu_addressing_absolute_x(nes_cpu);  cpu_and(nes_cpu);  cycles += 4; break;
        case 0x3E: cpu_addressing_absolute_x(nes_cpu);  cpu_rol(nes_cpu);  cycles += 7; break;
        case 0x3F: cpu_addressing_absolute_x(nes_cpu);  cpu_rol(nes_cpu);  cpu_and(nes_cpu);  cycles += 7; break;
        case 0x40: cpu_addressing_implied(nes_cpu);     cpu_rti(nes_cpu);  cycles += 6; break;
        case 0x41: cpu_addressing_indirect_x(nes_cpu);  cpu_eor(nes_cpu);  cycles += 6; break;
        /* 0x42 HLT */
        case 0x43: cpu_addressing_indirect_x(nes_cpu);  cpu_lsr(nes_cpu);  cpu_eor(nes_cpu);  cycles += 8; break;
        case 0x44: cpu_addressing_zeropage(nes_cpu);    cpu_nop(nes_cpu);  cycles += 3; break;
        case 0x45: cpu_addressing_zeropage(nes_cpu);    cpu_eor(nes_cpu);  cycles += 3; break;
        case 0x46: cpu_addressing_zeropage(nes_cpu);    cpu_lsr(nes_cpu);  cycles += 5; break;
        case 0x47: cpu_addressing_zeropage(nes_cpu);    cpu_lsr(nes_cpu);  cpu_eor(nes_cpu);  cycles += 5; break;
        case 0x48: cpu_addressing_implied(nes_cpu);     cpu_pha(nes_cpu);  cycles += 3; break;
        case 0x49: cpu_addressing_immediate(nes_cpu);   cpu_eor(nes_cpu);  cycles += 2; break;
        case 0x4A: cpu_addressing_accumulator(nes_cpu); cpu_lsra(nes_cpu); cycles += 2; break;

        case 0x4C: cpu_addressing_absolute(nes_cpu);    cpu_jmp(nes_cpu);  cycles += 3; break;
        case 0x4D: cpu_addressing_absolute(nes_cpu);    cpu_eor(nes_cpu);  cycles += 4; break;
        case 0x4E: cpu_addressing_absolute(nes_cpu);    cpu_lsr(nes_cpu);  cycles += 6; break;
        case 0x4F: cpu_addressing_absolute(nes_cpu);    cpu_lsr(nes_cpu);  cpu_eor(nes_cpu);  cycles += 6; break;
        case 0x50: cpu_addressing_relative(nes_cpu);    cpu_bvc(nes_cpu);  cycles += 2; break;
        case 0x51: cpu_addressing_indirect_y(nes_cpu);  cpu_eor(nes_cpu);  cycles += 5; break;
        /* 0x52 HLT */
        case 0x53: cpu_addressing_indirect_y(nes_cpu);  cpu_lsr(nes_cpu);  cpu_eor(nes_cpu);  cycles += 8; break;
        case 0x54: cpu_addressing_zeropage_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0x55: cpu_addressing_zeropage_x(nes_cpu);  cpu_eor(nes_cpu);  cycles += 4; break;
        case 0x56: cpu_addressing_zeropage_x(nes_cpu);  cpu_lsr(nes_cpu);  cycles += 6; break;
        case 0x57: cpu_addressing_zeropage_x(nes_cpu);  cpu_lsr(nes_cpu);  cpu_eor(nes_cpu);  cycles += 6; break;
        case 0x58: cpu_addressing_implied(nes_cpu);     cpu_cli(nes_cpu);  cycles += 2; break;
        case 0x59: cpu_addressing_absolute_y(nes_cpu);  cpu_eor(nes_cpu);  cycles += 4; break;
        case 0x5A: cpu_addressing_implied(nes_cpu);     cpu_nop(nes_cpu);  cycles += 2; break;
        case 0x5B: cpu_addressing_absolute_y(nes_cpu);  cpu_lsr(nes_cpu);  cpu_eor(nes_cpu);  cycles += 7; break;
        case 0x5C: cpu_addressing_absolute_x(nes_cpu);  check_page_cross_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0x5D: cpu_addressing_absolute_x(nes_cpu);  cpu_eor(nes_cpu);  cycles += 4; break;
        case 0x5E: cpu_addressing_absolute_x(nes_cpu);  cpu_lsr(nes_cpu);  cycles += 7; break;
        case 0x5F: cpu_addressing_absolute_x(nes_cpu);  cpu_lsr(nes_cpu);  cpu_eor(nes_cpu);  cycles += 7; break;
        case 0x60: cpu_addressing_implied(nes_cpu);     cpu_rts(nes_cpu);  cycles += 6; break;
        case 0x61: cpu_addressing_indirect_x(nes_cpu);  cpu_adc(nes_cpu);  cycles += 6; break;
        /* 0x62 HLT */
        case 0x63: cpu_addressing_indirect_x(nes_cpu);  cpu_ror(nes_cpu);  cpu_adc(nes_cpu);  cycles += 8; break;
        case 0x64: cpu_addressing_zeropage(nes_cpu);    cpu_nop(nes_cpu);  cycles += 3; break;
        case 0x65: cpu_addressing_zeropage(nes_cpu);    cpu_adc(nes_cpu);  cycles += 3; break;
        case 0x66: cpu_addressing_zeropage(nes_cpu);    cpu_ror(nes_cpu);  cycles += 5; break;
        case 0x67: cpu_addressing_zeropage(nes_cpu);    cpu_ror(nes_cpu);  cpu_adc(nes_cpu);  cycles += 5; break;
        case 0x68: cpu_addressing_implied(nes_cpu);     cpu_pla(nes_cpu);  cycles += 4; break;
        case 0x69: cpu_addressing_immediate(nes_cpu);   cpu_adc(nes_cpu);  cycles += 2; break;
        case 0x6A: cpu_addressing_accumulator(nes_cpu); cpu_rora(nes_cpu); cycles += 2; break;

        case 0x6C: cpu_addressing_indirect(nes_cpu);    cpu_jmp(nes_cpu);  cycles += 5; break;
        case 0x6D: cpu_addressing_absolute(nes_cpu);    cpu_adc(nes_cpu);  cycles += 4; break;
        case 0x6E: cpu_addressing_absolute(nes_cpu);    cpu_ror(nes_cpu);  cycles += 6; break;
        case 0x6F: cpu_addressing_absolute(nes_cpu);    cpu_ror(nes_cpu);  cpu_adc(nes_cpu);  cycles += 6; break;
        case 0x70: cpu_addressing_relative(nes_cpu);    cpu_bvs(nes_cpu);  cycles += 2; break;
        case 0x71: cpu_addressing_indirect_y(nes_cpu);  cpu_adc(nes_cpu);  cycles += 5; break;
        /* 0x72 HLT */
        case 0x73: cpu_addressing_indirect_y(nes_cpu);  cpu_ror(nes_cpu);  cpu_adc(nes_cpu);  cycles += 8; break;
        case 0x74: cpu_addressing_zeropage_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0x75: cpu_addressing_zeropage_x(nes_cpu);  cpu_adc(nes_cpu);  cycles += 4; break;
        case 0x76: cpu_addressing_zeropage_x(nes_cpu);  cpu_ror(nes_cpu);  cycles += 6; break;
        case 0x77: cpu_addressing_zeropage_x(nes_cpu);  cpu_ror(nes_cpu);  cpu_adc(nes_cpu);  cycles += 6; break;
        case 0x78: cpu_addressing_implied(nes_cpu);     cpu_sei(nes_cpu);  cycles += 2; break;
        case 0x79: cpu_addressing_absolute_y(nes_cpu);  cpu_adc(nes_cpu);  cycles += 4; break;
        case 0x7A: cpu_addressing_implied(nes_cpu);     cpu_nop(nes_cpu);  cycles += 2; break;
        case 0x7B: cpu_addressing_absolute_y(nes_cpu);  cpu_ror(nes_cpu);  cpu_adc(nes_cpu);  cycles += 7; break;
        case 0x7C: cpu_addressing_absolute_x(nes_cpu);  check_page_cross_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0x7D: cpu_addressing_absolute_x(nes_cpu);  cpu_adc(nes_cpu);  cycles += 4; break;
        case 0x7E: cpu_addressing_absolute_x(nes_cpu);  cpu_ror(nes_cpu);  cycles += 7; break;
        case 0x7F: cpu_addressing_absolute_x(nes_cpu);  cpu_ror(nes_cpu);  cpu_adc(nes_cpu);  cycles += 7; break;
        case 0x80: cpu_addressing_immediate(nes_cpu);   cpu_nop(nes_cpu);  cycles += 2; break;
        case 0x81: cpu_addressing_indirect_x(nes_cpu);  cpu_sta(nes_cpu);  cycles += 6; break;

        case 0x83: cpu_addressing_indirect_x(nes_cpu);  cpu_axs(nes_cpu);  cycles += 6; break;
        case 0x84: cpu_addressing_zeropage(nes_cpu);    cpu_sty(nes_cpu);  cycles += 3; break;
        case 0x85: cpu_addressing_zeropage(nes_cpu);    cpu_sta(nes_cpu);  cycles += 3; break;
        case 0x86: cpu_addressing_zeropage(nes_cpu);    cpu_stx(nes_cpu);  cycles += 3; break;
        case 0x87: cpu_addressing_zeropage(nes_cpu);    cpu_axs(nes_cpu);  cycles += 3; break;
        case 0x88: cpu_addressing_implied(nes_cpu);     cpu_dey(nes_cpu);  cycles += 2; break;

        case 0x8A: cpu_addressing_implied(nes_cpu);     cpu_txa(nes_cpu);  cycles += 2; break;

        case 0x8C: cpu_addressing_absolute(nes_cpu);    cpu_sty(nes_cpu);  cycles += 4; break;
        case 0x8D: cpu_addressing_absolute(nes_cpu);    cpu_sta(nes_cpu);  cycles += 4; break;
        case 0x8E: cpu_addressing_absolute(nes_cpu);    cpu_stx(nes_cpu);  cycles += 4; break;
        case 0x8F: cpu_addressing_absolute(nes_cpu);    cpu_axs(nes_cpu);  cycles += 4; break;
        case 0x90: cpu_addressing_relative(nes_cpu);    cpu_bcc(nes_cpu);  cycles += 2; break;
        case 0x91: cpu_addressing_indirect_y(nes_cpu);  cpu_sta(nes_cpu);  cycles += 6; break;
        /* 0x92 HLT */
        case 0x94: cpu_addressing_zeropage_x(nes_cpu);  cpu_sty(nes_cpu);  cycles += 4; break;
        case 0x95: cpu_addressing_zeropage_x(nes_cpu);  cpu_sta(nes_cpu);  cycles += 4; break;
        case 0x96: cpu_addressing_zeropage_y(nes_cpu);  cpu_stx(nes_cpu);  cycles += 4; break;
        case 0x97: cpu_addressing_zeropage_y(nes_cpu);  cpu_axs(nes_cpu);  cycles += 4; break;
        case 0x98: cpu_addressing_implied(nes_cpu);     cpu_tya(nes_cpu);  cycles += 2; break;
        case 0x99: cpu_addressing_absolute_y(nes_cpu);  cpu_sta(nes_cpu);  cycles += 5; break;
        case 0x9A: cpu_addressing_implied(nes_cpu);     cpu_txs(nes_cpu);  cycles += 2; break;

        case 0x9D: cpu_addressing_absolute_x(nes_cpu);  cpu_sta(nes_cpu);  cycles += 5; break;

        case 0xA0: cpu_addressing_immediate(nes_cpu);   cpu_ldy(nes_cpu);  cycles += 2; break;
        case 0xA1: cpu_addressing_indirect_x(nes_cpu);  cpu_lda(nes_cpu);  cycles += 6; break;
        case 0xA2: cpu_addressing_immediate(nes_cpu);   cpu_ldx(nes_cpu);  cycles += 2; break;
        case 0xA3: cpu_addressing_indirect_x(nes_cpu);  cpu_lda(nes_cpu);  cpu_ldx(nes_cpu); cycles += 6; break;
        case 0xA4: cpu_addressing_zeropage(nes_cpu);    cpu_ldy(nes_cpu);  cycles += 3; break;
        case 0xA5: cpu_addressing_zeropage(nes_cpu);    cpu_lda(nes_cpu);  cycles += 3; break;
        case 0xA6: cpu_addressing_zeropage(nes_cpu);    cpu_ldx(nes_cpu);  cycles += 3; break;
        case 0xA7: cpu_addressing_zeropage(nes_cpu);    cpu_lda(nes_cpu);  cpu_ldx(nes_cpu);  cycles += 3; break;
        case 0xA8: cpu_addressing_implied(nes_cpu);     cpu_tay(nes_cpu);  cycles += 2; break;
        case 0xA9: cpu_addressing_immediate(nes_cpu);   cpu_lda(nes_cpu);  cycles += 2; break;
        case 0xAA: cpu_addressing_implied(nes_cpu);     cpu_tax(nes_cpu);  cycles += 2; break;

        case 0xAC: cpu_addressing_absolute(nes_cpu);    cpu_ldy(nes_cpu);  cycles += 4; break;
        case 0xAD: cpu_addressing_absolute(nes_cpu);    cpu_lda(nes_cpu);  cycles += 4; break;
        case 0xAE: cpu_addressing_absolute(nes_cpu);    cpu_ldx(nes_cpu);  cycles += 4; break;
        case 0xAF: cpu_addressing_absolute(nes_cpu);    cpu_lda(nes_cpu);  cpu_ldx(nes_cpu);  cycles += 4; break;
        case 0xB0: cpu_addressing_relative(nes_cpu);    cpu_bcs(nes_cpu);  cycles += 2; break;
        case 0xB1: cpu_addressing_indirect_y(nes_cpu);  check_page_cross_y(nes_cpu);  cpu_lda(nes_cpu);  cycles += 5; break;
        /* 0xB2 HLT */
        case 0xB3: cpu_addressing_indirect_y(nes_cpu);  check_page_cross_y(nes_cpu);  cpu_lda(nes_cpu);  cpu_ldx(nes_cpu);  cycles += 5; break;
        case 0xB4: cpu_addressing_zeropage_x(nes_cpu);  cpu_ldy(nes_cpu);  cycles += 4; break;
        case 0xB5: cpu_addressing_zeropage_x(nes_cpu);  cpu_lda(nes_cpu);  cycles += 4; break;
        case 0xB6: cpu_addressing_zeropage_y(nes_cpu);  cpu_ldx(nes_cpu);  cycles += 4; break;
        case 0xB7: cpu_addressing_zeropage_y(nes_cpu);  cpu_lda(nes_cpu); cpu_ldx(nes_cpu);  cycles += 4; break;
        case 0xB8: cpu_addressing_implied(nes_cpu);     cpu_clv(nes_cpu);  cycles += 2; break;
        case 0xB9: cpu_addressing_absolute_y(nes_cpu);  check_page_cross_y(nes_cpu);  cpu_lda(nes_cpu);  cycles += 4; break;
        case 0xBA: cpu_addressing_implied(nes_cpu);     cpu_tsx(nes_cpu);  cycles += 2; break;

        case 0xBC: cpu_addressing_absolute_x(nes_cpu);  check_page_cross_x(nes_cpu);  cpu_ldy(nes_cpu);  cycles += 4; break;
        case 0xBD: cpu_addressing_absolute_x(nes_cpu);  check_page_cross_x(nes_cpu);  cpu_lda(nes_cpu);  cycles += 4; break;
        case 0xBE: cpu_addressing_absolute_y(nes_cpu);  check_page_cross_y(nes_cpu);  cpu_ldx(nes_cpu);  cycles += 4; break;
        case 0xBF: cpu_addressing_absolute_y(nes_cpu);  cpu_lda(nes_cpu);  cpu_ldx(nes_cpu); cycles += 4; break;
        case 0xC0: cpu_addressing_immediate(nes_cpu);   cpu_cpy(nes_cpu);  cycles += 2; break;
        case 0xC1: cpu_addressing_indirect_x(nes_cpu);  cpu_cmp(nes_cpu);  cycles += 6; break;

        case 0xC3: cpu_addressing_indirect_x(nes_cpu);  cpu_dec(nes_cpu);  cpu_cmp(nes_cpu); cycles += 8; break;
        case 0xC4: cpu_addressing_zeropage(nes_cpu);    cpu_cpy(nes_cpu);  cycles += 3; break;
        case 0xC5: cpu_addressing_zeropage(nes_cpu);    cpu_cmp(nes_cpu);  cycles += 3; break;
        case 0xC6: cpu_addressing_zeropage(nes_cpu);    cpu_dec(nes_cpu);  cycles += 5; break;
        case 0xC7: cpu_addressing_zeropage(nes_cpu);    cpu_dec(nes_cpu);  cpu_cmp(nes_cpu);  cycles += 5; break;
        case 0xC8: cpu_addressing_implied(nes_cpu);     cpu_iny(nes_cpu);  cycles += 2; break;
        case 0xC9: cpu_addressing_immediate(nes_cpu);   cpu_cmp(nes_cpu);  cycles += 2; break;
        case 0xCA: cpu_addressing_implied(nes_cpu);     cpu_dex(nes_cpu);  cycles += 2; break;

        case 0xCC: cpu_addressing_absolute(nes_cpu);    cpu_cpy(nes_cpu);  cycles += 4; break;
        case 0xCD: cpu_addressing_absolute(nes_cpu);    cpu_cmp(nes_cpu);  cycles += 4; break;
        case 0xCE: cpu_addressing_absolute(nes_cpu);    cpu_dec(nes_cpu);  cycles += 6; break;
        case 0xCF: cpu_addressing_absolute(nes_cpu);    cpu_dec(nes_cpu);  cpu_cmp(nes_cpu);  cycles += 6; break;
        case 0xD0: cpu_addressing_relative(nes_cpu);    cpu_bne(nes_cpu);  cycles += 2; break;
        case 0xD1: cpu_addressing_indirect_y(nes_cpu);  cpu_cmp(nes_cpu);  cycles += 5; break;
        /* 0xD2 HLT */
        case 0xD3: cpu_addressing_indirect_y(nes_cpu);  cpu_dec(nes_cpu);  cpu_cmp(nes_cpu);  cycles += 8; break;
        case 0xD4: cpu_addressing_zeropage_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0xD5: cpu_addressing_zeropage_x(nes_cpu);  cpu_cmp(nes_cpu);  cycles += 4; break;
        case 0xD6: cpu_addressing_zeropage_x(nes_cpu);  cpu_dec(nes_cpu);  cycles += 6; break;
        case 0xD7: cpu_addressing_zeropage_x(nes_cpu);  cpu_dec(nes_cpu);  cpu_cmp(nes_cpu);  cycles += 6; break;
        case 0xD8: cpu_addressing_implied(nes_cpu);     cpu_cld(nes_cpu);  cycles += 2; break;
        case 0xD9: cpu_addressing_absolute_y(nes_cpu);  cpu_cmp(nes_cpu);  cycles += 4; break;
        case 0xDA: cpu_addressing_implied(nes_cpu);     cpu_nop(nes_cpu);  cycles += 2; break;
        case 0xDB: cpu_addressing_absolute_y(nes_cpu);  cpu_dec(nes_cpu); cpu_cmp(nes_cpu);  cycles += 7; break;
        case 0xDC: cpu_addressing_absolute_x(nes_cpu);  check_page_cross_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0xDD: cpu_addressing_absolute_x(nes_cpu);  cpu_cmp(nes_cpu);  cycles += 4; break;
        case 0xDE: cpu_addressing_absolute_x(nes_cpu);  cpu_dec(nes_cpu);  cycles += 7; break;
        case 0xDF: cpu_addressing_absolute_x(nes_cpu);  cpu_dec(nes_cpu);  cpu_cmp(nes_cpu);  cycles += 7; break;
        case 0xE0: cpu_addressing_immediate(nes_cpu);   cpu_cpx(nes_cpu);  cycles += 2; break;
        case 0xE1: cpu_addressing_indirect_x(nes_cpu);  cpu_sbc(nes_cpu);  cycles += 6; break;

        case 0xE3: cpu_addressing_indirect_x(nes_cpu);  cpu_inc(nes_cpu);  cpu_sbc(nes_cpu); cycles += 8; break;
        case 0xE4: cpu_addressing_zeropage(nes_cpu);    cpu_cpx(nes_cpu);  cycles += 3; break;
        case 0xE5: cpu_addressing_zeropage(nes_cpu);    cpu_sbc(nes_cpu);  cycles += 3; break;
        case 0xE6: cpu_addressing_zeropage(nes_cpu);    cpu_inc(nes_cpu);  cycles += 5; break;
        case 0xE7: cpu_addressing_zeropage(nes_cpu);    cpu_inc(nes_cpu);  cpu_sbc(nes_cpu); cycles += 5; break;
        case 0xE8: cpu_addressing_implied(nes_cpu);     cpu_inx(nes_cpu);  cycles += 2; break;
        case 0xE9: cpu_addressing_immediate(nes_cpu);   cpu_sbc(nes_cpu);  cycles += 2; break;
        case 0xEA: cpu_addressing_implied(nes_cpu);     cpu_nop(nes_cpu);  cycles += 2; break;
        case 0xEB: cpu_addressing_immediate(nes_cpu);   cpu_sbc(nes_cpu);  cycles += 2; break;
        case 0xEC: cpu_addressing_absolute(nes_cpu);    cpu_cpx(nes_cpu);  cycles += 4; break;
        case 0xED: cpu_addressing_absolute(nes_cpu);    cpu_sbc(nes_cpu);  cycles += 4; break;
        case 0xEE: cpu_addressing_absolute(nes_cpu);    cpu_inc(nes_cpu);  cycles += 6; break;
        case 0xEF: cpu_addressing_absolute(nes_cpu);    cpu_inc(nes_cpu);  cpu_sbc(nes_cpu); cycles += 6; break;
        case 0xF0: cpu_addressing_relative(nes_cpu);    cpu_beq(nes_cpu);  cycles += 2; break;
        case 0xF1: cpu_addressing_indirect_y(nes_cpu);  cpu_sbc(nes_cpu);  cycles += 5; break;
        /* 0xF2 HLT */
        case 0xF3: cpu_addressing_indirect_y(nes_cpu);  cpu_inc(nes_cpu);  cpu_sbc(nes_cpu); cycles += 8; break;
        case 0xF4: cpu_addressing_zeropage_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0xF5: cpu_addressing_zeropage_x(nes_cpu);  cpu_sbc(nes_cpu);  cycles += 4; break;
        case 0xF6: cpu_addressing_zeropage_x(nes_cpu);  cpu_inc(nes_cpu);  cycles += 6; break;
        case 0xF7: cpu_addressing_zeropage_x(nes_cpu);  cpu_inc(nes_cpu);  cpu_sbc(nes_cpu); cycles += 6; break;
        case 0xF8: cpu_addressing_implied(nes_cpu);     cpu_sed(nes_cpu);  cycles += 2; break;
        case 0xF9: cpu_addressing_absolute_y(nes_cpu);  cpu_sbc(nes_cpu);  cycles += 4; break;
        case 0xFA: cpu_addressing_implied(nes_cpu);     cpu_nop(nes_cpu);  cycles += 2; break;
        case 0xFB: cpu_addressing_absolute_y(nes_cpu);  cpu_inc(nes_cpu);  cpu_sbc(nes_cpu); cycles += 7; break;
        case 0xFC: cpu_addressing_absolute_x(nes_cpu);  check_page_cross_x(nes_cpu);  cpu_nop(nes_cpu);  cycles += 4; break;
        case 0xFD: cpu_addressing_absolute_x(nes_cpu);  cpu_sbc(nes_cpu);  cycles += 4; break;
        case 0xFE: cpu_addressing_absolute_x(nes_cpu);  cpu_inc(nes_cpu);  cycles += 7; break;
        case 0xFF: cpu_addressing_absolute_x(nes_cpu);  cpu_inc(nes_cpu);  cpu_sbc(nes_cpu);  cycles += 7; break;
        default:
            die("!!!Unknown instruction: %x\n", opcode);
            break;
    }
    cycles += nes_cpu->additional_cycles;
    nes_cpu->num_cycles += cycles;

    return cycles;
}

uint32_t nes_cpu_nmi(nes_cpu_t *nes_cpu) 
{
    debug_print("NMI! cycles: %lld\n", nes_cpu->num_cycles);
    nes_cpu->regs.P |= FLAG_INTERRUPT;
    cpu_stack_push_word(nes_cpu, nes_cpu->regs.PC);
    cpu_stack_push_byte(nes_cpu, nes_cpu->regs.P);
    nes_cpu->regs.PC = memory_read_word(nes_cpu, 0xfffa);
    return 0;
}

void nes_cpu_reset(nes_cpu_t *nes_cpu) {
    nes_cpu->regs.S  -= 3;
    nes_cpu->regs.P |= FLAG_INTERRUPT;
    memory_write_byte(nes_cpu, 0x4015, 0);  // APU was silenced
    nes_cpu->regs.PC = memory_read_word(nes_cpu, 0xfffc);
}

void nes_cpu_init(nes_cpu_t *nes_cpu, nes_mem_td *nes_mem)
{
    memset(nes_cpu, 0, sizeof(*nes_cpu));

    nes_cpu->num_cycles = 0;

    /* At first set the memory interface */
    nes_cpu->nes_mem = nes_mem;

    //nes_cpu->regs.PC = 0xC000;
    nes_cpu->regs.PC = memory_read_word(nes_cpu, 0xFFFC); /* Reset vector */

    //printf("PC: 0x%x\n", nes_cpu->regs.PC);

    nes_cpu->regs.S = 0xfd;
    nes_cpu->regs.P = 0x24;
    nes_cpu->regs.A = 0;
    nes_cpu->regs.X = 0;
    nes_cpu->regs.Y = 0;
}

