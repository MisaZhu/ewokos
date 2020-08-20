#include <mm/mmudef.h>

#define PDE_SHIFT     20   // shift how many bits to get PDE index
#define KPDE_TYPE     0x02 // use "section" type for kernel page directory
#define AP_KO         0x03 // privilaged access, kernel: RW, user: no access
#define DOMAIN        (0xf << 5) // execute never on strongly-ordered mem
#define XN            (1 << 4) // execute never on strongly-ordered mem

#define DEV_BASE      0x3f000000
#define DEV_MEM_SIZE  0x00400000
static volatile uint32_t mmio_base = 0;


__attribute__((__aligned__(PAGE_DIR_SIZE))) 
volatile uint32_t startup_page_dir[PAGE_DIR_NUM] = { 0 };

// setup the boot page table: dev_mem whether it is device memory
//static void __attribute__((optimize("O0"))) set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len, uint8_t is_dev) {
static void set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len, uint8_t is_dev) {
	(void)is_dev;
	volatile uint32_t idx;

	// convert all the parameters to indexes
	virt >>= PDE_SHIFT;
	phy  >>= PDE_SHIFT;
	len  >>= PDE_SHIFT;

	for (idx = 0; idx < len; idx++) {
		startup_page_dir[virt] = (phy << PDE_SHIFT) | KPDE_TYPE | AP_KO<< 10;
		virt++;
		phy++;
	}
}

static void load_boot_pgt(void) {
//static void load_boot_pgt(void) {
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
	__asm("ORR r0,  r0, #0x004");            // cpu_dcache_enable
	__asm("MCR p15, 0, r0, c1, c0, 0");      // Write CP15 System Control register
	
	// invalidate tlb
	__asm("MOV r0, #0");
	__asm("MCR p15, 0, r0, c8, c7, 0");      //I-TLB and D-TLB invalidation
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
	boot_act_led_flash();
	
	set_boot_pgt(0, 0, 1024*1024*32, 0);
	set_boot_pgt(KERNEL_BASE, 0, 1024*1024*32, 0);
	set_boot_pgt(DEV_BASE, DEV_BASE, DEV_MEM_SIZE, 1);
	set_boot_pgt(MMIO_BASE, DEV_BASE, DEV_MEM_SIZE, 1);

	load_boot_pgt();

	mmio_base = MMIO_BASE; //use high mem mmio
	boot_act_led_flash();
}
