#include <unistd.h>
#include <ewoksys/vfs.h>

int access(const char* fname, int mode) {
	fsinfo_t info;
	if(vfs_get_by_name(fname, &info) != 0)
		return -1;

	return vfs_check_access(getpid(), &info, mode);	
}
