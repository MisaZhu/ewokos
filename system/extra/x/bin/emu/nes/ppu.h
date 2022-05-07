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

#ifndef PPU_H
#define PPU_H

#include <stdint.h>

#include <memory_controller.h>

#define PPU_STATUS_FRAME_READY (1 << 0)
#define PPU_STATUS_NMI (1 << 1)
#define PPU_STATUS_OAM_ACCESS (1 << 2)
#ifdef __cplusplus
extern "C" {
#endif
typedef struct nes_ppu_s
{
	uint16_t current_scan_line;
    uint16_t current_pixel;

    nes_mem_td *nes_memory;

    uint32_t screen_bitmap[256*240];

    uint8_t sprite0_hit_occured;
    uint16_t sprite0_hit_scanline;
    uint16_t sprite0_hit_pixel;

} nes_ppu_t;

/* NTSC color pallete */
#pragma pack(1)
typedef struct ppu_pix{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
}ppu_pix_t;

typedef struct ppu_color_pallete_s
{
    union{
        uint32_t raw;
        ppu_pix_t   pix;
    };
} ppu_color_pallete_t;
#pragma pack()

uint16_t ppu_reg_access(nes_mem_td *memmap, uint16_t addr, uint16_t data, uint8_t access_type);
void nes_ppu_init(nes_ppu_t *nes_ppu, nes_mem_td *nes_memory);
uint8_t nes_ppu_run(nes_ppu_t *nes_ppu, uint32_t cpu_cycles);
void nes_ppu_dump_regs(nes_ppu_t *nes_ppu);
#ifdef __cplusplus
}
#endif
#endif
