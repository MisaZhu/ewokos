#ifndef X_H
#define X_H

#include <x/xclient.h>
#include <graph/graph.h>
#include <x++/XWin.h>

namespace Ewok {

class X {
	x_t x;
public:
	inline x_t* c_x(void) { return &x; }
	X(void);
	void run(void (*loop)(void*), void* p = NULL);
	void terminate(void);
	bool open(XWin* xwin, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style);
	bool screenInfo(xscreen_t& scr, int index);
};

}
#endif

