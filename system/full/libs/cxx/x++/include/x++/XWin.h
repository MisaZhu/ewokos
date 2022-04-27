#ifndef XWIN_H
#define XWIN_H

#include <x/xclient.h>
#include <graphxx/graphxx.h>

namespace Ewok {

class XWin {
protected:
	xwin_t* xwin;
	virtual void onRepaint(Graph& g) = 0;

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

	void setCWin(xwin_t* xw);

	inline xwin_t* getCWin(void) { return xwin; }

	inline void __doRepaint(graph_t* g) { Graph gxx(g->buffer, g->w, g->h); onRepaint(gxx); }
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
	void repaint(void);
};

}
#endif

