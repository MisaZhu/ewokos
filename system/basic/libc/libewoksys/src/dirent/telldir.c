#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

long telldir(DIR* dirp) {
	if(dirp == NULL) {
		errno = EBADF;
		return -1;
	}
	return (long)dirp->offset;
}
