#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ewoksys/mmio.h>
#include <ewoksys/dma.h>
#include <ewoksys/klog.h>
#include <ewoksys/kernel_tic.h>

#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/mailbox.h>
#include <arch/bcm283x/mmc.h>

#define SDHOST_BASE  (_mmio_base + 0x202000)

#define SDCMD  0x00 /* Command to SD card              - 16 R/W */
#define SDARG  0x04 /* Argument to SD card             - 32 R/W */
#define SDTOUT 0x08 /* Start value for timeout counter - 32 R/W */
#define SDCDIV 0x0c /* Start value for clock divider   - 11 R/W */
#define SDRSP0 0x10 /* SD card response (31:0)         - 32 R   */
#define SDRSP1 0x14 /* SD card response (63:32)        - 32 R   */
#define SDRSP2 0x18 /* SD card response (95:64)        - 32 R   */
#define SDRSP3 0x1c /* SD card response (127:96)       - 32 R   */
#define SDHSTS 0x20 /* SD host status                  - 11 R/W */
#define SDVDD  0x30 /* SD card power control           -  1 R/W */
#define SDEDM  0x34 /* Emergency Debug Mode            - 13 R/W */
#define SDHCFG 0x38 /* Host configuration              -  2 R/W */
#define SDHBCT 0x3c /* Host byte count (debug)         - 32 R/W */
#define SDDATA 0x40 /* Data to/from SD card            - 32 R/W */
#define SDHBLC 0x50 /* Host block count (SDIO/SDHC)    -  9 R/W */

#define SDCMD_NEW_FLAG			0x8000
#define SDCMD_FAIL_FLAG			0x4000
#define SDCMD_BUSYWAIT			0x800
#define SDCMD_NO_RESPONSE		0x400
#define SDCMD_LONG_RESPONSE		0x200
#define SDCMD_WRITE_CMD			0x80
#define SDCMD_READ_CMD			0x40
#define SDCMD_CMD_MASK			0x3f

#define SDCDIV_MAX_CDIV			0x7ff

#define SDHSTS_BUSY_IRPT		0x400
#define SDHSTS_BLOCK_IRPT		0x200
#define SDHSTS_SDIO_IRPT		0x100
#define SDHSTS_REW_TIME_OUT		0x80
#define SDHSTS_CMD_TIME_OUT		0x40
#define SDHSTS_CRC16_ERROR		0x20
#define SDHSTS_CRC7_ERROR		0x10
#define SDHSTS_FIFO_ERROR		0x08
#define SDHSTS_DATA_FLAG		0x01

#define SDHSTS_CLEAR_MASK		(SDHSTS_BUSY_IRPT | \
					 SDHSTS_BLOCK_IRPT | \
					 SDHSTS_SDIO_IRPT | \
					 SDHSTS_REW_TIME_OUT | \
					 SDHSTS_CMD_TIME_OUT | \
					 SDHSTS_CRC16_ERROR | \
					 SDHSTS_CRC7_ERROR | \
					 SDHSTS_FIFO_ERROR)

#define SDHSTS_TRANSFER_ERROR_MASK	(SDHSTS_CRC7_ERROR | \
					 SDHSTS_CRC16_ERROR | \
					 SDHSTS_REW_TIME_OUT | \
					 SDHSTS_FIFO_ERROR)

#define SDHSTS_ERROR_MASK		(SDHSTS_CMD_TIME_OUT | \
					 SDHSTS_TRANSFER_ERROR_MASK)

#define SDHCFG_BUSY_IRPT_EN	BIT(10)
#define SDHCFG_BLOCK_IRPT_EN	BIT(8)
#define SDHCFG_SDIO_IRPT_EN	BIT(5)
#define SDHCFG_DATA_IRPT_EN	BIT(4)
#define SDHCFG_SLOW_CARD	BIT(3)
#define SDHCFG_WIDE_EXT_BUS	BIT(2)
#define SDHCFG_WIDE_INT_BUS	BIT(1)
#define SDHCFG_REL_CMD_LINE	BIT(0)

#define SDVDD_POWER_OFF		0
#define SDVDD_POWER_ON		1

#define SDEDM_FORCE_DATA_MODE	BIT(19)
#define SDEDM_CLOCK_PULSE	BIT(20)
#define SDEDM_BYPASS		BIT(21)

#define SDEDM_FIFO_FILL_SHIFT	4
#define SDEDM_FIFO_FILL_MASK	0x1f

#define SDHOST_MAX_CLK 250000000

static uint32_t edm_fifo_fill(uint32_t edm)
{
	return (edm >> SDEDM_FIFO_FILL_SHIFT) & SDEDM_FIFO_FILL_MASK;
}

#define SDEDM_WRITE_THRESHOLD_SHIFT	9
#define SDEDM_READ_THRESHOLD_SHIFT	14
#define SDEDM_THRESHOLD_MASK		0x1f

#define SDEDM_FSM_MASK		0xf
#define SDEDM_FSM_IDENTMODE	0x0
#define SDEDM_FSM_DATAMODE	0x1
#define SDEDM_FSM_READDATA	0x2
#define SDEDM_FSM_WRITEDATA	0x3
#define SDEDM_FSM_READWAIT	0x4
#define SDEDM_FSM_READCRC	0x5
#define SDEDM_FSM_WRITECRC	0x6
#define SDEDM_FSM_WRITEWAIT1	0x7
#define SDEDM_FSM_POWERDOWN	0x8
#define SDEDM_FSM_POWERUP	0x9
#define SDEDM_FSM_WRITESTART1	0xa
#define SDEDM_FSM_WRITESTART2	0xb
#define SDEDM_FSM_GENPULSES	0xc
#define SDEDM_FSM_WRITEWAIT2	0xd
#define SDEDM_FSM_STARTPOWDOWN	0xf

#define SDDATA_FIFO_WORDS	16

#define FIFO_READ_THRESHOLD	4
#define FIFO_WRITE_THRESHOLD	4
#define SDDATA_FIFO_PIO_BURST	8

#define SDHST_TIMEOUT_MAX_USEC	100000

#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))
#define readl(addr) (*((volatile uint32_t *)(addr)))
#define writel(val, addr) (*((volatile uint32_t *)(addr)) = (uint32_t)(val))


struct bcm2835_host {
	void 		*ioaddr;

	int			clock;		/* Current clock speed */
	unsigned int		max_clk;	/* Max possible freq */
	unsigned int		blocks;		/* remaining PIO blocks */

	uint32_t			ns_per_fifo_word;

	/* cached registers */
	uint32_t			hcfg;
	uint32_t			cdiv;

	struct mmc_cmd	*cmd;		/* Current command */
	struct mmc_data		*data;		/* Current data request */
	bool			use_busy:1;	/* Wait for busy interrupt */
	unsigned int		firmware_sets_cdiv:1;
};


static inline int is_power_of_2(uint32_t x)
{
	return !(x & (x - 1));
}

static void bcm2835_dumpregs(struct bcm2835_host *host)
{
	klog("=========== REGISTER DUMP ===========\n");
	klog("SDCMD  0x%08x\n", readl(host->ioaddr + SDCMD));
	klog("SDARG  0x%08x\n", readl(host->ioaddr + SDARG));
	klog("SDTOUT 0x%08x\n", readl(host->ioaddr + SDTOUT));
	klog("SDCDIV 0x%08x\n", readl(host->ioaddr + SDCDIV));
	klog("SDRSP0 0x%08x\n", readl(host->ioaddr + SDRSP0));
	klog("SDRSP1 0x%08x\n", readl(host->ioaddr + SDRSP1));
	klog("SDRSP2 0x%08x\n", readl(host->ioaddr + SDRSP2));
	klog("SDRSP3 0x%08x\n", readl(host->ioaddr + SDRSP3));
	klog("SDHSTS 0x%08x\n", readl(host->ioaddr + SDHSTS));
	klog("SDVDD  0x%08x\n", readl(host->ioaddr + SDVDD));
	klog("SDEDM  0x%08x\n", readl(host->ioaddr + SDEDM));
	klog("SDHCFG 0x%08x\n", readl(host->ioaddr + SDHCFG));
	klog("SDHBCT 0x%08x\n", readl(host->ioaddr + SDHBCT));
	klog("SDHBLC 0x%08x\n", readl(host->ioaddr + SDHBLC));
	klog("===========================================\n");
}

static int bcm283x_sdio_gpio_init(void){
    bcm283x_gpio_init();
    bcm283x_gpio_config(48, GPIO_ALTF0);
    bcm283x_gpio_config(49, GPIO_ALTF0);
    bcm283x_gpio_config(50, GPIO_ALTF0);
    bcm283x_gpio_config(51, GPIO_ALTF0);
    bcm283x_gpio_config(52, GPIO_ALTF0);
    bcm283x_gpio_config(53, GPIO_ALTF0);
	return 0;
}

/*----------------------------------------------------------------*/
static uint32_t bcm2835_read_wait_sdcmd(struct bcm2835_host *host)
{
	uint32_t value;
	int timeout_us = SDHST_TIMEOUT_MAX_USEC;

	// ret = readl_poll_timeout(host->ioaddr + SDCMD, value,
	// 			 !(value & SDCMD_NEW_FLAG), timeout_us);
    while(timeout_us --){
        value = readl(host->ioaddr + SDCMD);
        if(!(value & SDCMD_NEW_FLAG))
            break;
    }
    if (timeout_us <= 0)
		klog("%s: timeout (%d us)\n", __func__, timeout_us);

	return value;
}

static void bcm2835_reset_internal(struct bcm2835_host *host)
{
	uint32_t temp;

	writel(SDVDD_POWER_OFF, host->ioaddr + SDVDD);
	writel(0, host->ioaddr + SDCMD);
	writel(0, host->ioaddr + SDARG);
	/* Set timeout to a big enough value so we don't hit it */
	writel(0xf00000, host->ioaddr + SDTOUT);
	writel(0, host->ioaddr + SDCDIV);
	/* Clear status register */
	writel(SDHSTS_CLEAR_MASK, host->ioaddr + SDHSTS);
	writel(0, host->ioaddr + SDHCFG);
	writel(0, host->ioaddr + SDHBCT);
	writel(0, host->ioaddr + SDHBLC);

	/* Limit fifo usage due to silicon bug */
	temp = readl(host->ioaddr + SDEDM);
	temp &= ~((SDEDM_THRESHOLD_MASK << SDEDM_READ_THRESHOLD_SHIFT) |
		  (SDEDM_THRESHOLD_MASK << SDEDM_WRITE_THRESHOLD_SHIFT));
	temp |= (FIFO_READ_THRESHOLD << SDEDM_READ_THRESHOLD_SHIFT) |
		(FIFO_WRITE_THRESHOLD << SDEDM_WRITE_THRESHOLD_SHIFT);
	writel(temp, host->ioaddr + SDEDM);
	/* Wait for FIFO threshold to populate */
	proc_usleep(20000);
	writel(SDVDD_POWER_ON, host->ioaddr + SDVDD);
	/* Wait for all components to go through power on cycle */
	proc_usleep(20000);
	host->clock = 0;
	writel(host->hcfg, host->ioaddr + SDHCFG);
	writel(SDCDIV_MAX_CDIV, host->ioaddr + SDCDIV);
}


static int bcm2835_check_cmd_error(struct bcm2835_host *host, uint32_t intmask)
{
	int ret = -EINVAL;

	if (!(intmask & SDHSTS_ERROR_MASK))
		return 0;

	if (!host->cmd)
		return -EINVAL;

	klog("sdhost_busy_irq: intmask %08x\n", intmask);
	if (intmask & SDHSTS_CRC7_ERROR) {
		ret = -EILSEQ;
	} else if (intmask & (SDHSTS_CRC16_ERROR |
			      SDHSTS_FIFO_ERROR)) {
		ret = -EILSEQ;
	} else if (intmask & (SDHSTS_REW_TIME_OUT | SDHSTS_CMD_TIME_OUT)) {
		ret = -ETIMEDOUT;
	}
	bcm2835_dumpregs(host);
	return ret;
}


static int bcm2835_check_data_error(struct bcm2835_host *host, uint32_t intmask)
{
	int ret = 0;

	if (!host->data)
		return 0;
	if (intmask & (SDHSTS_CRC16_ERROR | SDHSTS_FIFO_ERROR))
		ret = -EILSEQ;
	if (intmask & SDHSTS_REW_TIME_OUT)
		ret = -ETIMEDOUT;

	if (ret)
		klog("%s:%d %d\n", __func__, __LINE__, ret);

	return ret;
}

static int bcm2835_wait_transfer_complete(struct bcm2835_host *host)
{
	uint64_t tstart_ms = 0;
	//uint32_t retry_count = 0;
	while (1) {
		uint32_t edm, fsm;

		edm = readl(host->ioaddr + SDEDM);
		fsm = edm & SDEDM_FSM_MASK;

		if ((fsm == SDEDM_FSM_IDENTMODE) ||
		    (fsm == SDEDM_FSM_DATAMODE))
			break;

		if ((fsm == SDEDM_FSM_READWAIT) ||
		    (fsm == SDEDM_FSM_WRITESTART1) ||
		    (fsm == SDEDM_FSM_READDATA)) {
			writel(edm | SDEDM_FORCE_DATA_MODE,
			       host->ioaddr + SDEDM);
			break;
		}

		/* Error out after ~1s */
		if(tstart_ms > 0) {
			uint64_t tlapse_ms = kernel_tic_ms(0) - tstart_ms;
			if ( tlapse_ms > 1000 ) {

				klog("wait_transfer_complete - still waiting after %lld ms\n",
					tlapse_ms);
				bcm2835_dumpregs(host);
				return -ETIMEDOUT;
			}
		}
		tstart_ms = kernel_tic_ms(0);

		/*
		sleep(0);
		retry_count++;
		if(retry_count > 10000) {
			klog("wait_transfer_complete - still waiting after %d times\n",
				retry_count);
			bcm2835_dumpregs(host);
			return -ETIMEDOUT;
		}
		*/
	}

	return 0;
}


static void bcm2835_prepare_data(struct bcm2835_host *host, struct mmc_cmd *cmd,
				 struct mmc_data *data)
{
	(void)cmd;

	host->data = data;
	if (!data)
		return;

	/* Use PIO */
	host->blocks = data->blocks;

	writel(data->blocksize, host->ioaddr + SDHBCT);
	writel(data->blocks, host->ioaddr + SDHBLC);
}


static int bcm2835_transfer_block_pio(struct bcm2835_host *host, bool is_read)
{
	struct mmc_data *data = host->data;
	size_t blksize = data->blocksize;
	int copy_words;
	uint32_t hsts = 0;
	uint32_t *buf;

	if (blksize % sizeof(uint32_t))
		return -EINVAL;

	buf = is_read ? (uint32_t *)data->dest : (uint32_t *)data->src;

	if (is_read)
		data->dest += blksize;
	else
		data->src += blksize;

	copy_words = blksize / sizeof(uint32_t);

	/*
	 * Copy all contents from/to the FIFO as far as it reaches,
	 * then wait for it to fill/empty again and rewind.
	 */
	while (copy_words) {
		int burst_words, words;
		uint32_t edm;

		burst_words = min(SDDATA_FIFO_PIO_BURST, copy_words);
		edm = readl(host->ioaddr + SDEDM);
		if (is_read)
			words = edm_fifo_fill(edm);
		else
			words = SDDATA_FIFO_WORDS - edm_fifo_fill(edm);

		if (words < burst_words) {
			int fsm_state = (edm & SDEDM_FSM_MASK);

			if ((is_read &&
			     (fsm_state != SDEDM_FSM_READDATA &&
			      fsm_state != SDEDM_FSM_READWAIT &&
			      fsm_state != SDEDM_FSM_READCRC)) ||
			    (!is_read &&
			     (fsm_state != SDEDM_FSM_WRITEDATA &&
			      fsm_state != SDEDM_FSM_WRITEWAIT1 &&
			      fsm_state != SDEDM_FSM_WRITEWAIT2 &&
			      fsm_state != SDEDM_FSM_WRITECRC &&
			      fsm_state != SDEDM_FSM_WRITESTART1 &&
			      fsm_state != SDEDM_FSM_WRITESTART2))) {
				hsts = readl(host->ioaddr + SDHSTS);
				klog("fsm %x, hsts %08x\n", fsm_state, hsts);
				if (hsts & SDHSTS_ERROR_MASK)
					break;
			}
			continue;
		} else if (words > copy_words) {
			words = copy_words;
		}

		copy_words -= words;
		/* Copy current chunk to/from the FIFO */
		while (words) {
			if (is_read)
				*(buf++) = readl(host->ioaddr + SDDATA);
			else
				writel(*(buf++), host->ioaddr + SDDATA);
			words--;
		}
	}

	return 0;
}


static int bcm2835_transfer_pio(struct bcm2835_host *host)
{
	uint32_t sdhsts;
	bool is_read;
	int ret = 0;

	is_read = (host->data->flags & MMC_DATA_READ) != 0;
	ret = bcm2835_transfer_block_pio(host, is_read);
	if (ret)
		return ret;

	sdhsts = readl(host->ioaddr + SDHSTS);
	if (sdhsts & (SDHSTS_CRC16_ERROR |
		      SDHSTS_CRC7_ERROR |
		      SDHSTS_FIFO_ERROR)) {
		klog("%s transfer error - HSTS %08x\n",
		       is_read ? "read" : "write", sdhsts);
		ret =  -EILSEQ;
	} else if ((sdhsts & (SDHSTS_CMD_TIME_OUT |
			      SDHSTS_REW_TIME_OUT))) {
		klog("%s timeout error - HSTS %08x\n",
		       is_read ? "read" : "write", sdhsts);
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int bcm2835_finish_command(struct bcm2835_host *host)
{
	struct mmc_cmd *cmd = host->cmd;
	uint32_t sdcmd;
	int ret = 0;

	sdcmd = bcm2835_read_wait_sdcmd(host);

	/* Check for errors */
	if (sdcmd & SDCMD_NEW_FLAG) {
		klog("command never completed.\n");
		bcm2835_dumpregs(host);
		return -EIO;
	} else if (sdcmd & SDCMD_FAIL_FLAG) {
		uint32_t sdhsts = readl(host->ioaddr + SDHSTS);

		/* Clear the errors */
		writel(SDHSTS_ERROR_MASK, host->ioaddr + SDHSTS);

		if (!(sdhsts & SDHSTS_CRC7_ERROR) ||
		    (host->cmd->cmdidx != MMC_CMD_SEND_OP_COND)) {
			if (sdhsts & SDHSTS_CMD_TIME_OUT) {
				ret = -ETIMEDOUT;
			} else {
				klog("unexpected command %d error\n",
				       host->cmd->cmdidx);
				bcm2835_dumpregs(host);
				ret = -EILSEQ;
			}

			return ret;
		}
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			int i;

			for (i = 0; i < 4; i++) {
				cmd->response[3 - i] =
					readl(host->ioaddr + SDRSP0 + i * 4);
			}
		} else {
			cmd->response[0] = readl(host->ioaddr + SDRSP0);
		}
	}

	/* Processed actual command. */
	host->cmd = NULL;

	return ret;
}


static int bcm2835_transmit(struct bcm2835_host *host)
{
	uint32_t intmask = readl(host->ioaddr + SDHSTS);
	int ret;

	/* Check for errors */
	ret = bcm2835_check_data_error(host, intmask);
	if (ret)
		return ret;

	ret = bcm2835_check_cmd_error(host, intmask);
	if (ret)
		return ret;

	/* Handle wait for busy end */
	if (host->use_busy && (intmask & SDHSTS_BUSY_IRPT)) {
		writel(SDHSTS_BUSY_IRPT, host->ioaddr + SDHSTS);
		host->use_busy = false;
		bcm2835_finish_command(host);
	}

	/* Handle PIO data transfer */
	if (host->data) {
		ret = bcm2835_transfer_pio(host);
		if (ret)
			return ret;
		host->blocks--;
		if (host->blocks == 0) {
			/* Wait for command to complete for real */
			ret = bcm2835_wait_transfer_complete(host);
			if (ret)
				return ret;
			/* Transfer complete */
			host->data = NULL;
		}
	}

	return 0;
}


static int bcm2835_send_command(struct bcm2835_host *host, struct mmc_cmd *cmd,
				struct mmc_data *data)
{
	uint32_t sdcmd, sdhsts;

	if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY)) {
		klog("unsupported response type!\n");
		return -EINVAL;
	}

	sdcmd = bcm2835_read_wait_sdcmd(host);
	if (sdcmd & SDCMD_NEW_FLAG) {
		klog("previous command never completed.\n");
		bcm2835_dumpregs(host);
		return -EBUSY;
	}

	host->cmd = cmd;

	/* Clear any error flags */
	sdhsts = readl(host->ioaddr + SDHSTS);
	if (sdhsts & SDHSTS_ERROR_MASK)
		writel(sdhsts, host->ioaddr + SDHSTS);

	bcm2835_prepare_data(host, cmd, data);

	writel(cmd->cmdarg, host->ioaddr + SDARG);

	sdcmd = cmd->cmdidx & SDCMD_CMD_MASK;

	host->use_busy = false;
	if (!(cmd->resp_type & MMC_RSP_PRESENT)) {
		sdcmd |= SDCMD_NO_RESPONSE;
	} else {
		if (cmd->resp_type & MMC_RSP_136)
			sdcmd |= SDCMD_LONG_RESPONSE;
		if (cmd->resp_type & MMC_RSP_BUSY) {
			sdcmd |= SDCMD_BUSYWAIT;
			host->use_busy = true;
		}
	}

	if (data) {
		if (data->flags & MMC_DATA_WRITE)
			sdcmd |= SDCMD_WRITE_CMD;
		if (data->flags & MMC_DATA_READ)
			sdcmd |= SDCMD_READ_CMD;
	}

	writel(sdcmd | SDCMD_NEW_FLAG, host->ioaddr + SDCMD);

	return 0;
}


int bcm2835_send_cmd(struct bcm2835_host *host, struct mmc_cmd *cmd,
			    struct mmc_data *data)
{
	uint32_t edm, fsm;
	int ret = 0;

	if (data && !is_power_of_2(data->blocksize)) {
		klog("unsupported block size (%d bytes)\n", data->blocksize);

		if (cmd)
			return -EINVAL;
	}

	edm = readl(host->ioaddr + SDEDM);
	fsm = edm & SDEDM_FSM_MASK;

	if ((fsm != SDEDM_FSM_IDENTMODE) &&
	    (fsm != SDEDM_FSM_DATAMODE) &&
	    (cmd && cmd->cmdidx != MMC_CMD_STOP_TRANSMISSION)) {
		klog("previous command (%d) not complete (EDM %08x)\n",
		       readl(host->ioaddr + SDCMD) & SDCMD_CMD_MASK, edm);
		bcm2835_dumpregs(host);

		if (cmd)
			return -EILSEQ;

		return 0;
	}

	if (cmd) {
		ret = bcm2835_send_command(host, cmd, data);
		if (!ret && !host->use_busy)
			ret = bcm2835_finish_command(host);
	}

	/* Wait for completion of busy signal or data transfer */
	while (host->use_busy || host->data) {
		ret = bcm2835_transmit(host);
		if (ret)
			break;
	}

	return ret;
}

/* All message buffers must start with this header */
struct bcm2835_mbox_hdr {
	uint32_t buf_size;
	uint32_t code;
};

struct bcm2835_mbox_tag_hdr {
	uint32_t tag;
	uint32_t val_buf_size;
	uint32_t val_len;
};

struct bcm2835_mbox_tag_set_power_state {
	struct bcm2835_mbox_tag_hdr tag_hdr;
	union {
		struct {
			uint32_t device_id;
			uint32_t state;
		} req;
		struct {
			uint32_t device_id;
			uint32_t state;
		} resp;
	} body;
};

struct bcm2835_mbox_tag_set_sdhost_clock {
	struct bcm2835_mbox_tag_hdr tag_hdr;
	union {
		struct {
			uint32_t rate_hz;
		} req;
		struct {
			uint32_t rate_hz;
			uint32_t rate_1;
			uint32_t rate_2;
		} resp;
	} body;
};

struct bcm2835_mbox_tag_get_clock_rate {
	struct bcm2835_mbox_tag_hdr tag_hdr;
	union {
		struct {
			uint32_t clock_id;
		} req;
		struct {
			uint32_t clock_id;
			uint32_t rate_hz;
		} resp;
	} body;
};

struct msg_set_power_state {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_set_power_state set_power_state;
	uint32_t end_tag;
};


struct msg_set_sdhost_clock {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_set_sdhost_clock set_sdhost_clock;
	uint32_t end_tag;
};


struct msg_get_clock_rate {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_clock_rate get_clock_rate;
	uint32_t end_tag;
};

#define BCM2835_MBOX_INIT_HDR(_m_) { \
		memset((_m_), 0, sizeof(*(_m_))); \
		(_m_)->hdr.buf_size = sizeof(*(_m_)); \
		(_m_)->hdr.code = 0; \
		(_m_)->end_tag = 0; \
	}

#define BCM2835_MBOX_INIT_TAG(_t_, _id_) { \
		(_t_)->tag_hdr.tag = BCM2835_MBOX_TAG_##_id_; \
		(_t_)->tag_hdr.val_buf_size = sizeof((_t_)->body); \
		(_t_)->tag_hdr.val_len = sizeof((_t_)->body.req); \
	}

#define BCM2835_MBOX_TAG_GET_CLOCK_RATE	0x00030002
#define BCM2835_MBOX_TAG_GET_MAX_CLOCK_RATE	0x00030004
#define BCM2835_MBOX_TAG_GET_MIN_CLOCK_RATE	0x00030007


#define BCM2835_MBOX_POWER_DEVID_SDHCI		0
#define BCM2835_MBOX_POWER_DEVID_UART0		1
#define BCM2835_MBOX_POWER_DEVID_UART1		2
#define BCM2835_MBOX_POWER_DEVID_USB_HCD	3
#define BCM2835_MBOX_POWER_DEVID_I2C0		4
#define BCM2835_MBOX_POWER_DEVID_I2C1		5
#define BCM2835_MBOX_POWER_DEVID_I2C2		6
#define BCM2835_MBOX_POWER_DEVID_SPI		7
#define BCM2835_MBOX_POWER_DEVID_CCP2TX		8

#define BCM2835_MBOX_TAG_SET_POWER_STATE	0x00028001
#define BCM2835_MBOX_TAG_SET_SDHOST_CLOCK	0x00038042
#define BCM2835_MBOX_SET_POWER_STATE_REQ_ON	(1 << 0)
#define BCM2835_MBOX_SET_POWER_STATE_REQ_WAIT	(1 << 1)
#define BCM2835_MBOX_PROP_CHAN		8


int bcm2835_power_on_module(uint32_t module)
{
	//ALLOC_CACHE_ALIGN_BUFFER(struct msg_set_power_state, msg_pwr, 1);
    mail_message_t msg;
    struct msg_set_power_state* msg_pwr = (struct msg_set_power_state*)dma_alloc(0, sizeof(struct msg_set_power_state));

	BCM2835_MBOX_INIT_HDR(msg_pwr);
	BCM2835_MBOX_INIT_TAG(&msg_pwr->set_power_state,
			      SET_POWER_STATE);
	msg_pwr->set_power_state.body.req.device_id = module;
	msg_pwr->set_power_state.body.req.state =
		BCM2835_MBOX_SET_POWER_STATE_REQ_ON |
		BCM2835_MBOX_SET_POWER_STATE_REQ_WAIT;

	// ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN,
	// 			     &msg_pwr->hdr);
	// if (ret) {
	// 	klog("bcm2835: Could not set module %u power state\n",
	// 	       module);
	// 	return -EIO;
	// }
    msg.data = ((uint32_t)dma_phy_addr(0, msg_pwr) + 0x40000000) >> 4;	
	msg.channel = PROPERTY_CHANNEL;
    bcm283x_mailbox_call(&msg);

	return 0;
}

int bcm2835_set_sdhost_clock(uint32_t rate_hz, uint32_t *rate_1, uint32_t *rate_2)
{
	//ALLOC_CACHE_ALIGN_BUFFER(struct msg_set_sdhost_clock, msg_sdhost_clk, 1);
    mail_message_t msg;
    struct msg_set_sdhost_clock* msg_sdhost_clk = (struct msg_set_sdhost_clock*)dma_alloc(0, sizeof(struct msg_set_sdhost_clock));

	BCM2835_MBOX_INIT_HDR(msg_sdhost_clk);
	BCM2835_MBOX_INIT_TAG(&msg_sdhost_clk->set_sdhost_clock, SET_SDHOST_CLOCK);

	msg_sdhost_clk->set_sdhost_clock.body.req.rate_hz = rate_hz;

	// ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg_sdhost_clk->hdr);
	// if (ret) {
	// 	klog("bcm2835: Could not query sdhost clock rate\n");
	// 	return -EIO;
	// }
    msg.data = ((uint32_t)dma_phy_addr(0, msg_sdhost_clk) + 0x40000000) >> 4;	
	msg.channel = PROPERTY_CHANNEL;
    bcm283x_mailbox_call(&msg);

	*rate_1 = msg_sdhost_clk->set_sdhost_clock.body.resp.rate_1;
	*rate_2 = msg_sdhost_clk->set_sdhost_clock.body.resp.rate_2;

	return 0;
}

int bcm2835_get_mmc_clock(uint32_t clock_id)
{
	//ALLOC_CACHE_ALIGN_BUFFER(struct msg_get_clock_rate, msg_clk, 1);
	int ret;
	uint32_t clock_rate = 0;
    mail_message_t msg;
    struct msg_get_clock_rate *msg_clk = (struct msg_get_clock_rate*)dma_alloc(0, sizeof(struct msg_get_clock_rate)); 

	ret = bcm2835_power_on_module(BCM2835_MBOX_POWER_DEVID_SDHCI);
	if (ret)
		return ret;

	BCM2835_MBOX_INIT_HDR(msg_clk);
	BCM2835_MBOX_INIT_TAG(&msg_clk->get_clock_rate, GET_CLOCK_RATE);
	msg_clk->get_clock_rate.body.req.clock_id = clock_id;

	// ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg_clk->hdr);
	// if (ret) {
	// 	klog("bcm2835: Could not query eMMC clock rate\n");
	// 	return -EIO;
	// }
    msg.data = ((uint32_t)dma_phy_addr(0, msg_clk) + 0x40000000) >> 4;	
	msg.channel = PROPERTY_CHANNEL;
    bcm283x_mailbox_call(&msg);

	clock_rate = msg_clk->get_clock_rate.body.resp.rate_hz;

	if (clock_rate == 0) {
		BCM2835_MBOX_INIT_HDR(msg_clk);
		BCM2835_MBOX_INIT_TAG(&msg_clk->get_clock_rate, GET_MAX_CLOCK_RATE);
		msg_clk->get_clock_rate.body.req.clock_id = clock_id;

		// ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg_clk->hdr);
		// if (ret) {
		// 	klog("bcm2835: Could not query max eMMC clock rate\n");
		// 	return -EIO;
		// }
        msg.data = ((uint32_t)msg_clk + 0x40000000) >> 4;	
		msg.channel = PROPERTY_CHANNEL;
        bcm283x_mailbox_call(&msg);

		clock_rate = msg_clk->get_clock_rate.body.resp.rate_hz;
	}

	return clock_rate;
}


static void bcm2835_set_clock(struct bcm2835_host *host, unsigned int clock)
{
	int div;
	uint32_t clock_rate[2] = { 0 };

	/* The SDCDIV register has 11 bits, and holds (div - 2).  But
	 * in data mode the max is 50MHz wihout a minimum, and only
	 * the bottom 3 bits are used. Since the switch over is
	 * automatic (unless we have marked the card as slow...),
	 * chosen values have to make sense in both modes.  Ident mode
	 * must be 100-400KHz, so can range check the requested
	 * clock. CMD15 must be used to return to data mode, so this
	 * can be monitored.
	 *
	 * clock 250MHz -> 0->125MHz, 1->83.3MHz, 2->62.5MHz, 3->50.0MHz
	 *                 4->41.7MHz, 5->35.7MHz, 6->31.3MHz, 7->27.8MHz
	 *
	 *		 623->400KHz/27.8MHz
	 *		 reset value (507)->491159/50MHz
	 *
	 * BUT, the 3-bit clock divisor in data mode is too small if
	 * the core clock is higher than 250MHz, so instead use the
	 * SLOW_CARD configuration bit to force the use of the ident
	 * clock divisor at all times.
	 */

	if (host->firmware_sets_cdiv) {
		bcm2835_set_sdhost_clock(clock, &clock_rate[0], &clock_rate[1]);
		clock = max(clock_rate[0], clock_rate[1]);
	} else {
		if (clock < 100000) {
			/* Can't stop the clock, but make it as slow as possible
			* to show willing
			*/
			host->cdiv = SDCDIV_MAX_CDIV;
			writel(host->cdiv, host->ioaddr + SDCDIV);
			return;
		}

		div = host->max_clk / clock;
		if (div < 2)
			div = 2;
		if ((host->max_clk / div) > clock)
			div++;
		div -= 2;

		if (div > SDCDIV_MAX_CDIV)
			div = SDCDIV_MAX_CDIV;

		clock = host->max_clk / (div + 2);
		host->cdiv = div;
		writel(host->cdiv, host->ioaddr + SDCDIV);
	}

	//host->mmc->clock = clock;

	/* Calibrate some delays */

	host->ns_per_fifo_word = (1000000000 / clock) * 8;

	/* Set the timeout to 500ms */
	writel(clock, host->ioaddr + SDTOUT);
}

int bcm2835_set_ios(struct bcm2835_host *host, int clock, int bus_width)
{
	if (clock != host->clock) {
		bcm2835_set_clock(host, clock);
		host->clock = clock;
	}

	/* set bus width */
	host->hcfg &= ~SDHCFG_WIDE_EXT_BUS;
	if (bus_width == 4)
		host->hcfg |= SDHCFG_WIDE_EXT_BUS;

	host->hcfg |= SDHCFG_WIDE_INT_BUS;

	/* Disable clever clock switching, to cope with fast core clocks */
	host->hcfg |= SDHCFG_SLOW_CARD;

	writel(host->hcfg, host->ioaddr + SDHCFG);

	return 0;
}



struct bcm2835_host _host;

void* bcm283x_sdhost_init(void){
    uint32_t clock_rate[2] = { ~0 };
    bcm283x_sdio_gpio_init();

    _host.ioaddr = (void*)(_mmio_base + 0x202000);
    _host.max_clk = bcm2835_get_mmc_clock(4);
    bcm2835_set_sdhost_clock(0, &clock_rate[0], &clock_rate[1]);
    _host.firmware_sets_cdiv = (clock_rate[0] != (uint32_t)(~0));
	//klog("sdio max clock:%d cdev:%d\n", _host.max_clk, _host.firmware_sets_cdiv);

    bcm2835_reset_internal(&_host);
	return (void*)&_host;
}
