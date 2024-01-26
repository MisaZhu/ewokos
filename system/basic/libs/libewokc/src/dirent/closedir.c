#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int closedir(DIR* dirp) {
	if(dirp == NULL)
		return -1;
	if(dirp->kids != NULL)
		free(dirp->kids);
	free(dirp);
	return 0;
}

