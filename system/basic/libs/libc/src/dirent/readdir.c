#include <dirent.h>
#include <stddef.h>
#include <string.h>

struct dirent* readdir(DIR *dirp) {
	if(dirp == NULL || dirp->offset >= dirp->num)
		return NULL;
	static struct dirent ret;
	uint32_t i = dirp->offset;
	ret.d_ino = dirp->kids[i].node->data;
	ret.d_off = dirp->offset;
	ret.d_type = dirp->kids[i].node->type;
	ret.d_reclen = dirp->kids[i].node->size;
	strncpy(ret.d_name, dirp->kids[i].node->name, FS_NODE_NAME_MAX-1);

	dirp->offset++;
	return &ret;
}

