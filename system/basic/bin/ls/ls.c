#include <unistd.h>
#include <stdio.h>
#include <vprintf.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static inline const char* get_show_name(const char* name, int32_t type) {
	static char ret[128];
	if(type == DT_DIR) 
		snprintf(ret, 127, "[%s]", name);
	else if(type == DT_BLK || type == DT_CHR) 
		snprintf(ret, 127, "*%s", name);
	else
		strncpy(ret, name, 127);
	return ret;
}

static inline const char* get_show_type(int32_t type) {
	if(type == DT_BLK)
		return "b";
	else if(type == DT_CHR)
		return "c";
	else if(type == DT_DIR)
		return "r";
	return "f";
}

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
		const char* show_name = get_show_name(it->d_name, it->d_type);
		const char* show_type = get_show_type(it->d_type);

		if(it->d_reclen >= 1024)
			printf("  %24s  %4s %dK\n", show_name, show_type, sz);
		else
			printf("  %24s  %4s %d\n", show_name, show_type, it->d_reclen);
	}
	closedir(dirp);
	return 0;
}
