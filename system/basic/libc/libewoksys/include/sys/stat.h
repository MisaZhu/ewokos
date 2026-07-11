#ifndef SYS_STAT_H
#define SYS_STAT_H

#include <sys/types.h>

struct stat { 
	dev_t         st_dev;     /* ID of device containing file -文件所在设备的ID*/
	ino_t         st_ino;     /* inode number -inode节点号*/
	mode_t        st_mode;    /* protection -保护模式?*/
	nlink_t       st_nlink;   /* number of hard links -链向此文件的连接数(硬连接)*/
	uid_t         st_uid;     /* user ID of owner -user id*/
	gid_t         st_gid;     /* group ID of owner - group id*/
	dev_t         st_rdev;    /* device ID (if special file) -设备号，针对设备文件*/
	off_t         st_size;    /* total size, in bytes -文件大小，字节为单位*/
	blksize_t     st_blksize; /* blocksize for filesystem I/O -系统块的大小*/
	blkcnt_t      st_blocks;  /* number of blocks allocated -文件所占块数*/
	time_t        st_atime;   /* time of last access -最近存取时间*/
	time_t        st_mtime;   /* time of last modification -最近修改时间*/
	time_t        st_ctime;   /* time of last status change - */
};

#define F_OK    0
#define R_OK    4
#define W_OK    2
#define X_OK    1

int mkdir(const char* name, mode_t mode);
int stat(const char* name, struct stat* buf);
int lstat(const char* name, struct stat* buf);
int fstat(int fd, struct stat* buf);
int mkfifo(const char* name, mode_t mode);

#endif
