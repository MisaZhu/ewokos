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
	graph_t *logo;
	bool passwordMode;

	bool login() {
		session_info_t info;
		if(session_check(username.c_str(), password.c_str(), &info) != 0)
			return false;

		close();
		vfs_create(info.home, NULL, FS_TYPE_DIR, 0750, false, true);
		chown(info.home, info.uid, info.gid);

		setgid(info.gid);
		setuid(info.uid);
		setenv("HOME", info.home);

		proc_exec("/bin/shell /etc/x/xinit.rd");
		return true;
	}

	void drawBG(graph_t* g) {
		graph_draw_dot_pattern(g, 0, 0, g->w, g->h,
				theme.basic.bgColor, theme.basic.fgColor, 1);
	}

	void drawFrame(graph_t* g, const grect_t& r) {
		//graph_fill_3d(g, r.x, r.y, r.w, r.h, 0xff88aaff, false);
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme.basic.bgColor, false);
		graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
				g, r.x-logo->w, r.y, logo->w, logo->h, 0xff);
	}

	void drawInput(graph_t* g, const grect_t& r, const char* title, const char* input) {
		font_t* font = theme.getFont();
		int y = r.y;

		graph_draw_text_font_align(g, r.x, y, r.w, font->max_size.y, 
				title, font, theme.basic.fgColor, FONT_ALIGN_CENTER);

		y += font->max_size.y+8;

		graph_fill_3d(g, r.x+16, y, r.w-32, font->max_size.y, theme.basic.bgColor, true);
		if(!passwordMode)
			graph_draw_text_font_align(g, r.x, y, r.w, font->max_size.y,
					input, font, theme.basic.fgColor, FONT_ALIGN_CENTER);
	}

protected:
	void onRepaint(graph_t* g) {
		font_t* font = theme.getFont();
		const char* input = passwordMode ? password.c_str() : username.c_str();
		const char* title = passwordMode ? "password" : "username";

		uint32_t tw, th, iw, ih;
		font_text_size(title, font, &tw, &th);
		font_text_size(input, font, &iw, &ih);
		if(iw < tw)
			iw = tw;

		uint32_t fw = iw + 64;
		uint32_t fh = ih * 3;
		grect_t frameR = { (g->w - fw) / 2, (g->h - fh) / 2, fw, fh };

		drawBG(g);
		drawFrame(g, frameR);
		drawInput(g, frameR, title, input);
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_PRESS) {
			EwokSTL::string &input = passwordMode ? password:username;
			int c = ev->value.im.value;
			int len = input.length();

			if(c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
				if(len > 0)
					input = input.substr(0, len-1);
			}
			else if(c == KEY_ENTER) {
				if(!passwordMode) {
					if(!login())
						passwordMode = true;
				}
				else {
					if(!login()) {
						username = "";
						password = "";
						passwordMode = false;
					}
				}
			}
			else if(c > 20) {
				if(len < 16)
					input += c;
			}
			repaint();
		}
	}

public:
	XSession() {
		theme.setFont(theme.basic.fontName, 16);
		passwordMode = false;
		logo = png_image_new("/usr/system/icons/ewok.png");
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

