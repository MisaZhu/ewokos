#ifndef DIRENT_H
#define DIRENT_H

#include <sys/fsinfo.h>

typedef struct {
	uint32_t  num;    //kids num
	fsinfo_t* kids;	  //kids info
	uint32_t  offset; //read offset
} DIR;

#define	DT_DIR     FS_TYPE_DIR
#define DT_BLK     FS_TYPE_BLOCK
#define DT_CHR     FS_TYPE_CHAR
#define DT_FIFO    FS_TYPE_PIPE
#define DT_LNK     FS_TYPE_LINK
#define DT_REG     FS_TYPE_FILE
//#define DT_SOCK  FS_TYPE_SOCKET
#define DT_UNKNOWN FS_TYPE_UNKNOWN

struct dirent {
	uint32_t  d_ino;       /* Inode number */
	uint32_t  d_off;       /* Not an offset; see below */
	uint16_t  d_reclen;    /* Length of this record */
	uint8_t   d_type;      /* Type of file; not supported by all filesystem types */
	char      d_name[FS_NODE_NAME_MAX]; /* Null-terminated filename */
};

DIR*           opendir(const char* name);
int            closedir(DIR* dirp);
int            telldir(DIR* dirp);
struct dirent* readdir(DIR *dirp);
void           rewinddir(DIR* dirp);
void           seekdir(DIR* dirp, uint32_t loc);

#endif
