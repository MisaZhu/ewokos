#include <mm/mmu.h>

#define PDE_SHIFT     21   // shift how many bits to get PDE index
						   
#define NUM_PAGE_TABLE_ENTRIES 512

#define  MAIR1VAL ((0x00ul << (MT_DEVICE_NGNRNE * 8)) |\
                   (0x04ul << (MT_DEVICE_NGNRE * 8)) |\
                   (0x0cul << (MT_DEVICE_GRE * 8)) |\
                   (0x44ul << (MT_NORMAL_NC * 8)) |\
                   (0xfful << (MT_NORMAL * 8)) )
 
 #define TCREL1VAL  ((0x0ul << 37) | /* TBI=0, no tagging */\
                     (0x0ul << 32) | /* IPS= 32 bit ... 000 = 32bit, 001 = 36bit, 010 = 40bit */\
                     (0x2ul << 30) | /* TG1=4k ... options are 10=4KB, 01=16KB, 11=64KB ... take care differs from TG0 */\
                     (0x3ul << 28) | /* SH1=3 inner ... options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable */\
                     (0x1ul << 26) | /* ORGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
                     (0x1ul << 24) | /* IRGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
                     (0x0ul  << 23)| /* EPD1 ... Translation table walk disable for translations using TTBR1_EL1  0 = walk, 1 = generate fault */\
                     (25ul   << 16)| /* T1SZ=25 (512G) ... The region size is 2 POWER (64-T1SZ) bytes */\
                     (0x0ul << 14) | /* TG0=4k  ... options are 00=4KB, 01=64KB, 10=16KB,  ... take care differs from TG1 */\
                     (0x3ul << 12) | /* SH0=3 inner ... .. options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable */\
                     (0x1ul << 10) | /* ORGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
                     (0x1ul << 8)  | /* IRGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
                     (0x0ul  << 7) | /* EPD0  ... Translation table walk disable for translations using TTBR0_EL1  0 = walk, 1 = generate fault */\
                     (25ul   << 0) ) /* T0SZ=25 (512G)  ... The region size is 2 POWER (64-T0SZ) bytes */
 
 #define SCTLREL1VAL ((0xC00800)| /* set mandatory reserved bits */\
                      (1 << 12) | /* I, Instruction cache enable. This is an enable bit for instruction caches at EL0 and EL1 */\
                      (0 << 4)  | /* SA0, tack Alignment Check Enable for EL0 */\
                      (0 << 3)  | /* SA, Stack Alignment Check Enable */\
                      (1 << 2)  | /* C, Data cache enable. This is an enable bit for data caches at EL0 and EL1 */\
                      (0 << 1)  | /* A, Alignment check enable bit */\
                      (1 << 0))   /* set M, enable MMU */


static __attribute__((__aligned__(PAGE_DIR_SIZE))) 
page_table_entry_t startup_page_dir[NUM_PAGE_TABLE_ENTRIES] = { 0 };

static __attribute__((__aligned__(PAGE_DIR_SIZE))) 
uint64_t startup_page_table[4] = { 0 };

static int delay(volatile uint64_t n){
	n *= 10000;
	while(n--);
}

static void _putc(char c){
	delay(1);
	*(uint32_t*)0xE8000000 = c;//'0' + v;
	delay(1);
}

void print_hex(uint64_t val){
	for(int i = 0; i < 16; i++){
		char v = (val >> 60) & 0xF;
		if(v <= 9)
			_putc('0' + v);
		else
			_putc('A' + v - 10);
	
		val <<= 4;
	}
	_putc('\n');
}

// setup the boot page table: dev_mem whether it is device memory
// support 0 - 4GB @ aarch64 mode 
static void set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len) {
	// convert all the parameters to indexes
	uint32_t idx;
	virt >>= PDE_SHIFT;
	phy  >>= PDE_SHIFT;
	len  >>= PDE_SHIFT;

	for (idx =0 ; idx < len; idx++)
	{
		// Each block descriptor (2 MB)
		startup_page_dir[virt] = (page_table_entry_t){
			.Address = (uint64_t)phy << (21 - 12),
			.AF = 1,
			.SH = STAGE2_SH_INNER_SHAREABLE,
			.MemAttr = MT_NORMAL,
			.EntryType = 1,
		};
		virt++;
		phy++;
	}

	startup_page_table[0] = (0x8000000000000000ul) | (uint64_t)&startup_page_dir[0]    | 3;
	startup_page_table[1] = (0x8000000000000000ul) | (uint64_t)&startup_page_dir[512]  | 3;
	startup_page_table[2] = (0x8000000000000000ul) | (uint64_t)&startup_page_dir[1024] | 3;
	startup_page_table[3] = (0x8000000000000000ul) | (uint64_t)&startup_page_dir[1536] | 3;
}

static void load_boot_pgt(void) {
	__asm("dsb sy");

	/*set memory ng nr ne*/
	uint64_t mair = MAIR1VAL;
	__asm("msr mair_el1, %[v]": :[v]"r" (mair):);

	 /* Bring both tables online and execute memory barrier */
	__asm("msr ttbr0_el1, %[v]": :[v]"r" (startup_page_table):);
	//__asm("msr ttbr1_el1, %[v]": :[v]"r" (startup_page_table):);
	__asm("isb");

	uint64_t tcr = TCREL1VAL;
	__asm("msr tcr_el1, %[v]": :[v]"r" (tcr):);
	__asm("isb");

	/* enable mmu */
	uint64_t sctlr = 0ul;
	__asm("mrs %[v], sctlr_el1": [v]"=r" (sctlr)::);
	sctlr |= SCTLREL1VAL;
	__asm("msr sctlr_el1, %[v]": :[v]"r" (sctlr):);
}

extern void _kernel_entry_c(void);

void _boot_start(void) {
	set_boot_pgt(0x0,  0x0, 64*MB);
	set_boot_pgt(KERNEL_BASE, 0x0, 64*MB);
	set_boot_pgt(MMIO_BASE, 0x3f000000, 16*MB);
	load_boot_pgt();
	return;
}
