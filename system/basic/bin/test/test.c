#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL);
	printf("\033[1;33;44m\033[10;10Hhello world\033[0m\n");
	return 0;
}
