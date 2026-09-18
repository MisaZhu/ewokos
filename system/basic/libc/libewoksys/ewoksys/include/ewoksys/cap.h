#ifndef EWOKSYS_CAP_H
#define EWOKSYS_CAP_H

#include <stdint.h>
#include <cap.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Userland wrappers for the EwokOS capability syscalls (see kernel cap.h).
 *
 * cap_mint:   create an object cap in the caller's own cnode (CAP_ROOT
 *             only). a/b are the type-specific object arguments documented
 *             in cap_mint_arg_t. Returns the new slot or -1.
 * cap_revoke: drop a cap from the caller's own cnode.
 * cap_grant:  forward one of the caller's caps to another proc (needs the
 *             CAP_GRANT right, or CAP_ROOT); rights_mask attenuates the
 *             forwarded rights (0 = forward as-is). Returns the target's
 *             slot or -1.
 * cap_get:    read back one of the caller's own caps (slot < 0 only fetches
 *             the caller's sid into info->sid).
 * cap_policy_add: install one exec-time policy entry into the kernel
 *             (CAP_ROOT only, used by /sbin/init). Whenever a proc execs an
 *             image whose cmd first token matches, the kernel builds the
 *             cap into its fresh cnode. a/b follow cap_mint_arg_t.
 *             Returns 0 or -1.
 */
int32_t cap_mint(uint32_t type, uint32_t rights, uint64_t a, uint64_t b);
int32_t cap_revoke(uint32_t slot);
int32_t cap_grant(int32_t target_pid, uint32_t src_slot, uint32_t rights_mask);
int32_t cap_get(int32_t slot, cap_info_t* info);
int32_t cap_policy_add(const char* cmd, uint32_t type, uint32_t rights, uint64_t a, uint64_t b);

#ifdef __cplusplus
}
#endif

#endif
