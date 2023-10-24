#include <kernel/hw_info.h>
sys_info_t _sys_info;

void sys_info_init(void) {
    sys_info_init_arch();
}