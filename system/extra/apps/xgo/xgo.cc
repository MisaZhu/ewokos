#include <Widget/WidgetWin.h>
#include <Widget/Label.h>
#include <x++/X.h>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <font/font.h>
#include <ewoksys/kernel_tic.h>
#include <xgo/xgo.h>

using namespace Ewok;

class XgoWidget: public Widget {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		uint8_t res[DATA_MAX];
		if(timerStep == 0)
			xgo_cmd(CMD_SEND, 0x03, 0x01, res);
	}
public: 
	XgoWidget() {
		xgo_init();
	}
};

int main(int argc, char** argv) {
	X x;
	WidgetWin win;

	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);

	XgoWidget* xgoW = new XgoWidget();
	root->add(xgoW);

	win.open(&x, 0, -1, -1, 240, 240, "xgo", XWIN_STYLE_NORMAL);
	win.setTimer(1);
	win.max();
	x.run(NULL, &win);
	return 0;
}