#include <mm/mmu.h>
#include <stddef.h>

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

// #region debug-point boot16k-start-uart
#define BOOT_UART0_PHYS  0x09000000ull
#define BOOT_UART_DATA   0x00
#define BOOT_UART_FLAGS  0x18
#define BOOT_UART_TXFF   0x20

static void boot_dbg_puts_phys(const char* s) {
	while (*s != '\0') {
		while (get32(BOOT_UART0_PHYS + BOOT_UART_FLAGS) & BOOT_UART_TXFF);
		put32(BOOT_UART0_PHYS + BOOT_UART_DATA, (uint32_t)*s++);
	}
}

static void boot_dbg_put_hex64(uint64_t value) {
	static const char hex[] = "0123456789abcdef";
	char out[17];

	for(int i = 0; i < 16; i++) {
		out[15 - i] = hex[value & 0xf];
		value >>= 4;
	}
	out[16] = '\0';
	boot_dbg_puts_phys(out);
}

static void boot_zero_mem(void* p, size_t n) {
	volatile uint8_t* cur = (volatile uint8_t*)p;
	while(n-- > 0)
		*cur++ = 0;
}

static void boot_set_pte_flags(page_table_entry_t* pte, int is_dev) {
	pte->NSTable = 1;
	pte->EntryType = TYPE_PAGE;
	pte->AF = 1;
	pte->SH = STAGE2_SH_INNER_SHAREABLE;

	if(is_dev) {
		pte->PXN = 1;
		pte->UXN = 1;
		pte->SH = STAGE2_SH_OUTER_SHAREABLE;
		pte->MemAttr = MT_DEVICE_NGNRNE;
	}
	else {
		pte->MemAttr = MT_NORMAL;
	}
}
// #endregion debug-point boot16k-start-uart

static __attribute__((__aligned__(PAGE_DIR_SIZE)))
page_dir_entry_t startup_page_dir[NUM_PAGE_DIRS] = { 0 };

#ifdef PAGE_SIZE_16K
#define BOOT_PAGE_TABLE_COUNT 16
static __attribute__((__aligned__(PAGE_DIR_SIZE)))
page_table_entry_t startup_page_tables[BOOT_PAGE_TABLE_COUNT][PAGE_DIR_NUM] = { 0 };
static uint32_t startup_page_table_index;
#else
#define PDE_SHIFT     PAGE_BLOCK_SHIFT
#define NUM_PAGE_TABLE_ENTRIES (PAGE_DIR_NUM * 2)
static __attribute__((__aligned__(PAGE_DIR_SIZE)))
page_table_entry_t startup_page_table[NUM_PAGE_TABLE_ENTRIES] = { 0 };
static page_table_entry_t *entry_head;
#endif

static void boot_pgt_init(void){
	boot_zero_mem(startup_page_dir, sizeof(startup_page_dir));
#ifdef PAGE_SIZE_16K
	startup_page_table_index = 0;
	boot_zero_mem(startup_page_tables, sizeof(startup_page_tables));
#else
	entry_head = startup_page_table;
	boot_zero_mem(startup_page_table, sizeof(startup_page_table));
#endif
}

static page_table_entry_t* get_free_page_table(void){
#ifdef PAGE_SIZE_16K
	if(startup_page_table_index >= BOOT_PAGE_TABLE_COUNT){
		/* no more free page table */
		while(1);
	}

	return startup_page_tables[startup_page_table_index++];
#else
	if(entry_head >= &startup_page_table[NUM_PAGE_TABLE_ENTRIES]){
		/*no more free page table*/
		while(1);
	}

	page_table_entry_t *entry = entry_head;
	entry_head += PAGE_DIR_NUM;
	return entry;
#endif
}

static void set_boot_pgt(uint64_t virt, uint64_t phy, uint32_t len, int is_dev) {
#ifdef PAGE_SIZE_16K
	uint64_t end = virt + len;

	while(virt < end) {
		uint32_t l1 = PAGE_L1_INDEX(virt);
		uint32_t l2 = PAGE_L2_INDEX(virt);
		uint32_t l3 = PAGE_L3_INDEX(virt);
		page_table_entry_t* l2_table;
		page_table_entry_t* l3_table;

		if(startup_page_dir[l1].EntryType == 0){
			l2_table = get_free_page_table();
			boot_zero_mem(l2_table, PAGE_TABLE_SIZE);
			startup_page_dir[l1] = (page_dir_entry_t){
				.NSTable = 1,
				.EntryType = TYPE_TABLE,
				.Address = (uint64_t)l2_table >> PAGE_SHIFT,
				.AF = 1
			};
		}
		else{
			l2_table = (page_table_entry_t*)((uint64_t)startup_page_dir[l1].Address << PAGE_SHIFT);
		}

		if(l2_table[l2].EntryType == 0){
			l3_table = get_free_page_table();
			boot_zero_mem(l3_table, PAGE_TABLE_SIZE);
			l2_table[l2] = (page_table_entry_t){
				.NSTable = 1,
				.EntryType = TYPE_TABLE,
				.Address = (uint64_t)l3_table >> PAGE_SHIFT,
				.AF = 1
			};
		}
		else{
			l3_table = (page_table_entry_t*)((uint64_t)l2_table[l2].Address << PAGE_SHIFT);
		}

		l3_table[l3].Address = phy >> PAGE_SHIFT;
		boot_set_pte_flags(&l3_table[l3], is_dev);

		virt += PAGE_SIZE;
		phy += PAGE_SIZE;
	}
#else
	page_table_entry_t* entry;
	uint32_t l1 = PAGE_L1_INDEX(virt);
	uint32_t l2 = PAGE_L2_INDEX(virt);

	if( startup_page_dir[l1].EntryType == 0){
		entry = get_free_page_table();
		startup_page_dir[l1] = (page_dir_entry_t){
			.NSTable = 1,
			.EntryType = TYPE_TABLE,
			.Address = (uint64_t)entry >> PAGE_SHIFT,
			.AF = 1
		};
	}else{
		entry = (page_table_entry_t*)((uint64_t)startup_page_dir[l1].Address << PAGE_SHIFT);
	}

    phy  >>= PDE_SHIFT;
    len  >>= PDE_SHIFT;
    for (uint32_t idx =0 ; idx < len; idx++)
    {
        // Each block descriptor (2 MB)
		entry[l2] = (page_table_entry_t){
            .NSTable = 1,
            .EntryType = TYPE_BLOCK,
            .Address = phy << (PDE_SHIFT - PAGE_SHIFT),
            .AF = 1,
            .SH = STAGE2_SH_OUTER_SHAREABLE,
            .S2AP = 0,
            .MemAttr = is_dev?MT_DEVICE_NGNRNE:MT_NORMAL,
        };
        l2++;
        phy++;
    }
#endif
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
