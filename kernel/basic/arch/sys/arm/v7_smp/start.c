#include <mm/mmudef.h>

#include <mm/mmu.h>
#include <bcm283x/gpio.h>
#include <bcm283x/mini_uart.h>

#define AUX_OFFSET 0x00215000
#define UART_OFFSET 0x00215040

#define AUX_BASE (_mmio_base | AUX_OFFSET)
#define UART_BASE (_mmio_base | UART_OFFSET)

#define AUX_ENABLES (AUX_BASE+0x04)
#define UART_AUX_ENABLE 0x01

#define UART_IO_REG   (UART_BASE+0x00)
#define UART_IER_REG  (UART_BASE+0x04)
#define UART_IIR_REG  (UART_BASE+0x08)
#define UART_LCR_REG  (UART_BASE+0x0C)
#define UART_MCR_REG  (UART_BASE+0x10)
#define UART_LSR_REG  (UART_BASE+0x14)
#define UART_MSR_REG  (UART_BASE+0x18)
#define UART_SCRATCH  (UART_BASE+0x1C)
#define UART_CNTL_REG (UART_BASE+0x20)
#define UART_STAT_REG (UART_BASE+0x24)
#define UART_BAUD_REG (UART_BASE+0x28)

#define UART_BAUD_115200 270
#define UART_BAUD_9600 3254
#define UART_BAUD_DEFAULT UART_BAUD_115200

#define UART_TXD_GPIO 14
#define UART_RXD_GPIO 15

inline void _gpio_config(int32_t num, int32_t gpio_sel) {
  uint32_t raddr = (uint32_t)GPIO_FSEL0 + ((num/10)<<2);
  uint32_t shift = (num%10) * GPIO_SEL_BITS;
  uint32_t value = gpio_sel << shift;
  uint32_t mask = GPIO_SEL << shift;
  uint32_t data = get32(raddr);
  data &= ~mask;
  data |= value;
  put32(raddr, data);
}

inline void _gpio_pull(int32_t num, int32_t pull_dir) {
  uint32_t shift = (num % 32);
  uint32_t index = (num/32) + 1;
  *GPIO_PUD = pull_dir & GPIO_PULL_MASK;

  uint32_t n = 150; while(n > 0) n--; //delay 150
  put32((uint32_t)GPIO_PUD+(index<<2), 1<<shift); /* enable ppud clock */
  n = 150; while(n > 0) n--; //delay 150
  *GPIO_PUD = 0;
  put32((uint32_t)GPIO_PUD+(index<<2), 0); /* disable ppud clock */
}

static int32_t uart_init(void) {
	unsigned int data = get32(AUX_ENABLES);
	/* enable uart */
	put32(AUX_ENABLES, data|UART_AUX_ENABLE);
	/* configure uart */
	put32(UART_LCR_REG, 0x03); /** 8-bit data (errata in manual 0x01) */
	put32(UART_MCR_REG, 0x00);
	put32(UART_IER_REG, 0x00); /** no need interrupt */
	/** check requested baudrate **/
	/** baudrate count = ((sys_clk/baudrate)/8)-1 */
	put32(UART_BAUD_REG, UART_BAUD_DEFAULT); /** 16-bit baudrate counter */
	/* disable pull-down default on tx/rx pins */
	_gpio_pull(UART_TXD_GPIO, GPIO_PULL_NONE);
	_gpio_pull(UART_RXD_GPIO, GPIO_PULL_NONE);
	/* setup uart TX1/RX1 at pins 14/15 (ALTF5) */
	_gpio_config(UART_TXD_GPIO, GPIO_ALTF5);
	_gpio_config(UART_RXD_GPIO, GPIO_ALTF5);
	/** ready to go? */
	put32(UART_IIR_REG, 0xC6); /** clear TX/RX FIFO */
	put32(UART_CNTL_REG, 0x03); /** enable TX/RX */
	return 0;
}

#define UART_TXFIFO_EMPTY 0x20
#define UART_RXFIFO_AVAIL 0x01

static void uart_trans(uint32_t data) {
	while(!(get32(UART_LSR_REG) & UART_TXFIFO_EMPTY));
	if(data == '\r')
		data = '\n';
	put32(UART_IO_REG, data);
}

static void uart_write(const char* data) {
  int32_t i = 0;
	while(data[i] != 0) {
    char c = data[i];
    uart_trans(c);
		i++;
  }
}

#define PDE_SHIFT     20   // shift how many bits to get PDE index
#define KPDE_TYPE     0x02 // use "section" type for kernel page directory
#define AP_KO         0x01 // privilaged access, kernel: RW, user: no access

static __attribute__((__aligned__(PAGE_DIR_SIZE)))
volatile uint32_t startup_page_dir[PAGE_DIR_NUM] = { 0 };

// setup the boot page table: dev_mem whether it is device memory
static void __attribute__((optimize("O0"))) set_boot_pgt(uint32_t virt, uint32_t phy, uint32_t len, uint8_t is_dev) {
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

//static void __attribute__((optimize("O0"))) load_boot_pgt(void) {
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
	__asm("ORR r0,  r0, #0x004");            // cpu_dcache_enable
	__asm("MCR p15, 0, r0, c1, c0, 0");      // Write CP15 System Control register

	// invalidate tlb
	__asm("MOV r0, #0");
	__asm("MCR p15, 0, r0, c8, c7, 0");      //I-TLB and D-TLB invalidation
	__asm("MCR p15, 0, r0, c7, c10, 4");     //DSB
}

//void __attribute__((optimize("O0"))) _boot_start(void) {
void _boot_start(uint32_t core) {
	_mmio_base = 0x3f000000;
	uart_init();
	uart_trans('0'+core);
	uart_write(" hello\n");

while(1);
	set_boot_pgt(0, 0, 1024*1024*32, 0);
	set_boot_pgt(KERNEL_BASE, 0, 1024*1024*32, 0);

	load_boot_pgt();
}

