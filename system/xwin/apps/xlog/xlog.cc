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

class TermWidget : public ConsoleWidget {
	int logFD;
public:
	TermWidget() {
		logFD = open("/dev/log", O_WRONLY | O_NONBLOCK);
	}

	~TermWidget() {
		if(logFD >= 0)
			close(logFD);
	}

	void pushStr(const char* s, uint32_t sz) {
		push(s, sz);
		update();
	}
protected:
	void input(int32_t c) {
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		if(logFD < 0 || (timerStep % (timerFPS/2)) != 0)
			return;

		char buf[1024];
		int sz = read(logFD, buf, sizeof(buf));
		if(sz > 0) {
			pushStr(buf, sz);
			update();
		}
	}
};

static bool _fullscreen = false;
static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "f");
		if(c == -1)
			break;

		switch (c) {
		case 'f':
			_fullscreen = true;
			break;
		case '?':
			return -1;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	doargs(argc, argv);

	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);
	
	TermWidget *consoleWidget = new TermWidget();
	root->add(consoleWidget);
	root->focus(consoleWidget);

	XTheme* theme = win.getTheme();
	theme->basic.fgColor = 0xFF000000;
	theme->basic.bgColor = 0xFFFFFFFF;
	theme->basic.fontSize = 10;

	win.open(&x, -1, -1, -1, 0, 0, "xlog", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
	if(_fullscreen)
		win.max();
	win.setTimer(30);
	widgetXRun(&x, &win);
	return 0;
}