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
#include <ppu.h>
#include <memory_controller.h>
#include <controller.h>

#define debug_print(fmt, ...) \
            do { if (DEBUG_MEMORY) printf(fmt, __VA_ARGS__); } while (0)

//#define debug_print(fmt, ...) do{}while(0)


static uint16_t cpu_memory_address_demirror(uint16_t addr)
{
    if(addr < CPU_MEM_PPU_REGISTER_OFFSET)
        return addr % CPU_MEM_INTERNAL_RAM_SIZE;
    else if(addr < CPU_MEM_APU_REGISTER_OFFSET)
        return CPU_MEM_PPU_REGISTER_OFFSET + (addr % CPU_MEM_PPU_REGISTER_SIZE);
    else
        return addr;
}

static uint16_t cpu_memory_address_to_phys(uint16_t addr)
{
    if(addr < (CPU_MEM_INTERNAL_RAM_OFFSET + CPU_MEM_INTERNAL_RAM_SIZE))
        return addr;
    else if(addr < CPU_MEM_APU_REGISTER_OFFSET)
        return CPU_MEM_INTERNAL_RAM_OFFSET + CPU_MEM_INTERNAL_RAM_SIZE + (addr % CPU_MEM_PPU_REGISTER_SIZE);
    else if(addr < CPU_MEM_APU_IO_REGISTER_OFFSET)
        return CPU_MEM_INTERNAL_RAM_OFFSET + CPU_MEM_INTERNAL_RAM_SIZE + CPU_MEM_PPU_REGISTER_SIZE + (addr % CPU_MEM_APU_REGISTER_SIZE);
    else if(addr < CPU_MEM_CRTRDG_REGISTER_OFFSET)
        return CPU_MEM_INTERNAL_RAM_OFFSET + CPU_MEM_INTERNAL_RAM_SIZE + CPU_MEM_PPU_REGISTER_SIZE + CPU_MEM_APU_REGISTER_SIZE + (addr % CPU_MEM_APU_IO_REGISTER_SIZE);
    else
        return addr;
}

static uint8_t check_is_ppu_reg(uint16_t addr)
{
    if( ((addr >= CPU_MEM_PPU_REGISTER_OFFSET) && (addr < CPU_MEM_PPU_REGISTER_OFFSET+CPU_MEM_PPU_REGISTER_SIZE)) || (addr == CPU_MEM_PPU_OAMDMA_REGISTER) )
    {
        return 1;
    }
    return 0;
}

static uint8_t check_is_prg_rom(uint16_t addr)
{
    if((addr >= CPU_MEM_PRG_LOCATION0) && (addr <= 0xFFFF))
        return 1;
    return 0;
}

static uint8_t check_is_controller_reg(uint16_t addr)
{
    if((addr == CPU_CONTROLLER1_REGISTER) || (addr == CPU_CONTROLLER2_REGISTER))
    {
        return 1;
    }
    return 0;
}

static uint16_t cpu_memory_read_byte(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    return (memmap->cpu_memory[addr]); 
}

static uint16_t cpu_memory_read_word(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    return cpu_memory_read_byte(memmap, addr, 0) + (cpu_memory_read_byte(memmap, addr + 1, 0) << 8);
}

static uint16_t cpu_memory_write_byte(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    memmap->cpu_memory[addr] = (uint8_t)data;
    return 0;
}

static uint16_t cpu_memory_write_word(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    cpu_memory_write_byte(memmap, addr, data & 0xFF);
    cpu_memory_write_byte(memmap, addr + 1, data >> 8);
    return 0;
}

static uint16_t (*cpu_access_funcs[ACCESS_FUNC_MAX])(nes_mem_td *memmap, uint16_t addr, uint16_t data) = 
{
    cpu_memory_write_word,
    cpu_memory_write_byte,
    cpu_memory_read_word,
    cpu_memory_read_byte
};

static uint16_t prg_memory_read_byte(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    return (memmap->cpu_prg_memory[addr]); 
}

static uint16_t prg_memory_read_word(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    return prg_memory_read_byte(memmap, addr, 0) + (prg_memory_read_byte(memmap, addr + 1, 0) << 8);
}

static uint16_t prg_memory_write_byte(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    memmap->cpu_prg_memory[addr] = (uint8_t)data;
    return 0;
}

static uint16_t prg_memory_write_word(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    prg_memory_write_byte(memmap, addr, data & 0xFF);
    prg_memory_write_byte(memmap, addr + 1, data >> 8);
    return 0;
}

static uint16_t (*prg_access_funcs[ACCESS_FUNC_MAX])(nes_mem_td *memmap, uint16_t addr, uint16_t data) = 
{
    prg_memory_write_word,
    prg_memory_write_byte,
    prg_memory_read_word,
    prg_memory_read_byte
};

uint16_t cpu_memory_access(nes_mem_td *memmap, uint16_t addr, uint16_t data, uint8_t access_type)
{
    uint16_t demirrored_addr = cpu_memory_address_demirror(addr);
    uint16_t phys_addr = cpu_memory_address_to_phys(demirrored_addr);

    //printf("CPU addr:%x demirrored_addr:%x phys_addr:%x\n", addr, demirrored_addr, phys_addr);

    if(check_is_ppu_reg(demirrored_addr))
    {
        return ppu_reg_access(memmap, demirrored_addr, data, access_type);
    }
    else if(check_is_controller_reg(demirrored_addr))
    {
        if(access_type == ACCESS_WRITE_BYTE)
        {
            if(demirrored_addr == CPU_CONTROLLER1_REGISTER)
                psg_io_write(data);
            else if(demirrored_addr == CPU_CONTROLLER2_REGISTER)
                psg_io_write2(data);
        }
        else if (access_type == ACCESS_READ_BYTE)
        {
            if(demirrored_addr == CPU_CONTROLLER1_REGISTER)
                return psg_io_read();
            else if(demirrored_addr == CPU_CONTROLLER2_REGISTER)
                psg_io_read2();
        }
        return 0;
    }
    else if(check_is_prg_rom(demirrored_addr))
    {
        if((demirrored_addr-CPU_MEM_PRG_LOCATION0) >= (CPU_MEM_PRG_SIZE0+CPU_MEM_PRG_SIZE1))
        {
            die("Invalid PRG mem access! %x max: %x\n", (demirrored_addr-CPU_MEM_PRG_LOCATION0), CPU_MEM_PRG_SIZE0+CPU_MEM_PRG_SIZE1);
        }
        return prg_access_funcs[access_type](memmap, demirrored_addr-CPU_MEM_PRG_LOCATION0, data);
    }
    else
    {
        if(phys_addr >= CPU_MEM_PHYS_SIZE)
        {
            die("Invalid CPU mem access! %x\n", phys_addr);
        }
        return cpu_access_funcs[access_type](memmap, phys_addr, data);
    }
}


static uint16_t ppu_memory_address_demirror(uint16_t addr)
{
    if(addr < PPU_MEM_NAME_TABLE_MIRRORS_OFFSET)
        return addr;
    else if(addr < PPU_MEM_PALETTE_RAM_OFFSET)
        return PPU_MEM_NAME_TABLE0_OFFSET + (addr % 0x400);
    else if(addr == 0x3F10) 
        return 0x3F00;
    else if(addr == 0x3F14) 
        return 0x3F04;
    else if(addr == 0x3F18) 
        return 0x3F08;
    else if(addr == 0x3F1C) 
        return 0x3F0C;
    else if(addr < PPU_MEM_PALETTE_RAM_MIRRORS_OFFSET)
        return addr;
    else if(addr < PPU_MEM_SIZE)
        return PPU_MEM_PALETTE_RAM_OFFSET + (addr % PPU_MEM_PALETTE_RAM_MIRRORS_REPEAT);
    else
    {
        return addr;
    }
}

static uint16_t ppu_memory_address_to_phys(uint16_t addr, nes_mem_td *memmap)
{
    if(addr < PPU_MEM_NAME_TABLE_MIRRORS_OFFSET)
    {
        if(memmap->nt_mirroring == NT_MIRROR_HORIZONTAL)
        {
            /* Horizontal mirroring */
            if((addr >= 0x2400) && (addr < 0x2800))
            {
                return addr - 0x400;
            }
            else if((addr >= 0x2C00) && (addr < 0x3000))
            {
                return addr - 0x400;
            }
        }

        if(memmap->nt_mirroring == NT_MIRROR_VERTICAL)
        {
            /* Vertical mirroring */
            if((addr >= 0x2800) && (addr < 0x2C00))
            {
                return addr - 0x800;
            }
            if((addr >= 0x2C00) && (addr < 0x3000))
            {
                return addr - 0x800;
            }
        }
        return addr;
    }
    else if(addr < PPU_MEM_PALETTE_RAM_MIRRORS_OFFSET)
        return (PPU_MEM_NAME_TABLE3_OFFSET + PPU_MEM_NAME_TABLE3_SIZE + (addr % (PPU_MEM_PALETTE_RAM_SIZE)));
    else
        return addr;
}


static uint16_t ppu_memory_read_byte(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    return (memmap->ppu_memory[addr]); 
}

static uint16_t ppu_memory_read_word(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    return ppu_memory_read_byte(memmap, addr, 0) + (ppu_memory_read_byte(memmap, addr + 1, 0) << 8);
}

static uint16_t ppu_memory_write_byte(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    memmap->ppu_memory[addr] = (uint8_t)data;
    return 0;
}

static uint16_t ppu_memory_write_word(nes_mem_td *memmap, uint16_t addr, uint16_t data) 
{
    ppu_memory_write_byte(memmap, addr, data & 0xFF);
    ppu_memory_write_byte(memmap, addr + 1, data >> 8);
    return 0;
}

static uint16_t (*ppu_access_funcs[ACCESS_FUNC_MAX])(nes_mem_td *memmap, uint16_t addr, uint16_t data) = 
{
    ppu_memory_write_word,
    ppu_memory_write_byte,
    ppu_memory_read_word,
    ppu_memory_read_byte
};

uint16_t ppu_memory_access(nes_mem_td *memmap, uint16_t addr, uint16_t data, uint8_t access_type)
{
    uint16_t demirrored_addr = ppu_memory_address_demirror(addr);
    uint16_t phys_addr = ppu_memory_address_to_phys(demirrored_addr, memmap);

    //printf("PPU %d addr:%x demirrored_addr:%x phys_addr:%x\n", access_type, addr, demirrored_addr, phys_addr);

    if(phys_addr >= PPU_MEM_PHYS_SIZE)
    {
        die("Invalid PPU mem access! %x max: %x\n", phys_addr, PPU_MEM_PHYS_SIZE);
    }
    return ppu_access_funcs[access_type](memmap, phys_addr, data);
}
