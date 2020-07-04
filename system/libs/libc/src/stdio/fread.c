#include <stdio.h>
#include <unistd.h>
#include <sys/basic_math.h>

uint32_t fread(void* ptr, uint32_t size, uint32_t nmemb, FILE* fp) {
	if(size == 0 || fp == NULL)
		return 0;

	int32_t rd = read(fp->fd, ptr, size*nmemb);
	if(rd < 0)
		rd = 0;
	return div_u32(rd, size);
}

