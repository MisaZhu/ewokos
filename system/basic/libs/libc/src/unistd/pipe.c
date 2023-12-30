#include <unistd.h>
#include <ewoksys/vfs.h>

int pipe(int fds[2]) {
	return vfs_open_pipe(fds);
}

