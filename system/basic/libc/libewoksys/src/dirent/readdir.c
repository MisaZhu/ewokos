#include <dirent.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ewoksys/fsinfo.h>

struct dirent* readdir(DIR *dirp) {
	if(dirp == NULL) {
		errno = EBADF;
		return NULL;
	}
	if(dirp->kids == NULL || dirp->offset >= dirp->num)
		return NULL;

	uint32_t i = dirp->offset;
	memset(&dirp->entry, 0, sizeof(dirp->entry));
	dirp->entry.d_ino = dirp->kids[i].data;
	dirp->entry.d_off = (off_t)dirp->offset;
	dirp->entry.d_type = dirp->kids[i].type & FS_TYPE_MASK;
	dirp->entry.d_reclen = (uint16_t)sizeof(struct dirent);
	strncpy(dirp->entry.d_name, dirp->kids[i].name, FS_NODE_NAME_MAX-1);
	dirp->entry.d_name[FS_NODE_NAME_MAX-1] = '\0';
	dirp->offset++;
	return &dirp->entry;
}
