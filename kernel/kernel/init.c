#include <mm/mmu.h>
#include <kernel/hw_info.h>
#include <dev/actled.h>
#include <kprintf.h>

/*Copy interrupt talbe to phymen address 0x00000000.
	Virtual address #INTERRUPT_VECTOR_BASE(0xFFFF0000 for ARM) must mapped to phymen 0x00000000.
ref: set_kernel_vm(page_dir_entry_t* vm)
 */
void __attribute__((optimize("O0"))) _copy_interrupt_table(void) {
	extern uint32_t  interrupt_table_start, interrupt_table_end;
	uint32_t *vsrc = &interrupt_table_start;
	uint32_t *vdst = (uint32_t*)0;
	while(vsrc < &interrupt_table_end) {
		*vdst++ = *vsrc++;
	}
}

void _boot_init(void) {
	hw_info_init();
	_mmio_base = get_hw_info()->phy_mmio_base;
	flush_actled();
}
