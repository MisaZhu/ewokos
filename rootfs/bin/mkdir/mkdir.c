#include <unistd.h>
#include <stdio.h>
#include <cmain.h>
#include <vfs/fs.h>

int main() {
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();

	if(arg != NULL) {
		char cwd[NAME_MAX];
		int fd = fs_open(getcwd(cwd, NAME_MAX), 0);
		if(fd >= 0) {
			fs_add(fd, arg);
			fs_close(fd);
		}
	}
	return 0;
}
