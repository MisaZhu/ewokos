#include <unistd.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <ewoksys/session.h>

static inline const char* get_show_name(const char* name, int32_t type) {
	static char ret[128] = {0};
	if(type == DT_DIR) 
		snprintf(ret, 127, "[%s]", name);
	else if(type == DT_BLK || type == DT_CHR) 
		snprintf(ret, 127, "*%s", name);
	else
		strncpy(ret, name, 127);
	return ret;
}

static inline char get_show_type(int32_t type) {
	if(type == DT_BLK)
		return 'b';
	else if(type == DT_FIFO)
		return 'p';
	else if(type == DT_CHR)
		return 'c';
	else if(type == DT_DIR)
		return 'd';
	return '-';
}

static inline const char* get_show_mode(uint32_t mode, int32_t type) {
	static char ret[16];
	snprintf(ret, 15, "%c%c%c%c%c%c%c%c%c%c",
		get_show_type(type),
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

static uint32_t get_ksize(uint32_t sz) {
	if(sz == 0)
		return 0;
	uint32_t ret = sz/1024;
	if((sz % 1024) != 0)
		ret += 1;
	return ret;
}

static int _list_mode = 0;

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "l");
		if(c == -1)
			break;

		switch (c) {
		case 'l':
			_list_mode = 1;
			break;
		case '?':
			return -1;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	const char* r;
	char cwd[FS_FULL_NAME_MAX];
	char fname[FS_FULL_NAME_MAX];

	int argind = doargs(argc, argv);

	if(argind < argc)
		r = argv[argind];
	else
		r = getcwd(cwd, FS_FULL_NAME_MAX);

	DIR* dirp = opendir(r);
	if(dirp == NULL)
		return -1;
	while(1) {
		struct dirent* it = readdir(dirp);
		if(it == NULL) {
			break;
		}

		if(strcmp(r, "/") == 0)
			snprintf(fname, FS_FULL_NAME_MAX, "/%s", it->d_name);
		else
			snprintf(fname, FS_FULL_NAME_MAX, "%s/%s", r, it->d_name);

		struct stat st;
		stat(fname, &st);

		const char* show_name = get_show_name(it->d_name, it->d_type);
		const char* show_mode = get_show_mode(st.st_mode, it->d_type);

		session_info_t info, infog;
		if(session_get_by_uid(st.st_uid, &info) != 0)
			sprintf(info.user, "%d", st.st_uid);
		if(session_get_by_uid(st.st_gid, &infog) != 0)
			sprintf(infog.user, "%d", st.st_gid);

		if(_list_mode == 0)
			printf("%6dk %s\n", get_ksize(it->d_reclen), show_name);
		else
			printf("%-10s %-6s %-6s %6dk %s\n",
					show_mode,
					info.user,
					infog.user,
					get_ksize(it->d_reclen),
					show_name);
	}
	closedir(dirp);
	return 0;
}
