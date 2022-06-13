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
#include <sys/keydef.h>
#include <sys/klog.h>
#include <x++/X.h>
#include "Emulator.h" 

using namespace Ewok;
   /* NES part */

#define KEY_TIMEOUT 2	

class NesEmu : public XWin {
private:
	 sn::Emulator *emulator;
	 uint32_t lastSec;
	 uint64_t lastUsec;
	 uint32_t keyStatus;
	 uint8_t keyTimeout[8]; 

public:
	inline NesEmu() {
		emulator = new sn::Emulator();
		keyStatus = 0;
		memset(keyTimeout, 0, 8);
 	}
	
	inline ~NesEmu() {
	}

    bool loadGame(char* path){
		return emulator->loadGame(path);	
    } 

protected:
	void waitForNextFrame(){
		uint32_t sec;
		uint64_t usec;
		int wait;

		kernel_tic(&sec, &usec);
		wait = 1000000/60 - ((sec - lastSec) * 1000000 + (usec - lastUsec)); 
		if(wait > 0)
			usleep(wait);

		
		kernel_tic(&lastSec, &lastUsec);
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM){	
			switch(ev->value.im.value){
				case KEY_BUTTON_A:
					keyStatus|=0x1;
					keyTimeout[0] = KEY_TIMEOUT;
					break;
				case KEY_BUTTON_B:
					keyStatus|=0x2;
					keyTimeout[1] = KEY_TIMEOUT;
					break;
				case KEY_BUTTON_SELECT:
					keyStatus|=0x4;
					keyTimeout[2] = KEY_TIMEOUT;
					break;
				case KEY_BUTTON_START:
					keyStatus|=0x8;
					keyTimeout[3] = KEY_TIMEOUT;
					break;
				case KEY_UP:
					keyStatus |= 0x10;
					keyTimeout[4] = KEY_TIMEOUT;
					break;
				case KEY_DOWN:
					keyStatus |= 0x20;
					keyTimeout[5] = KEY_TIMEOUT;
					break;
				case KEY_LEFT:
					keyStatus |= 0x40;
					keyTimeout[6] = KEY_TIMEOUT;
					break;
				case KEY_RIGHT:
					keyStatus |= 0x80;
					keyTimeout[7] = KEY_TIMEOUT;
					break;
				default:
					break;
			}
		}
		//emulator->setKey(keyStatus);	
		(void)ev;
	}

	void onRepaint(graph_t* g) {
		for(int i = 0; i < 8; i++){
			if(keyTimeout[i])
				keyTimeout[i]--;
			else{
				keyStatus &= ~(0x1<<i);	
			}
		}
		emulator->setKey(keyStatus);	
		emulator->setFrameBuffer(g->w, g->h, g->buffer);
		emulator->run(29815);
		//waitForNextFrame();
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
		path = (char*)"/data/roms/mario.nes";
    }else{
		path = argv[1];
	}

    if(emu.loadGame(path) != true){
        printf("Error load rom file:%s\n", argv[1]);
        return -1;
    }

	X x;
	x.screenInfo(scr, 0);

	//x.open(&emu, scr.size.w /2  - 128 , scr.size.h /2 - 128, 256, 256, "NesEmu", X_STYLE_NO_RESIZE);
	x.open(&emu, 0 , 0, 640, 480, "NesEmu", X_STYLE_NO_RESIZE);
	emu.setVisible(true);

	x.run(loop, &emu);

    return 0;
}
