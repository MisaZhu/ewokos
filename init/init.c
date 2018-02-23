#include <lib/stdio.h>
#include <lib/fork.h>
#include <lib/string.h>
#include <lib/malloc.h>

void _start()
{
	char* p = (char*)malloc(33);
	char* p1 = (char*)malloc(5000);
	char* p2 = (char*)malloc(13);
	char* p3 = (char*)malloc(5000);

	memset(p2, '*', 13);
	
	free(p);
	free(p2);
	free(p1);
	free(p3);

	putstr("\nLet there be light!\n  (here comes the first process 'init').\n");
	int i = fork();
	if(i == 0) {
		putstr("child.\n");	
		exit(0);
	}
	else {
		putstr("father.\n");	
	}
	exit(0);
}
