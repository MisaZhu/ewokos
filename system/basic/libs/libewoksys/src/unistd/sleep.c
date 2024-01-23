#include <unistd.h>

unsigned int sleep(unsigned int seconds) {
	usleep(seconds * 1000 * 1000);
	return 0;
}

