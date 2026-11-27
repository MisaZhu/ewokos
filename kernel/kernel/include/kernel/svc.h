#ifndef SVC_H
#define SVC_H

#include <kernel/context.h>
#include <syscalls.h>
#include <ewokos_config.h>

extern void svc_handler(int32_t code, ewokos_addr_t arg0, ewokos_addr_t arg1, ewokos_addr_t arg2, context_t* ctx);

/* Reclaim raw physically-contiguous runs (SYS_ALLOC_PHY_CONTIG) owned by a
 * dying process; called from proc_funeral's TASK_TYPE_PROC branch alongside
 * dma_release()/shm_release_orphans(). Kernel lock held. */
extern void phy_contig_release_owner(int32_t pid);

#endif
