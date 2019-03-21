#include "mailbox.h"
#include "dev/fb.h"

void __mem_barrier();

#define VIDEO_FB_CHANNEL MAIL_CH_FBUFF
#define VIDEO_INIT_RETRIES 3
#define VIDEO_INITIALIZED 0
#define VIDEO_ERROR_RETURN 1
#define VIDEO_ERROR_POINTER 2

#define MAIL_CH_FBUFF 0x00000001

int32_t videoInit(fb_info_t *p_fbinfo) {
	uint32_t init = VIDEO_INIT_RETRIES;
	uint32_t test, addr = ((uint32_t)p_fbinfo);
	while(init>0) {
		__mem_barrier();
		mailboxWrite(VIDEO_FB_CHANNEL,addr);
		__mem_barrier();
		test = mailboxRead(VIDEO_FB_CHANNEL);
		__mem_barrier();
		if (test) 
			test = VIDEO_ERROR_RETURN;
		else if(p_fbinfo->pointer == 0x0)
			test = VIDEO_ERROR_POINTER;
		else { 
			test = VIDEO_INITIALIZED; break; 
		}
		init--;
	}
	return test;
}

static fb_info_t _fbInfo __attribute__((aligned(16)));

inline fb_info_t* _fb_get_info() {
	return &_fbInfo;
}

bool _fb_init() {
	TagsInfoT info;
	mailboxGetVideoInfo(&info);
	/** initialize fbinfo */
	_fbInfo.height = info.fb_height;
	_fbInfo.width = info.fb_width;
	_fbInfo.vheight = info.fb_height;
	_fbInfo.vwidth = info.fb_width;
	_fbInfo.pitch = 0;
	_fbInfo.depth = 32;
	_fbInfo.xoffset = 0;
	_fbInfo.yoffset = 0;
	_fbInfo.pointer = 0;
	_fbInfo.size = 0;

	if(videoInit(&_fbInfo) == 0)
		return true;
	return false;
}

