#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vfs.h>

DIR* opendir(const char* name) {
	fsinfo_t info;
	if(vfs_get(name, &info) != 0)
		return NULL;
	
	uint32_t num = 0;
	fsinfo_t* kids = vfs_kids(&info, &num);
	if(kids == NULL || num == 0)
		return NULL;

	DIR* ret = (DIR*)malloc(sizeof(DIR));
	if(ret == NULL) {
		free(kids);
		return NULL;
	}

	ret->kids = kids;
	ret->num = num;
	ret->offset = 0;
	return ret;
}

int closedir(DIR* dirp) {
	if(dirp == NULL)
		return -1;
	if(dirp->kids != NULL)
		free(dirp->kids);
	free(dirp);
	return 0;
}

struct dirent* readdir(DIR *dirp) {
	if(dirp == NULL || dirp->offset >= dirp->num)
		return NULL;
	static struct dirent ret;
	uint32_t i = dirp->offset;
	ret.d_ino = dirp->kids[i].data;
	ret.d_off = dirp->offset;
	ret.d_type = dirp->kids[i].type;
	ret.d_reclen = dirp->kids[i].size;
	strncpy(ret.d_name, dirp->kids[i].name, FS_NODE_NAME_MAX-1);

	dirp->offset++;
	return &ret;
}

void rewinddir(DIR* dirp) {
	if(dirp == NULL)
		return;
	dirp->offset = 0;
}

void seekdir(DIR* dirp, uint32_t loc) {
	if(dirp == NULL || loc >= dirp->num)
		return;
	dirp->offset = loc;
}

int telldir(DIR* dirp) {
	if(dirp == NULL)
		return -1;
	return (int)dirp->offset;
}

