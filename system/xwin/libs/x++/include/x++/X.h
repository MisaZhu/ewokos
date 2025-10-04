#ifndef X_HH
#define X_HH

#include <x/xwin.h>
#include <graph/graph.h>
#include <x++/XWin.h>
#include <ewoksys/vfs.h>
#include <stdio.h>
#include <font/font.h>
#include <string>
#include <UniObject/UniObject.h>

using namespace std;

namespace Ewok {

class X : public UniObject {
	x_t x;
public:
	inline x_t* c_x(void) { return &x; }
	X(void);
	void run(void (*loop)(void*), void* p = NULL);
	void terminate(void);
	bool terminated(void);

	static string getResName(const char* name);

	static uint32_t getDisplayNum();
	static bool    getScreenInfo(xscreen_info_t& scr, int index = 0);
	static bool    getDesktopSpace(grect_t& r, int index = 0);
	static bool    setDesktopSpace(const grect_t& r, int index = 0);
	static int     getTheme(x_theme_t* theme);
	static font_t* getSysFont(void);
};

}
#endif
