#ifndef FCTRL_H
#define FCTRL_H

#define O_RDONLY 0
#define O_WRONLY 2
#define O_RDWR   3

#define O_CREAT  8

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#include <proto.h>

enum {
	CNTL_NONE = 0,
	CNTL_INFO
}; //cntl command

int  fcntl_raw(int fd, int cmd, proto_t* in, proto_t* out);

int  open(const char* name, int oflag);
void close(int fd);
int  dma(int fd, int* size);
void flush(int fd);

int dev_ping(int pid);

#endif
