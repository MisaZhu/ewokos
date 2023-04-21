#include <mm/mmudef.h>
#include <mm/mmu_arch.h>
#include <kprintf.h>
#include <csr.h>

// setup the boot page table: dev_mem whether it is device memory

static __attribute__((__aligned__(PAGE_DIR_SIZE)))
volatile page_dir_entry_t  startup_page[PAGE_DIR_NUM] = {0};

static void set_boot_pgt(void) {
	for(int i = 0; i < PAGE_DIR_NUM; i++){
		startup_page[i].valid = 1;
		startup_page[i].permission = 7;	
		startup_page[i].global = 1;	
		startup_page[i].access = 1;	
		startup_page[i].ppn = (i*0x40000000L)>>12;	
	}
}

void load_boot_pgt(uint64_t page_tbl) {
	page_tbl >>= 12;
	page_tbl |= 0x8L << 60;
	page_tbl |= 0xFFFFL << 44;
	asm volatile("csrw 0x180, %0" :: "r" (page_tbl));
 	asm volatile("sfence.vma");
}

void trap_init(void){
	extern void interrupt_table_start(void);
    csr_write(CSR_STVEC, interrupt_table_start);
}

void _boot_start(void) {
	trap_init();
	set_boot_pgt();
	load_boot_pgt(startup_page);
}
