#include <unistd.h>
#include <stdio.h>

void _start()
{
	char pwd[NAME_MAX];
	printf("%s\n", getcwd(pwd, NAME_MAX));
	exit(0);
}
