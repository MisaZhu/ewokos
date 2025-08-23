#ifndef XWM_HH
#define XWM_HH

#include <x/xwm.h>
#include <graph/graph_ex.h>
#include <font/font.h>
#include <UniObject/UniObject.h>

namespace Ewok {


class XWM : public UniObject {
protected:
	xwm_t xwm;
	font_t* font;

	graph_t* desktopPattern;
	virtual graph_t* genDesktopPattern(void);

	virtual void getColor(uint32_t *fg, uint32_t* bg, bool top);
	virtual void getWinSpace(int style, grect_t* xr, grect_t* wsr);
	virtual void getClose(xinfo_t* info, grect_t* r);
	virtual void getMin(xinfo_t* info, grect_t* r);
	virtual void getMinSize(xinfo_t* info, int* w, int* h);
	virtual void getMax(xinfo_t* info, grect_t* r);
	virtual void getTitle(xinfo_t* info, grect_t* r);
	virtual void getResize(xinfo_t* info, grect_t* r);
	virtual void getFrame(xinfo_t* info, grect_t* r);

	virtual void drawDesktop(graph_t* g);
	virtual void drawTitle(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawFrame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawShadow(graph_t* desktop_g, graph_t* g, xinfo_t* info, bool top);
	virtual void drawDragFrame(graph_t* g, grect_t* r);
	virtual void onLoadTheme(void) { }
	virtual void drawBGEffect(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, bool top) { };

	void updateTheme(bool loadFromX);
public:
	inline void __getWinSpace(int style, grect_t* xr, grect_t* wsr) {getWinSpace(style, xr, wsr);}
	inline void __getClose(xinfo_t* info, grect_t* r) { getClose(info, r); }
	inline void __getMin(xinfo_t* info, grect_t* r) { getMin(info, r); }
	inline void __getMinSize(xinfo_t* info, int* w, int* h) { getMinSize(info, w, h); }
	inline void __getMax(xinfo_t* info, grect_t* r) { getMax(info, r); }
	inline void __getTitle(xinfo_t* info, grect_t* r) { getTitle(info, r); }
	inline void __getResize(xinfo_t* info, grect_t* r) { getResize(info, r); }
	inline void __getFrame(xinfo_t* info, grect_t* r) { getFrame(info, r); }
	inline void __drawDesktop(graph_t* g) { drawDesktop(g); }
	inline void __updateTheme(bool loadFromX) { updateTheme(loadFromX); }

	inline void __drawTitle(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top) {drawTitle(desktop_g, g, info, r, top);}
	inline void __drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {drawMax(g, info, r, top);}
	inline void __drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {drawMin(g, info, r, top);}
	inline void __drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {drawClose(g, info, r, top);}
	inline void __drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {drawResize(g, info, r, top); }
	inline void __drawFrame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top) { drawFrame(desktop_g, frame_g, ws_g, info, r, top); }
	inline void __drawShadow(graph_t* desktop_g, graph_t* g, xinfo_t* info, bool top) { drawShadow(desktop_g, g, info, top); }
	inline void __drawBGEffect(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, bool top) { drawBGEffect(desktop_g, frame_g, ws_g, info, top); }
	inline void __drawDragFrame(graph_t* g, grect_t* r) {drawDragFrame(g, r); }

	XWM(void);
	virtual ~XWM(void);

	void loadTheme(const char* name);
	void run(void);
};

}
#endif
