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
#include <sys/errno.h>
#include <ewoksys/proc.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/keydef.h>
#include <ewoksys/klog.h>
#include <ewoksys/timer.h>
#include <x++/X.h>
#include <pthread.h>

#define MIN(a, b) (((a) > (b))?(b):(a))

using namespace Ewok;

uint32_t *MacFB = NULL;

extern "C"{
	extern void* emu_main (const char* cfg);
	extern void update_mouse(int dx, int dy, uint8_t but);
}


class MacEmu : public XWin {
private:
	int offset_x = 0;
	int offset_y = 0;

public:
	inline MacEmu() {
	}
	
	inline ~MacEmu() {
	}

protected:
	void onEvent(xevent_t* ev) {
	static uint8_t but = 0;
		if(ev->type == XEVT_IM){	
			if(ev->state == XIM_STATE_PRESS){
				switch(ev->value.im.value){
					case KEY_BUTTON_A:
						but = 1;
						update_mouse(0,0, but);
						break;
					case KEY_BUTTON_B:
					case KEY_BUTTON_SELECT:
					case KEY_BUTTON_START:
						break;
					case KEY_UP:
						update_mouse(0,-5, but);
						break;
					case KEY_DOWN:
						update_mouse(0,5, but);
						break;
					case KEY_LEFT:
						update_mouse(-5,0, but);
						break;
					case KEY_RIGHT:
						update_mouse(5,0, but);
						break;
					default:
						break;
				}
			}else{
				switch(ev->value.im.value){
					case KEY_BUTTON_A:
						but = 0;
						update_mouse(0,0, but);
						break;
					case KEY_BUTTON_B:
					case KEY_BUTTON_SELECT:
					case KEY_BUTTON_START:
					case KEY_UP:
					case KEY_DOWN:
					case KEY_LEFT:
					case KEY_RIGHT:
					default:
						break;
				}
			}
		}else if(ev->type == XEVT_MOUSE){
			gpos_t pos =  getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
			if(ev->state == XEVT_MOUSE_DOWN || ev->state == XEVT_MOUSE_DRAG)
				update_mouse(pos.x - offset_x, pos.y - offset_y, 1);
			else
				update_mouse(pos.x - offset_x, pos.y - offset_y, 0);
		}
	}

	void onRepaint(graph_t* g) {
		if(!MacFB)
			return;
		offset_x = (g->w  - 512)/2;
		if(offset_x < 0)
			offset_x = 0;

		offset_y = (g->h  - 342)/2;
		if(offset_y < 0)
			offset_y = 0;


		int end_y = MIN(offset_y + 342, g->h);
		int end_x = MIN(offset_x + 512, g->w);

		for(int y = offset_y; y < end_y; y++){
			uint32_t *dst = g->buffer + y * g->w;
			uint32_t *src = MacFB + (y - offset_y) * 512;
			for(int x = offset_x; x < end_x; x++){
				dst[x] = *src++;
			}
		}
	}

};

extern "C"{
	MacEmu* XWIN = NULL;
	
	void repaint(uint32_t* fb){
		MacFB = (uint32_t*)fb;
	}
}

static void loop(void* p) {
	MacEmu* xwin = (MacEmu*)p;
	XWIN =  (MacEmu*)p;
	xwin->repaint();
	proc_usleep(30000);
}

inline void write_fd(int fd, char* data, int32_t size) {
    while(size) {
        int sz = write(fd, data, size);
		if(sz < 0 && errno == EAGAIN){
			continue;
		}
		if(sz <= 0){
			break;
		}
        size -= sz;
        data += sz;
    }
}

#define COPY_SIZE 128*1024
void copyfile(const char* src, const char* dst) {
    char *buf;
	int total = 0;

	buf = (char*)malloc(COPY_SIZE);
	if(!buf){
		return;
	}

    int src_fd = open(src, 0);
    if(src_fd < 0) {
        printf("'%s' not found!\n", src);
        return;
    }
	printf("copy %s -> %s\n", src, dst);
	int dst_fs = open(dst, O_WRONLY | O_CREAT);
	close(dst_fs);

	dst_fs = open(dst, O_WRONLY);
    while(1) {
        int sz = read(src_fd, buf, COPY_SIZE);
		if(sz <  0 && errno == EAGAIN){
			continue;
		}
		if(sz <= 0){
			break;
		}
        write_fd(dst_fs, buf, sz);
    }

	free(buf);
	close(dst_fs);
	close(src_fd);
}

int mkdir(const char* path){
	 fsinfo_t info;
    if(vfs_create(path, &info, FS_TYPE_DIR, 0775, false, true) != 0) {
        printf("mkdir '%s' failed!\n", path);
        return -1;
    }
    return 0;
}

void env_init(void){
	mkdir("/tmp/run");
    //char* path = (char*)X::getResName("hd1.img");
	//copyfile(path, "/tmp/run/hd1.img");
	char *path =  (char*)X::getResName("mac-plus-pram.dat");
	copyfile(path, "/tmp/run/mac-plus-pram.dat");
//	copyfile("/apps/macplus/res/rom/mac-plus.rom", "/tmp/res/rom/mac-plus.rom");
//	copyfile("/apps/macplus/res/rom/macplus-pcex.rom", "/tmp/res/rom/macplus-pcex.rom");
//	copyfile("/apps/macplus/res/fd2.image", "/tmp/res/fd2.image");
//	copyfile("/apps/macplus/res/mac-plus.cfg", "/tmp/res/mac-plus.cfg");
//	copyfile("/apps/macplus/res/fd1.image", "/tmp/res/fd1.image");
}

void* emu_thread(void* param){
	env_init();
	return emu_main((char*)X::getResName("mac-plus.cfg"));
}

int main(int argc, char *argv[])
{
	MacEmu emu;

	X x;

	emu.open(&x, 0, -1, -1, 512, 342, "Macintosh", XWIN_STYLE_NO_RESIZE);
	//emu.fullscreen();

	pthread_t tid;
	pthread_create(&tid, NULL, emu_thread, NULL);

	x.run(loop, &emu);

    return 0;
}
