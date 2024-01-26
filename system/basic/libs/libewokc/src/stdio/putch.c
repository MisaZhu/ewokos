#include <stdio.h>
#include <unistd.h>

void putch(int c) {
	write(1, &c, 1);
}

