#include <unistd.h>
#include <stdio.h>

int main() {
	char pwd[FULL_NAME_MAX];
	printf("%s\n", getcwd(pwd, FULL_NAME_MAX));
	return 0;
}
