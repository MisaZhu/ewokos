#include <unistd.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	char pwd[FULL_NAME_MAX];
	printf("%s\n", getcwd(pwd, FULL_NAME_MAX));
	return 0;
}
