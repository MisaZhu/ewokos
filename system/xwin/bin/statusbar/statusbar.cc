#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Blank.h>
#include <Widget/Label.h>
#include <x++/X.h>
#include <unistd.h>
#include <fcntl.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/kernel_tic.h>
#include <time.h>

using namespace Ewok;

class PowerInfo : public Widget {
	int powerFD;
	int batt;
	int charging;

	void drawCharging(graph_t* g, XTheme* theme, const grect_t& r, int bat) {
		static int b = 0;
		int w = r.w*bat*b/300;
		graph_gradation(g, r.x+r.w-w, r.y, w, r.h, 0xffffffff, 0xff22dd22, true);

		char txt[8];
		snprintf(txt, 7, "%d%%", bat);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
					txt, theme->getFont(), theme->basic.fontSize, 0xff000000, FONT_ALIGN_CENTER);
		b++;
		b%=4;
	}

	void drawBat(graph_t* g, XTheme* theme, const grect_t& r, int bat) {
		int w = r.w*bat/100;
		graph_gradation(g, r.x+r.w-w, r.y, w, r.h, 0xffffffff, 0xff22dd22, true);

		char txt[8];
		snprintf(txt, 7, "%d%%", bat);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
					txt, theme->getFont(), theme->basic.fontSize, 0xff000000, FONT_ALIGN_CENTER);
	}

	void drawBase(graph_t* g, grect_t& r) {
		graph_gradation(g, r.x, r.y+2, 5, r.h-4, 0xffffffff, 0xffaaaaaa, true);
		graph_box(g, r.x, r.y+2, 5, r.h-4, 0xff000000);
		r.x += 2;
		r.w -= 2;

		graph_gradation(g, r.x, r.y, r.w, r.h, 0xffffffff, 0xff888888, true);
		graph_box(g, r.x, r.y, r.w, r.h, 0xff000000);
		r.x++;
		r.y++;
		r.w -= 2;
		r.h -= 2;
	}

protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		//graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
		grect_t rb = {r.x+4, r.y+4, r.w-8, r.h-8};
		drawBase(g, rb);
		if(charging)
			drawCharging(g, theme, rb, batt);
		else
			drawBat(g, theme, rb, batt);
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		if((timerStep % 4) == 0) {
			if(powerFD < 0)
				powerFD = ::open("/dev/power0", O_RDONLY);

			if(powerFD < 0)
				return;

			uint8_t buf[4];
			if(read(powerFD, buf, 3) != 3)
				return;
			if(buf[0] == 0)
				return;

			if(buf[1])
				charging = 1;
			batt = buf[2];
		}
		update();
	}

public:
	inline PowerInfo() {
		powerFD = -1;
		batt = 0;
		charging = 0;
		setAlpha(true);
	}
	
	inline ~PowerInfo() {
		if(powerFD > 0)
			::close(powerFD);
	}
};


class DateTime : public Widget {
	uint32_t sec;
    uint32_t min;
    uint32_t hour;

protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		char txt[16];
		snprintf(txt, 15, "%02d:%02d:%02d", hour, min, sec);
		graph_draw_text_font_align(g, r.x+1, r.y+1, r.w, r.h,
					txt, theme->getFont(), theme->basic.fontSize, 0xff000000, FONT_ALIGN_CENTER);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
					txt, theme->getFont(), theme->basic.fontSize, 0xffcccccc, FONT_ALIGN_CENTER);
	}

    uint32_t updateTime() {
		time_t now = time(NULL);
		struct tm time_info;
        localtime_r(&now, &time_info);
		return time_info.tm_hour * 3600 + time_info.tm_min * 60 + time_info.tm_sec;
    }

    void onTimer(uint32_t timerFPS, uint32_t timerStep) {
        uint32_t ksec = updateTime();
		if(ksec == 0)
			return;

        min = (ksec / 60);
        hour = (min / 60);
        min = min % 60;
        sec = ksec % 60;
        update();
    }
public:
	inline DateTime() {
        sec = 0;
        min = 0;
        hour = 0;
		setAlpha(true);
    }
};

class RWidget: public RootWidget {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_set(g, r.x, r.y, r.w, r.h, 0x44000000);

		const char* logo = "EwokOS micro-kernel";
		graph_draw_text_font(g, r.x+4+1, r.y+1, logo,
					theme->getFont(), theme->basic.fontSize, 0xff000000);
		graph_draw_text_font(g, r.x+4, r.y, logo,
					theme->getFont(), theme->basic.fontSize, 0xffcccccc);
	}
};

int main(int argc, char** argv) {
	X x;
	xscreen_info_t scr;
	X::getScreenInfo(scr, 0);

	WidgetWin win;
	RWidget* root = new RWidget();
	win.setRoot(root);
	win.setAlpha(true);
	root->setType(Container::HORIZONTAL);

	Blank* blank = new Blank();
	blank->setAlpha(true);
	root->add(blank);

	DateTime* datetime = new DateTime();
	datetime->fix(96, 0);
	root->add(datetime);

	PowerInfo* powerInfo = new PowerInfo();
	powerInfo->fix(48, 0);
	root->add(powerInfo);

	win.open(&x, -1, 0, 0, scr.size.w, 20, "statusbar", XWIN_STYLE_NO_FOCUS | XWIN_STYLE_SYSBOTTOM | XWIN_STYLE_NO_FRAME | XWIN_STYLE_NO_BG_EFFECT);
	win.setTimer(2);
	widgetXRun(&x, &win);
	return 0;
}