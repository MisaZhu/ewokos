#include <stddef.h>
#include <unistd.h>
#include <sys/errno.h>
#include <ewoksys/syscall.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/core.h>
#include <ewoksys/vfs.h>

int chdir(const char* path) {
    proto_t in, out;
    /* cored resolves the name from the vfs root, so a relative path has to
     * be expanded against the current cwd first (git -C, sh -c, ...). */
    char fullname[FS_FULL_NAME_MAX+1];
    if(path != NULL && path[0] != 0 && path[0] != '/') {
        vfs_fullname(path, fullname, FS_FULL_NAME_MAX);
        path = fullname;
    }
    PF->init(&out);
    PF->init(&in)->adds(&in, path);
    int res = ipc_call(get_cored_pid(), CORE_CMD_SET_CWD, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        res = proto_read_int(&out);
        if(res != 0)
            errno = proto_read_int(&out);
    }
    PF->clear(&out);
    return res;
}

