#include <ewoksys/shm.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

ewokos_addr_t shm_contig_phy_addr(int32_t shm_id, ewokos_addr_t vaddr) {
    return syscall2(SYS_SHM_CONTIG_PHY_ADDR, shm_id, vaddr);
}


#ifdef __cplusplus
}
#endif
