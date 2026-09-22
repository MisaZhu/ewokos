#include <stdio.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/ipc.h>

/*
 * Ask the filesystem/device daemon that owns <mnt_point> to release the mount.
 * Every umount goes through FS_CMD_DEV_CNTL/DEV_CMD_UMOUNT: a driver with a
 * dev_cntl handler (e.g. fat32fsd) finalizes it itself (flush + detach), and
 * the vdevice framework provides a default detach+stop for drivers without
 * one, so this single call covers all mounts. The daemon reports busy while
 * files are still open.
 */
int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("usage: umount <mnt_point>\n");
        return -1;
    }
    const char* path = argv[1];

    for(int32_t i = 0; i < FS_MOUNT_MAX; i++) {
        mount_t mnt;
        if(vfs_get_mount_by_id(i, &mnt) != 0)
            continue;
        if(strcmp(mnt.org_name, path) != 0)
            continue;

        proto_t out;
        PF->init(&out);
        int res = dev_cntl_by_pid(mnt.pid, DEV_CMD_UMOUNT, NULL, &out);
        PF->clear(&out);

        if(res == 0) {
            printf("%s: unmounted\n", path);
            return 0;
        }
        printf("%s: busy or daemon not responding\n", path);
        return -1;
    }

    printf("%s: not mounted\n", path);
    return -1;
}
