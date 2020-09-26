#ifndef HW_INFO_H
#define HW_INFO_H

#include <stdint.h>
#include <sysinfo.h>
#include <mm/mmu.h>

extern sys_info_t _sys_info;

extern void       sys_info_init(void);
extern void       arch_vm(page_dir_entry_t* vm);

#endif
