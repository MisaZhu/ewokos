#include <timer.h>

void timerWait(uint32_t delay) {
	uint32_t init = timerRead();
	while((timerRead()-init) < delay);
}
