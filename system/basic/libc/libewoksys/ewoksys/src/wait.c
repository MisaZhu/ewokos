#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif


int ewok_waitpid(int pid) {
    int res;
    /*
     * The kernel parks the caller while the child is alive and returns
     * SYS_WAITPID_AGAIN on any wake; the exit is only observed by the
     * re-check at the next entry (zombie -> exit code, gone -> -1). Stray
     * edges just cost one extra round trip instead of ending the wait early.
     */
    do {
        res = syscall1(SYS_WAIT_PID, (ewokos_addr_t)pid);
    } while(res == SYS_WAITPID_AGAIN);
    return res;
}

#ifdef __cplusplus
}
#endif
