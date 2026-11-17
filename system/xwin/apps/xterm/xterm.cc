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
#include <clipboard/clipboard.h>
#include <ewoksys/vfs.h>
#include <ewoksys/keydef.h>
#include <ewoksys/ipc.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/proc.h>
#include <ewoksys/syscall.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/timer.h>
#include <ewoksys/wait.h>
#include <ewoksys/tty.h>

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
static int _shell_pid = -1;
static tty_state_t _tty;   /* shared terminal line discipline */

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

/* Line-discipline emit callbacks: queue bytes for read(), and echo typed bytes
 * back through the normal output path (a separate lock, so no deadlock with the
 * term_lock held by input()). */
static void xterm_emit_read(void* arg, const char* buf, int size) {
	(void)arg;
	for(int i = 0; i < size; i++)
		buffer_push_char(buf[i]);
}

static void xterm_emit_echo(void* arg, const char* buf, int size) {
	(void)arg;
	output_queue_push(buf, size);
}

#define MENU_ITEM_COPY  1
#define MENU_ITEM_PASTE 2

#define LONG_PRESS_MS   500   /* hold time before the copy/paste menu pops */
#define LONG_PRESS_SLOP 10    /* max pixel movement to still count as a hold */

#define DOUBLE_CLICK_MS   350 /* max gap between the two taps of a double-click */
#define DOUBLE_CLICK_SLOP 24  /* max drift between the two tap positions */

static void termMenuCallback(MenuItem* it, void* p);

class TermWidget : public ConsoleWidget {
	pthread_mutex_t term_lock;
	bool showXIM;
	Menu* ctxMenu;

	/* long-press tracking (touch): pops the copy/paste menu */
	bool longPressPending;
	bool longPressTriggered;
	bool pressInSel;        /* the press landed inside the old selection */
	bool dragSelActive;     /* the drag has replaced the old selection */
	bool selMode;           /* double-click armed: next drag is a selection */
	bool doublePress;       /* this press is the 2nd click: swallow its CLICK */
	uint64_t longPressTime;
	uint64_t lastClickTime;
	int32_t longPressX, longPressY;   /* screen coords of the press */
	int32_t lastClickX, lastClickY;   /* position of the previous tap */
	gpos_t anchorPos;                 /* widget-local coords of the press */
public:
	TermWidget() {
		pthread_mutex_init(&term_lock, NULL);
		showXIM = false;
		ctxMenu = NULL;
		longPressPending = false;
		longPressTriggered = false;
		pressInSel = false;
		dragSelActive = false;
		selMode = false;
		doublePress = false;
		lastClickTime = 0;
		lastClickX = 0;
		lastClickY = 0;
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
		if(ctxMenu != NULL)
			delete ctxMenu;
		pthread_mutex_destroy(&term_lock);
	}

	void lock() {
		pthread_mutex_lock(&term_lock);
	}

	void unlock() {
		pthread_mutex_unlock(&term_lock);
	}

	/* Report the current textgrid geometry for TIOCGWINSZ. */
	void queryWinsize(unsigned short* rows, unsigned short* cols) {
		lock();
		*rows = (unsigned short)terminal.rows;
		*cols = (unsigned short)terminal.cols;
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

	/* Copy the current selection into the global clipboard. */
	void copySelection() {
		lock();
		char* text = gterminal_select_copy(&terminal);
		unlock();
		if(text != NULL) {
			if(text[0] != 0)
				clipboard_set_text(text);
			free(text);
		}
	}

	/* Feed the clipboard content to the shell through the line discipline. */
	void pasteClipboard() {
		char* text = clipboard_get_text();
		if(text == NULL || text[0] == 0) {
			if(text != NULL)
				free(text);
			return;
		}
		lock();
		for(uint32_t i = 0; text[i] != 0; i++)
			input((int32_t)(unsigned char)text[i]);
		gterminal_scroll(&terminal, 0);
		unlock();
		free(text);
		flushOutput();
		update();
	}

	/* Popup the copy/paste menu at screen coordinates. */
	void showContextMenu(int32_t screenX, int32_t screenY) {
		if(ctxMenu != NULL) {
			delete ctxMenu;
			ctxMenu = NULL;
		}
		ctxMenu = new Menu();
		ctxMenu->setItemSize(X::hidpiSize(24));

		bool hasSel = false;
		lock();
		hasSel = gterminal_has_selection(&terminal);
		unlock();
		bool hasClip = false;
		char* clip = clipboard_get_text();
		if(clip != NULL) {
			hasClip = (clip[0] != 0);
			free(clip);
		}

		if(hasSel)
			ctxMenu->add(MENU_ITEM_COPY, "Copy", NULL, NULL, termMenuCallback, this);
		if(hasClip)
			ctxMenu->add(MENU_ITEM_PASTE, "Paste", NULL, NULL, termMenuCallback, this);

		uint32_t n = ctxMenu->getItemNum();
		if(n == 0) {
			delete ctxMenu;
			ctxMenu = NULL;
			return;
		}
		uint32_t h = n * ctxMenu->getItemSize();
		uint32_t w = X::hidpiSize(120);
		ctxMenu->popup(getWin(), screenX, screenY, w, h, "menu", XWIN_STYLE_NO_TITLE);
		ctxMenu->pop();
	}
protected:
	void input(int32_t c) {
		char ch = (char)c;
		tty_input(&_tty, &ch, 1, xterm_emit_read, NULL, xterm_emit_echo, NULL);
		vfs_wakeup(_dev->mnt_info.node, VFS_EVT_RD);
	}

	bool onMouse(xevent_t* ev) {
		/* right button: context menu, never selection */
		if(ev->value.mouse.button == MOUSE_BUTTON_RIGHT) {
			if(ev->state == MOUSE_STATE_DOWN) {
				longPressPending = false;
				showContextMenu(ev->value.mouse.x, ev->value.mouse.y);
				return true;
			}
			return true; /* swallow UP/CLICK of the right button */
		}

		if(ev->state == MOUSE_STATE_DOWN && ev->value.mouse.button == MOUSE_BUTTON_LEFT) {
			gpos_t p = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
			lock();
			pressInSel = gterminal_cell_selected_at(&terminal, p.x, p.y);
			unlock();
			anchorPos = p;
			dragSelActive = false;
			mouse_last_y = ev->value.mouse.y;
			longPressPending = true;
			longPressTriggered = false;
			longPressTime = kernel_tic_ms(0);
			longPressX = ev->value.mouse.x;
			longPressY = ev->value.mouse.y;
			/* second press of a double-click: this drag selects right away,
			 * no third press needed */
			uint64_t now = kernel_tic_ms(0);
			int32_t dcx = ev->value.mouse.x - lastClickX;
			int32_t dcy = ev->value.mouse.y - lastClickY;
			if((now - lastClickTime) <= DOUBLE_CLICK_MS &&
					dcx*dcx + dcy*dcy <= DOUBLE_CLICK_SLOP*DOUBLE_CLICK_SLOP) {
				selMode = true;
				doublePress = true;  /* swallow the release CLICK of this press */
				lastClickTime = 0;
			}
			update();
			return true;
		}

		if(ev->state == MOUSE_STATE_DRAG) {
			if(longPressPending) {
				int32_t dx = ev->value.mouse.x - longPressX;
				int32_t dy = ev->value.mouse.y - longPressY;
				if(dx*dx + dy*dy > LONG_PRESS_SLOP*LONG_PRESS_SLOP)
					longPressPending = false;
			}
			if(selMode) {
				/* double-click armed selection mode: this drag selects */
				lock();
				if(!dragSelActive) {
					/* first real movement: replace the old selection */
					gterminal_select_begin_at(&terminal, anchorPos.x, anchorPos.y);
					dragSelActive = true;
				}
				gpos_t p = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
				/* Dragging past the edges pages through the history while the
				 * selection keeps growing: above the top reveals newer rows,
				 * below the bottom reveals older rows (finger-drag semantics,
				 * same direction as the plain drag-scroll gesture). */
				if(p.y < 0)
					gterminal_scroll(&terminal, 1);
				else if(p.y >= (int32_t)area.h)
					gterminal_scroll(&terminal, -1);
				gterminal_select_extend_at(&terminal, p.x, p.y);
				unlock();
				update();
				return true;
			}
			/* plain drag anywhere: page through the history (finger down
			 * reveals older rows, finger up reveals newer) */
			lock();
			bool needUpdate = false;
			if(ev->value.mouse.y > mouse_last_y)
				needUpdate = gterminal_scroll(&terminal, -1);
			else if(ev->value.mouse.y < mouse_last_y)
				needUpdate = gterminal_scroll(&terminal, 1);
			mouse_last_y = ev->value.mouse.y;
			unlock();
			if(needUpdate)
				update();
			return true;
		}

		if(ev->state == MOUSE_STATE_UP) {
			longPressPending = false;
			lock();
			gterminal_select_end(&terminal);
			unlock();
			if(dragSelActive)
				selMode = false;   /* the selection drag consumed the armed mode */
			update();
			return true;
		}

		if(ev->state == MOUSE_STATE_CLICK) {
			if(longPressTriggered) {
				/* release of the long-press finger: keep the selection for
				 * Copy, don't toggle the soft keyboard */
				longPressTriggered = false;
				return true;
			}
			if(doublePress) {
				/* release of the second click of a double-click: selection
				 * mode was already armed at its press, no keyboard toggle */
				doublePress = false;
				return true;
			}
			/* plain tap: clear the selection, toggle the soft keyboard */
			lastClickTime = kernel_tic_ms(0);
			lastClickX = ev->value.mouse.x;
			lastClickY = ev->value.mouse.y;
			selMode = false;
			lock();
			gterminal_select_clear(&terminal);
			unlock();
			update();
			showXIM = !showXIM;
			getWin()->callXIM(showXIM);
			return true;
		}

		if(ev->state == MOUSE_STATE_MOVE) {
			bool handled = false;
			bool needUpdate = false;
			lock();
			if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
				needUpdate = gterminal_scroll(&terminal, 1);
				handled = true;
			}
			else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
				needUpdate = gterminal_scroll(&terminal, -1);
				handled = true;
			}
			unlock();
			if(needUpdate)
				update();
			if(handled)
				return true;
		}
		return false;
	}

	bool onIM(xevent_t* ev) {
		if(ev->state == XIM_STATE_PRESS) {
			int c = ev->value.im.value;
			if(c > 0 && c < 128) {
				bool needUpdate = true;
				lock();
				/* Arrow keys are not terminal-view shortcuts any more: they are
				 * handed to the shell as-is (up/down recall history, left/right
				 * move the in-line cursor). Scrolling stays on the mouse, font
				 * settings stay on the menubar. */
				if(c != KEY_LSHIFT && c != KEY_RSHIFT && c != KEY_CTRL) {
					input(c);
					gterminal_scroll(&terminal, 0);
				}
				else {
					needUpdate = false;
				}
				unlock();
				if(flushOutput())
					needUpdate = true;
				if(needUpdate)
					update();
			}
			return true;	
		}
		return false;	
	}

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
		/* touch long-press: pop the copy/paste menu */
		if(longPressPending && !longPressTriggered &&
				(kernel_tic_ms(0) - longPressTime) >= LONG_PRESS_MS) {
			longPressTriggered = true;
			longPressPending = false;
			selMode = false;     /* long-press gesture disarms selection mode */
			doublePress = false; /* its release CLICK is swallowed here */
			if(!pressInSel) {
				lock();
				gterminal_select_clear(&terminal);
				unlock();
			}
			showContextMenu(longPressX, longPressY);
		}

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

static void termMenuCallback(MenuItem* it, void* p) {
	TermWidget* self = (TermWidget*)p;
	switch(it->id) {
	case MENU_ITEM_COPY:
		self->copySelection();
		break;
	case MENU_ITEM_PASTE:
		self->pasteClipboard();
		break;
	}
}


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

	x_theme_t theme;
	if(x_get_theme(&theme) == 0 && theme.hidpi != 0) {
		font_size = X::hidpiSize(font_size);
		char_space = X::hidpiSize(char_space);
		line_space = X::hidpiSize(line_space);
	}

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
static bool _win_failed = false;

static void terminate_shell_session(void) {
	if(_shell_pid <= 0)
		return;

	uint32_t live_uuid = proc_get_uuid(_shell_pid);
	if(live_uuid == 0)
		return;
	kill(_shell_pid, SIGKILL);
	for(int i = 0; i < 100; i++) {
		if(proc_get_uuid(_shell_pid) == 0)
			break;
		usleep(10000);
	}
}

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
	menubar->setItemSize(X::hidpiSize(42));
	menubar->add(2, "font", NULL, NULL, onFontFunc, &win);
	menubar->add(7, "color", NULL, menu, NULL, NULL);
	menubar->fix(0, X::hidpiSize(20));
	root->add(menubar);
	_menubar = menubar;

	TermWidget *consoleWidget = new TermWidget();
	root->add(consoleWidget);
	win.consoleWidget = consoleWidget;
	root->focus(consoleWidget);
	_consoleWidget = consoleWidget;
	readConfig(X::getResFullName("config.json").c_str());

	x.getDesktopSpace(desk, 0);
	if(!win.open(&x, -1, -1, -1, 0, 0, "xconsole", 0)) {
		_win_failed = true;
		_win_opened = true;
		return NULL;
	}
	_win_opened = true;

	win.setAlpha(true);
	win.setTimer(30);

	widgetXRun(&x, &win);
	_consoleWidget = NULL;
	vfs_wakeup(_dev->mnt_info.node, VFS_EVT_RD | VFS_EVT_CLOSE);
	terminate_shell_session();
	device_stop(_dev);
	return NULL;
}

static int console_write(vdevice_t* dev, int fd,
		int from_pid,
		fsinfo_t* info,
		const void* buf,
		int size,
		off_t offset,
		void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;

	if(size <= 0 || _consoleWidget == NULL)
		return 0;

	if((_tty.tio.c_oflag & OPOST) == 0) {
		output_queue_push((const char*)buf, size);
	}
	else {
		/* OPOST may expand NL->CRLF, so give tty_output room to grow. */
		char* tmp = (char*)malloc((size_t)size * 2);
		if(tmp != NULL) {
			int m = tty_output(&_tty, (const char*)buf, size, tmp, size * 2);
			output_queue_push(tmp, m);
			free(tmp);
		}
		else {
			output_queue_push((const char*)buf, size);
		}
	}
	return size;
}

static int console_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, off_t offset, void* p) {
	(void)dev;
	(void)fd;
	(void)offset;
	(void)p;
	(void)size;

	tty_set_foreground(&_tty, from_pid);

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

	if(_consoleWidget == NULL) {
		return VFS_EVT_CLOSE;
	}
	if(!buffer_is_empty()) {
		return VFS_EVT_RD;
	}
	return 0;
}

static int console_loop(vdevice_t* dev, void* p) {
	(void)dev;
	(void)p;
	usleep(20000);
	return 0;
}

static int console_dcntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)dev;
	(void)from_pid;
	(void)p;
	/* Report the live textgrid geometry so TIOCGWINSZ answers without a probe. */
	if(_consoleWidget != NULL) {
		unsigned short r = 0, c = 0;
		_consoleWidget->queryWinsize(&r, &c);
		tty_set_winsize(&_tty, r, c);
	}
	return tty_dev_cntl(&_tty, cmd, in, ret);
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
	tty_init(&_tty);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.desc, "xconsole");
	dev.write = console_write;
	dev.read = console_read;
	dev.loop_step = console_loop;
	dev.check_poll_events = console_check_poll_events;
	dev.dev_cntl = console_dcntl;
	_dev = &dev;

	pthread_t tid;
	pthread_create(&tid, NULL, thread_loop, NULL);

	while(!_win_opened)
		usleep(100000);

	/* Without a window there is nothing to attach a shell to; bail out instead
	 * of publishing a console device that can never show anything. */
	if(_win_failed)
		return -1;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0600, false);
	syscall1(SYS_EXIT, (ewokos_addr_t)0);
	__builtin_unreachable();
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
		_shell_pid = getppid();
		if(run(dev) != 0) {
			exit(-1);
		}
	}

	/* Wait for the child to publish the console device, but give up as soon as
	 * the child is gone, otherwise a failed window open hangs us forever. */
	uint32_t child_uuid = proc_get_uuid(pid);
	while(set_stdio(dev) != 0) {
		if(child_uuid == 0 || proc_get_uuid(pid) != child_uuid)
			exit(-1);
		usleep(10000);
	}
	proc_exec("/bin/shell");
	ewok_waitpid(pid);
	return 0;
}
