#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console/console.h>
#include <sconf/sconf.h>
#include <ewoksys/vfs.h>
#include <ewoksys/keydef.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/klog.h>
#include <ttf/ttf.h>
#include <ewoksys/basic_math.h>
#include <x++/X.h>

using namespace Ewok;

typedef struct {
	uint32_t fg_color;
	uint32_t bg_color;
} conf_t;

class XConsoled : public XWin {
	conf_t conf;
	console_t console;
	int32_t mouse_last_y;
	uint32_t hide_count;
	uint32_t auto_hide;
	uint32_t height;
	uint32_t width;
public:
	XConsoled() {
		hide_count = 0;
		auto_hide = 6; //auto hide after 6 sec
		height = 0;
		width = 0;
		console_init(&console);
	}

	~XConsoled() {
		console_close(&console);
	}

	void tryHide() {
		usleep(100000); //count every 100 ms
		if(auto_hide == 0) //never hide
			return;
			
		hide_count++;
		if(hide_count > (auto_hide*10)) { //after 6 sec
			hide_count = 0;
			setVisible(false);
		}
	}

	uint32_t getHeight() {
			return height;
	}

	uint32_t getWidth() {
			return width;
	}

	void show() {
		hide_count = 0;
		setVisible(true);
	}

	bool readConfig(const char* fname) {
		memset(&conf, 0, sizeof(conf_t));
		klog("%s\n", fname);
		sconf_t *sconf = sconf_load(fname);	
		if(sconf == NULL)
			return false;

		const char* v = sconf_get(sconf, "bg_color");
		if(v[0] != 0) 
			conf.bg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "fg_color");
		if(v[0] != 0) 
			conf.fg_color = atoi_base(v, 16);

		uint32_t font_size = 16;
		v = sconf_get(sconf, "font_size");
		if(v[0] != 0) 
			font_size = atoi(v);
		console.textview.font_size = font_size;

		v = sconf_get(sconf, "font_fixed");
		if(v[0] != 0) 
			console.textview.font_fixed = atoi(v);
		
		v = sconf_get(sconf, "auto_hide");
		if(v[0] != 0) 
			auto_hide = atoi(v);

		v = sconf_get(sconf, "font");
		if(v[0] == 0) 
			v = DEFAULT_SYSTEM_FONT;
		
		console.textview.font = font_new(v, font_size, true);

		v = sconf_get(sconf, "height");
		if(v[0] != 0) 
			height = atoi(v);
		v = sconf_get(sconf, "width");
		if(v[0] != 0) 
			width = atoi(v);

		sconf_free(sconf);

		console.textview.fg_color = conf.fg_color;
		console.textview.bg_color = conf.bg_color;
		return true;
	}

	void put(const char* buf, int size) {
		console_put_string(&console, buf, size);
		repaint();
	}
protected:
	void onRepaint(graph_t* g) {
		if(console.textview.w != g->w || console.textview.h != g->h) {
			console_reset(&console, g->w, g->h);
		}
		console_refresh(&console, g);
	}
};

static int console_write(int fd, 
		int from_pid,
		fsinfo_t* info,
		const void* buf,
		int size,
		int offset,
		void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;

	XConsoled *xwin = (XConsoled*)p;
	if(size <= 0 || xwin == NULL)
		return 0;
	const char* pb = (const char*)buf;

	xwin->show();
	xwin->put(pb, size);
	return size;
}

int try_hide(void* p) {
	XConsoled* xwin = (XConsoled*)p;
	xwin->tryHide();
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/xconsole";

	XConsoled xwin;
	xwin.readConfig(x_get_theme_fname(X_THEME_ROOT, "xconsoled", "theme.conf"));

	X x;
	xscreen_t scr;
 	x.getScreenInfo(scr, 0);

	uint32_t w = xwin.getWidth();
	uint32_t h = xwin.getHeight();

	w = w==0 ? scr.size.w:w;
	h = h==0 ? scr.size.h:h;

	x.open(0, &xwin, (scr.size.w-w)/2, scr.size.h-h, w, h, "xconsoled",
			XWIN_STYLE_NO_TITLE | XWIN_STYLE_NO_FOCUS | XWIN_STYLE_SYSTOP | XWIN_STYLE_LAZY);
	xwin.setVisible(false);
	xwin.setAlpha(true);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xconsole");
	dev.write = console_write;
	dev.extra_data = &xwin;
	dev.loop_step = try_hide;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	xwin.close();
	return 0;
}