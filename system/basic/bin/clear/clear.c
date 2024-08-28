#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL);
	printf("\033[2J");
	return 0;
}
