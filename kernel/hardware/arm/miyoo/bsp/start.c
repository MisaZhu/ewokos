#include <mm/mmudef.h>

#define PDE_SHIFT     20   // shift how many bits to get PDE index
#define KPDE_TYPE     0x02 // use "section" type for kernel page directory
#define AP_KO         0x01 // privilaged access, kernel: RW, user: no access

static __attribute__((__aligned__(PAGE_DIR_SIZE)))
volatile uint32_t startup_page_dir[PAGE_DIR_NUM] = { 0 };

// setup the boot page table with one-level section type paging : is_dev whether it is device memory
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

static void load_boot_pgt(void) {
	volatile uint32_t val;
	// set domain access control: all domain will be checked for permission
	val = 0x55555555;
	__asm("MCR p15, 0, %[v], c3, c0, 0": :[v]"r" (val):);

	val = (uint32_t)&startup_page_dir;
	// set the user page table
	__asm("MCR p15, 0, %[v], c2, c0, 0": :[v]"r" (val):);
	__asm("DMB");

	__asm("MRC p15, 0, r0, c1, c0, 0");      // Read CP15 System Control register
	__asm("ORR r0, r0, #0x001");             // Set M bit 0 to enable MMU before scatter loading
	__asm("MCR p15, 0, r0, c1, c0, 0");      // Write CP15 System Control register
	__asm("dsb");

	__asm("MRC p15, 0, r0, c1, c0, 0");      // Read CP15 System Control register
	__asm("ORR r0,  r0, #0x00002000");       // high int vector
	__asm("ORR r0,  r0, #0x00001000");       // cpu_icache_enable
	__asm("ORR r0,  r0, #0x00000800");       // cpu_icache_enable
	__asm("ORR r0,  r0, #0x004");            // cpu_dcache_enable
	__asm("MCR p15, 0, r0, c1, c0, 0");      // Write CP15 System Control register

	// invalidate tlb
	__asm("MOV r0, #0");
	__asm("MCR p15, 0, r0, c8, c7, 0");      //I-TLB and D-TLB invalidation
	__asm("MCR p15, 0, r0, c7, c10, 4");     //DSB
}

#define PHY_OFFSET 		0x20000000
#define MMIO_OFFSET     0x1F000000
#define REGW16(a,v) 	(*(volatile unsigned short *)(a) = (v))
#define REGR16(a) 		(*(volatile unsigned short *)(a))

void init_cpu_clock(void) {
	REGW16(MMIO_OFFSET + (0x1032A4 << 1), 0x78D4); //set target freq to LPF high
	REGW16(MMIO_OFFSET + (0x1032A6 << 1), 0x0029);
	REGW16(MMIO_OFFSET + (0x1032B0 << 1), 0x0001); //switch to LPF control
	REGW16(MMIO_OFFSET + (0x1032AA << 1), 0x0006); //mu[2:0]
	REGW16(MMIO_OFFSET + (0x1032AE << 1), 0x0008); //lpf_update_cnt[7:0]
	REGW16(MMIO_OFFSET + (0x1032B2 << 1), 0x1000);  //from low to high
	REGW16(MMIO_OFFSET + (0x1032A8 << 1), 0x0000); //toggle LPF enable
	REGW16(MMIO_OFFSET + (0x1032A8 << 1), 0x0001);

	while( !(REGR16(MMIO_OFFSET + (0x1032BA << 1))) ); //polling done

	REGW16(MMIO_OFFSET + (0x1032A8 << 1), 0x0000);
	REGW16(MMIO_OFFSET + (0x1032A0 << 1), 0x78D4);  //store freq to LPF low
	REGW16(MMIO_OFFSET + (0x1032A2 << 1), 0x0029);
}


void _boot_start(void) {
	init_cpu_clock();
	set_boot_pgt(PHY_OFFSET, PHY_OFFSET, 8*MB, 0);
	set_boot_pgt(KERNEL_BASE, PHY_OFFSET, 8*MB, 0);
	load_boot_pgt();
}
