#include <arch/bcm283x/gpio.h>
#include <sys/vdevice.h>
#include <string.h>
#include <unistd.h>

#define PWM_BASE        (_mmio_base + 0x20C000) /* PWM0 register base address on RPi */
#define CLOCK_BASE      (_mmio_base + 0x101000)

#define BCM283x_PWMCLK_CNTL 40
#define BCM283x_PWMCLK_DIV  41
#define PM_PASSWORD 0x5A000000 

#define BCM283x_PWM_CONTROL 0
#define BCM283x_PWM_STATUS  1
#define BCM283x_PWM_DMAC    2
#define BCM283x_PWM0_RANGE  4
#define BCM283x_PWM0_DATA   5
#define BCM283x_PWM_FIFO    6

#define BCM283x_PWM0_USEFIFO  0x0020  /* Data from FIFO */
#define BCM283x_PWM0_ENABLE   0x0001  /* Channel enable */
#define BCM283x_PWM_ENAB      0x80000000  /* PWM DMA Configuration: DMA Enable (bit 31 set) */

#define BCM283x_GAPO2 0x20
#define BCM283x_GAPO1 0x10
#define BCM283x_RERR1 0x8
#define BCM283x_WERR1 0x4
#define BCM283x_FULL1 0x1
#define ERRORMASK (BCM283x_GAPO2 | BCM283x_GAPO1 | BCM283x_RERR1 | BCM283x_WERR1)

#define DMA_CS        0       /* Control/status register offset for DMA channel 0 */
#define DMA_CONBLK_AD 1
#define DMA_EN1       1 << 1  /* Enable DMA engine 1 */
#define DMA_ACTIVE    1       /* Active bit set */
#define DMA_DEST_DREQ 0x40    /* Use DREQ to pace peripheral writes */
#define DMA_SRC_INC   0x100   /* Increment source address */


static void audio_init(void) {   
	volatile unsigned* clk = (void*)CLOCK_BASE;
	volatile unsigned* pwm = (void*)PWM_BASE;
	bcm283x_gpio_config(18, GPIO_ALTF5); // Ensure PWM0 is mapped to GPIO 18
	usleep(2000);

	// Setup clock

	*(clk + BCM283x_PWMCLK_CNTL) = PM_PASSWORD | (1 << 5); // Stop clock
	usleep(2000);

	int idiv = 2;
	*(clk + BCM283x_PWMCLK_DIV)  = PM_PASSWORD | (idiv<<12);
	*(clk + BCM283x_PWMCLK_CNTL) = PM_PASSWORD | 16 | 1;  // Osc + Enable
	usleep(2000);

	// Setup PWM

	*(pwm + BCM283x_PWM_CONTROL) = 0;
	usleep(2000);

	*(pwm+BCM283x_PWM0_RANGE) = 0x264; // 44.1khz, Stereo, 8-bit (54Mhz / 44100 / 2)

	*(pwm+BCM283x_PWM_CONTROL) =
			BCM283x_PWM0_USEFIFO | 
			BCM283x_PWM0_ENABLE | 1<<6;
}

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

static int sound_write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)info;
	(void)from_pid;
	(void)offset;
	(void)p;

	playaudio_cpu((uint8_t*)buf, size);
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
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
