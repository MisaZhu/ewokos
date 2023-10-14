#include "MacWM.h"
using namespace Ewok;

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	MacWM xwm;
	xwm.readConfig("/etc/x/xwm_solaris.conf");	
	xwm.run();
	return 0;
}
