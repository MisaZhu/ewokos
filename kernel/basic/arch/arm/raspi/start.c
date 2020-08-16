#include <mm/mmudef.h>

#define PDE_SHIFT     20   // shift how many bits to get PDE index
#define DEV_BASE      0x20000000
#define DEV_MEM_SIZE  0x00400000
static uint32_t mmio_base = 0;

__attribute__((__aligned__(PAGE_DIR_SIZE))) 
uint32_t startup_page_dir[PAGE_DIR_NUM] = { 0 };

// setup the boot page table: dev_mem whether it is device memory
//static void __attribute__((optimize("O0"))) set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len, uint8_t is_dev) {
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
