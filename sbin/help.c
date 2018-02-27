#include <stdio.h>
#include <fork.h>

void _start()
{
	printf("==========================\n"
			"Hello, This is EwokOS.\n"
			"--------------------------\n"
			"May the Force be with you!\n"
			"==========================\n");
	exit(0);
}
