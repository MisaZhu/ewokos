#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Label.h>
#include <x++/X.h>
#include <unistd.h>
#include <string>
#include <font/font.h>
#include <ewoksys/vdevice.h>
#include <pthread.h>

using namespace Ewok;

class IPConfigWidget: public Widget {
	std::string ipinfo;
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_rect(g, r.x, r.y, r.w, r.h, theme->basic.docBGColor);

		font_t* font = theme->getFont();
		if(font == NULL)
			return;

		uint32_t fontSize = theme->basic.fontSize;
		int x = r.x + 10;
		int y = r.y + 10;
		int lineHeight = fontSize + 4;

		const char* text = ipinfo.c_str();
		const char* lineStart = text;
		const char* p = text;

		while(*p) {
			if(*p == '\n' || *p == '\r') {
				// Draw current line
				int lineLen = p - lineStart;
				if(lineLen > 0) {
					char line[256];
					strncpy(line, lineStart, lineLen);
					line[lineLen] = '\0';

					// Handle indentation
					int indent = 0;
					const char* linePtr = line;
					while(*linePtr == ' ') {
						indent += 2;
						linePtr++;
					}

					graph_draw_text_font(g, x + indent, y, linePtr, font, fontSize, theme->basic.docFGColor);
					y += lineHeight;
					if(y >= r.y + r.h - lineHeight)
						break;
				}

				// Skip newline characters
				if(*p == '\r' && *(p+1) == '\n')
					p++;
				p++;
				lineStart = p;
			} else {
				p++;
			}
		}

		// Draw the last line
		int lineLen = p - lineStart;
		if(lineLen > 0) {
			char line[256];
			strncpy(line, lineStart, lineLen);
			line[lineLen] = '\0';

			// Handle indentation
			int indent = 0;
			const char* linePtr = line;
			while(*linePtr == ' ') {
				indent += 2;
				linePtr++;
			}

			graph_draw_text_font(g, x + indent, y, linePtr, font, fontSize, theme->basic.docFGColor);
		}
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		getIPInfo();
	}
public: 
	IPConfigWidget() {
	}

	void getIPInfo() {
		const char* dev_name = "/dev/net0";
		char* ret = dev_cmd(dev_name, "ip");
		if(ret == NULL) {
			return;
		}
		if(ipinfo != ret) {
			ipinfo = ret;
			update();
		}
		free(ret);
	}
};

int main(int argc, char** argv) {
	X x;
	WidgetWin win;

	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);

	IPConfigWidget* ipW = new IPConfigWidget();
	root->add(ipW);

	win.open(&x, -1, -1, -1, 280, 280, "xipconfig", XWIN_STYLE_NO_RESIZE);	
	ipW->getIPInfo();
	win.setTimer(1);
	widgetXRun(&x, &win);
	return 0;
}