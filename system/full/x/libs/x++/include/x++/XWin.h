#ifndef XWIN_HH
#define XWIN_HH

#include <x/xwin.h>
#include <x++/XTheme.h>
#include <graph/graph_ex.h>

namespace Ewok {

class X;

class XWin {
protected:
	X* x;
	xwin_t* xwin;
	XTheme theme;
	virtual void onRepaint(graph_t* g) = 0;

	inline virtual void onClose(void)   { }
	inline virtual void onMin(void)     { }
	inline virtual void onResize(void)  { }
	inline virtual void onMove(void)  { }
	inline virtual void onFocus(void)   { }
	inline virtual void onUnfocus(void) { }
	inline virtual void onReorg(void)   { }
	inline virtual void onEvent(xevent_t* ev)  { (void)ev; }

	inline bool repaintLazy(void)  {
		return xwin->xinfo->repaint_lazy;
	}
public:
	XWin(void);

	inline virtual ~XWin(void) {
		this->close();
	}

	inline X* getX(void) {return x;}

	inline void setX(X* x) { this->x = x;}

	void setCWin(xwin_t* xw);

	inline xwin_t* getCWin(void) { return xwin; }

	inline void __doRepaint(graph_t* g) { onRepaint(g); }
	inline void __doClose(void) { onClose(); }
	inline void __doMin(void) { onMin(); }
	inline void __doResize(void) { onResize(); }
	inline void __doMove(void) { onMove(); }
	inline void __doFocus(void) { onFocus(); }
	inline void __doUnfocus(void) { onUnfocus(); }
	inline void __doReorg(void) { onReorg(); }
	inline void __doEvent(xevent_t* ev) { onEvent(ev); }

	void close(void);
	bool setVisible(bool visible);
	void setAlpha(bool alpha);
	bool callXIM(void);

	bool getInfo(xinfo_t& xinfo);
	void repaint(void);
	void resizeTo(int w, int h);
	void resize(int dw, int dh);
	void moveTo(int x, int y);
	void move(int dx, int dy);
	void setDisplay(int index);
	void top(void);
	void pop(void);
	gpos_t getInsidePos(int32_t x, int32_t y);
	gpos_t getScreenPos(int32_t x, int32_t y);
};

}
#endif

