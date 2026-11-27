#include <sys/shm.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Supported cmds (enforced in the kernel):
   - IPC_RMID: destroy the segment only when no process has it attached
     (refs == 0), so an unattached leftover from a failed creation path
     can be backed out without touching anyone.
   - IPC_DMA_PIN / IPC_DMA_UNPIN: hold/release the physical run while a
     device may still reference it. */
int shmctl(int shmid, int cmd, void* buf) {
    (void)buf;
    return syscall2(SYS_PROC_SHM_CTRL, (ewokos_addr_t)shmid, (ewokos_addr_t)cmd);
}

#ifdef __cplusplus
}
#endif
