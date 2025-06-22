#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <WidgetEx/FileDialog.h>
#include <WidgetEx/ColorDialog.h>
#include <WidgetEx/Menubar.h>

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
	StatusLabel(const char* label) : Label(label) {
		alpha = false;
	}
};

class ImageView: public Scrollable {
	graph_t* img;
	graph_t* imgOrig;
	int off_x;
	int off_y;
	float zoom;
	gpos_t  last_mouse_down;
	StatusLabel* statusLabel;
	uint32_t bgColor;

	void background(graph_t* g, uint32_t sz, const grect_t& r) {
		graph_set(g, r.x, r.y, r.w, r.h, bgColor);
	}
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		background(g, 20, r);
		if(img == NULL)
			return;

		int dw = img->w-off_x;
		if(dw > r.w)
			dw = r.w;

		int dh = img->h-off_y;
		if(dh > r.h)
			dh = r.h;
		graph_blt_alpha(img, off_x, off_y, img->w-off_x, img->h-off_y,
				g, r.x, r.y, dw, dh, 0xff);
	}

	bool onScroll(int step, bool horizontal) {
		if(img == NULL)
			return false;

		if(horizontal) {
			off_x -=  step * dragStep;
			if(off_x < 0 || (img->w-area.w) < 0)
				off_x = 0;
			else if(off_x > (img->w-area.w))
				off_x = img->w-area.w;
		}
		else {
			off_y -= step * dragStep;
			if(off_y < 0 || (img->h-area.h) < 0)
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
				scroll(-1, false);
				return true;
			}
			else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
				scroll(1, false);
				return true;
			}
		}
		return false;
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
		bgColor = 0x88000000;
		zoom = 1.0;
		last_mouse_down.x = 0;
		last_mouse_down.y = 0;
		statusLabel = NULL;
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
		zoomImgTo(1.0);
	}

	void setStatusLabel(StatusLabel* l) {
		statusLabel = l;
	}

	void zoomImgTo(float zoom) {
		if(zoom < 0.4)
			zoom = 0.4;
		else if(zoom > 8.0)
			zoom = 8.0;

		if(img != NULL)
			graph_free(img);
		img = graph_scalef(imgOrig, zoom);
		if(img->w < area.w)
			off_x = 0;
		if(img->h < area.h)
			off_y = 0;
		
		this->zoom = zoom;	
		updateScroller();
		update();
	}

	void zoomImg(bool in) {
		if(in)
			zoom += 0.2;
		else
			zoom -= 0.2;
		zoomImgTo(zoom);
	}

	void setBGColor(uint32_t color, uint8_t alpha) {
		bgColor = (color & 0x00ffffff) | (alpha << 24);
		update();
	}

	uint32_t getBGColor() {
		return bgColor;
	}
};

class PngWin: public WidgetWin{
	FileDialog fdialog;
	ColorDialog cdialog;
	ImageView* imgView;
protected:
	void onDialoged(XWin* from, int res) {
		if(res == Dialog::RES_OK) {
			if(from == &fdialog) {
				load(imgView, fdialog.getResult().c_str());
				string fname = fdialog.getResult();
				imgView->loadImage(fname.c_str());
				repaint();
			}
			else if(from == &cdialog) {
				uint32_t color = cdialog.getColor();
				uint8_t alpha = cdialog.getTransparent();
				imgView->setBGColor(color, alpha);
			}
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

	void setBGColor(ImageView* imgView) {
		this->imgView = imgView;
		cdialog.popup(this, 256, 160, "bgColor", XWIN_STYLE_NO_RESIZE);
		cdialog.setColor(imgView->getBGColor());
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

static void onLoadFunc(MenuItem* it, void* p) {
	ImageView* imgView = (ImageView*)p;
	PngWin* pngWin = (PngWin*)imgView->getWin();
	pngWin->load(imgView, "");
}

static void onZoomInFunc(MenuItem* it, void* p) {
	ImageView* imgView = (ImageView*)p;
	imgView->zoomImg(true);
}

static void onZoomOutFunc(MenuItem* it, void* p) {
	ImageView* imgView = (ImageView*)p;
	imgView->zoomImg(false);
}

static void onBGColorFunc(MenuItem* it, void* p) {
	ImageView* imgView = (ImageView*)p;
	PngWin* pngWin = (PngWin*)imgView->getWin();
	pngWin->setBGColor(imgView);
}

int main(int argc, char** argv) {
	X x;
	PngWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);
	

	Menubar* menubar = new Menubar();
	root->add(menubar);
	menubar->fix(0, 20);
	menubar->setItemSize(72);

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

	menubar->add("load", NULL, NULL, onLoadFunc, imgView);
	menubar->add("zoom_in", NULL, NULL, onZoomInFunc, imgView);
	menubar->add("zoom_out", NULL, NULL, onZoomOutFunc, imgView);
	menubar->add("BGColor", NULL, NULL, onBGColorFunc, imgView);

	win.open(&x, 0, -1, -1, 400, 300, "xpng", XWIN_STYLE_NORMAL);
	win.setAlpha(true);
	if(argc >= 2)
		win.load(imgView, argv[1]);

	widgetXRun(&x, &win);
	return 0;
}