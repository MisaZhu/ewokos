#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ewoksys/vfs.h>
#include <ewoksys/session.h>

static int parse_group(const char* spec, int* gid) {
	session_group_t ginfo;
	char c = spec[0];

	if(c >= '0' && c <= '9') {
		*gid = atoi(spec);
		return 0;
	}

	if(session_get_group_by_name(spec, &ginfo) != 0) {
		printf("Group [%s] not exist!\n", spec);
		return -1;
	}
	*gid = ginfo.gid;
	return 0;
}

int main(int argc, char* argv[]) {
	if(argc < 3) {
		printf("Usage: chgrp <group> <fname>\n");
		return -1;
	}

	int gid = 0;
	char fullname[FS_FULL_NAME_MAX+1] = {0};
	struct stat st;

	if(parse_group(argv[1], &gid) != 0)
		return -1;

	vfs_fullname(argv[2], fullname, FS_FULL_NAME_MAX);
	if(stat(fullname, &st) != 0) {
		printf("Can't stat [%s]!\n", fullname);
		return -1;
	}

	if(chown(fullname, st.st_uid, gid) != 0) {
		printf("Can't chgrp [%s]!\n", fullname);
		return -1;
	}
	return 0;
}
