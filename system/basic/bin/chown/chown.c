#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ewoksys/vfs.h>
#include <ewoksys/session.h>

int main(int argc, char* argv[]) {
	const char* fname;
	if(argc < 3) {
		printf("Usage: chown <user> <fname>\n");
		return -1;
	}

	char c = argv[1][0];
	int uid = 0;
	session_info_t sinfo;

	if(c >= '0' && c <= '9') {
		uid = atoi(argv[1]);
		if(session_get_by_uid(uid, &sinfo) != 0) {
			printf("UID [%d] not exist!\n", uid);
			return -1;
		}
	}
	else {
		if(session_get_by_name(argv[1], &sinfo) != 0) {
			printf("User [%s] not exist!\n", argv[1]);
			return -1;
		}
		uid = sinfo.uid;
	}	

	int gid = uid;
	fname = vfs_fullname(argv[2]);
	if(chown(fname, uid, gid) != 0) {
		printf("Can't chown [%s]!\n", fname);
		return -1;
	}
	return 0;
}
