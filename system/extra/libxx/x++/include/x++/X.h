#ifndef X_H
#define X_H

extern "C" {
	#include <stddef.h>
	#include <x/xclient.h>
}

#include <graphxx/graphxx.h>

class X {
	x_t x;
public:
	inline x_t* c_x(void) { return &x; }
	X(void);
	void run(void (*loop)(void*), void* p = NULL);
	void terminate(void);
};

class XWin {
protected:
	xwin_t* xwin;
	virtual void onRepaint(Graph& g) = 0;

	inline virtual void onClose(void)   { }
	inline virtual void onMin(void)     { }
	inline virtual void onResize(void)  { }
	inline virtual void onFocus(void)   { }
	inline virtual void onUnfocus(void) { }
	inline virtual void onLoop(void)    { }
	inline virtual void onEvent(xevent_t* ev)  {
		(void)ev;
	}

public:
	inline XWin(void) {
		this->xwin = NULL;
	}

	inline virtual ~XWin(void) {
		this->close();
	}

	inline void __doRepaint(graph_t* g) { Graph gxx(g->buffer, g->w, g->h); onRepaint(gxx); }
	inline void __doClose(void) { onClose(); }
	inline void __doMin(void) { onMin(); }
	inline void __doResize(void) { onResize(); }
	inline void __doFocus(void) { onFocus(); }
	inline void __doUnfocus(void) { onUnfocus(); }
	inline void __doEvent(xevent_t* ev) { onEvent(ev); }

	bool open(X* xp, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style);
	void close(void);
	bool setVisible(bool visible);

	bool updateInfo(const xinfo_t& xinfo);
	bool getInfo(xinfo_t& xinfo);
	static bool screenInfo(xscreen_t& scr);
	bool isTop(void);
	void repaint(void);
};

#endif

