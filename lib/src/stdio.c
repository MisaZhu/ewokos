#include <lib/stdio.h>
#include <lib/syscall.h>

void putch(int c) {
	syscall1(SYSCALL_UART_PUTCH, c);
}

int getch() {
	return syscall0(SYSCALL_UART_GETCH);
}

void putstr(const char* s) {
	int i = 0;
	while(1) {
		int c = (int)s[i++];
		if(c == 0)
			break;
		putch(c);
	}
}
