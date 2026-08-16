#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vfs.h>
#include <ewoksys/fsinfo.h>

static const char* get_cmd_name(char* cmd) {
    char* p = cmd;
    while(*p != 0) {
        if(*p == ' ') {
            *p = 0;
            break;
        }
        p++;
    }
    return cmd;
}

typedef struct {
    uint32_t used_bytes;
    uint32_t iused;
} usage_t;

/*
 * Recursively walk the filesystem tree rooted at `info` and accumulate used
 * bytes (regular files only) and inode count (every node). Nodes belonging to
 * a nested mount (different mount_pid) are skipped so each mount is accounted
 * for exactly once by its own df entry.
 */
static void walk_usage(fsinfo_t* info, int32_t mount_pid, int depth, usage_t* out) {
    if(info == NULL || out == NULL || depth > 64)
        return;

    out->iused++; /* every node consumes one inode */

    if(!FS_IS_TYPE(info->type, FS_TYPE_DIR)) {
        if(!FS_IS_TYPE(info->type, FS_TYPE_LINK) &&
                !FS_IS_TYPE(info->type, FS_TYPE_PIPE))
            out->used_bytes += info->stat.size;
        return;
    }

    uint32_t num = 0;
    fsinfo_t* kids = vfs_kids(info, &num);
    if(kids == NULL)
        return;

    for(uint32_t i = 0; i < num; i++) {
        if((int32_t)kids[i].mount_pid != mount_pid)
            continue;
        walk_usage(&kids[i], mount_pid, depth + 1, out);
    }
    free(kids);
}

/*
 * Format a byte count into a human-readable string (KB/MB/GB), following
 * the convention of get_mem_size_desc(): binary units, rounded up.
 */
static const char* size_desc(uint32_t bytes, char* ret) {
    if(bytes == 0) {
        snprintf(ret, 31, "0");
        return ret;
    }

    const uint32_t k = 1024;
    const uint32_t m = 1024 * 1024;
    const uint32_t g = 1024 * 1024 * 1024;

    if(bytes >= g) {
        uint32_t v = bytes / g;
        if(bytes % g) v++;
        snprintf(ret, 31, "%uG", v);
    }
    else if(bytes >= m) {
        uint32_t v = bytes / m;
        if(bytes % m) v++;
        snprintf(ret, 31, "%uM", v);
    }
    else {
        uint32_t v = bytes / k;
        if(bytes % k) v++;
        snprintf(ret, 31, "%uK", v);
    }
    return ret;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("%-24s %8s %8s  %s\n",
            "Filesystem", "Used", "iused", "Mounted on");

    int32_t i;
    for(i = 0; i < FS_MOUNT_MAX; i++) {
        mount_t mnt;
        if(vfs_get_mount_by_id(i, &mnt) != 0)
            continue;

        fsinfo_t root_info;
        if(vfs_get_by_name(mnt.org_name, &root_info) != 0)
            continue;

        /* only report filesystem mounts; skip char/block device nodes */
        if(!FS_IS_TYPE(root_info.type, FS_TYPE_DIR))
            continue;

        char cmd[128] = {0};
        syscall3(SYS_PROC_GET_CMD, (ewokos_addr_t)mnt.pid,
                (ewokos_addr_t)cmd, 127);
        const char* fs = get_cmd_name(cmd);
        if(fs[0] == 0)
            fs = "unknown";

        usage_t u = {0, 0};
        walk_usage(&root_info, mnt.pid, 0, &u);

        char used_str[32] = {0};
        size_desc(u.used_bytes, used_str);

        printf("%-24s %8s %8u  %s\n",
                fs, used_str, u.iused, mnt.org_name);
    }
    return 0;
}
