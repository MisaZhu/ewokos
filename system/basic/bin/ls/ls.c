#include <unistd.h>
#include <stdio.h>
#include <vprintf.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <ewoksys/session.h>

static inline const char* get_show_name(const char* name, int32_t type) {
	static char ret[128];
	if(type == DT_DIR) 
		snprintf(ret, 127, "[%s]", name);
	else if(type == DT_BLK || type == DT_CHR) 
		snprintf(ret, 127, "*%s", name);
	else
		sstrncpy(ret, name, 127);
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

static inline const char* get_show_mode(uint32_t mode) {
	static char ret[16];
	snprintf(ret, 15, "%c%c%c%c%c%c%c%c%c",
		(mode & 0400) == 0 ? '-':'r',
		(mode & 0200) == 0 ? '-':'w',
		(mode & 0100) == 0 ? '-':'x',
		(mode & 040) == 0 ? '-':'r',
		(mode & 020) == 0 ? '-':'w',
		(mode & 010) == 0 ? '-':'x',
		(mode & 04) == 0 ? '-':'r',
		(mode & 02) == 0 ? '-':'w',
		(mode & 01) == 0 ? '-':'x');
	return ret;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	const char* r;
	char cwd[FS_FULL_NAME_MAX];
	char fname[FS_FULL_NAME_MAX];

	if(argc >= 2)
		r = argv[1];
	else 
		r = getcwd(cwd, FS_FULL_NAME_MAX);

	DIR* dirp = opendir(r);
	if(dirp == NULL)
		return -1;
	while(1) {
		struct dirent* it = readdir(dirp);
		if(it == NULL)
			break;

		if(strcmp(r, "/") == 0)
			snprintf(fname, FS_FULL_NAME_MAX, "/%s", it->d_name);
		else
			snprintf(fname, FS_FULL_NAME_MAX, "%s/%s", r, it->d_name);

		const char* show_name = get_show_name(it->d_name, it->d_type);
		struct stat st;
		stat(fname, &st);
		const char* show_mode = get_show_mode(st.st_mode);

		session_info_t info;
		session_get(st.st_uid, &info);

		printf("%-10s %8s:%d %8d  %-24s\n", show_mode, info.user, info.gid, it->d_reclen, show_name);
	}
	closedir(dirp);
	return 0;
}
