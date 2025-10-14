#ifndef _QEMU_FW_CFG_H_
#define _QEMU_FW_CFG_H_

#define BE16(x) __builtin_bswap16(x)
#define BE64(x) __builtin_bswap64(x)
#define BE32(x) __builtin_bswap32(x)


int fw_init(void);
int fw_set_cfg(const char* path, void* cfg, int len);

#endif
