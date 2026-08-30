#include <sys/stat.h>
#include <ewoksys/vfs.h>

/*
 * Named pipes (FIFOs) are no longer functional: the in-vfsd pipe backend was
 * removed when anonymous pipes moved to the standalone pipe driver
 * (system/basic/drivers/piped), which only serves pipe()/pipe2() fds. The
 * call still creates an FS_TYPE_PIPE node so paths resolve, but open() on it
 * has no working data backend. Kept for source compatibility.
 */
int mkfifo(const char* name, mode_t mode) {
    return vfs_create(name, NULL, FS_TYPE_PIPE, mode, true, true);
}
