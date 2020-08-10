#include <mm/mmudef.h>

#define PDE_SHIFT     20   // shift how many bits to get PDE index
#define KPDE_TYPE     0x02 // use "section" type for kernel page directory
#define AP_KO         0x01 // privilaged access, kernel: RW, user: no access
#define PE_CACHE      (1 << 3) // cachable
#define PE_BUF        (1 << 2) // bufferable

#define DEV_BASE      0x3f000000
#define DEV_MEM_SIZE  0x400000

__attribute__((__aligned__(PAGE_DIR_SIZE))) 
uint32_t startup_page_dir[PAGE_DIR_NUM] = { 0 };

// setup the boot page table: dev_mem whether it is device memory
static void set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len, uint8_t is_dev) {
	uint32_t  pde, idx;

	// convert all the parameters to indexes
	virt >>= PDE_SHIFT;
	phy  >>= PDE_SHIFT;
	len  >>= PDE_SHIFT;

	for (idx = 0; idx < len; idx++) {
		pde = (phy << PDE_SHIFT);
		if (is_dev == 0) {
			// normal memory, make it kernel-only, cachable, bufferable
			pde |= (AP_KO << 10) | PE_CACHE | PE_BUF | KPDE_TYPE;
		} else {
			// device memory, make it non-cachable and non-bufferable
			pde |= (AP_KO << 10) | KPDE_TYPE;
		}

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

	// set the page table base registers. We use two page tables: TTBR0
	// for user space and TTBR1 for kernel space
	/*
	val = 32 - UADDR_BITS;
	asm("MCR p15, 0, %[v], c2, c0, 2": :[v]"r" (val):);
	*/

	// set the kernel page table
	val = (uint32_t)startup_page_dir | 0x00;
	__asm("MCR p15, 0, %[v], c2, c0, 1": :[v]"r" (val):);
	// set the user page table
	__asm("MCR p15, 0, %[v], c2, c0, 0": :[v]"r" (val):);

	// ok, enable paging using read/modify/write
	__asm("MRC p15, 0, %[r], c1, c0, 0": [r]"=r" (val)::); //read
	val |= 0x80300D; // enable MMU, cache, buffer, high vector tbl, disable subpage
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
