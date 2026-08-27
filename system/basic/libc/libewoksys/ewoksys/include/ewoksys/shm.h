#ifndef EWOK_SHM_H
#define EWOK_SHM_H

#include <stdint.h>
#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

ewokos_addr_t shm_contig_phy_addr(int32_t shm_id, ewokos_addr_t vaddr);

#ifdef __cplusplus
}
#endif
#endif
