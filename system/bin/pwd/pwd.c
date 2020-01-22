#include <unistd.h>
#include <stdio.h>
#include <fsinfo.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	char pwd[FS_FULL_NAME_MAX];
	printf("%s\n", getcwd(pwd, FS_FULL_NAME_MAX));
	return 0;
}
