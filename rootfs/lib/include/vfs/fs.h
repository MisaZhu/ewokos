#ifndef FS_H
#define FS_H

#include <fsinfo.h>

enum {
	FS_OPEN = 0,
	FS_CLOSE,
	FS_WRITE,
	FS_READ,
	FS_CTRL,
	FS_FLUSH,
	FS_DMA,
	FS_ADD
};

int fs_open(const char* name, int32_t flags);

int fs_close(int fd);

int fs_info(int fd, fs_info_t* info);

int fs_ninfo(uint32_t node_addr, fs_info_t* info);

int fs_finfo(const char* name, fs_info_t* info);

fs_info_t* fs_kids(int fd, uint32_t *num);

int fs_read(int fd, char* buf, uint32_t size);

int fs_ctrl(int fd, int32_t cmd, void* input, uint32_t isize, void* output, uint32_t osize);

int fs_getch(int fd);

int fs_putch(int fd, int c);

int fs_write(int fd, const char* buf, uint32_t size);

int fs_add(int dirFD, const char* name);

int32_t fs_flush(int fd);

int32_t fs_dma(int fd, uint32_t* size);

int32_t fs_inited();

char* fs_read_file(const char* fname, int32_t *size);

#endif
