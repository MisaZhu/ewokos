#include <arch/bcm283x/gpio.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/dma.h>

//#define USE_PWM1

#if defined(USE_PWM1)
#define PWM_BASE        (_mmio_base + 0x20C000 + 0x800) /* PWM1 register base address on RPi4 */
#define CLOCK_BASE      (_mmio_base + 0x101000 + 0x800)
#else
#define PWM_BASE        (_mmio_base + 0x20C000) /* PWM0 register base address on RPi */
#define CLOCK_BASE      (_mmio_base + 0x101000)
#endif

#define DMA_V_BASE        (_mmio_base + 0x007100)         /* DMA register base address */
#define DMA_ENABLE      (DMA_V_BASE + 0xFF0)                   /* DMA global enable bits */

#define BCM283x_PWMCLK_CNTL 40
#define BCM283x_PWMCLK_DIV  41
#define PM_PASSWORD 0x5A000000 

#define BCM283x_PWM_CONTROL 0
#define BCM283x_PWM_STATUS  1
#define BCM283x_PWM_DMAC    2
#define BCM283x_PWM_FIFO    6

#ifdef USE_PWM1
#define BCM283x_PWM_RANGE  8
#define BCM283x_PWM_DATA   9
#else
#define BCM283x_PWM_RANGE  4
#define BCM283x_PWM_DATA   5
#endif

#ifdef USE_PWM1
#define BCM283x_PWM_USEFIFO  0x2000  /* PWM1 Data from FIFO */
#define BCM283x_PWM_ENABLE   0x0100  /* PWM1 Channel enable */
#else
#define BCM283x_PWM_USEFIFO  0x0020  /* PWM0 Data from FIFO */
#define BCM283x_PWM_ENABLE   0x0001  /* PWM0 Channel enable */
#endif

#define BCM283x_PWM_ENAB      0x80000000  /* PWM DMA Configuration: DMA Enable (bit 31 set) */

#define BCM283x_GAPO2 0x20
#define BCM283x_GAPO1 0x10
#define BCM283x_RERR1 0x8
#define BCM283x_WERR1 0x4
#define BCM283x_FULL1 0x1
#define ERRORMASK (BCM283x_GAPO2 | BCM283x_GAPO1 | BCM283x_RERR1 | BCM283x_WERR1)

#define DMA_CS        0       /* Control/status register offset for DMA channel 0 */
#define DMA_CONBLK_AD 1
#define DMA_EN0       1 << 0  /* Enable DMA engine 0 */
//#define DMA_EN1       1 << 1  /* Enable DMA engine 1 */
#define DMA_ACTIVE    1       /* Active bit set */
#define DMA_DEST_DREQ 0x40    /* Use DREQ to pace peripheral writes */
#define DMA_SRC_INC   0x100   /* Increment source address */

#ifdef USE_PWM1
#define DMA_PERMAP  0x10000 /* PWM1 peripheral for DREQ */
#else
#define DMA_PERMAP  0x0 /* PWM0 peripheral for DREQ */
#endif	

#define DMA_BUF_SIZE  (1024*16)

typedef struct dma_cb {
   unsigned int ti;
   unsigned int source_ad;
   unsigned int dest_ad;
   unsigned int txfr_len;
   unsigned int stride;
   unsigned int nextconbk;
   unsigned int null1;
   unsigned int null2;
} dma_cb_t;

static dma_cb_t* _dma_cb = NULL;
static uint32_t _dma_data_addr = 0;

static void audio_init(void) {   
	volatile unsigned* clk = (void*)CLOCK_BASE;
	volatile unsigned* pwm = (void*)PWM_BASE;

#ifdef USE_PWM1
	bcm283x_gpio_config(40, GPIO_ALTF5); // Ensure PWM1 is mapped to GPIO 18
	bcm283x_gpio_config(41, GPIO_ALTF5); // Ensure PWM1 is mapped to GPIO 18
#else
	bcm283x_gpio_config(18, GPIO_ALTF5); // Ensure PWM0 is mapped to GPIO 18
#endif

	proc_usleep(2000);

	// Setup clock

	*(clk + BCM283x_PWMCLK_CNTL) = PM_PASSWORD | (1 << 5); // Stop clock
	proc_usleep(2000);

	int idiv = 2;
	*(clk + BCM283x_PWMCLK_DIV)  = PM_PASSWORD | (idiv<<12);
	*(clk + BCM283x_PWMCLK_CNTL) = PM_PASSWORD | 16 | 1;  // Osc + Enable
	proc_usleep(2000);

	// Setup PWM

	*(pwm + BCM283x_PWM_CONTROL) = 0;
	proc_usleep(2000);

	*(pwm+BCM283x_PWM_RANGE) = 0x264; // 44.1khz, Stereo, 8-bit (54Mhz / 44100 / 2)

	*(pwm+BCM283x_PWM_CONTROL) =
          BCM283x_PWM_USEFIFO |
          BCM283x_PWM_ENABLE | 1<<6;
	
	//_dma_cb = (dma_cb_t*)dma_phy_addr(0, dma_alloc(sizeof(dma_cb_t)));
	//_dma_data_addr = dma_phy_addr(0, (dma_alloc(DMA_BUF_SIZE))); //4k dma buffer
	_dma_cb = (dma_cb_t*)(dma_alloc(0, sizeof(dma_cb_t)));
	_dma_data_addr = ((dma_alloc(0, DMA_BUF_SIZE))); //4k dma buffer

	// Set up control block
	uint32_t phy_pwm_base = syscall1(SYS_V2P, PWM_BASE);
	slog("sound: pwd base 0x%x\n", phy_pwm_base);

	_dma_cb->ti = DMA_DEST_DREQ + DMA_PERMAP + DMA_SRC_INC;
	_dma_cb->source_ad = _dma_data_addr;
	_dma_cb->dest_ad = phy_pwm_base + 0x18; // Points to PWM_FIFO
	_dma_cb->stride = 0x00;
	_dma_cb->nextconbk = 0x00; // Don't loop
	_dma_cb->null1 = 0x00;
	_dma_cb->null2 = 0x00;
	proc_usleep(200);

	slog("sound: dma cb 0x%x, data 0x%x, size: %dk\n",
			_dma_cb, _dma_data_addr, DMA_BUF_SIZE/1024);
}


static void playaudio_dma(uint8_t* data, uint32_t size) {
	// Convert data
	if(size > (DMA_BUF_SIZE/4))
		size = (DMA_BUF_SIZE/4);

	//slog("sound: set data %d\n", size);
	uint32_t *pdata = (uint32_t *)_dma_data_addr;
	for (uint32_t i = 0; i < size; i++)
		*(pdata + i) = *(data + i); // Convert uint8_t to uint32_t
	proc_usleep(200);
	//slog("sound: data ready\n");

	_dma_cb->txfr_len = size * 4;						// They're unsigned ints now, not unsigned chars

	// Enable DMA
	volatile uint32_t *pwm = (uint32_t *)PWM_BASE;
	volatile uint32_t *dma = (uint32_t *)DMA_V_BASE;
	volatile uint32_t *dmae = (uint32_t *)DMA_ENABLE;

	*(pwm + BCM283x_PWM_DMAC) =
			BCM283x_PWM_ENAB + 0x0707; // Bits 0-7 Threshold For DREQ Signal = 1, Bits 8-15 Threshold For PANIC Signal = 0
	*dmae = DMA_EN0;
	*(dma + DMA_CONBLK_AD) = (long)_dma_cb; // checked and correct
	proc_usleep(200);
	*(dma + DMA_CS) = DMA_ACTIVE;

	//slog("sound: dma play cs:%d\n", (*(dma + DMA_CS)) & 0x1);
	while ((*(dma + DMA_CS)) & 0x1) // Wait for DMA transfer to finish - we could do anything here instead!
		proc_usleep(200);
	//slog("sound: dma played: %d, cs:%d\n", size, (*(dma + DMA_CS)) & 0x1);
}

/*
static void playaudio_cpu(uint8_t* data, uint32_t size) {
	volatile unsigned* pwm = (void*)PWM_BASE;
	uint32_t i=0;
	long status;

	// Write data out to FIFO

	while (i < size) {
		status = *(pwm + BCM283x_PWM_STATUS);
		if (!(status & BCM283x_FULL1)) {
			*(pwm+BCM283x_PWM_FIFO) = *(data + i);
			i++;
			*(pwm+BCM283x_PWM_FIFO) = *(data + i);
			i++;
		}
		if ((status & ERRORMASK)) {
			*(pwm+BCM283x_PWM_STATUS) = ERRORMASK;
		}
	}
}
*/

static int sound_write(int fd, int from_pid, fsinfo_t* node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)node;
	(void)from_pid;
	(void)offset;
	(void)p;

	//playaudio_cpu((uint8_t*)buf, size);
	playaudio_dma((uint8_t*)buf, size);
	return size;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/sound";
	bcm283x_gpio_init();
	audio_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "sound");
	dev.write = sound_write;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
