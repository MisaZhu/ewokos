#include <sys/ipc.h>
#include <ewoksys/vfs.h>

key_t ftok(const char* fname, int proj_id) {
	fsinfo_t info;
	if(vfs_get_by_name(fname, &info) != 0)
		return -1;
	return (info.node << 8) | (proj_id & 0xff);
}