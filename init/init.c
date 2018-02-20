#include <lib/stdio.h>
#include <lib/fork.h>

void _start()
{
	putstr("\nLet there be light!\n  (here comes the first process 'init').\n");
	int i = fork();
	while (1) {
		if(i == 0)
			putstr("child.\n");	
		else {
			putstr("father.\n");	
			exit(0);
		}
	}
}
