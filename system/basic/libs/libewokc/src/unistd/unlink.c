#include <unistd.h>
#include <ewoksys/vfs.h>
#include <ewoksys/devcmd.h>

int unlink(const char* fname) {
	fsinfo_t info;
	if(vfs_get_by_name(fname, &info) != 0)
		return -1;
	if(info.type != FS_TYPE_FILE) 
		return -1;
	return dev_unlink(info.mount_pid, &info, fname);
}
