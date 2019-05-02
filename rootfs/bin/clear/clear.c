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
	int32_t cmd = 0; //clear console
	fs_ctrl(fd, cmd, NULL, 0, NULL, 0);
	return 0;
}
