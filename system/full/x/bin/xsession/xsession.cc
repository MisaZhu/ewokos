#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <ewoksys/keydef.h>
#include <ewoksys/session.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <upng/upng.h>
#include <x++/X.h>

using namespace Ewok;

#ifdef __cplusplus
extern "C" { extern int setenv(const char*, const char*);}
#endif

class XSession : public XWin {
	EwokSTL::string username;
	EwokSTL::string password;
	bool passwordMode;

	void login() {
		session_info_t info;
		if(session_check(username.c_str(), password.c_str(), &info) != 0) {
			username = "";
			password = "";
			passwordMode = false;
			return;
		}	
		close();

		vfs_create(info.home, NULL, FS_TYPE_DIR, 0750, false, true);
		chown(info.home, info.uid, info.gid);

		setgid(info.gid);
		setuid(info.uid);
		setenv("HOME", info.home);

		proc_exec("/bin/shell /etc/x/xinit.rd");
	}
protected:
	void onRepaint(graph_t* g) {
		font_t* font = theme.getFont();
		const char* s = passwordMode ? password.c_str() : username.c_str();
		int32_t x, y;
		uint32_t tw, th;
		font_text_size(s, font, &tw, &th);
		tw = tw < 100 ? 100 : (tw+8);

		x = (g->w - tw)/2;
		y = (g->h - th)/2;

		graph_clear(g, theme.basic.bgColor);	
		graph_box_3d(g, x, y, tw, th, 0xff888888, 0xffbbbbbb);

		graph_draw_text_font_align(g, 0, y-th-4, g->w, th, 
				passwordMode ? "password":"username",
				font, theme.basic.fgColor, FONT_ALIGN_CENTER);

		if(!passwordMode)
			graph_draw_text_font_align(g, x, y, tw, th, s, font, theme.basic.fgColor, FONT_ALIGN_CENTER);
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_PRESS) {
			EwokSTL::string &input = passwordMode ? password:username;
			int c = ev->value.im.value;
			if(c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
				int len = input.length();
				if(len > 0)
					input = input.substr(0, len-1);
			}
			else if(c == KEY_ENTER) {
				if(!passwordMode)
					passwordMode = true;
				else
					login();
			}
			else if(c > 20) {
				input += c;
			}
			repaint();
		}
	}

public:
	XSession() {
		theme.setFont(theme.basic.fontName, 16);
		passwordMode = false;
	}

	~XSession() {
	}
};

int main(int argc, char* argv[]) {
	X x;
	XSession xwin;
	x.open(0, &xwin, 0, 0, "XSessioin", XWIN_STYLE_NO_FRAME);
	xwin.fullscreen();
	xwin.setVisible(true);
	xwin.callXIM();

	x.run(NULL);
	return 0;
}

