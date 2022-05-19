#include <mm/mmudef.h>

#define PDE_SHIFT     20   // shift how many bits to get PDE index

static __attribute__((__aligned__(PAGE_DIR_SIZE))) 
uint32_t startup_page_dir[PAGE_DIR_NUM] = { 0 };

// setup the boot page table: dev_mem whether it is device memory
static void set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len, uint8_t is_dev) {
	(void)is_dev;
	volatile uint32_t idx;

	// convert all the parameters to indexes
	virt >>= PDE_SHIFT;
	phy  >>= PDE_SHIFT;
	len  >>= PDE_SHIFT;

	/**startup with one level section descripter(32 bits)
		2bits  0-2   : pagedir type, 2 for section type
		2bits  10-11 : AP, 2 for read only
		12bits 20-31 : base address of section (1M for one section)
	 */
	for (idx = 0; idx < len; idx++) {
		startup_page_dir[virt] = (phy << PDE_SHIFT) | 2 | (2 << 10);
		virt++;
		phy++;
	}
}

static void load_boot_pgt(void) {
	volatile uint32_t val;
	// set domain access control: all domain will be checked for permission
	//val = 0x55555555;
	val = 0x1;
	__asm("MCR p15, 0, %[v], c3, c0, 0": :[v]"r" (val):);

	// set the kernel page table
	val = (uint32_t)&startup_page_dir;
	__asm("MCR p15, 0, %[v], c2, c0, 1": :[v]"r" (val):);
	// set the user page table
	__asm("MCR p15, 0, %[v], c2, c0, 0": :[v]"r" (val):);

	// ok, enable paging using read/modify/write
	__asm("MRC p15, 0, %[r], c1, c0, 0": [r]"=r" (val)::); //read
	val |= 0x2001; // enable MMU, high vector tbl
	__asm("MCR p15, 0, %[r], c1, c0, 0": :[r]"r" (val):); //write

	// flush all TLB
	val = 0;
	__asm("MCR p15, 0, %[r], c8, c7, 0": :[r]"r" (val):);
}

void _boot_start(void) {
	set_boot_pgt(0, 0, 32*MB, 0);
	set_boot_pgt(KERNEL_BASE, 0, 32*MB, 0);

	load_boot_pgt();
}
