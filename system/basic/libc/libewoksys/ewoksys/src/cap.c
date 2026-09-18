#include <ewoksys/cap.h>
#include <ewoksys/syscall.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t cap_mint(uint32_t type, uint32_t rights, uint64_t a, uint64_t b) {
    cap_mint_arg_t arg;
    arg.a = a;
    arg.b = b;
    return (int32_t)syscall3(SYS_CAP_MINT, (ewokos_addr_t)type, (ewokos_addr_t)rights, (ewokos_addr_t)&arg);
}

int32_t cap_revoke(uint32_t slot) {
    return (int32_t)syscall1(SYS_CAP_REVOKE, (ewokos_addr_t)slot);
}

int32_t cap_grant(int32_t target_pid, uint32_t src_slot, uint32_t rights_mask) {
    return (int32_t)syscall3(SYS_CAP_GRANT, (ewokos_addr_t)target_pid, (ewokos_addr_t)src_slot, (ewokos_addr_t)rights_mask);
}

int32_t cap_get(int32_t slot, cap_info_t* info) {
    return (int32_t)syscall2(SYS_CAP_GET, (ewokos_addr_t)slot, (ewokos_addr_t)info);
}

int32_t cap_policy_add(const char* cmd, uint32_t type, uint32_t rights, uint64_t a, uint64_t b) {
    cap_policy_add_t arg;
    memset(&arg, 0, sizeof(arg));
    if(cmd != NULL)
        strncpy(arg.cmd, cmd, CAP_POLICY_CMD_MAX - 1);
    arg.a = a;
    arg.b = b;
    arg.type = type;
    arg.rights = rights;
    return (int32_t)syscall1(SYS_CAP_POLICY_ADD, (ewokos_addr_t)&arg);
}

#ifdef __cplusplus
}
#endif
