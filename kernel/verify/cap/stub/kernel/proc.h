#ifndef VERIFY_CAP_PROC_H
#define VERIFY_CAP_PROC_H

/*
 * Verification stub for <kernel/proc.h>.
 *
 * It reproduces ONLY the surface cap.c actually touches: the proc_t fields it
 * reads/writes and the four kernel entry points it calls. Everything else in
 * the real proc.h (scheduler, mm, ipc, queues) is irrelevant to the capability
 * logic and would drag the whole kernel into the model. The real ABI header
 * <cap.h> (cap_t / cnode_t / CNODE_SLOTS / cap_*_arg_t) is used verbatim, so
 * the proofs run against the true on-disk capability layout.
 */

#include <cap.h>          /* real ABI structs - resolved via -I../../kernel/include */
#include <stdbool.h>
#include <stdint.h>

/* ewokos_addr_t is uint64_t on aarch64/x86_64, uint32_t on arm32; the proof
   models the 64-bit target (the widest, so wrap-around is exercised). */
typedef uint64_t ewokos_addr_t;

typedef struct {
    uint32_t uuid;
    int32_t  uid;
    char     cmd[CAP_POLICY_CMD_MAX];
} verify_procinfo_t;

typedef struct st_proc {
    verify_procinfo_t info;
    cnode_t           cnode;
    uint64_t          sid;
} proc_t;

/* kernel entry points cap.c calls - modelled in cap_proofs.c */
extern proc_t* proc_get_proc(proc_t* proc);
extern proc_t* proc_get(int32_t pid);
extern proc_t* get_current_proc(void);
extern bool    user_ptr_ok(proc_t* proc, ewokos_addr_t ptr, ewokos_addr_t size);

#endif
