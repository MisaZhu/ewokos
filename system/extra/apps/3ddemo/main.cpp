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

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/proc.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/klog.h>
#include <x++/X.h>
#include <ferox.h>

#include "src/engine.h"

using namespace Ewok;
xscreen_t scr;
graph_t* graph;

extern "C"{
void draw_pixel(int x, int y, uint32_t color){
	graph_pixel_safe(graph, x, y, color);
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color){

}
}

class Demo : public XWin {
private:
	int width = 0;
	int height = 0;
	
public:
	inline Demo() {

 	}
	
	inline ~Demo() {
	}

protected:

	void onRepaint(graph_t* g) {
		if(width != g->w || height != g->h){
			setup(g->w, g->h);
			width = g->w;
			height = g->h;
		}

		graph_clear(g, 0xff000000);
		graph = g;
		update();
		render();
	}

public:
    void waitForNextFrame(){
		//proc_usleep(16667);
    }
};

static void loop(void* p) {
	Demo* xwin = (Demo*)p;
	xwin->repaint();
	xwin->waitForNextFrame();
}

int main(int argc, char *argv[])
{
	const char* path;

	Demo demo;
    /*init window*/

	X x;
	x.getScreenInfo(scr, 0);
	demo.open(&x, 0, -1, -1, scr.size.w/2, scr.size.h/2, "Demo", XWIN_STYLE_NORMAL);
	x.run(loop, &demo);

    return 0;
}
