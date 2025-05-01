#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/errno.h>
#include <signal.h>

#ifdef __cplusplus
}
#endif

#include <gterminal/gterminal.h>
#include <tinyjson/tinyjson.h>
#include <ewoksys/vfs.h>
#include <ewoksys/keydef.h>
#include <ewoksys/ipc.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/timer.h>
#include <ewoksys/wait.h>

#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Text.h>
#include <Widget/Label.h>
#include <WidgetEx/Menubar.h>
#include <WidgetEx/Menu.h>
#include <Widget/LabelButton.h>
#include <WidgetEx/FileDialog.h>
#include <WidgetEx/FontDialog.h>

#include <x++/X.h>
#include <pthread.h>

using namespace Ewok;

static charbuf_t *_buffer;

class ConsoleWidget : public Widget{
	gterminal_t terminal;
	int32_t rollStepRows;
	int32_t mouse_last_y;
	uint32_t scrollW;
	bool dirty;

	void drawBG(graph_t* g, const grect_t& r) {
		graph_fill(g, terminal.bg_color, r.x, r.y, r.w, r.h);
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
	
	void drawScrollbar(graph_t* g, const grect_t& r) {
		graph_fill_3d(g, r.x + r.w - scrollW, r.y, scrollW, r.h, 0x88000000, true);
		int sh = r.h * ((float) terminal.rows / terminal.textgrid->rows);
		int start_row = (int32_t)terminal.textgrid->rows - (int32_t)terminal.rows;
		if(start_row < 0)
			start_row = 0;
		start_row += terminal.scroll_offset;

		int sy = r.h * ((float) start_row / terminal.textgrid->rows);
		graph_fill_3d(g, r.x + r.w-scrollW+1, r.y + sy, scrollW-2, sh, 0x88aaaaaa, false);
	}

public:
	ConsoleWidget() {
		gterminal_init(&terminal);
		scrollW = 8;
		dirty = false;
	}

	~ConsoleWidget() {
		gterminal_close(&terminal);
	}

	void put(const char* buf, int size) {
		gterminal_put(&terminal, buf, size);
	}

	void flash() {
		gterminal_flash(&terminal);
		dirty = true;
	}

	void refresh() {
		dirty = true;
	}

	bool isDirty() {
		return dirty;
	}

protected:
	void onFocus(void) {
		refresh();
		getWin()->callXIM();
	}

	void onUnfocus(void) {
		refresh();
	}

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		drawBG(g, r);
		int gw = g->w-scrollW;
		if(terminal.textgrid->rows > terminal.rows) {
			drawScrollbar(g, r);
		}
		gterminal_paint(&terminal, g, r.x, r.y, r.w, r.h);
		dirty = false;
	}

	void onResize() {
		gterminal_resize(&terminal, area.w-scrollW, area.h);
	}
	
	void mouseHandle(xevent_t* ev) {
		if(ev->state == MOUSE_STATE_DOWN) {
			mouse_last_y = ev->value.mouse.y;
			return;
		}
		else if(ev->state == MOUSE_STATE_DRAG) {
			if(ev->value.mouse.y > mouse_last_y)
				gterminal_scroll(&terminal, -1);
			else if(ev->value.mouse.y < mouse_last_y)
				gterminal_scroll(&terminal, 1);
			mouse_last_y = ev->value.mouse.y;
			refresh();
		}
		else if(ev->state == MOUSE_STATE_MOVE) {
			if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
				gterminal_scroll(&terminal, 1);
			}
			else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
				gterminal_scroll(&terminal, -1);
			}
		}
	}

	bool onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_PRESS) {
			int c = ev->value.im.value;
			if(c != 0) {
				if(c == KEY_UP) {
					gterminal_scroll(&terminal, -1);
				}
				else if(c == KEY_DOWN) {
					gterminal_scroll(&terminal, 1);
				}
				else if(c == KEY_LEFT) {
					if(terminal.font_size > 5)
						terminal.font_size--;
					gterminal_resize(&terminal, area.w-scrollW, area.h);
				}
				else if(c == KEY_RIGHT) {
					if(terminal.font_size < 99)
						terminal.font_size++;
					gterminal_resize(&terminal, area.w-scrollW, area.h);
				}
				else {
					gterminal_scroll(&terminal, 0);
					charbuf_push(_buffer, c, false);
					proc_wakeup(RW_BLOCK_EVT);
				}
			}
			return true;	
		}
		else if(ev->type == XEVT_MOUSE) {
			mouseHandle(ev);
			return true;	
		}
		return false;	
	}
};

static ConsoleWidget* _consoleWidget = NULL;
static vdevice_t* _dev = NULL;

static void timer_handler(void) {
	_consoleWidget->flash();
}

static bool _win_opened = false;
static void* thread_loop(void* p) {
	X x;
	grect_t desk;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);

	ConsoleWidget *consoleWidget = new ConsoleWidget();
	_consoleWidget = consoleWidget;

	x.getDesktopSpace(desk, 0);
	win.open(&x, 0, -1, -1, desk.w*2/3, desk.h*2/3, "xconsole", 0);
	_win_opened = true;
	uint32_t timer_id = timer_set(500000, timer_handler);

	widgetXRun(&x, &win);
	timer_remove(timer_id);
	device_stop(_dev);
	return NULL;
}

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

	if(size <= 0 || _consoleWidget == NULL)
		return 0;

	_consoleWidget->put((const char*)buf, size);
	_consoleWidget->refresh();
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

	if(res != 0) {
		return VFS_ERR_RETRY;
	}

	((char*)buf)[0] = c;
	return 1;
}

static void do_signal(int sig, void* p) {
	_dev->terminated = true;
}

#ifdef __cplusplus
extern "C" { 
	int setenv(const char*, const char*);
	int kill(int, int);
}
#endif

int run(const char* mnt_point) {
	sys_signal_init();
	sys_signal(SYS_SIG_STOP, do_signal, NULL);
	_buffer = charbuf_new(0);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xconsole");
	dev.write = console_write;
	dev.read = console_read;
	_dev = &dev;

	pthread_t tid;
	pthread_create(&tid, NULL, thread_loop, NULL);

	while(!_win_opened)
		proc_usleep(100000);

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0600);
	charbuf_free(_buffer);
	proc_wakeup(RW_BLOCK_EVT);
	exit(0);
	return 0;
}

static int set_stdio(const char* dev) {
	int fd = open(dev, O_RDWR);
	if(fd > 0) {
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, VFS_BACKUP_FD0);
		dup2(fd, VFS_BACKUP_FD1);
		close(fd);
		setenv("CONSOLE_ID", dev);
		return 0;
	}
	return -1;
}

int main(int argc, char* argv[]) {
	char dev[128];
	snprintf(dev, 127, "/dev/xconsole%d", getpid());

	int pid = fork();
	if(pid == 0) {
		if(run(dev) != 0) {
			exit(-1);
		}
	}
	else 
		ipc_wait_ready(pid);

	if(set_stdio(dev) != 0)
		exit(-1);
	proc_exec("/bin/shell");
	waitpid(pid);
	return 0;
}