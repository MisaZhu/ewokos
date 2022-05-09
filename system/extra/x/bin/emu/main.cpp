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
#include "Emulator.h" 

using namespace Ewok;
   /* NES part */


class NesEmu : public XWin {
private:
	 sn::Emulator *emulator;
	 uint32_t lastSec;
	 uint64_t lastUsec;

public:
	inline NesEmu() {
		emulator = new sn::Emulator();
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
		(void)ev;
	}

	void onRepaint(graph_t* g) {
		emulator->setFrameBuffer(g->w, g->h, g->buffer);
		emulator->run(29815);
		waitForNextFrame();
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
	x.screenInfo(scr);

	x.open(&emu, scr.size.w /2  - 128 , scr.size.h /2 - 128, 256, 256, "NesEmu", X_STYLE_NO_RESIZE);
	emu.setVisible(true);

	x.run(loop, &emu);

    return 0;
}
