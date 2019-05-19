#include <unistd.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <stdlib.h>
#include <kstring.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int32_t fd = _stdout;
	if(fd < 0)
		return -1;
	fs_ctrl(fd, FS_CTRL_CLEAR, NULL, NULL);
	return 0;
}
