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
#include <setenv.h>

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

#include <WidgetEx/ConsoleWidget.h>
#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Text.h>
#include <Widget/Label.h>
#include <WidgetEx/Menubar.h>
#include <WidgetEx/Menu.h>
#include <Widget/LabelButton.h>
#include <WidgetEx/FontDialog.h>
#include <WidgetEx/ColorDialog.h>

#include <x++/X.h>
#include <pthread.h>
#include <string>

using namespace Ewok;

static charbuf_t *_buffer;
static vdevice_t* _dev = NULL;
static pthread_mutex_t _buffer_lock;
static pthread_mutex_t _output_lock;
static std::string* _output_pending = NULL;

static inline void buffer_push_char(char c) {
	pthread_mutex_lock(&_buffer_lock);
	charbuf_push(_buffer, c, false);
	pthread_mutex_unlock(&_buffer_lock);
}

static inline int buffer_pop_char(char* c) {
	int ret;
	pthread_mutex_lock(&_buffer_lock);
	ret = charbuf_pop(_buffer, c);
	pthread_mutex_unlock(&_buffer_lock);
	return ret;
}

static inline bool buffer_is_empty(void) {
	bool empty;
	pthread_mutex_lock(&_buffer_lock);
	empty = charbuf_is_empty(_buffer);
	pthread_mutex_unlock(&_buffer_lock);
	return empty;
}

static inline void output_queue_push(const char* buf, int size) {
	if(buf == NULL || size <= 0)
		return;
	pthread_mutex_lock(&_output_lock);
	if(_output_pending != NULL)
		_output_pending->append(buf, size);
	pthread_mutex_unlock(&_output_lock);
}

static inline std::string output_queue_take(void) {
	std::string out;
	pthread_mutex_lock(&_output_lock);
	if(_output_pending != NULL)
		out.swap(*_output_pending);
	pthread_mutex_unlock(&_output_lock);
	return out;
}

class TermWidget : public ConsoleWidget {
	pthread_mutex_t term_lock;
	bool showXIM;
public:
	TermWidget() {
		pthread_mutex_init(&term_lock, NULL);
		showXIM = false;
		terminal.output_callback = [](void* p, const char* buf, int size) {
			(void)p;
			for(int i = 0; i < size; i++) {
				buffer_push_char(buf[i]);
			}
			vfs_wakeup(_dev->mnt_info.node, VFS_EVT_RD);
		};
		terminal.output_callback_arg = NULL;
	}

	~TermWidget() {
		pthread_mutex_destroy(&term_lock);
	}

	void lock() {
		pthread_mutex_lock(&term_lock);
	}

	void unlock() {
		pthread_mutex_unlock(&term_lock);
	}

	void fontZoom(bool zoomIn) {
		lock();
		if(zoomIn)
			terminal.font_size++;
		else  {
			if(terminal.font_size > 5)
				terminal.font_size--;
		}
		gterminal_resize(&terminal, area.w-scrollW, area.h);
		unlock();
	}

	void charSpaceChange(bool incr) {
		lock();
		if(incr) {
			if(terminal.char_space < 8)
				terminal.char_space++;
		}
		else  {
			if(terminal.char_space > -5)
				terminal.char_space--;
		}
		gterminal_resize(&terminal, area.w-scrollW, area.h);
		unlock();
	}

	void textColorChange(uint32_t color) {
		lock();
		terminal.fg_color = color;
		unlock();
		update();
	}

	void bgColorChange(uint32_t color, uint8_t alpha) {
		lock();
		terminal.transparent  = alpha;
		terminal.bg_color = (color & 0x00ffffff) | (terminal.transparent << 24);
		unlock();
		update();
	}

	void setFont(const string& fontName) {
		lock();
		if(terminal.font != NULL)
			font_free(terminal.font);
		terminal.font = font_new(fontName.c_str(), true);
		gterminal_resize(&terminal, area.w-scrollW, area.h);
		unlock();
		update();
	}

	void pushStr(const char* s, uint32_t sz) {
		lock();
		push(s, sz);
		unlock();
		update();
	}

	bool flushOutput(void) {
		std::string out = output_queue_take();
		if(out.empty())
			return false;
		lock();
		push(out.c_str(), out.size());
		unlock();
		return true;
	}
protected:
	void input(int32_t c) {
		buffer_push_char(c);
		vfs_wakeup(_dev->mnt_info.node, VFS_EVT_RD);
	}

	bool onMouse(xevent_t* ev) {
		static bool zooming = false;
		if(ev->state == MOUSE_STATE_CLICK) {
			showXIM = !showXIM;
			getWin()->callXIM(showXIM);
			return true;
		}
		else if(ev->state == MOUSE_STATE_DRAG) {
			if(abs(ev->value.mouse.rx) > abs(ev->value.mouse.ry)) {
				if(!zooming) {
					if(ev->value.mouse.rx > 0)
						fontZoom(true);
					else
						fontZoom(false);
				}
				zooming = true;
				return true;
			}
		}
		else if(ev->state == MOUSE_STATE_UP) {
			zooming = false;
		}

		static bool draging = false;
		bool needUpdate = false;
		if(ev->state == MOUSE_STATE_UP) {
			draging = false;
			return true;
		}

		if(ev->state == MOUSE_STATE_DOWN) {
			if(!draging) {
				mouse_last_y = ev->value.mouse.y;
				draging = true;
				return true;
			}
		}

		lock();
		if(ev->state == MOUSE_STATE_DRAG || draging) {
			if(ev->value.mouse.y > mouse_last_y) {
				needUpdate = gterminal_scroll(&terminal, -1);
			}
			else if(ev->value.mouse.y < mouse_last_y) {
				needUpdate = gterminal_scroll(&terminal, 1);
			}
			mouse_last_y = ev->value.mouse.y;
			unlock();
			if(needUpdate)
				update();
			return true;
		}
		else if(ev->state == MOUSE_STATE_MOVE) {
			if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
				needUpdate = gterminal_scroll(&terminal, 1);
				unlock();
				if(needUpdate)
					update();
				return true;
			}
			else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
				needUpdate = gterminal_scroll(&terminal, -1);
				unlock();
				if(needUpdate)
					update();
				return true;
			}
		}
		unlock();
		return false;
	}

	bool onIM(xevent_t* ev) {
		if(!showXIM) {
			showXIM = true;
			if(getWin()->callXIM(true))
				return true;
		}

		if(ev->state == XIM_STATE_PRESS) {
			int c = ev->value.im.value;
			if(c > 0 && c < 128) {
				bool needUpdate = true;
				lock();
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
					if(c != KEY_LSHIFT && c != KEY_RSHIFT && c != KEY_CTRL) {
						input(c);
						gterminal_scroll(&terminal, 0);
					}
					else {
						needUpdate = false;
					}
				}
				unlock();
				if(needUpdate)
					update();
			}
			return true;	
		}
		return false;	
	}

	/*void onFocus() {
		getWin()->callXIM(true);
		ConsoleWidget::onFocus();
	}
		*/

	void onResize() {
		lock();
		ConsoleWidget::onResize();
		unlock();
	}

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		lock();
		ConsoleWidget::onRepaint(g, theme, r);
		unlock();
	}

	void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
		bool needUpdate = flushOutput();
		uint32_t period = timerFPS > 1 ? (timerFPS/2) : 1;
		if((timerSteps % period) == 0) {
			lock();
			gterminal_flash(&terminal);
			unlock();
			needUpdate = true;
		}
		if(needUpdate)
			update();
	}
};


class TermWin: public WidgetWin{
	FontDialog fontDialog;
	ColorDialog colorDialog;
	ColorDialog bgColorDialog;

protected:
	void onDialoged(XWin* from, int res, void* arg) {
		if(res == Dialog::RES_OK) {
			if(from == &fontDialog) {
				string fontName = fontDialog.getResult();
				consoleWidget->setFont(fontName);
			}
			else if(from == &colorDialog) {
				uint32_t color = colorDialog.getColor();
				consoleWidget->textColorChange(color);
			}
			else if(from == &bgColorDialog) {
				uint32_t color = bgColorDialog.getColor();
				uint8_t alpha = bgColorDialog.getTransparent();
				consoleWidget->bgColorChange(color, alpha);
				if(alpha != 0xFF)
					setAlpha(true);
				else
					setAlpha(false);
			}
		}
	}
public:
	TermWidget* consoleWidget;

	void font() {
		fontDialog.popup(this, 300, 300, "fonts", XWIN_STYLE_NORMAL);
	}

	void color() {
		colorDialog.popup(this, 256, 200, "color", XWIN_STYLE_NO_RESIZE);
		colorDialog.setColor(consoleWidget->getTerminal()->fg_color);
	}

	void bgColor() {
		bgColorDialog.popup(this, 256, 200, "bgColor", XWIN_STYLE_NO_RESIZE);
		bgColorDialog.setColor(consoleWidget->getTerminal()->bg_color);
	}
};

static TermWidget* _consoleWidget = NULL;

static void onFontFunc(MenuItem* it, void* p) {
	TermWin* win = (TermWin*)p;
	win->font();
}

static void onTextColor(MenuItem* it, void* p) {
	TermWin* win = (TermWin*)p;
	win->color();
}

static void onBGColor(MenuItem* it, void* p) {
	TermWin* win = (TermWin*)p;
	win->bgColor();
}

static Menubar* _menubar = NULL;
static bool readConfig(const char* fname) {
	json_var_t *conf_var = json_parse_file(fname);	

	uint32_t font_size = json_get_int_def(conf_var, "font_size", 12);
	int32_t char_space = json_get_int_def(conf_var, "char_space", -1);
	int32_t line_space = json_get_int_def(conf_var, "line_space", 0);
	uint32_t fg_color = json_get_int_def(conf_var, "fg_color", 0xffdddddd);
	uint32_t bg_color = json_get_int_def(conf_var, "bg_color", 0xff000000);
	uint32_t transparent = json_get_int_def(conf_var, "transparent", 255);
	const char* font_name = json_get_str(conf_var, "font");

	_consoleWidget->config(font_name, font_size, char_space, line_space, fg_color, bg_color, transparent);
	bool show_menubar = (bool)json_get_int_def(conf_var, "menubar", 1);
	if(!show_menubar)
		_menubar->hide();

	uint32_t max_rows = json_get_int_def(conf_var, "max_rows", 1024);
	_consoleWidget->setMaxRows(max_rows);
	
	if(conf_var != NULL)
		json_var_unref(conf_var);
	return true;
}

static bool _win_opened = false;
static void* thread_loop(void* p) {
	X x;
	grect_t desk;
	TermWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICAL);

	Menu* menu = new Menu();
	menu->add(0, "txtcolor", NULL, NULL, onTextColor, &win);
	menu->add(1, "bgcolor", NULL, NULL, onBGColor, &win);

	Menubar* menubar = new Menubar();
	menubar->setItemSize(42);
	menubar->add(2, "font", NULL, NULL, onFontFunc, &win);
	menubar->add(7, "color", NULL, menu, NULL, NULL);
	menubar->fix(0, 20);
	root->add(menubar);
	_menubar = menubar;

	TermWidget *consoleWidget = new TermWidget();
	root->add(consoleWidget);
	win.consoleWidget = consoleWidget;
	root->focus(consoleWidget);
	_consoleWidget = consoleWidget;
	readConfig(X::getResFullName("config.json").c_str());

	x.getDesktopSpace(desk, 0);
	win.open(&x, -1, -1, -1, 0, 0, "xconsole", 0);
	_win_opened = true;

	win.setAlpha(true);
	win.setTimer(30);

	widgetXRun(&x, &win);
	_consoleWidget = NULL;
	vfs_wakeup(_dev->mnt_info.node, VFS_EVT_RD);
	device_stop(_dev);
	return NULL;
}

static int console_write(vdevice_t* dev, int fd,
		int from_pid,
		fsinfo_t* info,
		const void* buf,
		int size,
		int offset,
		void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;

	if(size <= 0 || _consoleWidget == NULL)
		return 0;

	output_queue_push((const char*)buf, size);
	return size;
}

static int console_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)size;


	if(_consoleWidget == NULL) {
		return 0; //closed
	}

	if(size <= 0) {
		return VFS_ERR_RETRY;
	}

	int i;
	for(i = 0; i < size; i++) {
		char c;
		int res = buffer_pop_char(&c);
		if(res != 0)
			break;
		((char*)buf)[i] = c;
	}

	return (i == 0) ? VFS_ERR_RETRY : i;
}

static uint32_t console_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)p;
		
	if(!buffer_is_empty()) {
		return VFS_EVT_RD;
	}
	return 0;
}

static int console_loop(vdevice_t* dev, void* p) {
	(void)dev;
	(void)p;
	proc_usleep(20000);
	return 0;
}

static void do_signal(int sig, void* p) {
	_dev->terminated = true;
}

#ifdef __cplusplus
extern "C" { 
	int kill(int, int);
}
#endif

int run(const char* mnt_point) {
	sys_signal_init();
	sys_signal(SYS_SIG_STOP, do_signal, NULL);
	_buffer = charbuf_new(4096);
	pthread_mutex_init(&_buffer_lock, NULL);
	pthread_mutex_init(&_output_lock, NULL);
	_output_pending = new std::string();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xconsole");
	dev.write = console_write;
	dev.read = console_read;
	dev.loop_step = console_loop;
	dev.check_poll_events = console_check_poll_events;
	_dev = &dev;

	pthread_t tid;
	pthread_create(&tid, NULL, thread_loop, NULL);

	while(!_win_opened)
		proc_usleep(100000);

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0600);
	charbuf_free(_buffer);
	if(_output_pending != NULL) {
		_output_pending->clear();
		delete _output_pending;
		_output_pending = NULL;
	}
	pthread_mutex_destroy(&_output_lock);
	pthread_mutex_destroy(&_buffer_lock);
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
	ewok_waitpid(pid);
	return 0;
}
