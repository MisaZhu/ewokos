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
#include <ewoksys/proc.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/keydef.h>
#include <ewoksys/klog.h>
#include <ewoksys/timer.h>
#include <x++/X.h>

#include "src/InfoNES_Types.h"
#include "src/InfoNES.h"
#include "src/InfoNES_System.h"
#include "src/InfoNES_pAPU.h"

using namespace Ewok;
   /* NES part */

#define KEY_TIMEOUT 2	
#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))

WORD padState;

DWORD RGBPalette[64] = {
	0xff707070,0xff201888,0xff0000a8,0xff400098,0xff880070,0xffa80010,0xffa00000,0xff780800,
	0xff402800,0xff004000,0xff005000,0xff003810,0xff183858,0xff000000,0xff000000,0xff000000,
	0xffb8b8b8,0xff0070e8,0xff2038e8,0xff8000f0,0xffb800b8,0xffe00058,0xffd82800,0xffc84808,
	0xff887000,0xff009000,0xff00a800,0xff009038,0xff008088,0xff000000,0xff000000,0xff000000,
	0xfff8f8f8,0xff38b8f8,0xff5890f8,0xff4088f8,0xfff078f8,0xfff870b0,0xfff87060,0xfff89838,
	0xfff0b838,0xff80d010,0xff48d848,0xff58f898,0xff00e8d8,0xff000000,0xff000000,0xff000000,
	0xfff8f8f8,0xffa8e0f8,0xffc0d0f8,0xffd0c8f8,0xfff8c0f8,0xfff8c0d8,0xfff8b8b0,0xfff8d8a8,
	0xfff8e0a0,0xffe0f8a0,0xffa8f0b8,0xffb0f8c8,0xff98f8f0,0xff000000,0xff000000,0xff000000,
};

WORD NesPalette[ 64 ] =
{
	0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f
};

graph_t *screen;
graph_t *paint;

/* Menu screen */
int InfoNES_Menu(){
	return 0;
}

/* Read ROM image file */
int InfoNES_ReadRom( const char *pszFileName ){
 
  FILE *fp;

  /* Open ROM file */
  fp=fopen(pszFileName,"rb");
  if(fp==NULL) return -1;

  /* Read ROM Header */
  fread( &NesHeader, 1, sizeof NesHeader, fp );
  if( memcmp( NesHeader.byID, "NES\x1a", 4 )!=0){
    /* not .nes file */
    fclose( fp );
    return -1;
  }
  /* Clear SRAM */
  memset( SRAM, 0, SRAM_SIZE );

  /* If trainer presents Read Triner at 0x7000-0x71ff */
  if(NesHeader.byInfo1 & 4){
    fread( &SRAM[ 0x1000 ], 1, 512, fp );
  }
  
  /* Allocate Memory for ROM Image */
  ROM = (BYTE *)malloc( NesHeader.byRomSize * 0x4000 );

  /* Read ROM Image */
  int ret = fread( ROM, 1, NesHeader.byRomSize * 0x4000, fp );

  if(NesHeader.byVRomSize>0){
    /* Allocate Memory for VROM Image */
    VROM = (BYTE *)malloc( NesHeader.byVRomSize * 0x2000 );

    /* Read VROM Image */
    ret = fread( VROM, 1, NesHeader.byVRomSize * 0x2000, fp );
  }

  /* File close */
  fclose( fp );

  /* Successful */
  return 0;
}

/* Release a memory for ROM */
void InfoNES_ReleaseRom(){

}

static int scale = 1;
void graph_scale_fix_center(graph_t *src, graph_t *dst){
	static int dstW,dstH;
	static int sx;
	static int sy; 
	static int ex;
	static int ey;

	dstW = dst->w;
	dstH = dst->h;

	sx = MAX((dstW- src->w * scale)/2, 0);
	ex = MIN(sx + (src->w * scale), dst->w);
	sy = MAX((dstH - src->h * scale)/2, 0);
	ey = MIN(sy + (src->h * scale), dst->h);
	int dstY = sy;
	int srcY = 0;

	if(scale == 2 || scale == 4) {
		int shift = scale / 2;
		for(; dstY < ey; dstY++){
			int dstX = sx;
			int srcX = 0;
			uint32_t *d = dst->buffer + dstY * dst->w;
			uint32_t *s = src->buffer + (srcY/scale)*src->w;
			for(; dstX < ex; dstX++){
				d[dstX] = s[srcX/shift];
				srcX++;
				if(srcX == 4)
					break;
			}
			for(; dstX < ex; dstX++){
				d[dstX] = s[srcX>>shift];
				srcX++;
			}
			srcY++;
			if(srcY >= 4)
				break;
		}
		for(; dstY < ey; dstY++){
			int dstX = sx;
			int srcX = 0;
			uint32_t *d = dst->buffer + dstY * dst->w;
			uint32_t *s = src->buffer + (srcY>>shift)*src->w;
			for(; dstX < ex; dstX++){
				d[dstX] = s[srcX/shift];
				srcX++;
				if(srcX == 4)
					break;
			}
			for(; dstX < ex; dstX++){
				d[dstX] = s[srcX>>shift];
				srcX++;
			}
			srcY++;
		}
	}
	else {
		for(; dstY < ey; dstY++){
			int dstX = sx;
			int srcX = 0;
			uint32_t *d = dst->buffer + dstY * dst->w;
			uint32_t *s = src->buffer + srcY/scale*src->w;
			for(; dstX < ex; dstX++){
				d[dstX] = s[srcX/scale];
				srcX++;
			}
			srcY++;
		}
	}
}

void InfoNES_LoadFrame(){
	WORD* s = WorkFrame;
	uint32_t* d=(uint32_t *)paint->buffer;  
	for(int i= 0; i < NES_DISP_WIDTH*NES_DISP_HEIGHT; i++ ){
		int idx = *s++ % 64;
		d[i] = RGBPalette[idx];
	};

	graph_scale_fix_center(paint, screen);
}

/* Get a joypad state */
void InfoNES_PadState( DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem ){
	*pdwPad1 = padState;
}

/* memcpy */
void *InfoNES_MemoryCopy( void *dest, const void *src, int count ){
	return memcpy(dest, src, count);
}


/* memset */
void *InfoNES_MemorySet( void *dest, int c, int count ){
	return memset(dest, 0, count);
}

/* Print debug message */
void InfoNES_DebugPrint( char *pszMsg ){

}

/* Wait */
void InfoNES_Wait(){

}

/* Sound Initialize */
void InfoNES_SoundInit( void ){

}

/* Sound Open */
int InfoNES_SoundOpen( int samples_per_sync, int sample_rate ){
	return 0;
}

/* Sound Close */
void InfoNES_SoundClose( void ){

}

/* Sound Output 5 Waves - 2 Pulse, 1 Triangle, 1 Noise, 1 DPCM */
void InfoNES_SoundOutput(int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5){

}

/* Print system message */
void InfoNES_MessageBox( char *pszMsg, ... ){

}

class NesEmu : public XWin {
private:
     uint32_t lastSec = 0;
     uint64_t lastUsec = 0;

public:
	inline NesEmu() {
		padState = 0;
		paint = graph_new(NULL, 256, 240);
	}
	
	inline ~NesEmu() {
		graph_free(paint);
	}

    bool loadGame(char* path){
		int i = InfoNES_Load(path);
		InfoNES_Init();
		lastSec = 0;
		lastUsec = 0;
		return true;
    } 

	void waitForNextFrame(){
		uint32_t sec;
		uint64_t usec;
		int wait;

		kernel_tic(&sec, &usec);
		wait = 1000000/60 - ((sec - lastSec) * 1000000 + (usec - lastUsec)); 
		if(wait > 0 && wait < (1000000/60))
			proc_usleep(wait);

		kernel_tic(&lastSec, &lastUsec);
	}

protected:
	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM){	
			if(ev->state == XIM_STATE_PRESS){
				switch(ev->value.im.value){
					case KEY_BUTTON_A:
						padState |= 0x1;
						break;
					case KEY_BUTTON_B:
						padState |= 0x2;
						break;
					case KEY_BUTTON_SELECT:
						padState |= 0x4;
						break;
					case KEY_BUTTON_START:
						padState |= 0x8;
						break;
					case KEY_UP:
						padState |= 0x10;
						break;
					case KEY_DOWN:
						padState |= 0x20;
						break;
					case KEY_LEFT:
						padState |= 0x40;
						break;
					case KEY_RIGHT:
						padState |= 0x80;
						break;
					default:
						break;
				}
			}else{
				switch(ev->value.im.value){
					case KEY_BUTTON_A:
						padState &= ~0x1;
						break;
					case KEY_BUTTON_B:
						padState &= ~0x2;
						break;
					case KEY_BUTTON_SELECT:
						padState &= ~0x4;
						break;
					case KEY_BUTTON_START:
						padState &= ~0x8;
						break;
					case KEY_UP:
						padState &= ~0x10;
						break;
					case KEY_DOWN:
						padState &= ~0x20;
						break;
					case KEY_LEFT:
						padState &= ~0x40;
						break;
					case KEY_RIGHT:
						padState &= ~0x80;
						break;
					default:
						break;
				}
			}
		}
	}

	void onRepaint(graph_t* g) {
		static int framecnt= 0;
		screen = g;
		InfoNES_Cycle();
		//printf("wait\n");
	}

};

static void loop(void* p) {
	NesEmu* xwin = (NesEmu*)p;
	xwin->repaint();
	xwin->waitForNextFrame();
}

/*static NesEmu* _xwin;
static void loop(void) {
	NesEmu* xwin = _xwin;
	xwin->repaint();
}
*/

int main(int argc, char *argv[])
{
	const char* path;
	NesEmu emu;

	/*init window*/
	xscreen_t scr;

	//init emulator
	if(argc < 2){
			path = X::getResName("roms/nes1200in1.nes");
	}else{
			path = argv[1];
	}

	printf("load game: %s\n", path);
	if(emu.loadGame((char*)path) != true){
			printf("Error load rom file:%s\n", argv[1]);
			return -1;
	}

	X x;
	x.getScreenInfo(scr, 0);

	int zoom;
	if(scr.size.h > 240)
		zoom = scr.size.h / 240;
	if(zoom >= 4)
		zoom = 4;
	else if(zoom >= 2)
		zoom = 2;
	else 
		zoom = 1;
	scale = zoom;

	emu.open(&x, 0, -1, -1, 256*zoom, 240*zoom, "NesEmu", XWIN_STYLE_NO_RESIZE);
	//emu.fullscreen();
	/*_xwin = &emu;
	uint32_t tid = timer_set(10000, loop);
	x.run(NULL, &emu);
	timer_remove(tid);
	*/

	x.run(loop, &emu);
	return 0;
}
