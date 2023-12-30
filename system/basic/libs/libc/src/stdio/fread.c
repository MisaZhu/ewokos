#include <stdio.h>
#include <unistd.h>
#include <ewoksys/vfs.h>

uint32_t fread(void* ptr, uint32_t size, uint32_t nmemb, FILE* fp) {
	if(size == 0 || fp == NULL)
		return 0;

	int fsize = size*nmemb;
	char* p = ptr;
	while(fsize > 0) {
		int sz = read(fp->fd, p, VFS_BUF_SIZE);
		if(sz < 0)
			break;
		if(sz > 0) {
			fsize -= sz;
			p += sz;
		}
	}

	int32_t rd = nmemb;
	if(fsize > 0)
		rd = 0;
	return rd;
}

