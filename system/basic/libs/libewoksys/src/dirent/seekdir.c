#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>

void seekdir(DIR* dirp, uint32_t loc) {
	if(dirp == NULL || loc >= dirp->num)
		return;
	dirp->offset = loc;
}

