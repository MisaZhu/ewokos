#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/keydef.h>
#include <upng/upng.h>

using namespace Ewok;

class StatusLabel: public Label {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.titleBGColor, true);
		font_t* font = theme->getFont();
		int y = r.y + (r.h-font->max_size.y)/2;
		graph_draw_text_font(g, r.x+4, y, label.c_str(), font, theme->basic.titleColor);
	}
public:
	StatusLabel(const char* label) : Label(label) {}
};

class ImageView: public Scrollable {
	graph_t* img;
	graph_t* imgOrig;
	int off_x;
	int off_y;
	float zoom;
	gpos_t  last_mouse_down;
	StatusLabel* statusLabel;

	void background(graph_t* g, uint32_t sz, const grect_t& r) {
		int x = r.x;
		int y = r.y;
		uint32_t c1;
		uint32_t c2;
		for(int i=0; ;i++) {
			if((i%2) == 0) {
				c1 = 0xff888888;
				c2 = 0xff4444444;
			}
			else {
				c2 = 0xff888888;
				c1 = 0xff444444;
			}

			for(int j=0; ;j++) {
				graph_fill(g, x, y, sz, sz, (j%2)==0? c1:c2);
				x += sz;
				if(x >= r.x + r.w)
					break;
			}
			x = 0;
			y += sz;
			if(y >= r.y + r.h)
				break;
		}
	}
	void zoomImg() {
		if(img != NULL)
			graph_free(img);
		img = graph_scalef(imgOrig, zoom);
		if(img->w < area.w)
			off_x = 0;
		if(img->h < area.h)
			off_y = 0;
	}
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		background(g, 20, r);
		if(img == NULL)
			return;

		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, r.x-off_x, r.y-off_y, img->w, img->h, 0xff);
	}

	void onScroll(int step, bool horizontal) {
	}

	void onResize() {
		updateScroller();
	}

	void updateScroller() {
		if(img == NULL)
			return;
		setScrollerInfo(img->w, off_x, area.w, true);
		setScrollerInfo(img->h, off_y, area.h, false);

		if(statusLabel != NULL) {
			char s[128] = { 0 };
			snprintf(s, 127, "zoom:%.2f, w:%d, h:%d, x:%d, y:%d", zoom, img->w, img->h, off_x, off_y);
			statusLabel->setLabel(s);
		}
	}

	bool onMouse(xevent_t* ev) {
		gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
		if(ev->state == XEVT_MOUSE_DOWN) {
			last_mouse_down.x = ipos.x;
			last_mouse_down.y = ipos.y;
		}
		else if(ev->state == XEVT_MOUSE_DRAG) {
			int dx = last_mouse_down.x - ipos.x;
			int dy =  last_mouse_down.y - ipos.y;

			if(abs_32(dx) > 10 && img->w > area.w) {
				off_x +=  dx;
				last_mouse_down.x = ipos.x;
				if(off_x < 0)
					off_x = 0;
				else if(off_x > (img->w-area.w))
					off_x = img->w-area.w;
				updateScroller();
				update();
			}
			
			if(abs_32(dy) > 10 && img->h > area.h) {
				off_y +=  dy;
				last_mouse_down.y = ipos.y;
				if(off_y < 0)
					off_y = 0;
				else if(off_y > (img->h-area.h))
					off_y = img->h-area.h;
				updateScroller();
				update();
			}
		}
		else if(ev->state == XEVT_MOUSE_MOVE) {
			if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
				zoom += 0.2;
				if(zoom > 2.0)
					zoom = 3.0;
				zoomImg();
				updateScroller();
				update();
			}
			else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
				zoom -= 0.4;
				if(zoom < 0.4)
					zoom = 0.4;
				zoomImg();
				updateScroller();
				update();
			}
		}
		return true;
	}


	bool onIM(xevent_t* ev) {
		if(ev->state == XIM_STATE_PRESS) {
			if(ev->value.im.value == KEY_UP) {
				off_y -= 10;
				if(off_y < 0)
					off_y = 0;
				updateScroller();
				update();
			}
			else if(ev->value.im.value == KEY_DOWN) {
				if(img->h > area.h) {
					off_y += 10;
					if(off_y > (img->h-area.h))
						off_y = img->h-area.h;
				}
				updateScroller();
				update();
			}
			else if(ev->value.im.value == KEY_LEFT) {
				off_x -= 10;
				if(off_x < 0)
					off_x = 0;
				updateScroller();
				update();
			}
			else if(ev->value.im.value == KEY_RIGHT) {
				if(img->w > area.w) {
					off_x += 10;
					if(off_x > (img->w-area.w))
						off_x = img->w-area.w;
				}
				updateScroller();
				update();
			}
		}
		return true;
	}

public: 
	ImageView() {
		img = NULL;
		imgOrig = NULL;
		off_x = 0;
		off_y = 0;
		zoom = 1.0;
		last_mouse_down.x = 0;
		last_mouse_down.y = 0;
		statusLabel = NULL;
	}

	~ImageView() {
		if(img != NULL)
			graph_free(img);
	}

	void loadImage(const char* fname) {
		imgOrig = png_image_new(fname);
		zoom = 1.0;
		zoomImg();
	}

	void setStatusLabel(StatusLabel* l) {
		statusLabel = l;
	}
};

int main(int argc, char** argv) {
	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);
	root->setAlpha(false);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	root->add(c);

	ImageView* imgView = new ImageView();
	c->add(imgView);

	Scroller *sr = new Scroller();
	sr->fix(8, 0);
	imgView->setScrollerV(sr);
	imgView->loadImage(argc < 2 ? "/data/test/test.png":argv[1]);
	c->add(sr);

	sr = new Scroller(true);
	sr->fix(0, 8);
	imgView->setScrollerH(sr);
	root->add(sr);

	StatusLabel* statusLabel = new StatusLabel("");
	statusLabel->fix(0, 16);
	imgView->setStatusLabel(statusLabel);
	root->add(statusLabel);

	x.open(0, &win, -1, -1, 400, 300, "xpng", XWIN_STYLE_NORMAL);
	win.setVisible(true);
	win.setTimer(12);
	x.run(NULL, &win);
	return 0;
}