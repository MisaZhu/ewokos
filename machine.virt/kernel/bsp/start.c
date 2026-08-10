#include <mm/mmu.h>
#ifdef __aarch64__
#include <mm/boot_pgt.h>
#endif

#ifdef __arm__
#define PDE_SHIFT     20   // shift how many bits to get PDE index
#define KPDE_TYPE     0x02 // use "section" type for kernel page directory
#define AP_KO         0x01 // privilaged access, kernel: RW, user: no access
// setup the boot page table with one-level section type paging : is_dev whether it is device memory

static __attribute__((__aligned__(PAGE_DIR_SIZE)))
volatile uint32_t startup_page_dir[PAGE_DIR_NUM] = { 0 };

static void boot_pgt_init(void){

}

static void set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len, uint8_t is_dev) {
	(void)is_dev;
	volatile uint32_t idx;

	// convert all the parameters to indexes
	virt >>= PDE_SHIFT;
	phy  >>= PDE_SHIFT;
	len  >>= PDE_SHIFT;

	for (idx = 0; idx < len; idx++) {
		startup_page_dir[virt] = (phy << PDE_SHIFT) | AP_KO<< 10 | KPDE_TYPE; //section type, system RW 
		virt++;
		phy++;
	}
}
#elif __aarch64__
#define NUM_PAGE_DIRS PAGE_DIR_NUM
// support 0 - 4GB @ aarch64 mode

static __attribute__((__aligned__(PAGE_DIR_SIZE)))
page_dir_entry_t startup_page_dir[NUM_PAGE_DIRS] = { 0 };

#ifdef PAGE_SIZE_16K
#define BOOT_PAGE_TABLE_COUNT 16
static __attribute__((__aligned__(PAGE_DIR_SIZE)))
page_table_entry_t startup_page_tables[BOOT_PAGE_TABLE_COUNT][PAGE_DIR_NUM] = { 0 };
static boot_pgt_ctx_t boot_pgt = {
	.page_dir = startup_page_dir,
	.page_table_count = BOOT_PAGE_TABLE_COUNT,
	.page_tables = startup_page_tables,
};
#else
#define BOOT_PAGE_TABLE_COUNT 4
static __attribute__((__aligned__(PAGE_DIR_SIZE)))
page_table_entry_t startup_page_table[BOOT_PAGE_TABLE_COUNT * PAGE_DIR_NUM] = { 0 };
static boot_pgt_ctx_t boot_pgt = {
	.page_dir = startup_page_dir,
	.page_table_count = BOOT_PAGE_TABLE_COUNT,
	.page_tables = startup_page_table,
};
#endif

static void boot_pgt_init(void){
	boot_pgt_ctx_init(&boot_pgt);
}

static void set_boot_pgt(uint64_t virt, uint64_t phy, uint32_t len, int is_dev) {
	boot_pgt_map_range(&boot_pgt, virt, phy, len, is_dev);
}
#endif

extern void load_boot_pgt(uint64_t page_table);

void _boot_start(void) {
	boot_pgt_init();
	set_boot_pgt(0x40000000,  0x40000000, 64*MB, 0);
	set_boot_pgt(KERNEL_BASE, 0x40000000, 64*MB, 0);
	set_boot_pgt(MMIO_BASE,   0x8000000,  256*MB, 1);
	load_boot_pgt((uint64_t)startup_page_dir);
}
