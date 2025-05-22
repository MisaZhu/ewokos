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


#define PIX3_MMIO_PHY  0x3f000000
#define PIX4_MMIO_PHY  0xfe000000
#define PIX5_MMIO_PHY  0x7c000000
#define PIX5_RP1_PHY   0x1c00000000L

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

static inline uint32_t cpu_get_id(void) {
    uint32_t midr;
#ifdef __aarch64__
    __asm volatile("mrs %0, midr_el1" : "=r" (midr));
#else
    __asm volatile("mrc p15, 0, %0, c0, c0, 0" : "=r" (midr));
#endif
    return midr;
}

#define ARM_CPU_PART_AEM_V8             0xD0F
#define ARM_CPU_PART_FOUNDATION         0xD00
#define ARM_CPU_PART_CORTEX_A57         0xD07
#define ARM_CPU_PART_CORTEX_A72         0xD08
#define ARM_CPU_PART_CORTEX_A53         0xD03
#define ARM_CPU_PART_CORTEX_A73         0xD09
#define ARM_CPU_PART_CORTEX_A75         0xD0A
#define ARM_CPU_PART_CORTEX_A35         0xD04
#define ARM_CPU_PART_CORTEX_A55         0xD05
#define ARM_CPU_PART_CORTEX_A76         0xD0B
#define ARM_CPU_PART_NEOVERSE_N1        0xD0C
#define ARM_CPU_PART_CORTEX_A77         0xD0D
#define ARM_CPU_PART_NEOVERSE_V1        0xD40
#define ARM_CPU_PART_CORTEX_A78         0xD41
#define ARM_CPU_PART_CORTEX_A78AE       0xD42
#define ARM_CPU_PART_CORTEX_X1          0xD44
#define ARM_CPU_PART_CORTEX_A510        0xD46
#define ARM_CPU_PART_CORTEX_A710        0xD47
#define ARM_CPU_PART_CORTEX_X2          0xD48
#define ARM_CPU_PART_NEOVERSE_N2        0xD49
#define ARM_CPU_PART_CORTEX_A78C        0xD4B

/* check Pi4 CPU */
static inline int32_t cpu_part(void) {
    uint32_t midr = cpu_get_id();
    //uint32_t implementer = (midr >> 24) & 0xFF;
    uint32_t part_num = (midr >> 4) & 0xFFF;

	return part_num;
}

void _boot_start(void) {
	boot_pgt_init();
	set_boot_pgt(0, 0, 64*MB, 0);
	set_boot_pgt(KERNEL_BASE, 0, 64*MB, 0);

    switch(cpu_part()){
		case ARM_CPU_PART_CORTEX_A76:	//Pi 5
			set_boot_pgt(MMIO_BASE, PIX5_MMIO_PHY, PIX_MMIO_SIZE, 1);
			set_boot_pgt(MMIO_BASE +  PIX_MMIO_SIZE, PIX5_RP1_PHY, PIX_MMIO_SIZE, 1);
			break;						
		case ARM_CPU_PART_CORTEX_A72:	//Pi 4
			set_boot_pgt(MMIO_BASE, PIX4_MMIO_PHY, PIX_MMIO_SIZE, 1);
			break;
		default:						//Pi 2, Pi 3
			set_boot_pgt(MMIO_BASE, PIX3_MMIO_PHY, PIX_MMIO_SIZE, 1);
			break;
	}

	load_boot_pgt(startup_page_dir);
}
