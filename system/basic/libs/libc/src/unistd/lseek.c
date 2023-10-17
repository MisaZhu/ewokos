#include <unistd.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/fsinfo.h>

int lseek(int fd, uint32_t offset, int whence) {
	if(whence == SEEK_CUR) {
		int cur = vfs_tell(fd);
		if(cur < 0)
			cur = 0;
		offset += cur;
	}	
	else if(whence == SEEK_END) {
		fsinfo_t info;
		int cur = 0;
		if(vfs_get_by_fd(fd, &info) == 0)
			cur = info.size;
		offset += cur;
	}
	return vfs_seek(fd, offset);
}

