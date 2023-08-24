#include "SolarisWM.h"
using namespace Ewok;

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	SolarisWM xwm;
	xwm.readConfig();
	xwm.run();
	return 0;
}
