#ifndef SYS_STAT_H
#define SYS_STAT_H

#include <stdint.h>

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

#define F_OK    0
#define R_OK    4
#define W_OK    2
#define X_OK    1

int mkdir(const char* name, int mode);
int stat(const char* name, struct stat* buf);
int fstat(int fd, struct stat* buf);
int mkfifo(const char* name, int mode);

#endif