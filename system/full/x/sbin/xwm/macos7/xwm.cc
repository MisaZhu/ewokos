#include "MacWM.h"
using namespace Ewok;

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	MacWM xwm;
	xwm.readConfig();
	xwm.run();
	return 0;
}
