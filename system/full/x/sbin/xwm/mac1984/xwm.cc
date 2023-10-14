#include "MacWM.h"
#include <x/x.h>
#include <sys/klog.h>
using namespace Ewok;

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	MacWM xwm;
	klog("%s\n", x_get_theme_fname("xwm.conf"));	
	xwm.readConfig(x_get_theme_fname("xwm.conf"));	
	xwm.run();
	return 0;
}
