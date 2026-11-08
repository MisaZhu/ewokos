#include <ewoksys/shm.h>
#include <ewoksys/ipc.h>      /* IPC_DMA_PIN / IPC_DMA_UNPIN */
#include <sys/shm.h>          /* shmctl() */
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

int shm_dma_pin(int32_t shm_id) {
    return shmctl(shm_id, IPC_DMA_PIN, NULL) == 0 ? 0 : -1;
}
int shm_dma_unpin(int32_t shm_id) {
    return shmctl(shm_id, IPC_DMA_UNPIN, NULL) == 0 ? 0 : -1;
}

ewokos_addr_t shm_contig_phy_addr(int32_t shm_id, ewokos_addr_t vaddr) {
    return syscall2(SYS_SHM_CONTIG_PHY_ADDR, shm_id, vaddr);
}


#ifdef __cplusplus
}
#endif
