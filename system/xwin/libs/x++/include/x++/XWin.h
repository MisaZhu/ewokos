#ifndef XWIN_HH
#define XWIN_HH

#include <x/xwin.h>
#include <x++/XTheme.h>
#include <graph/graph_ex.h>
#include <string>

using namespace std;
namespace Ewok {

class X;

class XWin {
protected:
	X* x;
	uint32_t displayIndex;
	xwin_t* xwin;
	XTheme theme;
	virtual void onRepaint(graph_t* g) = 0;

	inline virtual bool onClose(void)   { return true; }
	inline virtual void onOpen(void)   {  }
	inline virtual void onHide(void)   {  }
	inline virtual void onShow(void)   {  }
	inline virtual void onMin(void)     { }
	inline virtual void onResize(void)  { }
	inline virtual void onMove(void)  { }
	inline virtual void onFocus(void)   { }
	inline virtual void onUnfocus(void) { }
	inline virtual void onReorg(void)   { }
	inline virtual void onEvent(xevent_t* ev)  { (void)ev; }
	inline virtual void onDialoged(XWin* from, int res)  { (void)from; (void) res; }

	inline bool covered(void)  {
		return xwin->xinfo->covered;
	}
public:
	XWin(void);

	inline virtual ~XWin(void) {
		close();
	}

	inline X* getX(void) {return x;}
	inline XTheme* getTheme(void) {return &theme;}
	inline uint32_t getDisplayIndex(void) {return displayIndex;}

	void setCWin(xwin_t* xw);

	inline xwin_t* getCWin(void) { return xwin; }

	inline void __doRepaint(graph_t* g) { onRepaint(g); }
	inline bool __doClose(void) { return onClose(); }
	inline void __doMin(void) { onMin(); }
	inline void __doResize(void) { onResize(); }
	inline void __doMove(void) { onMove(); }
	inline void __doFocus(void) { onFocus(); }
	inline void __doUnfocus(void) { onUnfocus(); }
	inline void __doReorg(void) { onReorg(); }
	inline void __doEvent(xevent_t* ev) { onEvent(ev); }

	inline void dialoged(XWin* from, int res) { onDialoged(from, res); }

	void close(void);
	bool open(X* xp, uint32_t dispIndex, int x, int y, uint32_t w, uint32_t h,
			const char* title, uint32_t style, bool visible = true);
	bool open(X* xp, uint32_t dispIndex, const grect_t& r, const char* title, uint32_t style, bool visible = true);
	bool setVisible(bool visible);
	void setAlpha(bool alpha);
	bool callXIM(bool show = true);

	bool getInfo(xinfo_t& xinfo);
	void repaint(void);
	void resizeTo(int w, int h);
	void resize(int dw, int dh);
	void max(void);
	void moveTo(int x, int y);
	void move(int dx, int dy);
	void setDisplay(int index);
	void top(void);
	void pop(void);
	void busy(bool bs);
	gpos_t getInsidePos(int32_t x, int32_t y);
	gpos_t getScreenPos(int32_t x, int32_t y);
	bool focused();
};

}
#endif

