#include <unistd.h>
#include <errno.h>
#include <ewoksys/sys.h>
#include <ewoksys/proc.h>
#include <ewoksys/fsinfo.h>

long sysconf(int name) {
    sys_info_t info;
    int have_info = 0;

    switch (name) {
    case _SC_ARG_MAX:
        return 4096;
    case _SC_CHILD_MAX:
        return 128;
    case _SC_CLK_TCK:
        return 100;
    case _SC_NGROUPS_MAX:
        return 32;
    case _SC_OPEN_MAX:
        return MAX_OPEN_FILE_PER_PROC;
    case _SC_STREAM_MAX:
        return 128;
    case _SC_TZNAME_MAX:
        return 6;
    case _SC_JOB_CONTROL:
    case _SC_SAVED_IDS:
        return 1;
    case _SC_VERSION:
        return 200809L;
    case _SC_MONOTONIC_CLOCK:
        /* POSIX: the runtime answer for the monotonic-clock option is the
         * compile-time macro's value.  The clock itself is clock_gettime()'s
         * CLOCK_MONOTONIC, straight off the kernel tick counter. */
        return _POSIX_MONOTONIC_CLOCK;
    case _SC_PAGESIZE:
        return getpagesize();
    case _SC_THREAD_SAFE_FUNCTIONS:
    case _SC_THREADS:
    case _SC_REENTRANT:
        return 200809L;
    case _SC_GETPW_R_SIZE_MAX:
    case _SC_GETGR_R_SIZE_MAX:
        return 1024;
    case _SC_LOGIN_NAME_MAX:
        return 256;
    case _SC_TTY_NAME_MAX:
        return 64;
    case _SC_SYMLOOP_MAX:
        /* No symbolic links in the EwokOS VFS, so no resolution loop is
         * possible.  POSIX's minimum for SYMLOOP_MAX is 8; answering 8 is
         * correct and lets a caller's `for (i = 0; i < symloop; ++i)` loop
         * behave normally instead of erroring out. */
        return 8;
    case _SC_NPROCESSORS_CONF:
    case _SC_NPROCESSORS_ONLN:
    case _SC_PHYS_PAGES:
    case _SC_AVPHYS_PAGES:
        have_info = (sys_get_sys_info(&info) == 0);
        break;
    default:
        errno = EINVAL;
        return -1;
    }

    if (!have_info) {
        /* fall back to sane defaults when the kernel info is unavailable */
        if (name == _SC_NPROCESSORS_CONF || name == _SC_NPROCESSORS_ONLN)
            return 1;
        return 1024;
    }

    if (name == _SC_NPROCESSORS_CONF || name == _SC_NPROCESSORS_ONLN)
        return (long)(info.cores == 0 ? 1 : info.cores);

    if (info.page_size == 0)
        return 1024;
    return (long)(info.total_usable_mem_size / info.page_size);
}

/*
 * pathconf().  Sits in this file rather than a new one for the reasons given at
 * its declaration in include/unistd.h.
 *
 * `path` is accepted and then not consulted, which is honest rather than lazy:
 * NAME_MAX on EwokOS comes from the VFS node structure, not from a per-mount
 * setting, so every path in the system has the same answer and stat()ing the
 * argument would only add a failure mode for paths that do not exist yet -
 * which is exactly the case a file dialog asking "how long may the name I am
 * about to create be?" cares about.
 */
long pathconf(const char *path, int name) {
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    switch (name) {
    case _PC_NAME_MAX:
        /* FS_NODE_NAME_MAX sizes the d_name buffer in fsinfo_t and includes the
         * terminating NUL - it is documented as holding a null-terminated
         * filename - while POSIX defines NAME_MAX as the number of bytes the
         * implementation stores *excluding* the terminator.  Hence the -1.
         * Answering 64 here would let a caller build a 64-character name that
         * the VFS then refuses or truncates. */
        return FS_NODE_NAME_MAX - 1;
    case _PC_PATH_MAX:
        return PATH_MAX;
    default:
        errno = EINVAL;
        return -1;
    }
}
