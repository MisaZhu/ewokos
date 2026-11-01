#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ewoksys/fsinfo.h>
#include <ewoksys/klog.h>
#include <ewoksys/proto.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>

/*
 * The boot RAMFS is mounted at / while init selects the persistent system
 * root.  Mounting the system root over / hides the RAMFS mount and all of the
 * device daemons started from it.  Keep the daemons alive, but attach their
 * mounts to the new namespace: the boot filesystem itself becomes /boot and
 * every device mount under /dev keeps its original path.
 */

static int remount(int pid, const char* path) {
    proto_t in, out;
    PF->init(&in)->adds(&in, path);
    PF->init(&out);
    int ret = dev_cntl_by_pid(pid, DEV_CMD_REMOUNT, &in, &out);
    PF->clear(&in);
    PF->clear(&out);
    return ret;
}

static int is_bootfs(const mount_t* mount) {
    return mount != NULL && strcmp(mount->org_name, "/") == 0 &&
            strncmp(mount->desc, "bootfs:", 7) == 0;
}

static int is_device_mount(const mount_t* mount) {
    return mount != NULL && strncmp(mount->org_name, "/dev/", 5) == 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if((int16_t)getuid() > 0) {
        klog("switchroot: requires root\n");
        return -1;
    }

    mount_t mounts[FS_MOUNT_MAX];
    int count = 0;
    for(int i = 0; i < FS_MOUNT_MAX; i++) {
        if(count >= FS_MOUNT_MAX)
            break;
        if(vfs_get_mount_by_id(i, &mounts[count]) == 0)
            count++;
    }

    int bootfs_pid = -1;
    for(int i = 0; i < count; i++) {
        if(is_bootfs(&mounts[i])) {
            bootfs_pid = mounts[i].pid;
            break;
        }
    }
    if(bootfs_pid < 0) {
        klog("switchroot: bootfs mount not found\n");
        return -1;
    }

    int failed = 0;
    for(int i = 0; i < count; i++) {
        if(!is_device_mount(&mounts[i]))
            continue;
        klog("switchroot: /dev mount pid=%d %s\n",
                mounts[i].pid, mounts[i].org_name);
        if(remount(mounts[i].pid, mounts[i].org_name) != 0) {
            klog("switchroot: device remount failed %s\n", mounts[i].org_name);
            failed = 1;
        }
    }

    klog("switchroot: bootfs pid=%d /boot\n", bootfs_pid);
    if(remount(bootfs_pid, "/boot") != 0) {
        klog("switchroot: bootfs remount failed\n");
        failed = 1;
    }
    return failed ? -1 : 0;
}
