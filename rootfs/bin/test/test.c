#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <syscall.h>

static int i;

static void f() {
	i++;
	if(i > 'z') 
		i = '0';
}

void test() {
	while(true) {
		f();
		printf("%c", i);
		yield();
	}
	exit(0);
}

void _start() {
	i = '0';

	int32_t tid = syscall1(SYSCALL_THREAD, (int32_t)test);	
	while(true) {
		printf("father %x, %d\n", &i, i);
		yield();
	}
	exit(0);
}

