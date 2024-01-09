#ifndef FCTRL_H
#define FCTRL_H

#include <stdint.h>

#define O_RDONLY     0x0
#define O_WRONLY     0x2
#define O_RDWR       0x3

#define O_CREAT      0x4
#define O_NONBLOCK   0x8
#define O_TRUNC      0x10

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

enum {
	F_GETFL = 0,
	F_SETFL
};

#ifdef __cplusplus
extern "C" {
#endif

struct stat { 
	int32_t       st_dev;     /* ID of device containing file -文件所在设备的ID*/  
	int32_t       st_ino;     /* inode number -inode节点号*/    
	uint32_t      st_mode;    /* protection -保护模式?*/    
	uint32_t      st_nlink;   /* number of hard links -链向此文件的连接数(硬连接)*/    
	int32_t       st_uid;     /* user ID of owner -user id*/    
	int32_t       st_gid;     /* group ID of owner - group id*/    
	int32_t       st_rdev;    /* device ID (if special file) -设备号，针对设备文件*/    
	int32_t       st_size;    /* total size, in bytes -文件大小，字节为单位*/    
	uint32_t      st_blksize; /* blocksize for filesystem I/O -系统块的大小*/    
	uint32_t      st_blocks;  /* number of blocks allocated -文件所占块数*/    
	uint32_t      st_atime;   /* time of last access -最近存取时间*/    
	uint32_t      st_mtime;   /* time of last modification -最近修改时间*/    
	uint32_t      st_ctime;   /* time of last status change - */    
};  

int  open(const char* name, int oflag);
int  stat(const char* name, struct stat* buf);
void close(int fd);
int  fcntl(int fd, int cmd, int data);
int  chmod(const char *pathname, int mode);


#ifdef __cplusplus
}
#endif

#endif
