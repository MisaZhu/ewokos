#include <mm/mmu.h>

#ifdef __arm__
#define PDE_SHIFT     20   // shift how many bits to get PDE index
#define KPDE_TYPE     0x02 // use "section" type for kernel page directory
#define AP_KO         0x01 // privilaged access, kernel: RW, user: no access
// setup the boot page table with one-level section type paging : is_dev whether it is device memory

static __attribute__((__aligned__(PAGE_DIR_SIZE)))
volatile uint32_t startup_page_dir[PAGE_DIR_NUM] = { 0 };

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
#define NUM_PAGE_TABLE_ENTRIES 512
// support 0 - 4GB @ aarch64 mode
static __attribute__((__aligned__(PAGE_DIR_SIZE)))
volatile uint64_t startup_page_dir[4] = { 0 };

static __attribute__((__aligned__(PAGE_DIR_SIZE)))
page_table_entry_t startup_page_table[NUM_PAGE_TABLE_ENTRIES] = { 0 };

static void set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len, int is_dev) {
    // convert all the parameters to indexes
    uint32_t idx;
    virt >>= PDE_SHIFT;
    phy  >>= PDE_SHIFT;
    len  >>= PDE_SHIFT;

    for (idx =0 ; idx < len; idx++)
    {
        // Each block descriptor (2 MB)
        startup_page_table[virt] = (page_table_entry_t){
            .NSTable = 1,
            .EntryType = 1,
            .Address = (uint64_t)phy << (21 - 12),
            .AF = 1,
            .SH = STAGE2_SH_OUTER_SHAREABLE,
            .S2AP = 0,
            .MemAttr = is_dev?MT_DEVICE_NGNRNE:MT_NORMAL,
        };
        virt++;
        phy++;
    }

    startup_page_dir[0] = (0x8000000000000400ul) | (uint64_t)&startup_page_table[0]    | 3;
    startup_page_dir[1] = (0x8000000000000400ul) | (uint64_t)&startup_page_table[512]  | 3;
    startup_page_dir[2] = (0x8000000000000400ul) | (uint64_t)&startup_page_table[1024] | 3;
    startup_page_dir[3] = (0x8000000000000400ul) | (uint64_t)&startup_page_table[1536] | 3;
}
#endif


#define PIX3_MMIO_PHY  0x3f000000
#define PIX4_MMIO_PHY  0xfe000000
#define PIX_MMIO_SIZE 16*MB

typedef struct {
    uint8_t channel: 4;
    uint32_t data: 28;
} _mail_message_t;

typedef struct {
    uint32_t reserved: 30;
    uint8_t empty: 1;
    uint8_t full:1;
} _mail_status_t;

extern void load_boot_pgt(uint32_t page_table);

void _boot_start(void) {
	set_boot_pgt(0, 0, 64*MB, 0);
	set_boot_pgt(KERNEL_BASE, 0, 64*MB, 0);
#ifdef PI4
	set_boot_pgt(MMIO_BASE, PIX4_MMIO_PHY, PIX_MMIO_SIZE, 1);
#else
	set_boot_pgt(MMIO_BASE, PIX3_MMIO_PHY, PIX_MMIO_SIZE, 1);
#endif
	load_boot_pgt(startup_page_dir);
}
