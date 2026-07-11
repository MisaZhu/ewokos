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

#ifndef S_IFMT
#define S_IFMT   0170000
#endif
#ifndef S_IFSOCK
#define S_IFSOCK 0140000
#endif
#ifndef S_IFLNK
#define S_IFLNK  0120000
#endif
#ifndef S_IFREG
#define S_IFREG  0100000
#endif
#ifndef S_IFBLK
#define S_IFBLK  0060000
#endif
#ifndef S_IFDIR
#define S_IFDIR  0040000
#endif
#ifndef S_IFCHR
#define S_IFCHR  0020000
#endif
#ifndef S_IFIFO
#define S_IFIFO  0010000
#endif

#ifndef S_ISUID
#define S_ISUID  0004000
#endif
#ifndef S_ISGID
#define S_ISGID  0002000
#endif
#ifndef S_ISVTX
#define S_ISVTX  0001000
#endif
#ifndef S_IRUSR
#define S_IRUSR  0000400
#endif
#ifndef S_IWUSR
#define S_IWUSR  0000200
#endif
#ifndef S_IXUSR
#define S_IXUSR  0000100
#endif
#ifndef S_IRGRP
#define S_IRGRP  0000040
#endif
#ifndef S_IWGRP
#define S_IWGRP  0000020
#endif
#ifndef S_IXGRP
#define S_IXGRP  0000010
#endif
#ifndef S_IROTH
#define S_IROTH  0000004
#endif
#ifndef S_IWOTH
#define S_IWOTH  0000002
#endif
#ifndef S_IXOTH
#define S_IXOTH  0000001
#endif

#ifndef S_IRWXU
#define S_IRWXU  (S_IRUSR | S_IWUSR | S_IXUSR)
#endif
#ifndef S_IRWXG
#define S_IRWXG  (S_IRGRP | S_IWGRP | S_IXGRP)
#endif
#ifndef S_IRWXO
#define S_IRWXO  (S_IROTH | S_IWOTH | S_IXOTH)
#endif

#ifndef S_ISREG
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISBLK
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif
#ifndef S_ISLNK
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif

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
