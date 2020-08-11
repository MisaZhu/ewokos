#include <mm/mmudef.h>

#define PDE_SHIFT     20   // shift how many bits to get PDE index
#define KPDE_TYPE     0x02 // use "section" type for kernel page directory
#define AP_KO         0x0f // privilaged access, kernel: RW, user: no access
#define PE_CACHE      (1 << 3) // cachable
#define PE_BUF        (1 << 2) // bufferable

#define DEV_BASE      0x3f000000
#define DEV_MEM_SIZE  0x00400000

static uint32_t mmio_base = 0;

static inline void delay(uint32_t count) {
  while(count > 0) 
    count--;
}

__attribute__((__aligned__(PAGE_DIR_SIZE))) 
uint32_t startup_page_dir[PAGE_DIR_NUM] = { 0 };

// setup the boot page table: dev_mem whether it is device memory
static void set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len, uint8_t is_dev) {
	uint32_t idx;

	// convert all the parameters to indexes
	virt >>= PDE_SHIFT;
	phy  >>= PDE_SHIFT;
	len  >>= PDE_SHIFT;

	for (idx = 0; idx < len; idx++) {
		if (is_dev == 0) {
			// normal memory, make it kernel-only, cachable, bufferable
			startup_page_dir[virt] = (phy << PDE_SHIFT) | (AP_KO << 10) | PE_CACHE | PE_BUF | KPDE_TYPE;
		} else {
			// device memory, make it non-cachable and non-bufferable
			startup_page_dir[virt] = (phy << PDE_SHIFT) | (AP_KO << 10) | KPDE_TYPE;
		}

		virt++;
		phy++;
	}
}

static void load_boot_pgt(void) {
	uint32_t val;
	// set domain access control: all domain will be checked for permission
	val = 0x55555555;
	__asm("MCR p15, 0, %[v], c3, c0, 0": :[v]"r" (val):);

	// set the page table base registers. We use TTBR0, disable TTBR1
	//val = 1 << 5;
	//__asm("MCR p15, 0, %[v], c2, c0, 2": :[v]"r" (val):);

	val = (uint32_t)&startup_page_dir;
	// set the user page table
	__asm("MCR p15, 0, %[v], c2, c0, 0": :[v]"r" (val):);
	val = (uint32_t)&startup_page_dir;
	// set the kernel page table
	__asm("MCR p15, 0, %[v], c2, c0, 1": :[v]"r" (val):);

	// ok, enable paging using read/modify/write
	__asm("MRC p15, 0, %[r], c1, c0, 0": [r]"=r" (val)::); //read
	val |= 0x80300D; // enable MMU, cache, buffer, high vector tbl, disable subpage
	__asm("MCR p15, 0, %[r], c1, c0, 0": :[r]"r" (val):); //write

	// invalidate tlb
	val = 0;
	__asm("MCR p15, 0, %[r], c8, c7, 0": :[r]"r" (val):);
}


#define GPIO_FSEL4         ((volatile uint32_t*)(mmio_base+0x00200010))
#define GPIO_SET1          ((volatile uint32_t*)(mmio_base+0x00200020))
#define GPIO_CLR1          ((volatile uint32_t*)(mmio_base+0x0020002C))

static void boot_act_led(uint8_t on) {
	uint32_t ra;
	ra = get32(GPIO_FSEL4);
	ra &= ~(7<<21);
	ra |= 1<<21;
	put32(GPIO_FSEL4, ra);

	if(on == 0)
		put32(GPIO_CLR1, 1<<(47-32));
	else
		put32(GPIO_SET1, 1<<(47-32));
}

static void boot_act_led_flash(void) {
	boot_act_led(1);
	delay(1000000);
	boot_act_led(0);
	delay(1000000);
}

void _boot_start(void) {
	mmio_base = DEV_BASE;
	__asm volatile(
		// enable data cache
		"mrc p15, #0, r0, c1, c0, #0;"
		"orr r0, r0, #0x00000004;"
		"mcr p15, #0, r0, c1, c0, #0;"
		// enable instruction cache
		"mrc p15, #0, r0, c1, c0, #0;"
		"orr r0, r0, #0x00001000;"
		"mcr p15, #0, r0, c1, c0, #0;"
		//You must ensure this bit is set to 1 before the caches and MMU are
		//enabled, or any cache and TLB maintenance operations are performed.
		"mrc	p15, #0, r0, c1, c0, #1;"	// Read Auxiliary Control Register
		"orr	r4, r0, #0x00000040;"
		"mcr	p15, #0, r0, c1, c0, #1;"
	);

	set_boot_pgt(0, 0, 1024*1024*32, 0);
	set_boot_pgt(KERNEL_BASE, 0, 1024*1024*32, 0);
	set_boot_pgt(DEV_BASE, DEV_BASE, DEV_MEM_SIZE, 1);
	set_boot_pgt(MMIO_BASE, DEV_BASE, DEV_MEM_SIZE, 1);
	load_boot_pgt();

	boot_act_led_flash();
	mmio_base = MMIO_BASE;
	boot_act_led_flash();
}
