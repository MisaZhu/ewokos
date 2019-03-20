#include <timer.h>

void timer_wait(uint32_t delay) {
	uint32_t init = timer_read();
	while((timer_read()-init) < delay);
}
