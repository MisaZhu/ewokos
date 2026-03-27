
#include <stdint.h>
#include <bcm283x/gpio.h>
#include <bcm283x/mmc.h>

#include <string.h>


#define SDHCI_CMD_MAX_TIMEOUT			3200
#define SDHCI_CMD_DEFAULT_TIMEOUT		10000
#define SDHCI_READ_STATUS_TIMEOUT		1000

#define SDHCI_DMA_ADDRESS	0x00
#define SDHCI_BLOCK_SIZE	0x04
#define  SDHCI_MAKE_BLKSZ(dma, blksz) (((dma & 0x7) << 12) | (blksz & 0xFFF))
#define SDHCI_BLOCK_COUNT	0x06
#define SDHCI_ARGUMENT		0x08
#define SDHCI_TRANSFER_MODE	0x0C
#define  SDHCI_TRNS_DMA		BIT(0)
#define  SDHCI_TRNS_BLK_CNT_EN	BIT(1)
#define  SDHCI_TRNS_ACMD12	BIT(2)
#define  SDHCI_TRNS_READ	BIT(4)
#define  SDHCI_TRNS_MULTI	BIT(5)

#define SDHCI_COMMAND		0x0E
#define  SDHCI_CMD_RESP_MASK	0x03
#define  SDHCI_CMD_CRC		0x08
#define  SDHCI_CMD_INDEX	0x10
#define  SDHCI_CMD_DATA		0x20
#define  SDHCI_CMD_ABORTCMD	0xC0

#define  SDHCI_CMD_RESP_NONE	0x00
#define  SDHCI_CMD_RESP_LONG	0x01
#define  SDHCI_CMD_RESP_SHORT	0x02
#define  SDHCI_CMD_RESP_SHORT_BUSY 0x03

#define SDHCI_MAKE_CMD(c, f) (((c & 0xff) << 8) | (f & 0xff))
#define SDHCI_GET_CMD(c) ((c>>8) & 0x3f)

#define SDHCI_RESPONSE		0x10
#define SDHCI_BUFFER		0x20

#define SDHCI_PRESENT_STATE	0x24
#define  SDHCI_CMD_INHIBIT	BIT(0)
#define  SDHCI_DATA_INHIBIT	BIT(1)
#define  SDHCI_DAT_ACTIVE	BIT(2)
#define  SDHCI_DOING_WRITE	BIT(8)
#define  SDHCI_DOING_READ	BIT(9)
#define  SDHCI_SPACE_AVAILABLE	BIT(10)
#define  SDHCI_DATA_AVAILABLE	BIT(11)
#define  SDHCI_CARD_PRESENT	BIT(16)
#define  SDHCI_CARD_STATE_STABLE	BIT(17)
#define  SDHCI_CARD_DETECT_PIN_LEVEL	BIT(18)
#define  SDHCI_WRITE_PROTECT	BIT(19)
#define  SDHCI_DATA_LVL_MASK	0x00F00000
#define   SDHCI_DATA_0_LVL_MASK BIT(20)

#define SDHCI_HOST_CONTROL	0x28
#define  SDHCI_CTRL_LED		BIT(0)
#define  SDHCI_CTRL_4BITBUS	BIT(1)
#define  SDHCI_CTRL_HISPD	BIT(2)
#define  SDHCI_CTRL_DMA_MASK	0x18
#define   SDHCI_CTRL_SDMA	0x00
#define   SDHCI_CTRL_ADMA1	0x08
#define   SDHCI_CTRL_ADMA32	0x10
#define   SDHCI_CTRL_ADMA64	0x18
#define  SDHCI_CTRL_8BITBUS	BIT(5)
#define  SDHCI_CTRL_CD_TEST_INS	BIT(6)
#define  SDHCI_CTRL_CD_TEST	BIT(7)

#define SDHCI_POWER_CONTROL	0x29
#define  SDHCI_POWER_ON		0x01
#define  SDHCI_POWER_180	0x0A
#define  SDHCI_POWER_300	0x0C
#define  SDHCI_POWER_330	0x0E

#define SDHCI_BLOCK_GAP_CONTROL	0x2A

#define SDHCI_WAKE_UP_CONTROL	0x2B
#define  SDHCI_WAKE_ON_INT	BIT(0)
#define  SDHCI_WAKE_ON_INSERT	BIT(1)
#define  SDHCI_WAKE_ON_REMOVE	BIT(2)

#define SDHCI_CLOCK_CONTROL	0x2C
#define  SDHCI_DIVIDER_SHIFT	8
#define  SDHCI_DIVIDER_HI_SHIFT	6
#define  SDHCI_DIV_MASK	0xFF
#define  SDHCI_DIV_MASK_LEN	8
#define  SDHCI_DIV_HI_MASK	0x300
#define  SDHCI_PROG_CLOCK_MODE  BIT(5)
#define  SDHCI_CLOCK_CARD_EN	BIT(2)
#define  SDHCI_CLOCK_INT_STABLE	BIT(1)
#define  SDHCI_CLOCK_INT_EN	BIT(0)

#define SDHCI_TIMEOUT_CONTROL	0x2E

#define SDHCI_SOFTWARE_RESET	0x2F
#define  SDHCI_RESET_ALL	0x01
#define  SDHCI_RESET_CMD	0x02
#define  SDHCI_RESET_DATA	0x04

#define SDHCI_INT_STATUS	0x30
#define SDHCI_INT_ENABLE	0x34
#define SDHCI_SIGNAL_ENABLE	0x38
#define  SDHCI_INT_RESPONSE	BIT(0)
#define  SDHCI_INT_DATA_END	BIT(1)
#define  SDHCI_INT_DMA_END	BIT(3)
#define  SDHCI_INT_SPACE_AVAIL	BIT(4)
#define  SDHCI_INT_DATA_AVAIL	BIT(5)
#define  SDHCI_INT_CARD_INSERT	BIT(6)
#define  SDHCI_INT_CARD_REMOVE	BIT(7)
#define  SDHCI_INT_CARD_INT	BIT(8)
#define  SDHCI_INT_ERROR	BIT(15)
#define  SDHCI_INT_TIMEOUT	BIT(16)
#define  SDHCI_INT_CRC		BIT(17)
#define  SDHCI_INT_END_BIT	BIT(18)
#define  SDHCI_INT_INDEX	BIT(19)
#define  SDHCI_INT_DATA_TIMEOUT	BIT(20)
#define  SDHCI_INT_DATA_CRC	BIT(21)
#define  SDHCI_INT_DATA_END_BIT	BIT(22)
#define  SDHCI_INT_BUS_POWER	BIT(23)
#define  SDHCI_INT_ACMD12ERR	BIT(24)
#define  SDHCI_INT_ADMA_ERROR	BIT(25)

#define  SDHCI_INT_NORMAL_MASK	0x00007FFF
#define  SDHCI_INT_ERROR_MASK	0xFFFF8000

#define  SDHCI_INT_CMD_MASK	(SDHCI_INT_RESPONSE | SDHCI_INT_TIMEOUT | \
		SDHCI_INT_CRC | SDHCI_INT_END_BIT | SDHCI_INT_INDEX)
#define  SDHCI_INT_DATA_MASK	(SDHCI_INT_DATA_END | SDHCI_INT_DMA_END | \
		SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL | \
		SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_DATA_CRC | \
		SDHCI_INT_DATA_END_BIT | SDHCI_INT_ADMA_ERROR)
#define SDHCI_INT_ALL_MASK	((unsigned int)-1)

#define SDHCI_ACMD12_ERR	0x3C
#define SDHCI_AUTO_CMD_STATUS	0x3C

#define SDHCI_HOST_CONTROL2	0x3E
#define  SDHCI_CTRL_UHS_MASK	0x0007
#define  SDHCI_CTRL_UHS_SDR12	0x0000
#define  SDHCI_CTRL_UHS_SDR25	0x0001
#define  SDHCI_CTRL_UHS_SDR50	0x0002
#define  SDHCI_CTRL_UHS_SDR104	0x0003
#define  SDHCI_CTRL_UHS_DDR50	0x0004
#define  SDHCI_CTRL_HS400	0x0005 /* Non-standard */
#define  SDHCI_CTRL_VDD_180	0x0008
#define  SDHCI_CTRL_DRV_TYPE_MASK	0x0030
#define  SDHCI_CTRL_DRV_TYPE_B	0x0000
#define  SDHCI_CTRL_DRV_TYPE_A	0x0010
#define  SDHCI_CTRL_DRV_TYPE_C	0x0020
#define  SDHCI_CTRL_DRV_TYPE_D	0x0030
#define  SDHCI_CTRL_EXEC_TUNING	0x0040
#define  SDHCI_CTRL_TUNED_CLK	0x0080
#define  SDHCI_CTRL_PRESET_VAL_ENABLE	0x8000

#define SDHCI_CAPABILITIES	0x40
#define  SDHCI_TIMEOUT_CLK_MASK	0x0000003F
#define  SDHCI_TIMEOUT_CLK_SHIFT 0
#define  SDHCI_TIMEOUT_CLK_UNIT	0x00000080
#define  SDHCI_CLOCK_BASE_MASK	0x00003F00
#define  SDHCI_CLOCK_V3_BASE_MASK	0x0000FF00
#define  SDHCI_CLOCK_BASE_SHIFT	8
#define  SDHCI_MAX_BLOCK_MASK	0x00030000
#define  SDHCI_MAX_BLOCK_SHIFT  16
#define  SDHCI_CAN_DO_8BIT	BIT(18)
#define  SDHCI_CAN_DO_ADMA2	BIT(19)
#define  SDHCI_CAN_DO_ADMA1	BIT(20)
#define  SDHCI_CAN_DO_HISPD	BIT(21)
#define  SDHCI_CAN_DO_SDMA	BIT(22)
#define  SDHCI_CAN_VDD_330	BIT(24)
#define  SDHCI_CAN_VDD_300	BIT(25)
#define  SDHCI_CAN_VDD_180	BIT(26)
#define  SDHCI_CAN_64BIT	BIT(28)

#define SDHCI_CAPABILITIES_1	0x44
#define  SDHCI_SUPPORT_SDR50	0x00000001
#define  SDHCI_SUPPORT_SDR104	0x00000002
#define  SDHCI_SUPPORT_DDR50	0x00000004
#define  SDHCI_SUPPORT_HS400	BIT(31)
#define  SDHCI_USE_SDR50_TUNING	0x00002000

#define  SDHCI_CLOCK_MUL_MASK	0x00FF0000
#define  SDHCI_CLOCK_MUL_SHIFT	16

#define SDHCI_MAX_CURRENT	0x48

/* 4C-4F reserved for more max current */

#define SDHCI_SET_ACMD12_ERROR	0x50
#define SDHCI_SET_INT_ERROR	0x52

#define SDHCI_ADMA_ERROR	0x54

/* 55-57 reserved */

#define SDHCI_ADMA_ADDRESS	0x58
#define SDHCI_ADMA_ADDRESS_HI	0x5c

/* 60-FB reserved */

#define SDHCI_SLOT_INT_STATUS	0xFC

#define SDHCI_HOST_VERSION	0xFE
#define  SDHCI_VENDOR_VER_MASK	0xFF00
#define  SDHCI_VENDOR_VER_SHIFT	8
#define  SDHCI_SPEC_VER_MASK	0x00FF
#define  SDHCI_SPEC_VER_SHIFT	0
#define   SDHCI_SPEC_100	0
#define   SDHCI_SPEC_200	1
#define   SDHCI_SPEC_300	2


#define SDHCI_MAX_DIV_SPEC_200	256
#define SDHCI_MAX_DIV_SPEC_300	2046
#define SDHCI_QUIRK_32BIT_DMA_ADDR	(1 << 0)
#define SDHCI_QUIRK_REG32_RW		(1 << 1)
#define SDHCI_QUIRK_BROKEN_R1B		(1 << 2)
#define SDHCI_QUIRK_NO_HISPD_BIT	(1 << 3)
#define SDHCI_QUIRK_BROKEN_VOLTAGE	(1 << 4)
#define SDHCI_QUIRK_BROKEN_HISPD_MODE	BIT(5)
#define SDHCI_QUIRK_WAIT_SEND_CMD	(1 << 6)
#define SDHCI_QUIRK_USE_WIDE8		(1 << 8)
#define SDHCI_QUIRK_NO_1_8_V		(1 << 9)
#define SDHCI_QUIRK_SUPPORT_SINGLE	(1 << 10)
#define SDHCI_QUIRK_CAPS_BIT63_FOR_HS400	BIT(11)


#define SDHCI_DEFAULT_BOUNDARY_ARG	(7)

#define SDHCI_GET_VERSION(x) (x->version & SDHCI_SPEC_VER_MASK)

#define readl(addr) (*((volatile uint32_t *)(addr)))
#define writel(val, addr) (*((volatile uint32_t *)(addr)) = (uint32_t)(val))

static uint64_t get_timer(uint64_t start){
    extern uint64_t timer_read_sys_usec(void);
    return timer_read_sys_usec() / 1000 - start;
}

static inline uint16_t readw(uint32_t *reg)
{
    uint32_t val = readl(((uint32_t)reg & ~3));
    uint32_t word_num = ((uint32_t)reg >> 1) & 1;
    uint32_t word_shift = word_num * 16;
    uint32_t word = (val >> word_shift) & 0xffff;

    return word;
}

static inline uint8_t readb(uint32_t *reg)
{
    uint32_t val = readl(((uint32_t)reg & ~3));
    uint32_t byte_num = (uint32_t)reg & 3;
    uint32_t byte_shift = byte_num * 8;
    uint32_t byte = (val >> byte_shift) & 0xff;

    return byte;
}

struct sdhci_host {
	const char *name;
	uint8_t *ioaddr;
	unsigned int quirks;
	unsigned int host_caps;
	unsigned int version;
	unsigned int max_clk;   /* Maximum Base Clock frequency */
	unsigned int clk_mul;   /* Clock Multiplier value */
	unsigned int clock;
	struct mmc *mmc;
	const struct sdhci_ops *ops;
	int index;

	int bus_width;
	int pwr_gpio;	/* Power GPIO */
	int cd_gpio;		/* Card Detect GPIO */

	unsigned int	voltages;

	struct mmc_config cfg;
	void *align_buffer;
	bool force_align_buffer;
	void* start_addr;
	int flags;
#define USE_SDMA	(0x1 << 0)
#define USE_ADMA	(0x1 << 1)
#define USE_ADMA64	(0x1 << 2)
#define USE_DMA		(USE_SDMA | USE_ADMA | USE_ADMA64)
	unsigned int twoticks_delay;
	unsigned long last_write;
};

static struct sdhci_host _host;

static void bcm283x_sdhci_gpio_init(void){
    bcm283x_gpio_init();


	bcm283x_gpio_config(43, GPIO_ALTF0);  //32k clock
	_delay_usec(20000);

	//init sdio interface
    bcm283x_gpio_config(34, GPIO_ALTF3);  //clock
    bcm283x_gpio_config(35, GPIO_ALTF3);  //cmd
    bcm283x_gpio_config(36, GPIO_ALTF3);
    bcm283x_gpio_config(37, GPIO_ALTF3);
    bcm283x_gpio_config(38, GPIO_ALTF3);
    bcm283x_gpio_config(39, GPIO_ALTF3);

    bcm283x_gpio_pull(34, bcm283x_gpio_pull_NONE);
    bcm283x_gpio_pull(35, bcm283x_gpio_pull_UP);
    bcm283x_gpio_pull(36, bcm283x_gpio_pull_UP);
    bcm283x_gpio_pull(37, bcm283x_gpio_pull_UP);
    bcm283x_gpio_pull(38, bcm283x_gpio_pull_UP);
    bcm283x_gpio_pull(39, bcm283x_gpio_pull_UP);

	// bcm283x_gpio_config(48, GPIO_ALTF3);
    // bcm283x_gpio_config(49, GPIO_ALTF3);
    // bcm283x_gpio_config(50, GPIO_ALTF3);
    // bcm283x_gpio_config(51, GPIO_ALTF3);
    // bcm283x_gpio_config(52, GPIO_ALTF3);
    // bcm283x_gpio_config(53, GPIO_ALTF3);
}

static void dump(struct sdhci_host *host)
{
    printf( ": =========== REGISTER DUMP ===========\n");

    printf( ": Sys addr: 0x%08x | Version:  0x%08x\n",
        readl(host->ioaddr + SDHCI_DMA_ADDRESS),
        readw(host->ioaddr + SDHCI_HOST_VERSION));
    printf( ": Blk size: 0x%08x | Blk cnt:  0x%08x\n",
        readw(host->ioaddr + SDHCI_BLOCK_SIZE),
        readw(host->ioaddr + SDHCI_BLOCK_COUNT));
    printf( ": Argument: 0x%08x | Trn mode: 0x%08x\n",
        readl(host->ioaddr + SDHCI_ARGUMENT),
        readw(host->ioaddr + SDHCI_TRANSFER_MODE));
    printf( ": Present:  0x%08x | Host ctl: 0x%08x\n",
        readl(host->ioaddr + SDHCI_PRESENT_STATE),
        readb(host->ioaddr + SDHCI_HOST_CONTROL));
    printf( ": Power:    0x%08x | Blk gap:  0x%08x\n",
        readb(host->ioaddr + SDHCI_POWER_CONTROL),
        readb(host->ioaddr + SDHCI_BLOCK_GAP_CONTROL));
    printf( ": Wake-up:  0x%08x | Clock:    0x%08x\n",
        readb(host->ioaddr + SDHCI_WAKE_UP_CONTROL),
        readw(host->ioaddr + SDHCI_CLOCK_CONTROL));
    printf( ": Timeout:  0x%08x | Int stat: 0x%08x\n",
        readb(host->ioaddr + SDHCI_TIMEOUT_CONTROL),
        readl(host->ioaddr + SDHCI_INT_STATUS));
    printf( ": Int enab: 0x%08x | Sig enab: 0x%08x\n",
        readl(host->ioaddr + SDHCI_INT_ENABLE),
        readl(host->ioaddr + SDHCI_SIGNAL_ENABLE));
    printf( ": AC12 err: 0x%08x | Slot int: 0x%08x\n",
        readw(host->ioaddr + SDHCI_AUTO_CMD_STATUS),
        readw(host->ioaddr + SDHCI_SLOT_INT_STATUS));
    printf( ": Caps:     0x%08x | Caps_1:   0x%08x\n",
        readl(host->ioaddr + SDHCI_CAPABILITIES),
        readl(host->ioaddr + SDHCI_CAPABILITIES_1));
    printf( ": Cmd:      0x%08x | Max curr: 0x%08x\n",
        readw(host->ioaddr + SDHCI_COMMAND),
        readl(host->ioaddr + SDHCI_MAX_CURRENT));
    printf( ": Host ctl2: 0x%08x\n",
        readw(host->ioaddr + SDHCI_HOST_CONTROL2));

    printf( ": ===========================================\n");
}

#define BCM2835_SDHCI_WRITE_DELAY(f)    (((2 * 1000000) / f) + 1)
static inline void bcm2835_sdhci_raw_writel(struct sdhci_host *host, uint32_t val,
					    int reg)
{
	volatile int delay = 20; 
	writel(val, host->ioaddr + reg);
	while(delay--);
}

static inline uint32_t bcm2835_sdhci_raw_readl(struct sdhci_host *host, int reg)
{
	return readl(host->ioaddr + reg);
}


static void sdhci_writeb(struct sdhci_host *host, uint8_t val, uint32_t reg){
	uint32_t oldval = bcm2835_sdhci_raw_readl(host, reg & ~3);
	uint32_t byte_num = reg & 3;
	uint32_t byte_shift = byte_num * 8;
	uint32_t mask = 0xff << byte_shift;
	uint32_t newval = (oldval & ~mask) | (val << byte_shift);

	bcm2835_sdhci_raw_writel(&_host, newval, reg & ~3);
}

static uint8_t sdhci_readb(struct sdhci_host *host, uint32_t reg){
	uint32_t val = bcm2835_sdhci_raw_readl(host, (reg & ~3));
	uint32_t byte_num = reg & 3;
	uint32_t byte_shift = byte_num * 8;
	uint32_t byte = (val >> byte_shift) & 0xff;

	return byte;
}

static void sdhci_writew(struct sdhci_host *host, uint16_t val, uint32_t reg){
	static uint32_t shadow;
	uint32_t oldval = (reg == SDHCI_COMMAND) ? shadow :
		bcm2835_sdhci_raw_readl(host, reg & ~3);
	uint32_t word_num = (reg >> 1) & 1;
	uint32_t word_shift = word_num * 16;
	uint32_t mask = 0xffff << word_shift;
	uint32_t newval = (oldval & ~mask) | (val << word_shift);

	if (reg == SDHCI_TRANSFER_MODE)
		shadow = newval;
	else
		bcm2835_sdhci_raw_writel(host, newval, reg & ~3);
}

static uint16_t sdhci_readw(struct sdhci_host *host, uint32_t reg){
	uint32_t val = bcm2835_sdhci_raw_readl(host, (reg & ~3));
	uint32_t word_num = (reg >> 1) & 1;
	uint32_t word_shift = word_num * 16;
	uint32_t word = (val >> word_shift) & 0xffff;

	return word;
}

static void sdhci_writel(struct sdhci_host *host, uint32_t val, uint32_t reg){
	bcm2835_sdhci_raw_writel(host, val, reg);
}

static int32_t sdhci_readl(struct sdhci_host *host, uint32_t reg){
	uint32_t val = bcm2835_sdhci_raw_readl(host, reg);
	return val;
}


static void sdhci_set_power(struct sdhci_host *host, uint32_t power)
{
	uint8_t pwr = 0;
	switch (power) {
	case MMC_VDD_165_195:
		pwr = SDHCI_POWER_180;
		break;
	case MMC_VDD_29_30:
	case MMC_VDD_30_31:
		pwr = SDHCI_POWER_300;
		break;
	case MMC_VDD_32_33:
	case MMC_VDD_33_34:
		pwr = SDHCI_POWER_330;
		break;
	}

	if (pwr == 0) {
		sdhci_writeb(host, 0, SDHCI_POWER_CONTROL);
		return;
	}

	pwr |= SDHCI_POWER_ON;

	sdhci_writeb(host, pwr, SDHCI_POWER_CONTROL);
}

int sdhci_set_clock(struct sdhci_host *host , unsigned int clock)
{
	unsigned int div, clk = 0, timeout;

	/* Wait max 20 ms */
	timeout = 200;
	while (sdhci_readl(host, SDHCI_PRESENT_STATE) &
			   (SDHCI_CMD_INHIBIT | SDHCI_DATA_INHIBIT)) {
		if (timeout == 0) {
			printf("%s: Timeout to wait cmd & data inhibit\n",
			       __func__);
			return -EBUSY;
		}

		timeout--;
		_delay_usec(100);
	}

	sdhci_writew(host, 0, SDHCI_CLOCK_CONTROL);

	if (clock == 0)
		return 0;

	if (SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300) {
		/*
		 * Check if the Host Controller supports Programmable Clock
		 * Mode.
		 */
		if (host->clk_mul) {
			for (div = 1; div <= 1024; div++) {
				if ((host->max_clk / div) <= clock)
					break;
			}

			/*
			 * Set Programmable Clock Mode in the Clock
			 * Control register.
			 */
			clk = SDHCI_PROG_CLOCK_MODE;
			div--;
		} else {
			/* Version 3.00 divisors must be a multiple of 2. */
			if (host->max_clk <= clock) {
				div = 1;
			} else {
				for (div = 2;
				     div < SDHCI_MAX_DIV_SPEC_300;
				     div += 2) {
					if ((host->max_clk / div) <= clock)
						break;
				}
			}
			div <<= 1;
		}
	} else {
		/* Version 2.00 divisors must be a power of 2. */
		for (div = 1; div < SDHCI_MAX_DIV_SPEC_200; div *= 2) {
			if ((host->max_clk / div) <= clock)
				break;
		}
		div >>= 1;
	}


	clk |= (div & SDHCI_DIV_MASK) << SDHCI_DIVIDER_SHIFT;
	clk |= ((div & SDHCI_DIV_HI_MASK) >> SDHCI_DIV_MASK_LEN)
		<< SDHCI_DIVIDER_HI_SHIFT;
	clk |= SDHCI_CLOCK_INT_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);

	/* Wait max 20 ms */
	timeout = 20;
	while (!((clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL))
		& SDHCI_CLOCK_INT_STABLE)) {
		if (timeout == 0) {
			printf("%s: Internal clock never stabilised.\n",
			       __func__);
			return -EBUSY;
		}
		timeout--;
		_delay_usec(1000);
	}

	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
	return 0;
}

static void sdhci_set_bus_width(struct sdhci_host *host, int bus_width){
	/* Set bus width */
	uint8_t ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);
	if (bus_width == 8) {
		ctrl &= ~SDHCI_CTRL_4BITBUS;
		if ((SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300) ||
				(host->quirks & SDHCI_QUIRK_USE_WIDE8))
			ctrl |= SDHCI_CTRL_8BITBUS;
	} else {
		if ((SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300) ||
				(host->quirks & SDHCI_QUIRK_USE_WIDE8))
			ctrl &= ~SDHCI_CTRL_8BITBUS;
		if (bus_width == 4)
			ctrl |= SDHCI_CTRL_4BITBUS;
		else
			ctrl &= ~SDHCI_CTRL_4BITBUS;
	}
	sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);
}

static void sdhci_set_select_mode(struct sdhci_host *host, int mode){
	bool no_hispd_bit = false;
	/* Set bus width */
	uint8_t ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);

	if ((host->quirks & SDHCI_QUIRK_NO_HISPD_BIT) ||
	    (host->quirks & SDHCI_QUIRK_BROKEN_HISPD_MODE)) {
		ctrl &= ~SDHCI_CTRL_HISPD;
		no_hispd_bit = true;
	}

	if (!no_hispd_bit) {
		if (mode == MMC_HS ||
		    mode == SD_HS ||
		    mode == MMC_HS_52 ||
		    mode == MMC_DDR_52 ||
		    mode == MMC_HS_200 ||
		    mode == MMC_HS_400 ||
		    mode == MMC_HS_400_ES ||
		    mode == UHS_SDR25 ||
		    mode == UHS_SDR50 ||
		    mode == UHS_SDR104 ||
		    mode == UHS_DDR50)
			ctrl |= SDHCI_CTRL_HISPD;
		else
			ctrl &= ~SDHCI_CTRL_HISPD;
	}
	sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);
}

static int sdhci_get_info(struct sdhci_host *host)
{
	uint32_t caps, caps_1 = 0;
	caps = sdhci_readl(host, SDHCI_CAPABILITIES);
	host->version = sdhci_readw(host, SDHCI_HOST_VERSION);

	/* Check whether the clock multiplier is supported or not */
	if (SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300) {
		caps_1 = sdhci_readl(host, SDHCI_CAPABILITIES_1);
		host->clk_mul = (caps_1 & SDHCI_CLOCK_MUL_MASK) >>
				SDHCI_CLOCK_MUL_SHIFT;
	}

	if (host->max_clk == 0) {
		if (SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300)
			host->max_clk = (caps & SDHCI_CLOCK_V3_BASE_MASK) >>
				SDHCI_CLOCK_BASE_SHIFT;
		else
			host->max_clk = (caps & SDHCI_CLOCK_BASE_MASK) >>
				SDHCI_CLOCK_BASE_SHIFT;
		host->max_clk *= 1000000;
		if (host->clk_mul)
			host->max_clk *= host->clk_mul;
	}
	if (host->max_clk == 0) {
		printf("%s: Hardware doesn't specify base clock frequency\n",
		       __func__);
		return -EINVAL;
	}

	return 0;
}


static void sdhci_set_uhs_timing(struct sdhci_host *host, uint32_t mode)
{
	uint32_t reg;

	reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	reg &= ~SDHCI_CTRL_UHS_MASK;

	switch (mode) {
	case UHS_SDR25:
	case MMC_HS:
		reg |= SDHCI_CTRL_UHS_SDR25;
		break;
	case UHS_SDR50:
	case MMC_HS_52:
		reg |= SDHCI_CTRL_UHS_SDR50;
		break;
	case UHS_DDR50:
	case MMC_DDR_52:
		reg |= SDHCI_CTRL_UHS_DDR50;
		break;
	case UHS_SDR104:
	case MMC_HS_200:
		reg |= SDHCI_CTRL_UHS_SDR104;
		break;
	case MMC_HS_400:
	case MMC_HS_400_ES:
		reg |= SDHCI_CTRL_HS400;
		break;
	default:
		reg |= SDHCI_CTRL_UHS_SDR12;
	}

	sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);
}


/****************************************************************************/

void sdhci_reset(uint8_t mask)
{
	unsigned long timeout;

	/* Wait max 100 ms */
	timeout = 100;
	sdhci_writeb(&_host, mask, SDHCI_SOFTWARE_RESET);
	while (sdhci_readb(&_host, SDHCI_SOFTWARE_RESET) & mask) {
		if (timeout == 0) {
			printf("%s: Reset 0x%x never completed.\n",
			       __func__, (int)mask);
			return;
		}
		timeout--;
		_delay_usec(1000);
	}
}

void sdhci_enable_irq(int enable)
{
	uint32_t ier = sdhci_readl(&_host, SDHCI_INT_ENABLE);
    if (enable)
        ier |= SDHCI_INT_CARD_INT;
    else
        ier &= ~SDHCI_INT_CARD_INT;

    sdhci_writel(&_host, ier, SDHCI_INT_ENABLE);
    sdhci_writel(&_host, ier, SDHCI_SIGNAL_ENABLE);
}


static void sdhci_transfer_pio(struct sdhci_host *host, struct mmc_data *data)
{
	uint32_t i;
	uint8_t *offs;
	for (i = 0; i < data->blocksize; i += 4) {
		offs = data->dest + i;
		if (data->flags == MMC_DATA_READ)
			*(uint32_t *)offs = sdhci_readl(host, SDHCI_BUFFER);
		else
			sdhci_writel(host, *(uint32_t *)offs, SDHCI_BUFFER);
	}
}

static int sdhci_transfer_data(struct sdhci_host *host, struct mmc_data *data)
{
	unsigned int stat, rdy, mask, timeout, block = 0;
	bool transfer_done = false;

	timeout = 100000;
	rdy = SDHCI_INT_SPACE_AVAIL | SDHCI_INT_DATA_AVAIL;
	mask = SDHCI_DATA_AVAILABLE | SDHCI_SPACE_AVAILABLE;
	do {
		stat = sdhci_readl(host, SDHCI_INT_STATUS);
		if (stat & SDHCI_INT_ERROR) {
			printf("%s: Error detected in status(0x%X)! %d\n",
				 __func__, stat, timeout);
			return -EIO;
		}
		if (!transfer_done && (stat & rdy)) {
			if (!(sdhci_readl(host, SDHCI_PRESENT_STATE) & mask))
				continue;
			sdhci_writel(host, rdy, SDHCI_INT_STATUS);
			sdhci_transfer_pio(host, data);
			data->dest += data->blocksize;
			if (++block >= data->blocks) {
				/* Keep looping until the SDHCI_INT_DATA_END is
				 * cleared, even if we finished sending all the
				 * blocks.
				 */
				transfer_done = true;
				continue;
			}
		}
		if (timeout-- == 0){
			printf("%s: Transfer data timeout\n", __func__);
			return -ETIMEDOUT;
		}
	} while (!(stat & SDHCI_INT_DATA_END));

	return 0;
}

static void sdhci_cmd_done(struct sdhci_host *host, struct mmc_cmd *cmd)
{
	int i;
	if (cmd->resp_type & MMC_RSP_136) {
		/* CRC is stripped so we need to do some shifting. */
		for (i = 0; i < 4; i++) {
			cmd->response[i] = sdhci_readl(host,
					SDHCI_RESPONSE + (3-i)*4) << 8;
			if (i != 3)
				cmd->response[i] |= sdhci_readb(host,
						SDHCI_RESPONSE + (3-i)*4-1);
		}
	} else {
		cmd->response[0] = sdhci_readl(host, SDHCI_RESPONSE);
	}
}

static int sdhci_send_command(struct mmc_cmd *cmd, struct mmc_data *data)
{
	struct sdhci_host *host = &_host;
	unsigned int stat = 0;
	int ret = 0;
	int trans_bytes = 0, is_aligned = 1;
	uint32_t mask, flags, mode = 0;
	unsigned int time = 0;
	uint64_t start = get_timer(0);

	host->start_addr = 0;
	/* Timeout unit - ms */
	static unsigned int cmd_timeout = SDHCI_CMD_DEFAULT_TIMEOUT;

	mask = SDHCI_CMD_INHIBIT | SDHCI_DATA_INHIBIT;

	/* We shouldn't wait for data inihibit for stop commands, even
	   though they might use busy signaling */
	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION ||
	    ((cmd->cmdidx == MMC_CMD_SEND_TUNING_BLOCK ||
	      cmd->cmdidx == MMC_CMD_SEND_TUNING_BLOCK_HS200) && !data))
		mask &= ~SDHCI_DATA_INHIBIT;

	while (sdhci_readl(host, SDHCI_PRESENT_STATE) & mask) {
		if (time >= cmd_timeout) {
			if (2 * cmd_timeout <= SDHCI_CMD_MAX_TIMEOUT) {
				cmd_timeout += cmd_timeout;
				printf("timeout increasing to: %u ms.\n",
				       cmd_timeout);
			} else {
				printf("timeout.\n");
				return -ECOMM;
			}
		}
		time++;
	}

	sdhci_writel(host, SDHCI_INT_ALL_MASK, SDHCI_INT_STATUS);

	mask = SDHCI_INT_RESPONSE;
	if ((cmd->cmdidx == MMC_CMD_SEND_TUNING_BLOCK ||
	     cmd->cmdidx == MMC_CMD_SEND_TUNING_BLOCK_HS200) && !data)
		mask = SDHCI_INT_DATA_AVAIL;

	if (!(cmd->resp_type & MMC_RSP_PRESENT))
		flags = SDHCI_CMD_RESP_NONE;
	else if (cmd->resp_type & MMC_RSP_136)
		flags = SDHCI_CMD_RESP_LONG;
	else if (cmd->resp_type & MMC_RSP_BUSY) {
		flags = SDHCI_CMD_RESP_SHORT_BUSY;
		//mask |= SDHCI_INT_DATA_END;
	} else
		flags = SDHCI_CMD_RESP_SHORT;

	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= SDHCI_CMD_CRC;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		flags |= SDHCI_CMD_INDEX;
	if (data || cmd->cmdidx ==  MMC_CMD_SEND_TUNING_BLOCK ||
	    cmd->cmdidx == MMC_CMD_SEND_TUNING_BLOCK_HS200)
		flags |= SDHCI_CMD_DATA;

	/* Set Transfer mode regarding to data flag */
	if (data) {
		sdhci_writeb(host, 0xe, SDHCI_TIMEOUT_CONTROL);

		mode = SDHCI_TRNS_BLK_CNT_EN;
		trans_bytes = data->blocks * data->blocksize;
		if (data->blocks > 1)
			mode |= SDHCI_TRNS_MULTI | SDHCI_TRNS_BLK_CNT_EN;

		if (data->flags == MMC_DATA_READ)
			mode |= SDHCI_TRNS_READ;

		sdhci_writew(host, SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG,
				data->blocksize),
				SDHCI_BLOCK_SIZE);
		sdhci_writew(host, data->blocks, SDHCI_BLOCK_COUNT);
		sdhci_writew(host, mode, SDHCI_TRANSFER_MODE);
	} else if (cmd->resp_type & MMC_RSP_BUSY) {
		sdhci_writeb(host, 0xe, SDHCI_TIMEOUT_CONTROL);
	}

	sdhci_writel(host, cmd->cmdarg, SDHCI_ARGUMENT);
	sdhci_writew(host, SDHCI_MAKE_CMD(cmd->cmdidx, flags), SDHCI_COMMAND);
	start = get_timer(0);
	do {
		stat = sdhci_readl(host, SDHCI_INT_STATUS);
		if (stat & SDHCI_INT_ERROR)
			break;

		if (host->quirks & SDHCI_QUIRK_BROKEN_R1B &&
		    cmd->resp_type & MMC_RSP_BUSY && !data) {
			unsigned int state =
				sdhci_readl(host, SDHCI_PRESENT_STATE);

			if (!(state & SDHCI_DAT_ACTIVE))
				return 0;
		}

		if (get_timer(start) >= SDHCI_READ_STATUS_TIMEOUT) {
			printf("%s: Timeout for status update: %08x %08x\n",
			       __func__, stat, mask);
			dump(host);
			return -ETIMEDOUT;
		}
	} while ((stat & mask) != mask);

	if ((stat & (SDHCI_INT_ERROR | mask)) == mask) {
		sdhci_cmd_done(host, cmd);
		sdhci_writel(host, mask, SDHCI_INT_STATUS);
	} else
		ret = -1;

	if (!ret && data)
		ret = sdhci_transfer_data(host, data);

	if (host->quirks & SDHCI_QUIRK_WAIT_SEND_CMD)
		_delay_usec(10);

	stat = sdhci_readl(host, SDHCI_INT_STATUS);
	sdhci_writel(host, SDHCI_INT_ALL_MASK, SDHCI_INT_STATUS);

	// if(cmd->cmdidx != 52 && cmd->cmdidx != 53)
    // 	printf("ret:%d resp: %x %x %x %x\n", ret, cmd->response[0], cmd->response[1],cmd->response[2],cmd->response[3]);

	if (!ret) {
		if ((host->quirks & SDHCI_QUIRK_32BIT_DMA_ADDR) &&
				!is_aligned && (data->flags == MMC_DATA_READ))
			memcpy(data->dest, host->align_buffer, trans_bytes);
		return 0;
	}
	printf("sdhci cmd: %d ret: %d error: %x\n", cmd->cmdidx, ret, stat);
	sdhci_reset(SDHCI_RESET_CMD);
	sdhci_reset(SDHCI_RESET_DATA);
	
	if (stat & SDHCI_INT_TIMEOUT)
		return -ETIMEDOUT;
	else
		return -ECOMM;
}

static int sdhci_set_ios(struct mmc *mmc)
{
    struct sdhci_host *host = &_host;

	if (mmc->clock != host->clock)
		sdhci_set_clock(host, mmc->clock);

	if (mmc->clk_disable)
		sdhci_set_clock(host, 0);

	sdhci_set_bus_width(host, mmc->bus_width);
	//sdhci_set_select_mode(host, mmc->selected_mode);
	return 0;
}

static struct bus_ops _ops = {
	.set_ios = sdhci_set_ios,
	.send_command = sdhci_send_command,
};

struct bus_ops* bcm2711_sdhci_init(void)
{
	bcm283x_sdhci_gpio_init();

	_host.bus_width  = 1;
	_host.max_clk = 50000000;
	_host.clock = 400000;
	_host.name = "sdhci";
	if((readl(MMIO_BASE + 0x2000d0) & 0x2) == 0){
		_host.ioaddr = (uint8_t*)(MMIO_BASE + 0x340000);
    }else{
		_host.ioaddr = (uint8_t*)(MMIO_BASE + 0x300000);
    }
	_host.twoticks_delay = ((2 * 1000000) / 400000) + 1;
	_host.last_write = 0;
	_host.quirks = SDHCI_QUIRK_BROKEN_VOLTAGE | SDHCI_QUIRK_BROKEN_R1B |
		SDHCI_QUIRK_WAIT_SEND_CMD | SDHCI_QUIRK_NO_HISPD_BIT;
	_host.voltages = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;

	sdhci_reset(SDHCI_RESET_ALL);
	sdhci_set_power(&_host,MMC_VDD_33_34);

	sdhci_get_info(&_host);

	sdhci_set_clock(&_host, 25000000);
	sdhci_set_bus_width(&_host, 4);
	sdhci_set_uhs_timing(&_host, 0);
	/* Enable only interrupts served by the SD controller */
	sdhci_writel(&_host, SDHCI_INT_DATA_MASK | SDHCI_INT_CMD_MASK,
	 	    SDHCI_INT_ENABLE);

	/* Mask all sdhci interrupt sources */
	sdhci_writel(&_host, SDHCI_INT_DATA_MASK | SDHCI_INT_CMD_MASK,
	 		SDHCI_SIGNAL_ENABLE);
	return &_ops;
}
