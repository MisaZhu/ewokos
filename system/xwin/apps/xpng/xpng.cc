#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <WidgetEx/FileDialog.h>

#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/keydef.h>
#include <graph/graph_png.h>

using namespace Ewok;

class StatusLabel: public Label {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.titleBGColor, true);
		font_t* font = theme->getFont();
		int y = r.y + (r.h- font_get_height(font, theme->basic.fontSize))/2;
		graph_draw_text_font(g, r.x+4, y, label.c_str(), font, theme->basic.fontSize, theme->basic.titleColor);
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
		graph_set(g, r.x, r.y, r.w, r.h, 0);
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

		grect_t ir = {r.x-off_x, r.y-off_y, img->w, img->h};
		int dw = r.x + r.w - ir.x - ir.w;
		ir.w = ir.w + dw;
		int dh = r.y + r.h - ir.y - ir.h;
		ir.h = ir.h + dh;

		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, ir.x, ir.y, ir.w, ir.h, 0xff);
	}

	bool onScroll(int step, bool horizontal) {
		if(horizontal) {
			off_x -=  step * dragStep;
			if(off_x < 0)
				off_x = 0;
			else if(off_x > (img->w-area.w))
				off_x = img->w-area.w;
		}
		else {
			off_y -= step * dragStep;
			if(off_y < 0)
				off_y = 0;
			else if(off_y > (img->h-area.h))
				off_y = img->h-area.h;
		}
		return true;
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
		Scrollable::onMouse(ev);

		if(ev->state == MOUSE_STATE_MOVE) {
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
		alpha = true;
	}

	~ImageView() {
		if(img != NULL)
			graph_free(img);

		if(imgOrig != NULL)
			graph_free(imgOrig);
	}

	void loadImage(const char* fname) {
		if(imgOrig != NULL)
			graph_free(imgOrig);

		imgOrig = png_image_new(fname);
		zoom = 1.0;
		zoomImg();
		updateScroller();
		update();
	}

	void setStatusLabel(StatusLabel* l) {
		statusLabel = l;
	}
};

class PngWin: public WidgetWin{
	FileDialog fdialog;
	ImageView* imgView;
protected:
	void onDialoged(XWin* from, int res) {
		if(res == Dialog::RES_OK) {
			string fname = fdialog.getResult();
			imgView->loadImage(fname.c_str());
			repaint();
		}
	}
public:

	void load(ImageView* imgView, const string& fname) {
		this->imgView = imgView;
		if(fname.length() == 0)
			fdialog.popup(this, 400, 300, "files", XWIN_STYLE_NORMAL);
		else
			imgView->loadImage(fname.c_str());
		repaint();
	}
};

class LoadButton: public LabelButton {
	PngWin* pngWin;
	ImageView* imgView;
protected:
	void onClick(xevent_t* ev) {
		pngWin->load(imgView, "");
	}
public:
	LoadButton(PngWin* pwin, ImageView* imgView) : LabelButton("load") {
		this->imgView = imgView;
		pngWin = pwin;
	}
};

int main(int argc, char** argv) {
	X x;
	PngWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);
	root->setAlpha(true);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	root->add(c);

	ImageView* imgView = new ImageView();
	c->add(imgView);

	Scroller *sr = new Scroller();
	sr->fix(8, 0);
	imgView->setScrollerV(sr);
	c->add(sr);

	sr = new Scroller(true);
	sr->fix(0, 8);
	imgView->setScrollerH(sr);
	root->add(sr);

	StatusLabel* statusLabel = new StatusLabel("");
	imgView->setStatusLabel(statusLabel);
	statusLabel->fix(0, 20);
	root->add(statusLabel);

	LoadButton *lbutton = new LoadButton(&win, imgView);
	lbutton->fix(0, 20);
	root->add(lbutton);

	win.open(&x, 0, -1, -1, 400, 300, "xpng", XWIN_STYLE_NORMAL);
	if(argc >= 2)
		win.load(imgView, argv[1]);
	x.run(NULL, &win);
	return 0;
}