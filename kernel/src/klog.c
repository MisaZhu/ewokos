#include "klog.h"
#include "dev/uart.h"

void klog(const char* msg) {
	uartPuts(msg);
}
