#include <lib/stdio.h>
#include <lib/fork.h>

void _start()
{

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
