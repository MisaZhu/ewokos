#include <unistd.h>
#include <sys/vfs.h>
#include <sys/ipc.h>

int unlink(const char* fname) {
	fsinfo_t info;
	if(vfs_get(fname, &info) != 0)
		return -1;
	if(info.type != FS_TYPE_FILE) 
		return -1;
	
	/*mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;
		*/
	
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		add(&in, &info, sizeof(fsinfo_t))->
		adds(&in, fname);

	ipc_call(info.mount_pid, FS_CMD_UNLINK, &in, &out);
	PF->clear(&in);
	int res = proto_read_int(&out);
	PF->clear(&out);
	if(res == 0)
		return vfs_del(&info);
	return -1;
}
