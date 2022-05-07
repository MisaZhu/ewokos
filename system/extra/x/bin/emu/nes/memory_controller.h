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

#ifndef MEMORY_CONTROLLER_H
#define MEMORY_CONTROLLER_H

#include <stdint.h>

/* CPU MEMORY */
#define CPU_MEM_INTERNAL_RAM_OFFSET         0x0000
#define CPU_MEM_INTERNAL_RAM_SIZE           0x0800

#define CPU_MEM_INTERNAL_RAM_MIRROR1_OFFSET 0x0800
#define CPU_MEM_INTERNAL_RAM_MIRROR1_SIZE   0x0800

#define CPU_MEM_INTERNAL_RAM_MIRROR2_OFFSET 0x1000
#define CPU_MEM_INTERNAL_RAM_MIRROR2_SIZE   0x0800

#define CPU_MEM_INTERNAL_RAM_MIRROR3_OFFSET 0x1800
#define CPU_MEM_INTERNAL_RAM_MIRROR3_SIZE   0x0800

#define CPU_MEM_PPU_REGISTER_OFFSET         0x2000
#define CPU_MEM_PPU_REGISTER_SIZE           0x0008

#define CPU_MEM_PPU_MIRRORS_OFFSET          0x2008
#define CPU_MEM_PPU_MIRRORS_SIZE            0x1FF8
#define CPU_MEM_PPU_MIRRORS_REPEAT          0x0008

#define CPU_MEM_PPU_REGISTER_AREA_END       0x3FFF

#define CPU_MEM_APU_REGISTER_OFFSET         0x4000
#define CPU_MEM_APU_REGISTER_SIZE           0x0018

#define CPU_MEM_APU_IO_REGISTER_OFFSET      0x4018
#define CPU_MEM_APU_IO_REGISTER_SIZE        0x0008

#define CPU_MEM_CRTRDG_REGISTER_OFFSET      0x4020
#define CPU_MEM_CRTRDG_REGISTER_SIZE        0xBFE0

#define CPU_MEM_PHYS_SIZE CPU_MEM_INTERNAL_RAM_SIZE + \
                          CPU_MEM_PPU_REGISTER_SIZE + \
                          CPU_MEM_APU_REGISTER_SIZE + \
                          CPU_MEM_APU_IO_REGISTER_SIZE + \
                          CPU_MEM_CRTRDG_REGISTER_SIZE

#define CPU_MEM_PPU_CTRL_REGISTER           0x2000
#define CPU_MEM_PPU_MASK_REGISTER           0x2001
#define CPU_MEM_PPU_STATUS_REGISTER         0x2002
#define CPU_MEM_PPU_OAMADDR_REGISTER        0x2003
#define CPU_MEM_PPU_OAMDATA_REGISTER        0x2004
#define CPU_MEM_PPU_SCROLL_REGISTER         0x2005
#define CPU_MEM_PPU_ADDR_REGISTER           0x2006
#define CPU_MEM_PPU_DATA_REGISTER           0x2007
#define CPU_MEM_PPU_OAMDMA_REGISTER         0x4014

#define CPU_CONTROLLER1_REGISTER 0x4016
#define CPU_CONTROLLER2_REGISTER 0x4017

/* CARTRIDGE ROM SPACE */
#define CPU_MEM_PRG_LOCATION0 0x8000
#define CPU_MEM_PRG_SIZE0     0x4000
#define CPU_MEM_PRG_LOCATION1 0xC000
#define CPU_MEM_PRG_SIZE1     0x4000


/* PPU MEMORY */
#define PPU_MEM_PATTERN_TABLE0_OFFSET       0x0000
#define PPU_MEM_PATTERN_TABLE0_SIZE         0x1000

#define PPU_MEM_PATTERN_TABLE1_OFFSET       0x1000
#define PPU_MEM_PATTERN_TABLE1_SIZE         0x1000

#define PPU_MEM_NAME_TABLE0_OFFSET          0x2000
#define PPU_MEM_NAME_TABLE0_SIZE            0x0400

#define PPU_MEM_NAME_TABLE1_OFFSET          0x2400
#define PPU_MEM_NAME_TABLE1_SIZE            0x0400

#define PPU_MEM_NAME_TABLE2_OFFSET          0x2800
#define PPU_MEM_NAME_TABLE2_SIZE            0x0400

#define PPU_MEM_NAME_TABLE3_OFFSET          0x2C00
#define PPU_MEM_NAME_TABLE3_SIZE            0x0400

#define PPU_MEM_NAME_TABLE0_MIRROR_OFFSET   0x3000
#define PPU_MEM_NAME_TABLE0_MIRROR_SIZE     0x0400

#define PPU_MEM_NAME_TABLE1_MIRROR_OFFSET   0x3400
#define PPU_MEM_NAME_TABLE1_MIRROR_SIZE     0x0400

#define PPU_MEM_NAME_TABLE2_MIRROR_OFFSET   0x3800
#define PPU_MEM_NAME_TABLE2_MIRROR_SIZE     0x0400

#define PPU_MEM_NAME_TABLE3_MIRROR_OFFSET   0x3C00
#define PPU_MEM_NAME_TABLE3_MIRROR_SIZE     0x0400

#define PPU_MEM_NAME_TABLE_MIRRORS_OFFSET   0x3000
#define PPU_MEM_NAME_TABLE_MIRRORS_SIZE     0x0F00
#define PPU_MEM_NAME_TABLE_MIRRORS_REPEAT   0x0400

#define PPU_MEM_PALETTE_RAM_OFFSET          0x3F00
#define PPU_MEM_PALETTE_RAM_SIZE            0x0020
#define PPU_MEM_PALETTE_INTERNAL_MIRRORS    4 /* $3F00 <-> $3F10 | $3F04 <-> $3F14 | $3F08 <-> $3F18 | $3F0C <-> $3F1C */
#define PPU_MEM_PALETTE_SPRITE_ADDR         0x3F11

#define PPU_MEM_PALETTE_RAM_MIRRORS_OFFSET  0x3F20
#define PPU_MEM_PALETTE_RAM_MIRRORS_SIZE    0x00E0
#define PPU_MEM_PALETTE_RAM_MIRRORS_REPEAT  0x0020

#define PPU_MEM_SIZE                        0x4000

#define PPU_MEM_ATTRIBUTE_TABLE0_OFFSET     0x23C0
#define PPU_MEM_ATTRIBUTE_TABLE1_OFFSET     0x27C0
#define PPU_MEM_ATTRIBUTE_TABLE2_OFFSET     0x2BC0
#define PPU_MEM_ATTRIBUTE_TABLE3_OFFSET     0x2FC0

#define PPU_MEM_PHYS_SIZE PPU_MEM_PATTERN_TABLE0_SIZE + \
                          PPU_MEM_PATTERN_TABLE1_SIZE + \
                          PPU_MEM_NAME_TABLE0_SIZE + \
                          PPU_MEM_NAME_TABLE1_SIZE + \
                          PPU_MEM_NAME_TABLE2_SIZE + \
                          PPU_MEM_NAME_TABLE3_SIZE + \
                          PPU_MEM_PALETTE_RAM_SIZE // - PPU_MEM_PALETTE_INTERNAL_MIRRORS

#define PPU_MEM_OAMD_SIZE 0x0100


#define PPU_NAMETABLE_MIRRORING_H 0
#define PPU_NAMETABLE_MIRRORING_V 1


#define ACCESS_WRITE_WORD 0
#define ACCESS_WRITE_BYTE 1
#define ACCESS_READ_WORD  2
#define ACCESS_READ_BYTE  3
#define ACCESS_FUNC_MAX   4

#define NT_MIRROR_HORIZONTAL 0
#define NT_MIRROR_VERTICAL 1
#ifdef __cplusplus
extern "C" {
#endif
typedef struct __attribute__((packed)) ppu_regs_s
{
    // $2000
    // uint8_t name_table_address : 2;
    // uint8_t vertical_write : 1;
    // uint8_t sprite_pattern_table_addr : 1;
    // uint8_t screen_pattern_table_addr : 1;
    // uint8_t sprite_size : 1;
    // uint8_t unused : 1;
    // uint8_t vblank_enabled : 1;
	uint8_t ctrl;

    // $2001
    // uint8 unused : 1;
    // uint8 screen_mask : 1;
    // uint8 sprite_mask : 1;
    // uint8 screen_enabled : 1;
    // uint8 sprites_enabled : 1;
    // uint8 screen_red_tint : 1;
    // uint8 screen_green_tint : 1;
    // uint8 screen_blue_tint : 1;
    uint8_t mask;

    // $2002
    // uint8 unused : 5;
    // uint8 sprite_overflow_bit : 1;
    // uint8 sprite_zero_hit : 1;
    // uint8 vblank_flag : 1;    
    uint8_t status;

    // $2003
    uint8_t oamaddr;

    // $2004
    uint8_t oamdata;

    // $2005
    uint8_t scroll;

    // $2006
    uint8_t addr;

    // $2007
    uint8_t data;

} ppu_regs_t;

typedef struct nes_mem_struct
{
    /* Only OxC808 bytes are actually used by the nes:
     * 0x10000−0x800−0x800−0x800−0x1ff8 = 0xC808
     */
    uint8_t cpu_memory[CPU_MEM_PHYS_SIZE];

    /* Cartridge space */
    uint8_t cpu_prg_memory[CPU_MEM_PRG_SIZE0+CPU_MEM_PRG_SIZE1];

    /* Only 0x3100 bytes are actually used by the ppu:
     * 0x4000−0xF00 = 0x3100
     */
    uint8_t ppu_memory[PPU_MEM_PHYS_SIZE];

    uint8_t nt_mirroring;

    /* PPU regs shared with the CPU */
    ppu_regs_t ppu_regs;
    uint8_t oam_memory[PPU_MEM_OAMD_SIZE];
    uint8_t internal_w;
    uint16_t internal_t;
    uint16_t internal_v;
    uint8_t internal_x;

    uint8_t vram_latch;
    uint8_t vram_add;

} nes_mem_td;

uint16_t cpu_memory_access(nes_mem_td *memmap, uint16_t addr, uint16_t data, uint8_t access_type);
uint16_t ppu_memory_access(nes_mem_td *memmap, uint16_t addr, uint16_t data, uint8_t access_type);
#ifdef __cplusplus
}
#endif
#endif /* MEMORY_CONTROLLER_H */
