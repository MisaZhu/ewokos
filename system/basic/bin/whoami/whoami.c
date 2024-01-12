#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ewoksys/session.h>

int main(int argc, char* argv[]) {
	int uid = getuid();
	session_info_t sinfo;

	if(session_get_by_uid(uid, &sinfo) != 0) {
		printf("UID [%d] not exist!\n", uid);
		return -1;
	}

	printf("%s\n", sinfo.user);
	return 0;
}
