#include <mm/mmudef.h>

#define PDE_SHIFT     20   // shift how many bits to get PDE index
#define KPDE_TYPE     0x02 // use "section" type for kernel page directory
#define AP_KO         0x03 // privilaged access, kernel: RW, user: no access
#define DOMAIN        (0xf << 5) // execute never on strongly-ordered mem
#define XN            (1 << 4) // execute never on strongly-ordered mem

#define DEV_BASE      0x3f000000
#define DEV_MEM_SIZE  0x00400000
static uint32_t mmio_base = 0;

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
			 //normal memory, make it kernel-only, cachable, bufferable
			//startup_page_dir[virt] = (phy << PDE_SHIFT) | (AP_KO << 10) | DOMAIN | XN | KPDE_TYPE;
			startup_page_dir[virt] = (phy << PDE_SHIFT) | (AP_KO << 10) | KPDE_TYPE;
		} else {
			//device memory, make it non-cachable and non-bufferable
			startup_page_dir[virt] = (phy << PDE_SHIFT) | (AP_KO << 10) | KPDE_TYPE;
		}

		virt++;
		phy++;
	}
}
/*
static void load_boot_pgt(void) {
	uint32_t val;
	// set domain access control: all domain will be checked for permission
	val = 0x55555555;
	__asm("MCR p15, 0, %[v], c3, c0, 0": :[v]"r" (val):);

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
*/

static void load_boot_pgt(void) {
	uint32_t val;
	// set domain access control: all domain will be checked for permission
	val = 0x55555555;
	__asm("MCR p15, 0, %[v], c3, c0, 0": :[v]"r" (val):);

	//0x48 means:
	//RGN=b01  (outer cacheable write-back cached, write allocate)
  //S=0      (translation table walk to non-shared memory)
  //IRGN=b01 (inner cacheability for the translation table walk is Write-back Write-allocate)
	val = (((uint32_t)&startup_page_dir) | 0x48);
	//val = (uint32_t)&startup_page_dir;
	// set the user page table
	__asm("MCR p15, 0, %[v], c2, c0, 0": :[v]"r" (val):);
	// set the kernel page table
	__asm("MCR p15, 0, %[v], c2, c0, 1": :[v]"r" (val):);

	__asm("MRC p15, 0, r0, c1, c0, 0");      // Read CP15 System Control register
	__asm("BIC r0, r0, #(0x1 << 12)");       // Clear I bit 12 to disable I Cache
	__asm("BIC r0, r0, #(0x1 <<  2)");       // Clear C bit  2 to disable D Cache
	__asm("BIC r0, r0, #0x2");               // Clear A bit  1 to disable strict alignment fault checking
	__asm("ORR r0, r0, #0x1");               // Set M bit 0 to enable MMU before scatter loading
	__asm("MCR p15, 0, r0, c1, c0, 0");      // Write CP15 System Control register
	
	// invalidate tlb
	__asm("MOV r0, #0");
	__asm("MCR p15, 0, r0, c8, c7, 0");      //I-TLB and D-TLB invalidation
	__asm("MCR p15, 0, r0, c7, c5, 1");      //ICIALLU: invalidate instruction cache
	//__asm("MCR p15, 0, r0, c7, c5, 6");      //BPIALL - Invalidate entire branch predictor array
	__asm("dsb");
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

static inline void delay(uint32_t count) {
  while(count > 0) 
    count--;
}

static void boot_act_led_flash(void) {
	boot_act_led(1);
	delay(1000000);
	boot_act_led(0);
	delay(1000000);
}

void _boot_start(void) {
	mmio_base = DEV_BASE;
	set_boot_pgt(0, 0, 1024*1024*32, 0);
	set_boot_pgt(KERNEL_BASE, 0, 1024*1024*32, 0);
	set_boot_pgt(DEV_BASE, DEV_BASE, DEV_MEM_SIZE, 1);
	set_boot_pgt(MMIO_BASE, DEV_BASE, DEV_MEM_SIZE, 1);

	boot_act_led_flash();
	load_boot_pgt();
	boot_act_led_flash();

	mmio_base = MMIO_BASE;
	boot_act_led_flash();
}
