#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/md5.h>

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;
	
	setbuf(stdout, NULL);

	const char* p = md5_encode_str((const uint8_t*)argv[1], strlen(argv[1]));
	printf("%s\n", p);
	return 0;
}
