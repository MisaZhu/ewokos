#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ewoksys/vfs.h>

uint32_t fwrite(const void* ptr, uint32_t size, uint32_t nmemb, FILE* fp) {
	if(size == 0)
		return 0;

	int fsize = size*nmemb;
	const char* p = ptr;
	while(fsize > 0) {
		int sz = write(fp->fd, p, VFS_BUF_SIZE < fsize ? VFS_BUF_SIZE:fsize);
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

	/*
	int32_t rd = write(fp->fd, ptr, size*nmemb);
	if(rd < 0)
		rd = 0;
	return (rd / size);
	*/
}

