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
#include <stdlib.h>
#include <string.h>

#include <nes.h>
#include <cartridge.h>

int nes_cart_load_rom(nes_cartridge_t *nes_cart, char *rom)
{
    FILE *fp;
    int i = 0;
    uint8_t *buffer;
     
    buffer = malloc(512*1024);  //max nes cartridge size
    if(buffer == NULL)
    {
        return ERR_NOT_ENOUGH_MEMORY;
    }

    fp = fopen(rom,"r");
    if(fp == NULL) 
    {
        return ERR_FILE_NOT_EXIST;
    }

    if(fread(nes_cart->header, sizeof(uint8_t), 16, fp) != 16) 
    {
        return ERR_NES_FILE_HEADER_READ_FAILED;
    } 
    else 
    {
        nes_cart->prg_rom_size = 16 * 1024 * nes_cart->header[4];
        nes_cart->chr_rom_size = 8  * 1024 * nes_cart->header[5];
        nes_cart->prg_ram_size = 8  * 1024 * nes_cart->header[8];

        /* Set mirroring */
        nes_cart->nes_mem->nt_mirroring = (nes_cart->header[6] & (1<<0));
    }

    nes_cart_print_rom_metadata(nes_cart);

	if(fread(buffer , nes_cart->prg_rom_size, 1, fp) != 1)
     {
        die("Err fread prg rom\n");
     }   

    for(i=0;i<nes_cart->prg_rom_size;i++)
    {
        if(nes_cart->prg_rom_size == 0x4000)
        {
            cpu_memory_access(nes_cart->nes_mem, CPU_MEM_PRG_LOCATION0+i, buffer[i], ACCESS_WRITE_BYTE);
            cpu_memory_access(nes_cart->nes_mem, CPU_MEM_PRG_LOCATION1+i, buffer[i], ACCESS_WRITE_BYTE);
        }
        else
        {
            cpu_memory_access(nes_cart->nes_mem, CPU_MEM_PRG_LOCATION0+i, buffer[i], ACCESS_WRITE_BYTE);
        }
        
    }

	if(fread(buffer , nes_cart->chr_rom_size, 1, fp) != 1)
	{
        die("Err fread chr rom\n");
    }

    for(i=0;i<nes_cart->chr_rom_size;i++)
    {
        ppu_memory_access(nes_cart->nes_mem, PPU_MEM_PATTERN_TABLE0_OFFSET+i, buffer[i], ACCESS_WRITE_BYTE);
    }

	free(buffer);
    fclose(fp);

    return 0;
}

void nes_cart_init(nes_cartridge_t *nes_cart, nes_mem_td *nes_mem)
{
    memset(nes_cart, 0, sizeof(*nes_cart));
    nes_cart->nes_mem = nes_mem;
}

void nes_cart_print_rom_metadata(nes_cartridge_t *nes_cart) 
{
    uint8_t mapper = 0;

    mapper = (nes_cart->header[7] & 0xF0) | ((nes_cart->header[6] & 0xFF) >> 4);

    printf("==============================================\n");
    printf("ROM Metadata:\n");
    printf("Sinature: %c%c%c\n", nes_cart->header[0], nes_cart->header[1], nes_cart->header[2]);
    printf("Flags 6: %d\n", nes_cart->header[6]);
    printf("Mirroring: %s\n", (nes_cart->header[6] & (1<<0)) ? "vertical" : "horizontal" );
    printf("Flags 7: %d\n", nes_cart->header[7]);
    printf("Mapper %x\n", mapper);
    printf("PRG ROM Size: %d B\n", nes_cart->prg_rom_size);
    printf("CHR ROM Size: %d B\n", nes_cart->chr_rom_size);
    printf("PRG RAM Size: %d B\n", nes_cart->prg_ram_size);
    printf("==============================================\n\n");

    if(mapper != 0)
    {
        die("Mapper %x is not supported\n", mapper);
    }
}
