#ifndef KERNEL_CAP_H
#define KERNEL_CAP_H

#include <cap.h>
#include <stdbool.h>
#include <stdint.h>

struct st_proc;

/*
 * Kernel-side capability operations. All checks resolve a thread to its
 * owner process (proc_get_proc) so a thread always acts with the authority
 * of the process it belongs to.
 */

/* sid generation: monotonic per-boot security id for identity checks. */
extern uint64_t cap_new_sid(void);

/* cnode primitives */
extern void    cap_cnode_init(cnode_t* cnode);
extern void    cap_cnode_copy(cnode_t* dst, const cnode_t* src);
extern int32_t cap_cnode_insert(cnode_t* cnode, const cap_t* cap); /* -> slot or -1 */
extern void    cap_cnode_revoke(cnode_t* cnode, uint32_t slot);

/*
 * Authority checks. Every one of them silently passes when the proc holds
 * CAP_ROOT; otherwise the proc needs a cap of the requested type carrying
 * (at least) the requested rights, object-scoped where applicable.
 */
extern bool proc_cap_has(struct st_proc* proc, uint32_t type, uint32_t rights);
extern bool proc_cap_check_frame(struct st_proc* proc, uint64_t paddr, uint64_t size, uint32_t rights);
extern bool proc_cap_check_irq(struct st_proc* proc, uint32_t irq, uint32_t rights);
extern bool proc_cap_check_proc(struct st_proc* proc, int32_t target_pid, uint32_t rights);
extern bool proc_cap_check_ep(struct st_proc* proc, int32_t serv_pid, uint32_t rights);

/*
 * Lifecycle hooks.
 *  - proc_cap_grant_root: install the kernel-issued root capset (a single
 *    full-rights CAP_ROOT cap). Used for kernel-created procs (init, idle)
 *    and for exec of a privileged (uid <= 0) image.
 *  - proc_cap_on_exec: reset the cnode according to the proc's current uid -
 *    privileged images get the root capset, user images start with an empty
 *    cnode. Mirrors the privilege reset POSIX exec implies.
 *  - proc_cap_on_setuid: dropping to a real user (uid > 0) revokes every
 *    cap, matching the old "uid > 0 fails all privileged syscalls" rule.
 */
extern void proc_cap_grant_root(struct st_proc* proc);
extern void proc_cap_on_exec(struct st_proc* proc);
extern void proc_cap_on_setuid(struct st_proc* proc, int32_t new_uid);

/*
 * Syscall-level entry points (SYS_CAP_* implementations), kept here so
 * svc.c stays a thin dispatcher.
 */
extern int32_t proc_cap_mint(uint32_t type, uint32_t rights, cap_mint_arg_t* uarg);
extern int32_t proc_cap_grant(int32_t target_pid, uint32_t src_slot, uint32_t rights_mask);
extern int32_t proc_cap_revoke(uint32_t slot);
extern int32_t proc_cap_get(int32_t slot, cap_info_t* uinfo);
extern int32_t proc_cap_policy_add(const cap_policy_add_t* uarg);

#endif
