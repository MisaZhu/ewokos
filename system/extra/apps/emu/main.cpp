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
#include <sys/kernel_tic.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <x++/X.h>

#include "src/InfoNES_Types.h"
#include "src/InfoNES.h"
#include "src/InfoNES_System.h"
#include "src/InfoNES_pAPU.h"

using namespace Ewok;
   /* NES part */

#define KEY_TIMEOUT 2	

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

/* Transfer the contents of work frame on the screen */
inline void InfoNes_LoadLineScale2(uint32_t *fb, WORD* frame, int width){
	do{
		int i = *frame++ % 64;
		*fb++ = *fb++ = RGBPalette[i];
	}while(width--);
}

inline void InfoNES_LoadFrameScale2(void){
  int offX = (screen->w - NES_DISP_WIDTH*2)/2;
  int offY =(screen->h - NES_DISP_HEIGHT*2)/2; 
  uint32_t* d=(uint32_t *)screen->buffer + offY*screen->w + offX; 
  WORD* s = WorkFrame;
  for(int y=0; y<NES_DISP_HEIGHT; y++){
	InfoNes_LoadLineScale2(d, s, NES_DISP_WIDTH);
	d+=screen->w;
	InfoNes_LoadLineScale2(d, s, NES_DISP_WIDTH);
    d+=screen->w; 
	s+=NES_DISP_WIDTH;
  }
}

inline void InfoNes_LoadLineScale1(uint32_t *fb, WORD* frame, int width){
	do{
		int i = *frame++ % 64;
		*fb++ = RGBPalette[i];
	}while(width--);
}

inline void InfoNES_LoadFrameScale1(void){
  int offX = (screen->w - NES_DISP_WIDTH)/2;
  int offY =(screen->h - NES_DISP_HEIGHT)/2; 
  uint32_t* d=(uint32_t *)screen->buffer + offY*screen->w + offX; 
  WORD* s = WorkFrame;
  for(int y=0; y<NES_DISP_HEIGHT; y++){
	InfoNes_LoadLineScale1(d, s, NES_DISP_WIDTH);
	d+=screen->w;
	s+=NES_DISP_WIDTH;
  }
}


void InfoNES_LoadFrame(){
	if(screen->w >= NES_DISP_WIDTH * 2 && screen->h >= NES_DISP_HEIGHT * 2)
		InfoNES_LoadFrameScale2();
	else
		InfoNES_LoadFrameScale1();	
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
 	}
	
	inline ~NesEmu() {
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
		if(wait > 0)
			usleep(wait);

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

int main(int argc, char *argv[])
{
	const char* path;
	NesEmu emu;

    /*init window*/
    xscreen_t scr;

	//init emulator
    if(argc < 2){
        path = X::getResName("roms/mario.nes");
    }else{
        path = argv[1];
    }

	printf("load game: %s\n", path);
    if(emu.loadGame((char*)path) != true){
        printf("Error load rom file:%s\n", argv[1]);
        return -1;
    }

	X x;
	x.screenInfo(scr, 0);

	//x.open(&emu, scr.size.w /2  - 128 , scr.size.h /2 - 128, 256, 256, "NesEmu", X_STYLE_NO_RESIZE);
	//x.open(&emu, 10, 10, scr.size.w-20, scr.size.h-20, "NesEmu", X_STYLE_NORMAL);
	x.open(&scr, &emu, 256, 241, "NesEmu", X_STYLE_NO_RESIZE);
	emu.setVisible(true);

	x.run(loop, &emu);

    return 0;
}
