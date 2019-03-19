#include "mm/mmu.h"
#include "dev/fb.h"
#include "hardware.h"

int32_t videoInit(FBInfoT *fbInfo) {
	*((uint32_t*)(MMIO_BASE | 0x1c)) = 0x2c77; //640x480
	*((uint32_t*)(MMIO_BASE | 0x00120000)) = 0x3f1f3f9c; 
	*((uint32_t*)(MMIO_BASE | 0x00120004)) = 0x090b61df; 
	*((uint32_t*)(MMIO_BASE | 0x00120008)) = 0x067f1800; 
	*((uint32_t*)(MMIO_BASE | 0x00120010)) = fbInfo->pointer; 
	*((uint32_t*)(MMIO_BASE | 0x00120018)) = 0x082b;
	return 0;
}

static FBInfoT _fbInfo __attribute__((aligned(16)));

inline FBInfoT* fbGetInfo() {
	return &_fbInfo;
}

bool fbInit() {
	// initialize fbinfo
	_fbInfo.height = 480;
	_fbInfo.width = 640;
	_fbInfo.vheight = 480;
	_fbInfo.vwidth = 640;
	_fbInfo.pitch = 0;
	_fbInfo.depth = 32;
	_fbInfo.xoffset = 0;
	_fbInfo.yoffset = 0;
	_fbInfo.pointer = V2P(_fbStart);
	_fbInfo.size = 0;

	if(videoInit(&_fbInfo) == 0)
		return true;
	return false;
}

