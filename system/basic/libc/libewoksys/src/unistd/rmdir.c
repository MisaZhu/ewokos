#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <ewoksys/vfs.h>
#include <ewoksys/devcmd.h>

int rmdir(const char *pathname) {
	fsinfo_t info;
	fsinfo_t *kids;
	uint32_t num = 0;

	if (pathname == NULL || pathname[0] == 0) {
		errno = ENOENT;
		return -1;
	}

	if (vfs_get_by_name(pathname, &info) != 0) {
		return -1;
	}
	if ((info.type & FS_TYPE_MASK) != FS_TYPE_DIR) {
		errno = ENOTDIR;
		return -1;
	}

	kids = vfs_kids(&info, &num);
	if (kids != NULL) {
		free(kids);
	}
	if (num != 0) {
		errno = ENOTEMPTY;
		return -1;
	}

	return dev_unlink(info.mount_pid, info.node, pathname);
}
