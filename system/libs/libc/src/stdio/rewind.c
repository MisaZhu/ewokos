#include <stdio.h>
#include <unistd.h>

void rewind(FILE* fp) {
	fseek(fp, 0, SEEK_SET);
}

