#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>

void rewinddir(DIR* dirp) {
	if(dirp == NULL)
		return;
	dirp->offset = 0;
}

