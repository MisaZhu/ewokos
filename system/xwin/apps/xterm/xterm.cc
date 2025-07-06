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

using namespace Ewok;

static charbuf_t *_buffer;

class TermWidget : public ConsoleWidget {
	pthread_mutex_t term_lock;
	bool showXIM;

public:
	TermWidget() {
		pthread_mutex_init(&term_lock, NULL);
		showXIM = false;
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

	bool readConfig(const char* fname) {
		json_var_t *conf_var = json_parse_file(fname);	

		uint32_t font_size = json_get_int_def(conf_var, "font_size", 12);
		int32_t char_space = json_get_int_def(conf_var, "char_space", -1);
		int32_t line_space = json_get_int_def(conf_var, "line_space", 0);
		uint32_t fg_color = json_get_int_def(conf_var, "fg_color", 0xffdddddd);
		uint32_t bg_color = json_get_int_def(conf_var, "bg_color", 0xff000000);
		uint32_t transparent = json_get_int_def(conf_var, "transparent", 255);
		const char* font_name = json_get_str(conf_var, "font");

		config(font_name, font_size, char_space, line_space, fg_color, bg_color, transparent);

		if(conf_var != NULL)
			json_var_unref(conf_var);
		return true;
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
		update();
		unlock();
	}

	void bgColorChange(uint32_t color, uint8_t alpha) {
		lock();
		terminal.transparent  = alpha;
		terminal.bg_color = (color & 0x00ffffff) | (terminal.transparent << 24);
		update();
		unlock();
	}

	void pushStr(const char* s, uint32_t sz) {
		lock();
		push(s, sz);
		update();
		unlock();
	}
protected:
	void input(int32_t c) {
		charbuf_push(_buffer, c, false);
		proc_wakeup(RW_BLOCK_EVT);
	}

	bool onMouse(xevent_t* ev) {
		if(ev->state == MOUSE_STATE_CLICK) {
			showXIM = !showXIM;
			getWin()->callXIM(showXIM);
			return true;
		}
		return ConsoleWidget::onMouse(ev);
	}

	void onResize() {
		lock();
		ConsoleWidget::onResize();
		unlock();
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		if((timerStep % (timerFPS/2)) == 0)
			flash();
	}
};


class TermWin: public WidgetWin{
	FontDialog fontDialog;
	ColorDialog colorDialog;
	ColorDialog bgColorDialog;

protected:
	void onDialoged(XWin* from, int res) {
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
static vdevice_t* _dev = NULL;

static void onFontFunc(MenuItem* it, void* p) {
	TermWin* win = (TermWin*)p;
	win->font();
}

static void onQuitFunc(MenuItem* it, void* p) {
	WidgetWin* win = (WidgetWin*)p;
	win->close();
}

static void onFontZoomInFunc(MenuItem* it, void* p) {
	_consoleWidget->fontZoom(true);
}

static void onFontZoomOutFunc(MenuItem* it, void* p) {
	_consoleWidget->fontZoom(false);
}

static void onFontCharSpaceIncrFunc(MenuItem* it, void* p) {
	_consoleWidget->charSpaceChange(true);
}

static void onFontCharSpaceDecrFunc(MenuItem* it, void* p) {
	_consoleWidget->charSpaceChange(false);
}

static void onTextColor(MenuItem* it, void* p) {
	TermWin* win = (TermWin*)p;
	win->color();
}

static void onBGColor(MenuItem* it, void* p) {
	TermWin* win = (TermWin*)p;
	win->bgColor();
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
	menu->add("txtcolor", NULL, NULL, onTextColor, &win);
	menu->add("bgcolor", NULL, NULL, onBGColor, &win);

	Menubar* menubar = new Menubar();
	menubar->add("font", NULL, NULL, onFontFunc, &win);
	menubar->add("F+", NULL, NULL, onFontZoomInFunc, NULL);
	menubar->add("F-", NULL, NULL, onFontZoomOutFunc, NULL);
	menubar->add("]+[", NULL, NULL, onFontCharSpaceIncrFunc, NULL);
	menubar->add("]-[", NULL, NULL, onFontCharSpaceDecrFunc, NULL);
	menubar->add("color", NULL, menu, NULL, NULL);
	menubar->fix(0, 20);
	root->add(menubar);

	TermWidget *consoleWidget = new TermWidget();
	root->add(consoleWidget);
	win.consoleWidget = consoleWidget;
	root->focus(consoleWidget);
	consoleWidget->readConfig(X::getResName("theme.json"));
	_consoleWidget = consoleWidget;

	x.getDesktopSpace(desk, 0);
	win.open(&x, 0, -1, -1, desk.w*2/3, desk.h*2/3, "xconsole", 0);
	_win_opened = true;

	win.setAlpha(true);
	win.setTimer(10);

	widgetXRun(&x, &win);
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

	_consoleWidget->pushStr((const char*)buf, size);
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