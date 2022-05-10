#ifndef XWIN_HH
#define XWIN_HH

#include <x/xwin.h>
#include <graph/graph.h>

namespace Ewok {

class X;

class XWin {
protected:
	X* x;
	xwin_t* xwin;
	virtual void onRepaint(graph_t* g) = 0;

	inline virtual void onClose(void)   { }
	inline virtual void onMin(void)     { }
	inline virtual void onResize(void)  { }
	inline virtual void onFocus(void)   { }
	inline virtual void onUnfocus(void) { }
	inline virtual void onEvent(xevent_t* ev)  {
		(void)ev;
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
	inline void __doFocus(void) { onFocus(); }
	inline void __doUnfocus(void) { onUnfocus(); }
	inline void __doEvent(xevent_t* ev) { onEvent(ev); }

	void close(void);
	bool setVisible(bool visible);
	bool callXIM(void);

	bool updateInfo(const xinfo_t& xinfo);
	bool getInfo(xinfo_t& xinfo);
	void repaint(bool thread = false);
	void resizeTo(int w, int h);
	void resize(int dw, int dh);
	void moveTo(int x, int y);
	void move(int dx, int dy);
	void setDisplay(int index);
};

}
#endif

