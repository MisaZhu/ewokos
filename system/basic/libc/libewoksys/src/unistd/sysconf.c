#include <unistd.h>
#include <errno.h>
#include <ewoksys/sys.h>
#include <ewoksys/proc.h>

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
