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

#if DEBUG_PPU 
#define debug_print(...) printf(__VA_ARGS__) 
#else
#define debug_print(...) {}
#endif

#define PPU_CTRL_REG_GEN_NMI           (0x1 << 7)
#define PPU_CTRL_SPRITE_SIZE           (0x1 << 5)
#define PPU_CTRL_BG_PATTERN_TABLE_ADDR (0x1 << 4)
#define PPU_CTRL_SP_PATTERN_TABLE_ADDR (0x1 << 3)
#define PPU_CTRL_VRAM_INCREMENT        (0x1 << 2)
#define PPU_CTRL_BASE_NAME_TABLE_ADDR  (0x3 << 0)

#define PPU_MASK_SHOW_BG       (1 << 3)
#define PPU_MASK_SHOW_SPRITES  (1 << 4)

#define PPU_STATUS_REG_VBLANK  (1 << 7)
#define PPU_STATUS_SPRITE0_HIT (1 << 6)

ppu_color_pallete_t color_pallete_2C02[] = {
    {.pix = {   84,    84,     84,0 }}, 
    {.pix = {  116,    30,      0,0 }}, 
    {.pix = {  144,    16,      8,0 }}, 
    {.pix = {  136,     0,     48,0 }}, 
    {.pix = {  100,     0,     68,0 }}, 
    {.pix = {   48,     0,     92,0 }}, 
    {.pix = {    0,     4,     84,0 }}, 
    {.pix = {    0,    24,     60,0 }}, 
    {.pix = {    0,    42,     32,0 }}, 
    {.pix = {    0,    58,      8,0 }}, 
    {.pix = {    0,    64,      0,0 }}, 
    {.pix = {    0,    60,      0,0 }}, 
    {.pix = {   60,    50,      0,0 }}, 
    {.pix = {    0,     0,      0,0 }}, 
    {.pix = {    0,     0,      0,0 }}, 
    {.pix = {    0,     0,      0,0 }},
    {.pix = {  152,   150,    152,0 }}, 
    {.pix = {  196,    76,      8,0 }}, 
    {.pix = {  236,    50,     48,0 }}, 
    {.pix = {  228,    30,     92,0 }}, 
    {.pix = {  176,    20,    136,0 }}, 
    {.pix = {  100,    20,    160,0 }}, 
    {.pix = {   32,    34,    152,0 }}, 
    {.pix = {    0,    60,    120,0 }}, 
    {.pix = {    0,    90,     84,0 }}, 
    {.pix = {    0,   114,     40,0 }}, 
    {.pix = {    0,   124,      8,0 }}, 
    {.pix = {   40,   118,      0,0 }}, 
    {.pix = {  120,   102,      0,0 }}, 
    {.pix = {    0,     0,      0,0 }}, 
    {.pix = {    0,     0,      0,0 }}, 
    {.pix = {    0,     0,      0,0 }},
    {.pix = {  236,   238,    236,0 }}, 
    {.pix = {  236,   154,     76,0 }}, 
    {.pix = {  236,   124,    120,0 }}, 
    {.pix = {  236,    98,    176,0 }}, 
    {.pix = {  236,    84,    228,0 }}, 
    {.pix = {  180,    88,    236,0 }}, 
    {.pix = {  100,   106,    236,0 }}, 
    {.pix = {   32,   136,    212,0 }}, 
    {.pix = {    0,   170,    160,0 }}, 
    {.pix = {    0,   196,    116,0 }}, 
    {.pix = {   32,   208,     76,0 }}, 
    {.pix = {  108,   204,     56,0 }}, 
    {.pix = {  204,   180,     56,0 }}, 
    {.pix = {   60,    60,     60,0 }}, 
    {.pix = {    0,     0,      0,0 }}, 
    {.pix = {    0,     0,      0,0 }},
    {.pix = {  236,   238,    236,0 }}, 
    {.pix = {  236,   204,    168,0 }}, 
    {.pix = {  236,   188,    188,0 }}, 
    {.pix = {  236,   178,    212,0 }}, 
    {.pix = {  236,   174,    236,0 }}, 
    {.pix = {  212,   174,    236,0 }}, 
    {.pix = {  176,   180,    236,0 }}, 
    {.pix = {  144,   196,    228,0 }}, 
    {.pix = {  120,   210,    204,0 }}, 
    {.pix = {  120,   222,    180,0 }}, 
    {.pix = {  144,   226,    168,0 }}, 
    {.pix = {  180,   226,    152,0 }}, 
    {.pix = {  228,   214,    160,0 }}, 
    {.pix = {  160,   162,    160,0 }}, 
    {.pix = {    0,     0,      0,0 }}, 
    {.pix = {    0,     0,      0,0 }}
};

uint16_t ppu_reg_access(nes_mem_td *memmap, uint16_t addr, uint16_t data, uint8_t access_type)
{
    uint16_t i = 0;

    if((access_type == ACCESS_READ_WORD) || (access_type == ACCESS_WRITE_WORD))
    {
        die("Invalid PPU reg access! %d\n", access_type);
    }

    if(access_type == ACCESS_WRITE_BYTE)
    {
        /* Update least significant bits previously written into a PPU register */
        memmap->ppu_regs.status &= (~0x1F);
        memmap->ppu_regs.status |= (data & 0x1F);

        if(addr == CPU_MEM_PPU_CTRL_REGISTER)
        {
            memmap->ppu_regs.ctrl = data;
            /* Reset vram nametable address */
            memmap->internal_t &= ~(0x3 << 10);
            memmap->internal_t |= (memmap->ppu_regs.ctrl & PPU_CTRL_BASE_NAME_TABLE_ADDR) << 10;
        }
        else if(addr == CPU_MEM_PPU_MASK_REGISTER)
        {
            memmap->ppu_regs.mask = data;
        }
        else if(addr == CPU_MEM_PPU_OAMADDR_REGISTER)
        {
            memmap->ppu_regs.oamaddr = data;
        }
        else if(addr == CPU_MEM_PPU_OAMDATA_REGISTER)
        {
            memmap->oam_memory[memmap->ppu_regs.oamaddr] = data;
            memmap->ppu_regs.oamaddr++;
        }
        else if(addr == CPU_MEM_PPU_ADDR_REGISTER)
        {
            memmap->ppu_regs.addr = data;

            if(!memmap->internal_w)
            {
                memmap->internal_t &= ~(0xff << 8);
                memmap->internal_t |= (memmap->ppu_regs.addr << 8);
                /* Clear bit 14 and 15 */
                memmap->internal_t &= ~(0x3 << 14);
            }
            else
            {
                memmap->internal_t &= ~(0xff);
                memmap->internal_t |= (memmap->ppu_regs.addr);
                memmap->internal_v = memmap->internal_t;
            }
            memmap->internal_w = (memmap->internal_w + 1) % 2;
        }
        else if(addr == CPU_MEM_PPU_DATA_REGISTER)
        {
            memmap->ppu_regs.data = data;
            if(memmap->internal_v >= PPU_MEM_SIZE)
            {
                die("ILLEGAL ADDRESS: %x\n", memmap->internal_v);
            }

            ppu_memory_access(memmap, memmap->internal_v, memmap->ppu_regs.data, ACCESS_WRITE_BYTE);

            /* check address increment bit */
            memmap->vram_add = (memmap->ppu_regs.ctrl & PPU_CTRL_VRAM_INCREMENT) ? 32 : 1;
        }
        else if(addr == CPU_MEM_PPU_SCROLL_REGISTER)
        {
            memmap->ppu_regs.scroll = data;
            if(!memmap->internal_w)
            {
                /* set scroll x */
                memmap->internal_t &= ~0x1f;
                memmap->internal_t |= ((memmap->ppu_regs.scroll >> 3) & 0x1f);
                memmap->internal_x = (memmap->ppu_regs.scroll & 0x7);
                debug_print("SCROLLx %d!\n", ((memmap->internal_t & 0x1f) << 3) | (memmap->internal_x & 0x7) );
            }
            else
            {
                memmap->internal_t &= ~(0x1f << 5);
                memmap->internal_t &= ~(0x7 << 12);
                memmap->internal_t |= ((memmap->ppu_regs.scroll >> 3) << 5);
                memmap->internal_t |= ((memmap->ppu_regs.scroll & 0x7) << 12);
            }
            memmap->internal_w = (memmap->internal_w + 1) % 2;
        }
        else if(addr == CPU_MEM_PPU_OAMDMA_REGISTER)
        {
            for(i=0;i<256;i++)
            {
                memmap->oam_memory[i] = (uint8_t)cpu_memory_access(memmap, ((data << 8) | i), 0, ACCESS_READ_BYTE);
            }
        }
    }
    /* read */
    else if(access_type == ACCESS_READ_BYTE)
    {
        if(addr == CPU_MEM_PPU_CTRL_REGISTER)
        {
            return memmap->ppu_regs.ctrl;
        }
        else if(addr == CPU_MEM_PPU_MASK_REGISTER)
        {
            return memmap->ppu_regs.mask;
        }
        else if(addr == CPU_MEM_PPU_STATUS_REGISTER)
        {
            uint8_t tmp_status = memmap->ppu_regs.status;
            memmap->ppu_regs.addr = 0;
            memmap->ppu_regs.status &= ~PPU_STATUS_REG_VBLANK;
            memmap->internal_w = 0;
            return tmp_status;
        }
        else if(addr == CPU_MEM_PPU_OAMADDR_REGISTER)
        {
            return memmap->ppu_regs.oamaddr;
        }
        else if(addr == CPU_MEM_PPU_OAMDATA_REGISTER)
        {
            return memmap->oam_memory[memmap->ppu_regs.oamaddr];
        }
        else if(addr == CPU_MEM_PPU_SCROLL_REGISTER)
        {
            return memmap->ppu_regs.scroll;
        }
        else if(addr == CPU_MEM_PPU_ADDR_REGISTER)
        {
            return memmap->ppu_regs.addr;
        }
        else if(addr == CPU_MEM_PPU_DATA_REGISTER)
        {
            uint8_t latched = memmap->vram_latch;
            uint8_t retval = 0;

            if(memmap->internal_v >= PPU_MEM_SIZE)
            {
                die("ILLEGAL ADDRESS: %x\n", memmap->internal_v);
            }

            memmap->vram_latch = (uint8_t)ppu_memory_access(memmap, memmap->internal_v, 0, ACCESS_READ_BYTE);

            if(memmap->internal_v >= PPU_MEM_PALETTE_RAM_OFFSET)
                retval = memmap->vram_latch;
            else
                retval = latched;

            memmap->vram_add = (memmap->ppu_regs.ctrl & PPU_CTRL_VRAM_INCREMENT) ? 32 : 1;
            return retval;
        }
    }

    return 0;
}

void nes_ppu_init(nes_ppu_t *nes_ppu, nes_mem_td *nes_memory)
{
    memset(nes_ppu, 0, sizeof(*nes_ppu));
    nes_ppu->nes_memory = nes_memory;
	printf("ppu_color_pallete_t:%08x %02x %02x %02x %02x \n", color_pallete_2C02[0].raw,color_pallete_2C02[0].pix.a, color_pallete_2C02[0].pix.r,color_pallete_2C02[0].pix.g,color_pallete_2C02[0].pix.b);
	printf("ppu_color_pallete_t:%08x\n", color_pallete_2C02[0].pix.r << 16 | color_pallete_2C02[0].pix.g << 8 | color_pallete_2C02[0].pix.b);
}

uint8_t nes_ppu_run(nes_ppu_t *nes_ppu, uint32_t cpu_cycles)
{   
    uint8_t ppu_ret_status = 0;

    /* Common Vars */
    uint16_t color_pallete_address = 0;
    uint8_t color_pallete_index = 0;
    uint16_t attribute_table_load_addr = 0;
    uint8_t sprite0_hit_candidate = 0;
    uint8_t sprite0_hit_bits = 0;

    /* Nametable Vars */
    uint16_t pattern_table_load_addr = 0;
    uint16_t name_table_load_addr = 0;
    uint8_t tile_low_bit = 0, tile_high_bit = 0;
    uint8_t pattern_low_val = 0, pattern_high_val = 0;
    uint8_t current_tile_pixel_col = 0, current_tile_pixel_row = 0;
    uint8_t attribute_bits = 0;
    uint8_t attribute_bit_quadrant = 0;
    ppu_color_pallete_t color_pallete_value = { 0 };
    uint16_t scroll_offset_x = ((nes_ppu->nes_memory->internal_t & 0x1f) << 3) | (nes_ppu->nes_memory->internal_x & 0x7);
    uint16_t scroll_offset_y = ((nes_ppu->nes_memory->internal_t & 0x3E0) >> 2) | ((nes_ppu->nes_memory->internal_t & 0x7000) >> 12);
    uint16_t current_pixel_with_scroll_offset_x = (nes_ppu->current_pixel + scroll_offset_x) % 256;
    uint16_t current_scanline_with_scroll_offset_y = (nes_ppu->current_scan_line + scroll_offset_y) % 240;
    uint16_t nt_offset = (nes_ppu->nes_memory->internal_t & (0xC00));

    /* sprite variables */
    int16_t i = 0; /* NEEDS to be signed, because of counting down for loop */
    uint16_t x_index,y_index = 0;
    uint8_t sprite_val_low = 0;
    uint8_t sprite_val_high = 0;
    uint8_t sprite_bit_low = 0;
    uint8_t sprite_bit_high = 0;
    uint16_t sprite_start_pix_x = 0;
    uint16_t sprite_start_pix_y = 0;
    uint8_t flip_vertical = 0;
    uint8_t flip_horizontal = 0;
    uint8_t sprite_x_index = 0;
    uint8_t sprite_y_index = 0;

    /* Increment vreg after $2007 access */
    nes_ppu->nes_memory->internal_v += nes_ppu->nes_memory->vram_add;
    nes_ppu->nes_memory->vram_add = 0;

    if((nes_ppu->current_scan_line == 241) && (nes_ppu->current_pixel == 1))
    {
        nes_ppu->nes_memory->ppu_regs.status |= PPU_STATUS_REG_VBLANK;

        if(nes_ppu->nes_memory->ppu_regs.ctrl & PPU_CTRL_REG_GEN_NMI)
        {
            ppu_ret_status |= PPU_STATUS_NMI;
        }
    }
    else if((nes_ppu->current_scan_line == 261) && (nes_ppu->current_pixel == 1))
    {
        nes_ppu->sprite0_hit_occured = 0;
        nes_ppu->nes_memory->ppu_regs.status &= ~PPU_STATUS_REG_VBLANK;
        nes_ppu->nes_memory->ppu_regs.status &= ~PPU_STATUS_SPRITE0_HIT;
        debug_print("vblank cycles: %d\n", cpu_cycles);
    }
    else if((nes_ppu->current_scan_line == 240) && (nes_ppu->current_pixel >= 257) && (nes_ppu->current_pixel <= 320))
    {
        nes_ppu->nes_memory->ppu_regs.oamaddr = 0;
    }

    if(nes_ppu->nes_memory->ppu_regs.ctrl & PPU_CTRL_SPRITE_SIZE)
    {
        die("Spritesize of 8x16 currently not supported %x\n", nes_ppu->nes_memory->ppu_regs.ctrl);
    }

    /* actual pixel generation begins here */
    if((nes_ppu->current_scan_line == 0) && (nes_ppu->current_pixel == 0))
    {
        /* Clear bitmap */
        memset(nes_ppu->screen_bitmap, 0, sizeof(nes_ppu->screen_bitmap));

        if(nes_ppu->nes_memory->ppu_regs.mask & PPU_MASK_SHOW_SPRITES)
        {
            uint16_t temp_pix_x_i = 0;
            uint16_t temp_pix_y_i = 0;

            for(temp_pix_y_i=0;temp_pix_y_i<240;temp_pix_y_i++)
            for(temp_pix_x_i=0;temp_pix_x_i<256;temp_pix_x_i++)
            {
                pattern_table_load_addr = (nes_ppu->nes_memory->ppu_regs.ctrl & PPU_CTRL_SP_PATTERN_TABLE_ADDR) ? PPU_MEM_PATTERN_TABLE1_OFFSET : PPU_MEM_PATTERN_TABLE0_OFFSET;

                /* We are counting down here, as lower address OAMs overwrite higher address OAMs, 
                   simple solution is to just count down. So the higher ones get overwritten.
                 */
                for(i=(PPU_MEM_OAMD_SIZE-4);i>=0;i-=4)
                {
                    if(temp_pix_y_i == nes_ppu->nes_memory->oam_memory[i] && temp_pix_x_i == nes_ppu->nes_memory->oam_memory[i+3])
                    {
                        pattern_table_load_addr += (nes_ppu->nes_memory->oam_memory[i+1] << 4);
                        attribute_table_load_addr = 0x3F10 + ((nes_ppu->nes_memory->oam_memory[i+2] & 0x3) << 2);

                        sprite_start_pix_x = nes_ppu->nes_memory->oam_memory[i+3];
                        sprite_start_pix_y = nes_ppu->nes_memory->oam_memory[i];

                        flip_horizontal = nes_ppu->nes_memory->oam_memory[i+2] & (1 << 7) ? 1 : 0;
                        flip_vertical = nes_ppu->nes_memory->oam_memory[i+2] & (1 << 6) ? 1 : 0;

                        for(y_index=0;y_index<8;y_index++)
                        {
                            if((sprite_start_pix_y+y_index) >= 240)
                                break;

                            sprite_y_index = flip_horizontal ? (7-y_index) : y_index;
                            sprite_val_low = (uint8_t)ppu_memory_access(nes_ppu->nes_memory, pattern_table_load_addr + sprite_y_index, 0, ACCESS_READ_BYTE);
                            sprite_val_high = (uint8_t)ppu_memory_access(nes_ppu->nes_memory, pattern_table_load_addr + sprite_y_index + 8, 0, ACCESS_READ_BYTE);

                            for(x_index=0;x_index<8;x_index++)
                            {
                                if((sprite_start_pix_x+x_index) >= 256)
                                    break;

                                sprite_x_index = flip_vertical ? (1 << (x_index)) : (1 << (7-x_index));

                                sprite_bit_low = (sprite_val_low & sprite_x_index) ? 1 : 0;
                                sprite_bit_high = (sprite_val_high & sprite_x_index) ? 1 : 0;

                                color_pallete_index = (uint8_t)ppu_memory_access(nes_ppu->nes_memory, attribute_table_load_addr + ((sprite_bit_high << 1) | sprite_bit_low ), 0, ACCESS_READ_BYTE);
                                color_pallete_value = color_pallete_2C02[color_pallete_index];

                                /* Draw the sprite pixel here, and encode some values in the A-Sector (Bit 31 - 24) of the RGBA32 uint32_t
                                   for sprite0 hit detection */
                                if((sprite_bit_high | sprite_bit_low) != 0)
                                {
                                    nes_ppu->screen_bitmap[(sprite_start_pix_x+x_index) + (256*(sprite_start_pix_y+y_index))] =
                                                                                            ( (i == 0) ? 1 << 28 : 0 << 28) | /* SPRITE0 hit canditate */
                                                                                            ( ((sprite_bit_high << 1) | sprite_bit_low ) << 24) |
                                                                                            color_pallete_value.raw;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if((nes_ppu->current_scan_line < 240) && (nes_ppu->current_pixel < 256))
    {
        current_tile_pixel_row = current_scanline_with_scroll_offset_y%8;
        current_tile_pixel_col = 7-((current_pixel_with_scroll_offset_x)%8);

        if(nes_ppu->nes_memory->ppu_regs.mask & PPU_MASK_SHOW_BG) 
        {
            if((nes_ppu->current_pixel+scroll_offset_x) > 255)
            {
                if(nt_offset == 0x000)
                    nt_offset = 0x400;
                else if(nt_offset == 0x400)
                    nt_offset = 0x000;
                else if(nt_offset == 0x800)
                    nt_offset = 0xC00;
                else if(nt_offset == 0xC00)
                    nt_offset = 0x800;
            }

            if((nes_ppu->current_scan_line+scroll_offset_y) > 239)
            {
                if(nt_offset == 0x000)
                    nt_offset = 0x800;
                else if(nt_offset == 0x800)
                    nt_offset = 0x000;
                else if(nt_offset == 0xC00)
                    nt_offset = 0x400;
                else if(nt_offset == 0x400)
                    nt_offset = 0xC00;
            }

            // if(nes_ppu->current_scan_line == 239)
            //     printf("nt offs: %x scroll: %d\n", nt_offset, scroll_offset_y);
            
            nes_ppu->nes_memory->internal_v &= ~(0x3 << 10);
            nes_ppu->nes_memory->internal_v |= nt_offset;
            nes_ppu->nes_memory->internal_v &= ~0x001F;
            nes_ppu->nes_memory->internal_v &= ~0x03E0;
            nes_ppu->nes_memory->internal_v |= (((current_scanline_with_scroll_offset_y/8) << 12) & 0x7000) + (((current_scanline_with_scroll_offset_y/8) << 5) & 0x3E0) + ((current_pixel_with_scroll_offset_x/8) & 0x1F);

            /* calculate tile row */
            name_table_load_addr = 0x2000 | (nes_ppu->nes_memory->internal_v & 0x0FFF);

            /* get pattern from table */
            pattern_table_load_addr = (nes_ppu->nes_memory->ppu_regs.ctrl & PPU_CTRL_BG_PATTERN_TABLE_ADDR) ? PPU_MEM_PATTERN_TABLE1_OFFSET : PPU_MEM_PATTERN_TABLE0_OFFSET;
            pattern_table_load_addr += (ppu_memory_access(nes_ppu->nes_memory, name_table_load_addr, 0, ACCESS_READ_BYTE) << 4) + current_tile_pixel_row;

            /* get low and high pattern bits */
            pattern_low_val = (uint8_t)ppu_memory_access(nes_ppu->nes_memory, pattern_table_load_addr, 0, ACCESS_READ_BYTE);
            pattern_high_val = (uint8_t)ppu_memory_access(nes_ppu->nes_memory, pattern_table_load_addr+8, 0, ACCESS_READ_BYTE);

            tile_low_bit = (pattern_low_val >> current_tile_pixel_col) & 1;
            tile_high_bit = (pattern_high_val >> current_tile_pixel_col) & 1;

            /* Get attribute bits */
            attribute_bit_quadrant = ( ((current_scanline_with_scroll_offset_y%32)/16) << 1 ) | ((current_pixel_with_scroll_offset_x%32)/16);
            attribute_bit_quadrant *= 2;
            attribute_table_load_addr = 0x23C0 | (nes_ppu->nes_memory->internal_v & 0x0C00) | ((nes_ppu->nes_memory->internal_v >> 4) & 0x38) | ((nes_ppu->nes_memory->internal_v >> 2) & 0x07);
            attribute_bits = ((uint8_t)ppu_memory_access(nes_ppu->nes_memory, attribute_table_load_addr, 0, ACCESS_READ_BYTE) >> attribute_bit_quadrant) & 0x3;

            color_pallete_address = PPU_MEM_PALETTE_RAM_OFFSET + (attribute_bits << 2) + ((tile_high_bit<<1) | tile_low_bit);
            /* use backdrop color (0x3F00) */
            if((color_pallete_address == 0x3F04) || (color_pallete_address == 0x3F08) || (color_pallete_address == 0x3F0C)) color_pallete_address = 0x3F00;

            color_pallete_index = (uint8_t)ppu_memory_access(nes_ppu->nes_memory, color_pallete_address, 0, ACCESS_READ_BYTE);
            color_pallete_value = color_pallete_2C02[color_pallete_index];

            /* SPRITE0 HIT handling */
            if(nes_ppu->screen_bitmap[nes_ppu->current_pixel + (256*nes_ppu->current_scan_line)] != 0)
            {
                sprite0_hit_candidate = nes_ppu->screen_bitmap[nes_ppu->current_pixel + (256*nes_ppu->current_scan_line)] >> 28;
                sprite0_hit_bits = (nes_ppu->screen_bitmap[nes_ppu->current_pixel + (256*nes_ppu->current_scan_line)] >> 24) & 0x3;

                if(!nes_ppu->sprite0_hit_occured)
                {
                    if(sprite0_hit_candidate && ( ((tile_high_bit << 1) | tile_low_bit) & sprite0_hit_bits) )
                    {
                        /* It seems sprite0 hits are delayed by 1 scanline so we need to add some quirks */
                        nes_ppu->sprite0_hit_scanline = nes_ppu->current_scan_line + 1;
                        nes_ppu->sprite0_hit_pixel = nes_ppu->current_pixel;
                        nes_ppu->sprite0_hit_occured = 1;
                        //printf("Sprite 0 hit! %d %d %x %x\n", nes_ppu->sprite0_hit_scanline, nes_ppu->sprite0_hit_pixel, ((tile_high_bit << 1) | tile_low_bit), sprite0_hit_bits);
                    }
                }

                nes_ppu->screen_bitmap[nes_ppu->current_pixel + (256*nes_ppu->current_scan_line)] |= (0xFF << 24);
            }

            if(!(nes_ppu->nes_memory->ppu_regs.status & PPU_STATUS_SPRITE0_HIT))
            {
                if( (nes_ppu->current_scan_line == nes_ppu->sprite0_hit_scanline) && (nes_ppu->current_pixel == nes_ppu->sprite0_hit_pixel))
                {
                    nes_ppu->nes_memory->ppu_regs.status |= PPU_STATUS_SPRITE0_HIT;
                }
            }

            /* Draw the pixel */
            if(nes_ppu->screen_bitmap[nes_ppu->current_pixel + (256*nes_ppu->current_scan_line)] == 0)
                nes_ppu->screen_bitmap[nes_ppu->current_pixel + (256*nes_ppu->current_scan_line)] =
                    0xFF00000000 | color_pallete_value.raw;
        }
    }

    nes_ppu->current_pixel++;
    if(nes_ppu->current_pixel >= 340)
    {
        nes_ppu->current_pixel = 0;
        nes_ppu->current_scan_line++;
        if(nes_ppu->current_scan_line >= 262)
        {
            nes_ppu->current_scan_line = 0;
            ppu_ret_status |= PPU_STATUS_FRAME_READY;
        }
    }

    return ppu_ret_status;
}

void nes_ppu_dump_regs(nes_ppu_t *nes_ppu)
{
    debug_print("ctrl: %x mask: %x status: %x oamaddr: %x oamdata: %x scroll: %x addr: %x data: %x pixel: %d scanline %d\n", 
            nes_ppu->nes_memory->ppu_regs.ctrl, nes_ppu->nes_memory->ppu_regs.mask, nes_ppu->nes_memory->ppu_regs.status, nes_ppu->nes_memory->ppu_regs.oamaddr, 
            nes_ppu->nes_memory->ppu_regs.oamdata, nes_ppu->nes_memory->ppu_regs.scroll, nes_ppu->nes_memory->ppu_regs.addr, nes_ppu->nes_memory->ppu_regs.data,
            nes_ppu->current_pixel, nes_ppu->current_scan_line);
}
