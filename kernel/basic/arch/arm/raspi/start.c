#include <mm/mmudef.h>

#define PDE_SHIFT     20   // shift how many bits to get PDE index
#define DEV_BASE      0x20000000
#define DEV_MEM_SIZE  0x400000

__attribute__((__aligned__(PAGE_DIR_SIZE))) 
uint32_t startup_page_dir[PAGE_DIR_NUM] = { 0 };

// setup the boot page table: dev_mem whether it is device memory
static void set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len, uint8_t is_dev) {
	(void)is_dev;
	uint32_t  pde, idx;

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
		pde = (phy << PDE_SHIFT) | 2 | (2 << 10);
		startup_page_dir[virt] = pde;
		virt++;
		phy++;
	}
}

static void load_boot_pgt(void) {
	uint32_t val;
	// set domain access control: all domain will be checked for permission
	val = 0x55555555;
	__asm("MCR p15, 0, %[v], c3, c0, 0": :[v]"r" (val):);

	// set the kernel page table
	val = (uint32_t)startup_page_dir | 0x00;
	__asm("MCR p15, 0, %[v], c2, c0, 1": :[v]"r" (val):);
	// set the user page table
	__asm("MCR p15, 0, %[v], c2, c0, 0": :[v]"r" (val):);

	// ok, enable paging using read/modify/write
	__asm("MRC p15, 0, %[r], c1, c0, 0": [r]"=r" (val)::); //read
	val |= 0x2001; // enable MMU, high vector tbl
	__asm("MCR p15, 0, %[r], c1, c0, 0": :[r]"r" (val):); //write

	// flush all TLB
	__asm("MCR p15, 0, r0, c8, c7, 0");
}

void _boot_start(void) {
	set_boot_pgt(0, 0, 1024*1024*32, 0);
	set_boot_pgt(KERNEL_BASE, 0, 1024*1024*32, 0);
	set_boot_pgt(KERNEL_BASE + DEV_BASE, DEV_BASE, DEV_MEM_SIZE, 1);
	load_boot_pgt();
}
