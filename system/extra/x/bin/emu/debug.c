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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>

/* NES specific */
#include <nes.h>
#include <ppu.h>
#include <cpu.h>
#include <cartridge.h>
#include <controller.h>

#define FPS 60
#define FPS_UPDATE_TIME_MS (1000/FPS)
//#define DEBUG
#ifdef DEBUG
#include<ncurses.h>
#endif 

#define debug_print(fmt, ...) \
            do { if (DEBUG_MAIN) printf(fmt, __VA_ARGS__); } while (0)

long getInterval(struct timeval *tv){
	struct timeval now;
	gettimeofday(&now, NULL);
	long interval = (now.tv_sec - tv->tv_sec) * 1000000 + (now.tv_usec - tv->tv_usec);
	gettimeofday(tv, NULL);
	return interval;
}

int main(int argc, char *argv[])
{
	struct timeval tv;

    nes_ppu_t *nes_ppu;
    nes_cpu_t *nes_cpu;
    nes_cartridge_t *nes_cart;
    nes_mem_td *nes_memory;
 
    uint32_t cpu_clocks;
    uint32_t ppu_clocks;
    uint32_t ppu_rest_clocks;
    uint32_t ppu_clock_index;
    uint8_t ppu_status;
	uint32_t frame_count;


    if(argc < 2){
        printf("please set rom file\n");
        return -1;
    }

    nes_memory = (nes_mem_td*)calloc(1, sizeof(nes_mem_td));
    nes_cpu = (nes_cpu_t*)calloc(1, sizeof(nes_cpu_t));
    nes_ppu = (nes_ppu_t*)calloc(1, sizeof(nes_ppu_t));
    nes_cart = (nes_cartridge_t*)calloc(1, sizeof(nes_cartridge_t));
        
    cpu_clocks = 0;
    ppu_clocks = 0;
    ppu_rest_clocks = 0;
    ppu_clock_index = 0;
    ppu_status = 0;
	frame_count = 0;

    /* init cartridge */
    nes_cart_init(nes_cart, nes_memory);

    /* load rom */
    if(nes_cart_load_rom(nes_cart, argv[1]) != 0)
    {
        printf("ROM does not exist\n");
        return -1;
    }

    /* init cpu */
    nes_cpu_init(nes_cpu, nes_memory);
    nes_cpu_reset(nes_cpu);

    /* init ppu */
    nes_ppu_init(nes_ppu, nes_memory);

	while(1){
         for(;;)
        {
            cpu_clocks = 0;
            if(!ppu_rest_clocks)
            {
                if(ppu_status & PPU_STATUS_NMI)
                    cpu_clocks += nes_cpu_nmi(nes_cpu);
                cpu_clocks += nes_cpu_run(nes_cpu);
            }

            /* the ppu runs at a 3 times higher clock rate than the cpu
            so we need to give the ppu some clocks here to catchup */
            ppu_clocks = (cpu_clocks*3) + ppu_rest_clocks;
            ppu_status = 0;
            for(ppu_clock_index=0;ppu_clock_index<ppu_clocks;ppu_clock_index++)
            {
                ppu_status |= nes_ppu_run(nes_ppu, nes_cpu->num_cycles);
                if(ppu_status & PPU_STATUS_FRAME_READY) break;
                else ppu_rest_clocks = 0;
            }

            ppu_rest_clocks = (ppu_clocks - ppu_clock_index);

            //nes_ppu_dump_regs(nes_ppu);

            if(ppu_status & PPU_STATUS_FRAME_READY) break;
        }
		frame_count++;
		printf("%d fps\r", 1000000/getInterval(&tv));
		fflush(stdout);
	}

	return 0;
}
