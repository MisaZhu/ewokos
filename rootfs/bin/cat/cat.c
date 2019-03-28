#include <unistd.h>
#include <cmain.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <kstring.h>
#include <stdlib.h>

void _start() {
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg == NULL) {
		exit(0);
	}

	char fname[NAME_MAX];
	if(arg[0] == '/') {
		strcpy(fname, arg);
	}
	else {
		char pwd[NAME_MAX];
		getcwd(pwd, NAME_MAX);
		if(pwd[1] == 0) /*root*/
			snprintf(fname, NAME_MAX-1, "/%s", arg);
		else
			snprintf(fname, NAME_MAX-1, "%s/%s", pwd, arg);
	}

	int fd = open(fname, 0);
	if(fd < 0) {
		exit(0);
	}

	while(true) {
		char buf[32];
		int sz = read(fd, buf, 31);
		if(sz <= 0)
			break;
		buf[sz] = 0;
		printf("%s", buf);
	}
	close(fd);
	exit(0);
}
