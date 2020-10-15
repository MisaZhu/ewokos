#include <unistd.h>
#include <stdio.h>
#include <vprintf.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	const char* r;
	char cwd[FS_FULL_NAME_MAX];

	if(argc >= 2)
		r = argv[1];
	else 
		r = getcwd(cwd, FS_FULL_NAME_MAX);

	DIR* dirp = opendir(r);
	if(dirp == NULL)
		return -1;
	printf("  NAME                     TYPE  SIZE\n");
	while(1) {
		struct dirent* it = readdir(dirp);
		if(it == NULL)
			break;

		int sz = it->d_reclen / 1024;
		if(it->d_reclen != 0 && sz == 0)
			sz++;

		if(it->d_type == DT_REG)
			printf("  %24s  f    %dK\n", it->d_name, sz);
		else if(it->d_type == DT_DIR)
			printf("  %24s  r    %d\n", it->d_name, it->d_reclen);
		else if(it->d_type == DT_BLK || it->d_type == DT_CHR)
			printf("  %24s  d    %d\n", it->d_name, it->d_reclen);
	}
	closedir(dirp);
	return 0;
}
