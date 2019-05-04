#include "mailbox.h"
#include "dev/fb.h"
#include "kstring.h"

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

static fb_info_t _fb_info __attribute__((aligned(16)));

bool fb_init() {
	TagsInfoT info;
	mailboxGetVideoInfo(&info);
	/** initialize fbinfo */
	_fb_info.height = info.fb_height;
	_fb_info.width = info.fb_width;
	_fb_info.vheight = info.fb_height;
	_fb_info.vwidth = info.fb_width;
	_fb_info.pitch = 0;
	_fb_info.depth = 32;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;
	_fb_info.pointer = 0;
	_fb_info.size = 0;

	if(videoInit(&_fb_info) == 0)
		return true;
	return false;
}

int32_t dev_fb_info(int16_t id, void* info) {
	(void)id;

	fb_info_t* fb_info = (fb_info_t*)info;
	memcpy(fb_info, &_fb_info, sizeof(fb_info_t));
	return 0;
}

int32_t dev_fb_write(int16_t id, void* buf, uint32_t size) {
	(void)id;
	uint32_t sz = (_fb_info.depth/8) * _fb_info.width * _fb_info.height;
	if(size > sz)
		size = sz;
	memcpy((void*)_fb_info.pointer, buf, size);
	return (int32_t)size;
}

