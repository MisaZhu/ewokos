#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>

void seekdir(DIR* dirp, long loc) {
	if(dirp == NULL || loc < 0 || (uint32_t)loc >= dirp->num)
		return;
	dirp->offset = (uint32_t)loc;
}
