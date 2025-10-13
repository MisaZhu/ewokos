#include <mm/mmu.h>

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
#define PDE_SHIFT     21
#define NUM_PAGE_DIRS 512
#define NUM_PAGE_TABLE_ENTRIES 4096
// support 0 - 4GB @ aarch64 mode

static __attribute__((__aligned__(PAGE_DIR_SIZE)))
page_dir_entry_t startup_page_dir[NUM_PAGE_DIRS] = { 0 };

static __attribute__((__aligned__(PAGE_DIR_SIZE)))
page_table_entry_t startup_page_table[NUM_PAGE_TABLE_ENTRIES] = { 0 };

static page_table_entry_t *entry_head;

static void boot_pgt_init(void){
	entry_head = startup_page_table;
	for(int i = 0; i < NUM_PAGE_DIRS; i++){
		startup_page_dir[i].EntryType = 0;
	}
}

static page_table_entry_t* get_free_page_table(void){
	if(entry_head >= &startup_page_table[NUM_PAGE_TABLE_ENTRIES]){
		/*no more free page table*/
		while(1);
	}

	page_table_entry_t *entry = entry_head;
	entry_head += 512;
	return entry;
}

static void set_boot_pgt(uint64_t virt, uint64_t phy, uint32_t len, int is_dev) {
    // convert all the parameters to indexes
	page_table_entry_t* entry;
	uint32_t l1 = PAGE_L1_INDEX(virt);
	uint32_t l2 = PAGE_L2_INDEX(virt);

	if( startup_page_dir[l1].EntryType == 0){
		entry = get_free_page_table();
		startup_page_dir[l1] = (page_dir_entry_t){
			.NSTable = 1,
			.EntryType = TYPE_TABLE,
			.Address = (uint64_t)entry >> 12,
			.AF = 1
		};
	}else{
		entry = startup_page_dir[l1].Address << 12;
	}

    phy  >>= PDE_SHIFT;
    len  >>= PDE_SHIFT;
    for (uint32_t idx =0 ; idx < len; idx++)
    {
        // Each block descriptor (2 MB)
		entry[l2] = (page_table_entry_t){
            .NSTable = 1,
            .EntryType = TYPE_BLOCK,
            .Address = phy << (21 - 12),
            .AF = 1,
            .SH = STAGE2_SH_OUTER_SHAREABLE,
            .S2AP = 0,
            .MemAttr = is_dev?MT_DEVICE_NGNRNE:MT_NORMAL,
        };
        l2++;
        phy++;
    }
}
#endif

extern void load_boot_pgt(uint32_t page_table);

void _boot_start(void) {
	boot_pgt_init();
	set_boot_pgt(0x40000000,  0x40000000, 64*MB, 0);
	set_boot_pgt(KERNEL_BASE, 0x40000000, 64*MB, 0);
	set_boot_pgt(MMIO_BASE,   0x8000000,  256*MB, 1);
	load_boot_pgt(startup_page_dir);
}
