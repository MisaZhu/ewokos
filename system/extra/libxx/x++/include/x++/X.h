#ifndef X_H
#define X_H

extern "C" {
	#include <stddef.h>
	#include <x/xclient.h>

}

class X {
protected:
	x_t* xp;
	virtual void onRepaint(graph_t* g) = 0;

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
	inline X() {
		xp = NULL;
	}

	inline virtual ~X() {
		if(xp == NULL)
			return;
		x_close(xp);
		xp = NULL;
	}

	inline void __doRepaint(graph_t* g) { onRepaint(g); }
	inline void __doClose(void) { onClose(); }
	inline void __doMin(void) { onMin(); }
	inline void __doResize(void) { onResize(); }
	inline void __doFocus(void) { onFocus(); }
	inline void __doUnfocus(void) { onUnfocus(); }
	inline void __doLoop(void) { onLoop(); }
	inline void __doEvent(xevent_t* ev) { onEvent(ev); }

	bool open(int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style);
	void close(void);
	bool setVisible(bool visible);
	void run(void);

	bool updateInfo(xinfo_t* xinfo);
	bool getInfo(xinfo_t* xinfo);
	static bool  screenInfo(xscreen_t* scr);
	bool isTop(void);
	void repaint();
};

#endif

