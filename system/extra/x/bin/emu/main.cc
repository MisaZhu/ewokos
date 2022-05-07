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
#include <vprintf.h>
#include <sys/basic_math.h>
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <x++/X.h>

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

 
using namespace Ewok;
   /* NES part */


class NesEmu : public XWin {
private:
    nes_ppu_t *nes_ppu;
    nes_cpu_t *nes_cpu;
    nes_cartridge_t *nes_cart;
    nes_mem_td *nes_memory;
 
    uint32_t cpu_clocks;
    uint32_t ppu_clocks;
    uint32_t ppu_rest_clocks;
    uint32_t ppu_clock_index;
    uint8_t ppu_status;

    graph_t emuScr; 

public:
	inline NesEmu() {
        nes_memory = (nes_mem_td*)calloc(1, sizeof(nes_mem_td));
        nes_cpu = (nes_cpu_t*)calloc(1, sizeof(nes_cpu_t));
        nes_ppu = (nes_ppu_t*)calloc(1, sizeof(nes_ppu_t));
        nes_cart = (nes_cartridge_t*)calloc(1, sizeof(nes_cartridge_t));
            
        cpu_clocks = 0;
        ppu_clocks = 0;
        ppu_rest_clocks = 0;
        ppu_clock_index = 0;
        ppu_status = 0;

 	}
	
	inline ~NesEmu() {
	}

    bool loadGame(char* path){
       /* init cartridge */
        nes_cart_init(nes_cart, nes_memory);

        /* load rom */
        if(nes_cart_load_rom(nes_cart, path) != 0)
        {
            printf("ROM does not exist\n");
            return false;
        }

        /* init cpu */
        nes_cpu_init(nes_cpu, nes_memory);
        nes_cpu_reset(nes_cpu);

        /* init ppu */
        nes_ppu_init(nes_ppu, nes_memory);

        emuScr.w = 256;
        emuScr.h = 240;
        emuScr.buffer = nes_ppu->screen_bitmap;
        emuScr.need_free = false; 

        return true;
    } 

protected:
	void onEvent(xevent_t* ev) {
	}

	void onRepaint(graph_t* g) {
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
       graph_blt(&emuScr, 0, 0, 256, 240, g, 0, 0, 256, 240); 
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint();
}

int main(int argc, char *argv[])
{
	char* path;
	NesEmu emu;
    /*init window*/
    xscreen_t scr;

    if(argc < 2){
		path = "/data/roms/mario.nes";
    }else{
		path = argv[1];
	}

    if(emu.loadGame(path) != true){
        printf("Error load rom file:%s\n", argv[1]);
        return -1;
    }

	X x;
	x.screenInfo(scr);

	x.open(&emu, scr.size.w /2  - 128 , scr.size.h /2 - 128, 256, 256, "NesEmu", X_STYLE_NO_RESIZE);
	emu.setVisible(true);

	x.run(loop, &emu);

    return 0;
}
