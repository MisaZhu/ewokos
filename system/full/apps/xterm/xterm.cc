#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/errno.h>

#ifdef __cplusplus
}
#endif

#include <gterminal/gterminal.h>
#include <sconf/sconf.h>
#include <ewoksys/vfs.h>
#include <ewoksys/keydef.h>
#include <ewoksys/ipc.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>
#include <ttf/ttf.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/timer.h>
#include <x++/X.h>
#include <pthread.h>

using namespace Ewok;

static charbuf_t *_buffer;

class XConsole : public XWin {
	gterminal_t terminal;
	int32_t rollStepRows;
	int32_t mouse_last_y;

	void drawBG(graph_t* g) {
		graph_clear(g, terminal.bg_color);
		if(color_a(terminal.bg_color) != 0xff)
			setAlpha(true);

		/*uint32_t cw = g->w / terminal.terminal.cols;
		uint32_t ch = g->h / terminal.terminal.rows;
		uint32_t i = 0;
		while(i < g->w) {
			i += cw;
			graph_line(g, i, 0, i, g->h, 0xff222222);
		}

		i = 0;
		while(i < g->h) {
			i += ch;
			graph_line(g, 0, i, g->w, i, 0xff222222);
		}
		*/
	}

	bool readConfigRaw(const char* fname) {
		sconf_t *sconf = sconf_load(fname);	
		if(sconf == NULL)
			return false;
		theme.loadConfig(sconf);
		sconf_free(sconf);
		return true;
	}
public:
	XConsole() {
		gterminal_init(&terminal);
	}

	~XConsole() {
		gterminal_close(&terminal);
	}

	bool readConfig(const char* fname) {
		readConfigRaw(fname);
		terminal.bg_color = theme.basic.bgColor;
		terminal.fg_color = theme.basic.fgColor;
		terminal.font_size = theme.basic.fontSize;
		terminal.font_fixed = theme.basic.fontFixedSize;
		if(terminal.font != NULL)
			font_free(terminal.font);
		terminal.font = font_new(theme.basic.fontName, terminal.font_size, true);
		return true;
	}

	void put(const char* buf, int size) {
		gterminal_put(&terminal, buf, size);
	}

	void flash() {
		gterminal_flash(&terminal);
		repaint();
	}

protected:
	void onFocus(void) {
		repaint();
		callXIM();
	}

	void onUnfocus(void) {
		repaint();
	}

	void onRepaint(graph_t* g) {
		drawBG(g);
		gterminal_paint(&terminal, g);
	}

	void onResize() {
		xinfo_t xinfo;
		getInfo(xinfo);
		gterminal_resize(&terminal, xinfo.wsr.w, xinfo.wsr.h);
	}
	
	void mouseHandle(xevent_t* ev) {
		if(ev->state == XEVT_MOUSE_DOWN) {
			mouse_last_y = ev->value.mouse.y;
			return;
		}
		else if(ev->state == XEVT_MOUSE_DRAG) {
			mouse_last_y = ev->value.mouse.y;
			//console_roll(&console, mv);
			repaint();
		}
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_PRESS) {
			int c = ev->value.im.value;
			if(c != 0) {
				charbuf_push(_buffer, c, false);
				proc_wakeup(RW_BLOCK_EVT);
			}
		}
		else if(ev->type == XEVT_MOUSE) {
			mouseHandle(ev);
			return;	
		}
	}
};

static XConsole* _xwin = NULL;
static vdevice_t* _dev = NULL;

static int dev_loop(void* p) {
	return 0;
}

static void timer_handler(void) {
	_xwin->flash();
}

static void win_loop(void* p) {
	proc_usleep(10000);
}

static void* thread_loop(void* p) {
	X* x = (X*)p;

	uint32_t timer_id = timer_set(500000, timer_handler);
	x->run(win_loop, _xwin);
	timer_remove(timer_id);
	klog("terminated!\n");
	_dev->terminated = true;
	return NULL;
}

#ifdef __cplusplus
extern "C" { extern int setenv(const char*, const char*);}
#endif

static int console_write(int fd, 
		int from_pid,
		fsinfo_t* node,
		const void* buf,
		int size,
		int offset,
		void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;

	XConsole *xwin = (XConsole*)p;
	if(size <= 0 || xwin == NULL)
		return 0;

	xwin->put((const char*)buf, size);
	xwin->repaint();
	return size;
}

static int console_read(int fd, int from_pid, fsinfo_t* node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)size;
	(void)node;

	char c;
	int res = charbuf_pop(_buffer, &c);

	if(res != 0)
		return VFS_ERR_RETRY;

	((char*)buf)[0] = c;
	return 1;
}

int run(const char* mnt_point) {
	_buffer = charbuf_new(0);

	XConsole xwin;
	xwin.readConfig(x_get_theme_fname(X_THEME_ROOT, "xconsole", "theme.conf"));

	X x;
	grect_t desk;
	x.getDesktopSpace(desk, 0);
	x.open(0, &xwin, desk.w*2/3, desk.h*2/3, "xconsole", 0);
	xwin.setVisible(true);

	_xwin = &xwin;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xconsole");
	dev.write = console_write;
	dev.read = console_read;
	dev.extra_data = &xwin;
	dev.loop_step = dev_loop;
	_dev = &dev;

	pthread_t tid;
	pthread_create(&tid, NULL, thread_loop, &x);

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	charbuf_free(_buffer);
	proc_wakeup(RW_BLOCK_EVT);
	xwin.close();
	exit(0);
}

#ifdef __cplusplus
extern "C" { extern int setenv(const char*, const char*);}
#endif

int main(int argc, char* argv[]) {
	const char* dev = "/dev/xconsole0";
	if(argc > 1) 
		dev = argv[1];

	int pid = fork();
	if(pid == 0) {
		if(run(dev) != 0) {
			exit(-1);
		}
	}
	else 
		ipc_wait_ready(pid);

	klog("open dev %s\n", dev);
	int fd = open(dev, O_RDONLY);
	if(fd < 0) {
		printf("Error: %s open failed\n", dev);
		return -1;
	}
	klog("open dev %d\n", fd);

	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);

	setenv("CONSOLE_ID", dev);
	proc_exec("/bin/shell");
	return 0;
}