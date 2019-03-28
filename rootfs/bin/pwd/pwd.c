#include <unistd.h>
#include <stdio.h>

int main() {
	char pwd[NAME_MAX];
	printf("%s\n", getcwd(pwd, NAME_MAX));
	return 0;
}
