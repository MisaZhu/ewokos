#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ewoksys/md5.h>
#include <ewoksys/mstr.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/keydef.h>
#include <ewoksys/session.h>
#include <ewoksys/klog.h>
#include <graph/graph.h>
#include <graph/graph_ex.h>
#include <graph/graph_png.h>
#include <display/display.h>
#include <fb/fb.h>
#include <font/font.h>

typedef struct {
    fb_t* fb;
    graph_t* scr_g;
    font_t* font;
	graph_t* img;
	uint32_t persantage;

	uint32_t w;
	uint32_t h;
	uint32_t item_h;
	uint32_t font_size;
} splashd_info_t;

static splashd_info_t _splash_info;

static uint32_t  _font_size = DEFAULT_SYSTEM_FONT_SIZE;
static uint32_t  _w = 240;
static uint32_t  _h = 240;
static bool      _dark_mode = false;

static void paint_bg(void) {
	graph_t* g = _splash_info.scr_g;
	if(_dark_mode)
		graph_clear(g, 0xFF000000);
	else
		graph_clear(g, 0xFFFFFFFF);

	/*
	int32_t off_x = (g->w- _w)/2;
	int32_t off_y = (g->h- _h)/2;
	graph_box_3d(g, off_x, off_y, _w, _h, 0xFFaaaaaa, 0xFF444444);
	*/
}

static void paint_img(const char* img_fname) {
	graph_t* g = _splash_info.scr_g;
	graph_t* img = NULL;
	if(img_fname != NULL && img_fname[0] != 0)
		img = png_image_new(img_fname);

	if(img != NULL) {
		if(_splash_info.img != NULL)
			graph_free(_splash_info.img);
		_splash_info.img = img;
	}

	img = _splash_info.img;
	if(img != NULL) {
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, (g->w-img->w)/2, (g->h-img->h)/2, img->w, img->h, 0xff);
	}
}

static void paint_progress(uint32_t persantage, int32_t off_y) {
	graph_t* g = _splash_info.scr_g;
	uint32_t color = 0xFFAAAAFF;
	int32_t off_x = (g->w-_splash_info.w)/2;

	uint32_t persantage_w = _splash_info.w * persantage / 100;

	graph_fill_3d(g, off_x-1, off_y, _splash_info.w+2, _splash_info.item_h, 
		graph_get_dark_color(color), true);
	graph_fill_3d(g, off_x, off_y+2, persantage_w, _splash_info.item_h-4, 
		color, false);
}

static void paint_msg(const char* msg, int32_t off_y) {
	graph_t* g = _splash_info.scr_g;
	uint32_t color = 0xFF000000;
	if(_dark_mode)
		color = 0xFFFFFFFF;
	
	if(msg != NULL && msg[0] != 0) {
		int32_t off_x = (g->w-_splash_info.w)/2;
		graph_draw_text_font(g, off_x, off_y,
				msg, _splash_info.font, _splash_info.font_size, color);
	}
}

static void paint(uint32_t persantage, const char* msg, const char* img_fname) {
	paint_bg();
	paint_img(img_fname);
	graph_t* g = _splash_info.scr_g;
	int32_t off_y = (g->h-_splash_info.h)/2 + 
		_splash_info.h - (_splash_info.item_h*2);

	paint_progress(persantage, off_y);
	off_y += _splash_info.item_h;
	paint_msg(msg, off_y);

	fb_flush(_splash_info.fb, true);
}

static void handle_ipc(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)p;
	pid = proc_getpid(pid);

	switch(cmd) {
	case 0: 
		{
			uint32_t persantage = proto_read_int(in);
			if(persantage == 0)
				persantage = _splash_info.persantage + 1;
			if(persantage > 100)
				persantage = 100;
			_splash_info.persantage = persantage;

			const char* msg = proto_read_str(in);
			const char* img_fname = proto_read_str(in);
			paint(persantage, msg, img_fname);
		}
		return;
	}
}

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "w:h:f:d");
		if(c == -1)
			break;

		switch (c) {
		case 'f':
			_font_size = atoi(optarg);
			break;
		case 'w':
			_w = atoi(optarg);
			break;
		case 'h':
			_h = atoi(optarg);
			break;
		case 'd':
			_dark_mode = true;
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	_font_size = DEFAULT_SYSTEM_FONT_SIZE;
	_w = 240;
	_h = 240;
	_dark_mode = false;

	doargs(argc, argv);

	fb_t fb;
	if(display_fb_open("/dev/display", 0, &fb) != 0)
		return -1;

	_splash_info.persantage = 0;
	_splash_info.w = _w - 8;
	_splash_info.h = _h - 8;
	_splash_info.font_size = _font_size;
	_splash_info.item_h = _splash_info.font_size+4;
	_splash_info.scr_g = fb_fetch_graph(&fb);
	_splash_info.img = NULL;
    _splash_info.font = font_new(DEFAULT_SYSTEM_FONT, true);
	_splash_info.fb = &fb;

	if(ipc_serv_reg("sys.splashd") != 0) {
		slog("reg sys.splashd ipc_serv error!\n");
		return -1;
	}

	ipc_serv_run(handle_ipc, NULL, NULL, IPC_NON_BLOCK);
	while(_splash_info.persantage < 100) {
		usleep(300000);
	}
	return 0;
}

