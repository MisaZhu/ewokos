#include <string.h>
#include <vfs.h>
#include <tree.h>
#include <syscall.h>
#include <kserv/fs.h>
#include <stdio.h>


int readTTY(TreeNodeT* node, int seek, char* buf, uint32_t size) {
	(void)node;
	(void)seek;
	for(uint32_t i=0; i<size; i++) {
		char c = syscall0(SYSCALL_UART_GETCH);
		if(c == 0) {
			return i;
		}
		buf[i] = c;
	}
	return size;
}

int writeTTY(TreeNodeT* node, int seek, const char* buf, uint32_t size) {
	(void)node;
	(void)seek;
	for(uint32_t i=0; i<size; i++) {
		syscall1(SYSCALL_UART_PUTCH, (int)buf[i]);
	}
	return size;
}
