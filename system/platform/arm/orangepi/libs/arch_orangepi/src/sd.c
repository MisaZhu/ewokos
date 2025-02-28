#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <sysinfo.h>
#include "sdmmc.h"

#define SUNXI_MMC_CLK_POWERSAVE     (0x1 << 17)
#define SUNXI_MMC_CLK_ENABLE        (0x1 << 16)
#define SUNXI_MMC_CLK_DIVIDER_MASK  (0xff)

#define SUNXI_MMC_GCTRL_SOFT_RESET  (0x1 << 0)
#define SUNXI_MMC_GCTRL_FIFO_RESET  (0x1 << 1)
#define SUNXI_MMC_GCTRL_DMA_RESET   (0x1 << 2)
#define SUNXI_MMC_GCTRL_RESET       (SUNXI_MMC_GCTRL_SOFT_RESET|\
                     SUNXI_MMC_GCTRL_FIFO_RESET|\
                     SUNXI_MMC_GCTRL_DMA_RESET)
#define SUNXI_MMC_GCTRL_DMA_ENABLE  (0x1 << 5)
#define SUNXI_MMC_GCTRL_ACCESS_BY_AHB   (0x1 << 31)

#define SUNXI_MMC_CMD_RESP_EXPIRE   (0x1 << 6)
#define SUNXI_MMC_CMD_LONG_RESPONSE (0x1 << 7)
#define SUNXI_MMC_CMD_CHK_RESPONSE_CRC  (0x1 << 8)
#define SUNXI_MMC_CMD_DATA_EXPIRE   (0x1 << 9)
#define SUNXI_MMC_CMD_WRITE     (0x1 << 10)
#define SUNXI_MMC_CMD_AUTO_STOP     (0x1 << 12)
#define SUNXI_MMC_CMD_WAIT_PRE_OVER (0x1 << 13)
#define SUNXI_MMC_CMD_STOP_ABORT    (0x1 << 14)
#define SUNXI_MMC_CMD_SEND_INIT_SEQ (0x1 << 15)
#define SUNXI_MMC_CMD_UPCLK_ONLY    (0x1 << 21)
#define SUNXI_MMC_CMD_START     (0x1 << 31)

#define SUNXI_MMC_RINT_RESP_ERROR       (0x1 << 1)
#define SUNXI_MMC_RINT_COMMAND_DONE     (0x1 << 2)
#define SUNXI_MMC_RINT_DATA_OVER        (0x1 << 3)
#define SUNXI_MMC_RINT_TX_DATA_REQUEST      (0x1 << 4)
#define SUNXI_MMC_RINT_RX_DATA_REQUEST      (0x1 << 5)
#define SUNXI_MMC_RINT_RESP_CRC_ERROR       (0x1 << 6)
#define SUNXI_MMC_RINT_DATA_CRC_ERROR       (0x1 << 7)
#define SUNXI_MMC_RINT_RESP_TIMEOUT     (0x1 << 8)
#define SUNXI_MMC_RINT_DATA_TIMEOUT     (0x1 << 9)
#define SUNXI_MMC_RINT_VOLTAGE_CHANGE_DONE  (0x1 << 10)
#define SUNXI_MMC_RINT_FIFO_RUN_ERROR       (0x1 << 11)
#define SUNXI_MMC_RINT_HARD_WARE_LOCKED     (0x1 << 12)
#define SUNXI_MMC_RINT_START_BIT_ERROR      (0x1 << 13)
#define SUNXI_MMC_RINT_AUTO_COMMAND_DONE    (0x1 << 14)
#define SUNXI_MMC_RINT_END_BIT_ERROR        (0x1 << 15)
#define SUNXI_MMC_RINT_SDIO_INTERRUPT       (0x1 << 16)
#define SUNXI_MMC_RINT_CARD_INSERT      (0x1 << 30)
#define SUNXI_MMC_RINT_CARD_REMOVE      (0x1 << 31)
#define SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT      \
    (SUNXI_MMC_RINT_RESP_ERROR |        \
     SUNXI_MMC_RINT_RESP_CRC_ERROR |    \
     SUNXI_MMC_RINT_DATA_CRC_ERROR |    \
     SUNXI_MMC_RINT_RESP_TIMEOUT |      \
     SUNXI_MMC_RINT_DATA_TIMEOUT |      \
     SUNXI_MMC_RINT_VOLTAGE_CHANGE_DONE |   \
     SUNXI_MMC_RINT_FIFO_RUN_ERROR |    \
     SUNXI_MMC_RINT_HARD_WARE_LOCKED |  \
     SUNXI_MMC_RINT_START_BIT_ERROR |   \
     SUNXI_MMC_RINT_END_BIT_ERROR) /* 0xbfc2 */
#define SUNXI_MMC_RINT_INTERRUPT_DONE_BIT   \
    (SUNXI_MMC_RINT_AUTO_COMMAND_DONE | \
     SUNXI_MMC_RINT_DATA_OVER |     \
     SUNXI_MMC_RINT_COMMAND_DONE |      \
     SUNXI_MMC_RINT_VOLTAGE_CHANGE_DONE)

#define SUNXI_MMC_STATUS_RXWL_FLAG      (0x1 << 0)
#define SUNXI_MMC_STATUS_TXWL_FLAG      (0x1 << 1)
#define SUNXI_MMC_STATUS_FIFO_EMPTY     (0x1 << 2)
#define SUNXI_MMC_STATUS_FIFO_FULL      (0x1 << 3)
#define SUNXI_MMC_STATUS_CARD_PRESENT       (0x1 << 8)
#define SUNXI_MMC_STATUS_CARD_DATA_BUSY     (0x1 << 9)
#define SUNXI_MMC_STATUS_DATA_FSM_BUSY      (0x1 << 10)


#if 0
#define MMCDBG printf
#define MMCINFO printf
#define MMCMSG printf
#else
#define MMCDBG(...) do{}while(0)
#define MMCINFO(...) do{}while(0)
#define MMCMSG(...) do{}while(0)
#endif

#define writel(val, addr) put32(addr, val)
#define readl(addr)  get32(addr)

struct sunxi_mmc {
    uint32_t gctrl;      /* 0x00 global control */
    uint32_t clkcr;      /* 0x04 clock control */
    uint32_t timeout;        /* 0x08 time out */
    uint32_t width;      /* 0x0c bus width */
    uint32_t blksz;      /* 0x10 block size */
    uint32_t bytecnt;        /* 0x14 byte count */
    uint32_t cmd;        /* 0x18 command */
    uint32_t arg;        /* 0x1c argument */
    uint32_t resp0;      /* 0x20 response 0 */
    uint32_t resp1;      /* 0x24 response 1 */
    uint32_t resp2;      /* 0x28 response 2 */
    uint32_t resp3;      /* 0x2c response 3 */
    uint32_t imask;      /* 0x30 interrupt mask */
    uint32_t mint;       /* 0x34 masked interrupt status */
    uint32_t rint;       /* 0x38 raw interrupt status */
    uint32_t status;     /* 0x3c status */
    uint32_t ftrglevel;      /* 0x40 FIFO threshold watermark*/
    uint32_t funcsel;        /* 0x44 function select */
    uint32_t cbcr;       /* 0x48 CIU byte count */
    uint32_t bbcr;       /* 0x4c BIU byte count */
    uint32_t dbgc;       /* 0x50 debug enable */
    uint32_t csdc;       /* 0x54 CRC status detect control register */
    uint32_t a12a;       /* 0x58 Auto command 12 argument */
    uint32_t ntsr;       /* 0x5c New timing set register */
    uint32_t res1[6];
    uint32_t hwrst;      /* 0x78 SMC eMMC Hardware Reset Register */
    uint32_t res6;       /* 0x7c */
    uint32_t dmac;       /* 0x80 internal DMA control */
    uint32_t dlba;       /* 0x84 internal DMA descr list base address */
    uint32_t idst;       /* 0x88 internal DMA status */
    uint32_t idie;       /* 0x8c internal DMA interrupt enable */
    uint32_t chda;       /* 0x90 */
    uint32_t cbda;       /* 0x94 */
    uint32_t res2[90];
    uint32_t fifo;       /* 0x100 / 0x200 FIFO access address */
};

void _delay_msec(volatile uint64_t ms){
	proc_usleep(ms * 1000);
//	ms *= 100000;
//	while(ms--){
//		__asm("NOP");
//	};
}


static int mmc_rint_wait(void *priv, uint32_t timeout_msecs, uint32_t done_bit)
{
    unsigned int status;
    unsigned int done = 0;
    unsigned long start =0;
	struct sunxi_mmc *reg = (struct sunxi_mmc*)priv;

    do {
        status = reg->rint;
        if ((start > timeout_msecs) || (status & SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT)) {
            printf("mmc %d timeout %x status %x\n", 0,
                    status & SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT, status);
            return -1;
        }

        done = (status & done_bit);
		start++;
		_delay_msec(1);
    } while (!done);

    return 0;
}

static int mmc_trans_data_by_cpu(void *priv, struct mmc_data *data)
{
    const int reading = !!(data->flags & MMC_DATA_READ);
    const uint32_t status_bit = reading ? SUNXI_MMC_STATUS_FIFO_EMPTY :
                          SUNXI_MMC_STATUS_FIFO_FULL;
    unsigned i;
    unsigned *buff = (unsigned int *)(reading ? data->dest : data->src);
    unsigned byte_cnt = data->blocksize * data->blocks;
    unsigned timeout_msecs = byte_cnt;
    unsigned long  start;
	struct sunxi_mmc *reg = (struct sunxi_mmc*)priv;
	


    if (timeout_msecs < 2000)
        timeout_msecs = 2000;

    start = 0;

    for (i = 0; i < (byte_cnt >> 2); i++) {
        while (reg->status & status_bit) {
            if (start > timeout_msecs){
                return -1;
			}
			start++;
			_delay_msec(1);
        }
		
        if (reading)
            buff[i] = reg->fifo;
        else
            reg->fifo = buff[i];
    }
    return 0;
}

static void sunxi_mmc_set_rdtmout_reg(void *priv, unsigned int rdtmout)
{
    unsigned int rval = 0;
    unsigned int rdto_clk = 0;
	struct sunxi_mmc *reg = (struct sunxi_mmc*)priv;
	
    rdto_clk = (25000000 / 1000 * rdtmout) << 8;
    rval = reg->ntsr;

    rval = reg->gctrl;
    /*ddr50 mode don't use 256x timeout unit*/
   	rdto_clk = 0xffffff;
   	rval &= ~(0x1 << 11);

    reg->gctrl = rval;

    rval = reg->timeout;
    rval &= ~(0xffffff << 8);
    rval |= (rdto_clk << 8);
    reg->timeout = rval;
}

static int send_command(void *priv, struct mmc_cmd *cmd, struct mmc_data *data){

	unsigned int cmdval = SUNXI_MMC_CMD_START;
    unsigned int timeout_msecs;
    int error = 0;
    unsigned int status = 0;
    unsigned int timeout = 0;
	struct sunxi_mmc *reg = (struct sunxi_mmc*)priv;

    if (cmd->resp_type & MMC_RSP_PRESENT){
        cmdval |= SUNXI_MMC_CMD_RESP_EXPIRE;
    }

    if (cmd->resp_type & MMC_RSP_136){
        cmdval |= SUNXI_MMC_CMD_LONG_RESPONSE;
    }

    if (cmd->resp_type & MMC_RSP_CRC){
        cmdval |= SUNXI_MMC_CMD_CHK_RESPONSE_CRC;
    }

	if (data) {
        cmdval |= SUNXI_MMC_CMD_DATA_EXPIRE|SUNXI_MMC_CMD_WAIT_PRE_OVER;
        if (data->flags & MMC_DATA_WRITE)
            cmdval |= SUNXI_MMC_CMD_WRITE;
        if (data->blocks > 1)
            cmdval |= SUNXI_MMC_CMD_AUTO_STOP;
        reg->blksz = data->blocksize;
        reg->bytecnt = data->blocks * data->blocksize;
    }

    reg->arg = cmd->cmdarg;

    if (!data)
       reg->cmd = cmdval | cmd->cmdidx;
    else{
        /*dto set to 200ms*/
        sunxi_mmc_set_rdtmout_reg(priv, 200);

        reg->gctrl |= SUNXI_MMC_GCTRL_ACCESS_BY_AHB;
        reg->cmd = cmdval | cmd->cmdidx;
        error = mmc_trans_data_by_cpu(priv, data);

        if (error) {
            error = reg->rint & SUNXI_MMC_RINT_INTERRUPT_ERROR_BIT;
            goto out;
        }
    }

    error = mmc_rint_wait(priv, 1000, SUNXI_MMC_RINT_COMMAND_DONE);
    if (error) {
        goto out;
    }

	 if (data) {
        timeout_msecs = 6000;
        error = mmc_rint_wait(priv, timeout_msecs,
                      data->blocks > 1 ?
                      SUNXI_MMC_RINT_AUTO_COMMAND_DONE :
                      SUNXI_MMC_RINT_DATA_OVER);
        if (error) {
            goto out;
        }
    }

    if ((cmd->resp_type & MMC_RSP_BUSY) || ((data) && (data->flags & MMC_DATA_WRITE))) {
        timeout = 0;
        if ((cmd->cmdidx == MMC_CMD_ERASE) ||
                ((cmd->cmdidx == MMC_CMD_SWITCH) &&
                (((cmd->cmdarg >> 16) & 0xFF) == EXT_CSD_SANITIZE_START)))
            timeout_msecs = 0x1fffffff;
        else
            timeout_msecs = 2000;

        do {
            status = reg->status;
            if (timeout > timeout_msecs) {
                printf("busy timeout\n");
                error = -1;
                goto out;
            }
			timeout++;
			_delay_msec(1);
        } while (status & SUNXI_MMC_STATUS_CARD_DATA_BUSY);
    }

out:
    if (error < 0) {
        /* during tuning sample point, some sample point may cause timing problem.
        for example, if a RTO error occurs, host may stop clock and device may still output data.
        we need to read all data(512bytes) from device to avoid to update clock fail.
        */
        reg->gctrl |= 0x80000000;
        reg->dbgc |= 0xdeb;
        timeout = 1000;
        while (reg->bbcr < 512) {
            unsigned int tmp = reg->fifo;
            tmp = tmp + 1;
            _delay_msec(1);
            if (!(timeout--)) {
                break;
            }
		}
		
		reg->gctrl = 0x7;
    }
	return error;
}

/**
 * initialize EMMC to read SDHC card
 */
int32_t orangepi_sd_init(void) {
	_mmio_base = mmio_map();
	return 0;
}


static  struct mmc_cmd cmd;
static  struct mmc_data data;

static uint8_t sector_buf[1024] = {0};

int32_t orangepi_sd_read_sector(int32_t sector, void* buf) {

	char* p  = (char*)(((uint32_t)(sector_buf + 63))&(~(63)));
	int32_t  temp = sector;
    cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;
    cmd.cmdarg = temp;

    cmd.resp_type = MMC_RSP_R1;

    data.dest = p;
    data.blocks = 1;
    data.blocksize = 512;
    data.flags = MMC_DATA_READ;

    int ret = send_command((void*)(_mmio_base + 0x4020000L), &cmd, &data);
	if(ret){
        return -1;
    }
	for(int i = 0; i < 512; i++){
		*((char*)buf + i) = p[i];
	}
    //memcpy(buf, p, 512);
	return 0;
}

int32_t orangepi_sd_write_sector(int32_t sector, const void* buf) {

	return 0;
}

