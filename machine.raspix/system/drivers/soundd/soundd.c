#include <arch/bcm283x/gpio.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <ewoksys/proto.h>
#include <sysinfo.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/dma.h>
#include <stdbool.h>

#define UNUSED(v) ((void)(v))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CTRL_PCM_DEV_HW 0xF0
#define CTRL_PCM_DEV_HW_FREE 0xF1
#define CTRL_PCM_DEV_PRPARE 0xF2
#define CTRL_PCM_BUF_AVAIL 0xF3

#define USE_PWM1

#if defined(USE_PWM1)
#define PWM_BASE        (_mmio_base + 0x20C000 + 0x800)
#define CLOCK_BASE      (_mmio_base + 0x101000 + 0x800)
#else
#define PWM_BASE        (_mmio_base + 0x20C000)
#define CLOCK_BASE      (_mmio_base + 0x101000)
#endif

static sys_info_t _sysinfo;

#define DMA_V_BASE      _sysinfo.sys_dma.v_base
#define DMA_ENABLE      (DMA_V_BASE + 0xFF0)

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
#define BCM283x_PWM_USEFIFO  0x2000
#define BCM283x_PWM_ENABLE   0x0100
#else
#define BCM283x_PWM_USEFIFO  0x0020
#define BCM283x_PWM_ENABLE   0x0001
#endif

#define BCM283x_PWM_ENAB      0x80000000

#define BCM283x_GAPO2 0x20
#define BCM283x_GAPO1 0x10
#define BCM283x_RERR1 0x8
#define BCM283x_WERR1 0x4
#define BCM283x_FULL1 0x1
#define ERRORMASK (BCM283x_GAPO2 | BCM283x_GAPO1 | BCM283x_RERR1 | BCM283x_WERR1)

#define DMA_CS        0
#define DMA_CONBLK_AD 1
#define DMA_EN0       1 << 0
#define DMA_ACTIVE    1
#define DMA_DEST_DREQ 0x40
#define DMA_SRC_INC   0x100

#ifdef USE_PWM1
#define DMA_PERMAP  0x10000
#else
#define DMA_PERMAP  0x0
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

struct pcm_config {
	int bit_depth;
	int rate;
	int channels;
	int period_size;
	int period_count;
	int start_threshold;
	int stop_threshold;
};

typedef struct {
	dma_cb_t* dma_cb;
	uint32_t dma_data_addr;
	uint32_t dma_cb_phy;
	uint32_t dma_data_addr_phy;
	struct pcm_config pcm_cfg;
	bool configured;
	bool prepared;
	bool started;
	int occupied_pid;
} snd_dev_t;

static snd_dev_t _snd = {0};

static void audio_deinit(void) {
	if (_snd.dma_cb != NULL) {
		dma_free(0, (ewokos_addr_t)_snd.dma_cb);
		_snd.dma_cb = NULL;
	}
	if (_snd.dma_data_addr != 0) {
		dma_free(0, _snd.dma_data_addr);
		_snd.dma_data_addr = 0;
	}
	_snd.configured = false;
	_snd.prepared = false;
	_snd.started = false;
}

static int audio_init_pcm(const struct pcm_config *cfg) {
	volatile uint32_t *pwm = (uint32_t *)PWM_BASE;
	volatile uint32_t *clk = (uint32_t *)CLOCK_BASE;

	uint32_t range_val;
	switch (cfg->rate) {
	case 8000:
		range_val = 6750;
		break;
	case 16000:
		range_val = 3375;
		break;
	case 44100:
		range_val = 1228;
		break;
	case 48000:
		range_val = 1125;
		break;
	default:
		range_val = 1228;
	}

	*(pwm + BCM283x_PWM_CONTROL) = 0;
	proc_usleep(2000);
	*(pwm + BCM283x_PWM_RANGE) = range_val;
	*(pwm + BCM283x_PWM_CONTROL) = BCM283x_PWM_USEFIFO | BCM283x_PWM_ENABLE | 1<<6;

	_snd.dma_cb = (dma_cb_t*)(dma_alloc(0, sizeof(dma_cb_t)));
	_snd.dma_data_addr = dma_alloc(0, DMA_BUF_SIZE);

	if (_snd.dma_cb == NULL || _snd.dma_data_addr == 0) {
		klog("sound: failed to allocate DMA buffer\n");
		audio_deinit();
		return -1;
	}

	_snd.dma_cb_phy = dma_phy_addr(0, (ewokos_addr_t)_snd.dma_cb);
	_snd.dma_data_addr_phy = dma_phy_addr(0, _snd.dma_data_addr);

	uint32_t phy_pwm_base = syscall1(SYS_V2P, PWM_BASE);
	_snd.dma_cb->ti = DMA_DEST_DREQ + DMA_PERMAP + DMA_SRC_INC;
	_snd.dma_cb->source_ad = _snd.dma_data_addr_phy;
	_snd.dma_cb->dest_ad = phy_pwm_base + 0x18;
	_snd.dma_cb->stride = 0x00;
	_snd.dma_cb->nextconbk = 0x00;
	_snd.dma_cb->null1 = 0x00;
	_snd.dma_cb->null2 = 0x00;

	_snd.configured = true;
	_snd.prepared = false;
	_snd.started = false;
	return 0;
}

static int audio_hw_params(const struct pcm_config *cfg) {
	if (cfg->bit_depth != 8 && cfg->bit_depth != 16) {
		klog("sound: unsupported bit depth: %d\n", cfg->bit_depth);
		return -1;
	}
	if (cfg->rate != 8000 && cfg->rate != 16000 && cfg->rate != 44100 && cfg->rate != 48000) {
		klog("sound: unsupported rate: %d\n", cfg->rate);
		return -1;
	}
	if (cfg->channels < 1 || cfg->channels > 2) {
		klog("sound: unsupported channels: %d\n", cfg->channels);
		return -1;
	}

	audio_deinit();

	memcpy(&_snd.pcm_cfg, cfg, sizeof(*cfg));
	return audio_init_pcm(cfg);
}

static int audio_prepare(void) {
	if (!_snd.configured) {
		return -1;
	}
	_snd.prepared = true;
	return 0;
}

static int audio_start(void) {
	if (!_snd.prepared) {
		return -1;
	}
	_snd.started = true;
	return 0;
}

static int audio_stop(void) {
	if (_snd.started) {
		volatile uint32_t *pwm = (uint32_t *)PWM_BASE;
		*(pwm + BCM283x_PWM_CONTROL) = 0;
		_snd.started = false;
	}
	return 0;
}

static int sound_open(int fd, int from_pid, fsinfo_t *info, int oflag, void *p) {
	UNUSED(fd);
	UNUSED(from_pid);
	UNUSED(info);
	UNUSED(oflag);
	UNUSED(p);

	_snd.occupied_pid = proc_getpid(from_pid);
	return 0;
}

static int sound_close(int fd, int from_pid, uint32_t node, fsinfo_t *info, void *p) {
	UNUSED(fd);
	UNUSED(node);
	UNUSED(info);
	UNUSED(p);

	if (_snd.occupied_pid != proc_getpid(from_pid)) {
		return -1;
	}

	audio_stop();
	audio_deinit();
	_snd.occupied_pid = 0;
	return 0;
}

static int sound_write(int fd, int from_pid, fsinfo_t *node,
					   const void *buf, int size, int offset, void *p) {
	UNUSED(fd);
	UNUSED(node);
	UNUSED(p);

	if (!_snd.configured || size <= 0 || _snd.occupied_pid != proc_getpid(from_pid)) {
		return -1;
	}

	if (!_snd.prepared) {
		if (audio_prepare() != 0) {
			return -1;
		}
	}
	if (!_snd.started) {
		if (audio_start() != 0) {
			return -1;
		}
	}

	uint32_t max_size = DMA_BUF_SIZE / 4;
	if ((uint32_t)size > max_size) {
		size = max_size;
	}

	uint32_t *pdata = (uint32_t *)_snd.dma_data_addr;
	for (int i = 0; i < size; i++) {
		*(pdata + i) = *((uint8_t*)buf + offset + i);
	}

	_snd.dma_cb->txfr_len = (uint32_t)size * 4;

	volatile uint32_t *pwm = (uint32_t *)PWM_BASE;
	volatile uint32_t *dma = (uint32_t *)DMA_V_BASE;
	volatile uint32_t *dmae = (uint32_t *)DMA_ENABLE;

	*(pwm + BCM283x_PWM_DMAC) = BCM283x_PWM_ENAB + 0x0707;
	*dmae = DMA_EN0;
	*(dma + DMA_CONBLK_AD) = _snd.dma_cb_phy;
	proc_usleep(200);
	*(dma + DMA_CS) = DMA_ACTIVE;

	while ((*(dma + DMA_CS)) & 0x1) {
		proc_usleep(200);
	}

	return size;
}

static int sound_dev_cntl(int from_pid, int cmd, proto_t *in, proto_t *ret, void *p) {
	UNUSED(p);

	int result = 0;
	struct pcm_config cfg;

	if (_snd.occupied_pid != proc_getpid(from_pid)) {
		return -1;
	}

	switch (cmd) {
	case CTRL_PCM_DEV_HW:
		memset(&cfg, 0, sizeof(cfg));
		proto_read_to(in, &cfg, sizeof(cfg));
		result = audio_hw_params(&cfg);
		break;
	case CTRL_PCM_DEV_HW_FREE:
		audio_stop();
		audio_deinit();
		result = 0;
		break;
	case CTRL_PCM_DEV_PRPARE:
		result = audio_prepare();
		break;
	default:
		result = -1;
		break;
	}

	PF->addi(ret, result);
	return 0;
}

static void audio_hw_init(void) {
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&_sysinfo);

	volatile uint32_t* clk = (void*)CLOCK_BASE;

#ifdef USE_PWM1
	bcm283x_gpio_config(40, GPIO_ALTF5);
	bcm283x_gpio_config(41, GPIO_ALTF5);
#else
	bcm283x_gpio_config(18, GPIO_ALTF5);
#endif

	proc_usleep(2000);

	*(clk + BCM283x_PWMCLK_CNTL) = PM_PASSWORD | (1 << 5);
	proc_usleep(2000);

	int idiv = 2;
	*(clk + BCM283x_PWMCLK_DIV)  = PM_PASSWORD | (idiv<<12);
	*(clk + BCM283x_PWMCLK_CNTL) = PM_PASSWORD | 16 | 1;
	proc_usleep(2000);
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1] : "/dev/sound0";

	_mmio_base = mmio_map();
	bcm283x_gpio_init();
	audio_hw_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "bcm283x-snd");
	dev.open = sound_open;
	dev.close = sound_close;
	dev.write = sound_write;
	dev.dev_cntl = sound_dev_cntl;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
