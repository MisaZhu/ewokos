#include <unistd.h>
#include <stdio.h>

void _start()
{
	char pwd[FNAME_MAX];
	printf("%s\n", getcwd(pwd, FNAME_MAX));
	exit(0);
}
