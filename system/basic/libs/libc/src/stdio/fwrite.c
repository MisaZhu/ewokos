#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

uint32_t fwrite(const void* ptr, uint32_t size, uint32_t nmemb, FILE* fp) {
	if(size == 0)
		return 0;

	int32_t rd = write(fp->fd, ptr, size*nmemb);
	if(rd < 0)
		rd = 0;
	return (rd / size);
}

