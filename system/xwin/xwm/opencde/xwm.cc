#include "OpenCDEWM.h"
#include <x/x.h>
using namespace Ewok;

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	OpenCDEWM xwm;
	xwm.readConfig(x_get_theme_fname(X_THEME_ROOT, "xwm", "theme.json"));
	xwm.run();
	return 0;
}
