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
#include <WidgetEx/FontDialog.h>

#include <x++/X.h>
#include <pthread.h>

using namespace Ewok;

static charbuf_t *_buffer;

class ConsoleWidget : public Widget {
	FontDialog fontDialog;
	gterminal_t terminal;

	int32_t rollStepRows;
	int32_t mouse_last_y;
	uint32_t scrollW;

	void drawBG(graph_t* g, const grect_t& r) {
		graph_set(g, r.x, r.y, r.w, r.h, terminal.bg_color);
		if(color_a(terminal.bg_color) != 0xff) {
			setAlpha(true);
			RootWidget* root = getRoot();
			if(root)
				root->setAlpha(true);
		}
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
	bool readConfig(const char* fname) {
		json_var_t *conf_var = json_parse_file(fname);	
		terminal.font_size = json_get_int_def(conf_var, "font_size", 12);

		terminal.char_space = json_get_int_def(conf_var, "char_space", -1);
		terminal.line_space = json_get_int_def(conf_var, "line_space", 0);
		terminal.fg_color = json_get_int_def(conf_var, "fg_color", 0xffdddddd);
		terminal.bg_color = json_get_int_def(conf_var, "bg_color", 0xff000000);

		const char* v = json_get_str(conf_var, "font");
		if(v[0] != 0) {
			if(terminal.font != NULL)
				font_free(terminal.font);
			terminal.font = font_new(v, true);
		}

		if(conf_var != NULL)
			json_var_unref(conf_var);
		return true;
	}

	ConsoleWidget() {
		gterminal_init(&terminal);
		scrollW = 8;
	}

	~ConsoleWidget() {
		gterminal_close(&terminal);
	}

	void put(const char* buf, int size) {
		gterminal_put(&terminal, buf, size);
	}

	void flash() {
		gterminal_flash(&terminal);
		update();
	}

	void loadFont(const string& fontName) {
		if(terminal.font != NULL)
			font_free(terminal.font);
		terminal.font = font_new(fontName.c_str(), true);
		gterminal_resize(&terminal, area.w-scrollW, area.h);
		update();
	}

protected:
	void onFocus(void) {
		update();
		getWin()->callXIM();
	}

	void onUnfocus(void) {
		update();
	}

	void onResize() {
		gterminal_resize(&terminal, area.w-scrollW, area.h);
	}

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		if(terminal.font == NULL) {
			terminal.font_size = theme->basic.fontSize;
			terminal.bg_color = 0xff000000; //theme->basic.bgColor;
			terminal.fg_color = 0xffdddddd; //theme->basic.fgColor;
			loadFont(theme->basic.fontName);
		}

		drawBG(g, r);
		int gw = r.w-scrollW;
		if(terminal.textgrid->rows > terminal.rows) {
			drawScrollbar(g, r);
		}
		gterminal_paint(&terminal, g, r.x, r.y, gw, r.h);
	}
	
	bool onMouse(xevent_t* ev) {
		if(ev->state == MOUSE_STATE_DOWN) {
			mouse_last_y = ev->value.mouse.y;
		}
		else if(ev->state == MOUSE_STATE_DRAG) {
			if(ev->value.mouse.y > mouse_last_y)
				gterminal_scroll(&terminal, -1);
			else if(ev->value.mouse.y < mouse_last_y)
				gterminal_scroll(&terminal, 1);
			mouse_last_y = ev->value.mouse.y;
			update();
		}
		else if(ev->state == MOUSE_STATE_MOVE) {
			if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
				gterminal_scroll(&terminal, 1);
				update();
			}
			else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
				gterminal_scroll(&terminal, -1);
				update();
			}
		}
		return true;
	}

	bool onIM(xevent_t* ev) {
		if(ev->state == XIM_STATE_PRESS) {
			int c = ev->value.im.value;
			if(c != 0) {
				if(c == KEY_UP) {
					gterminal_scroll(&terminal, -1);
					update();
				}
				else if(c == KEY_DOWN) {
					gterminal_scroll(&terminal, 1);
					update();
				}
				else if(c == KEY_LEFT) {
					if(terminal.font_size > 5)
						terminal.font_size--;
					gterminal_resize(&terminal, area.w-scrollW, area.h);
					update();
				}
				else if(c == KEY_RIGHT) {
					if(terminal.font_size < 99)
						terminal.font_size++;
					gterminal_resize(&terminal, area.w-scrollW, area.h);
					update();
				}
				else {
					gterminal_scroll(&terminal, 0);
					charbuf_push(_buffer, c, false);
					update();
					proc_wakeup(RW_BLOCK_EVT);
				}
			}
			return true;	
		}
		return false;	
	}
};


class TermWin: public WidgetWin{
	FontDialog fontDialog;

protected:
	void onDialoged(XWin* from, int res) {
		if(res == Dialog::RES_OK) {
			string fontName = fontDialog.getResult();
			consoleWidget->loadFont(fontName);
		}
	}
public:
	ConsoleWidget* consoleWidget;

	void font() {
		fontDialog.popup(this, 300, 300, "fonts", XWIN_STYLE_NORMAL);
	}
};

static ConsoleWidget* _consoleWidget = NULL;
static vdevice_t* _dev = NULL;

static void timer_handler(void) {
	_consoleWidget->flash();
}

static void onFontFunc(MenuItem* it, void* p) {
	TermWin* win = (TermWin*)p;
	win->font();
}

static void onQuitFunc(MenuItem* it, void* p) {
	WidgetWin* win = (WidgetWin*)p;
	win->close();
}

static bool _win_opened = false;
static void* thread_loop(void* p) {
	X x;
	grect_t desk;
	TermWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);

	Menu* menu = new Menu();
	menu->add("font", NULL, NULL, onFontFunc, &win);
	menu->add("quit", NULL, NULL, onQuitFunc, &win);

	Menubar* menubar = new Menubar();
	menubar->add("term", NULL, menu, NULL, NULL);
	menubar->fix(0, 20);
	root->add(menubar);

	ConsoleWidget *consoleWidget = new ConsoleWidget();
	root->add(consoleWidget);
	win.consoleWidget = consoleWidget;
	root->focus(consoleWidget);
	consoleWidget->readConfig(X::getResName("theme.json"));
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
	_consoleWidget->update();
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
	if(_consoleWidget)
		_consoleWidget->update();
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