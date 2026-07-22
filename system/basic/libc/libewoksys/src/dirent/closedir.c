#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int closedir(DIR* dirp) {
	if(dirp == NULL) {
		errno = EBADF;
		return -1;
	}
	if(dirp->kids != NULL)
		free(dirp->kids);
	free(dirp);
	return 0;
}
