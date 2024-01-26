#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>

int telldir(DIR* dirp) {
	if(dirp == NULL)
		return -1;
	return (int)dirp->offset;
}

