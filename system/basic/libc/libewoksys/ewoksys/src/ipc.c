#include <ewoksys/ipc.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

    static inline int ipc_ping(int pid)
    {
        return syscall1(SYS_IPC_PING, (int32_t)pid);
    }

    inline void ipc_ready(void)
    {
        syscall0(SYS_IPC_READY);
    }

    int ipc_wait_ready(int pid)
    {
        procinfo_t info;

        while (1)
        {
            if (ipc_ping(pid) == 0)
                return 0;
            if (proc_info(pid, &info) != 0 || info.state == UNUSED || info.state == ZOMBIE)
                return -1;
            proc_usleep(10000);
        }
    }

    int ipc_disable(void)
    {
        while (true)
        {
            int res = syscall0(SYS_IPC_DISABLE);
            if (res == 0)
                break;
            sleep(0);
        }
        return 0;
    }

    void ipc_enable(void)
    {
        syscall0(SYS_IPC_ENABLE);
    }

    inline int ipc_call(int to_pid, int call_id, const proto_t *ipkg, proto_t *opkg)
    {
        if (to_pid < 0)
            return -1;

        int ipc_id = 0;
        while (true) {
            if (opkg == NULL)
                call_id |= IPC_NON_RETURN;
            int res = syscall3(SYS_IPC_CALL, (ewokos_addr_t)to_pid, (ewokos_addr_t)call_id, (ewokos_addr_t)ipkg);

            if (res < 0) {
                if (res == IPC_ERROR_RETRY)
                    continue;
                return res;
            }
            ipc_id = res;
            break;
        }

    if (opkg == NULL) {
            return 0;
    }

        int res = -1;
        PF->clear(opkg);
        while (true) {
            res = syscall3(SYS_IPC_GET_RETURN, (ewokos_addr_t)to_pid, (ewokos_addr_t)ipc_id, (ewokos_addr_t)opkg);
            if(res == 0)
                break;

            if (res == -1) // retry
                continue;

            if (res < 0) // error!
                return -1;

            if (res > 0) //opkg not big enough, must resize it.
                PF->reserve(opkg, res);

            res = syscall3(SYS_IPC_GET_RETURN, (ewokos_addr_t)to_pid, (ewokos_addr_t)ipc_id, (ewokos_addr_t)opkg);
            break;
        }
        return res;
    }

    inline int ipc_call_wait(int to_pid, int call_id, const proto_t *ipkg)
    {
        proto_t out;
        PF->init(&out);
        int res = ipc_call(to_pid, call_id, ipkg, &out);
        PF->clear(&out);
        return res;
    }

#ifdef __cplusplus
}
#endif
