#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console/console.h>
#include <sconf/sconf.h>
#include <sys/vfs.h>
#include <sys/keydef.h>
#include <sys/vdevice.h>
#include <sys/klog.h>
#include <ttf/ttf.h>
#include <sys/basic_math.h>
#include <x++/X.h>

using namespace Ewok;

typedef struct {
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t buffer_rows;
} conf_t;

class XConsoled : public XWin {
	conf_t conf;
	console_t console;
	int32_t mouse_last_y;
public:
	XConsoled() {
		console_init(&console);
		console.show_cursor = false;
	}

	~XConsoled() {
		console_close(&console);
	}

	bool readConfig(const char* fname) {
		memset(&conf, 0, sizeof(conf_t));
		sconf_t *sconf = sconf_load(fname);	
		if(sconf == NULL)
			return false;

		const char* v = sconf_get(sconf, "bg_color");
		if(v[0] != 0) 
			conf.bg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "fg_color");
		if(v[0] != 0) 
			conf.fg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "buffer_rows");
		if(v[0] != 0) 
			conf.buffer_rows = atoi(v);

		uint32_t font_size = 16;
		v = sconf_get(sconf, "font_size");
		if(v[0] != 0) 
			font_size = atoi(v);

		v = sconf_get(sconf, "font");
		if(v[0] == 0) 
			v = "/data/fonts/system.ttf";
		
		font_load(v, font_size, &console.font);

		v = sconf_get(sconf, "font_margin");
		if(v[0] != 0) 
			console.font_margin = atoi(v);

		sconf_free(sconf);

		console.fg_color = conf.fg_color;
		console.bg_color = conf.bg_color;
		return true;
	}

	void put(const char* buf, int size) {
		console_put_string(&console, buf, size);
		repaint();
	}
protected:
	void onRepaint(graph_t* g) {
		if(console.w != g->w || console.h != g->h) {
			uint32_t buffer_rows = 0;
			if(console.font.id >=0) {
				buffer_rows = (g->h / console.font.max_size.y)*4;
			}
			if(conf.buffer_rows > buffer_rows) {
				buffer_rows = conf.buffer_rows;
			}
			console_reset(&console, g->w, g->h, buffer_rows);
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
	/*int i;
	for(i=0; i<size; i++) {
		char c = pb[i];
		console_put_char(&console->console, c);
	}
	*/
	xwin->put(pb, size);
	return size;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/xconsole";

	XConsoled xwin;
	xwin.readConfig("/etc/x/xconsoled.conf");

	X x;
	xscreen_t scr;
 	x.screenInfo(scr, 0);
	x.open(&xwin, 0, 0, scr.size.w, scr.size.h, "xconsole",
			X_STYLE_NO_FRAME | X_STYLE_NO_FOCUS | X_STYLE_SYSBOTTOM | X_STYLE_ALPHA);
	xwin.setVisible(true);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xconsole");
	dev.write = console_write;
	dev.extra_data = &xwin;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	xwin.close();
	return 0;
}