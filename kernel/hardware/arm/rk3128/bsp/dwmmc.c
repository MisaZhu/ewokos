#include <kernel/kernel.h>
#include <kernel/system.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "dwmmc.h"
#include "mmc.h"
#define ARCH_DMA_MINALIGN 8
#define roundup(x, y)		((((x) + ((y) - 1)) / (y)) * (y))

#define min(a,b) ((a>b)?b:a)
//#define TRACE(...)  do{printf("%s %d:", __func__, __LINE__);printf(__VA_ARGS__);}while(0)
//#define TRACE()  do{printf("%s %d\n", __func__, __LINE__);}while(0)
#define TRACE()  
void udelay(volatile int x){
    x*=50;
    while(x--){
        __asm("nop");
    }
}
#define DWMMC_DELAY(x) udelay(x) 

static int dwmci_wait_reset(struct dwmci_host *host, uint32_t value)
{
    unsigned long timeout = 1000;
    uint32_t ctrl;

    dwmci_writel(host, DWMCI_CTRL, value);

    while (timeout--) {
        ctrl = dwmci_readl(host, DWMCI_CTRL);
        if (!(ctrl & DWMCI_RESET_ALL))
            return 1;
        DWMMC_DELAY(10);
    }
    return 0;
}

static int dwmci_fifo_ready(struct dwmci_host *host, uint32_t bit, uint32_t *len)
{
    uint32_t timeout = 20000;

    *len = dwmci_readl(host, DWMCI_STATUS);
    while (--timeout && (*len & bit)) {
        *len = dwmci_readl(host, DWMCI_STATUS);
        DWMMC_DELAY(200);
    }

    if (!timeout) {
        printf("%s: FIFO underflow timeout\n", __func__);
        return -ETIMEDOUT;
    }
    return 0;
}

static int dwmci_set_transfer_mode(struct dwmci_host *host,
        struct mmc_data *data)
{
    (void)host;
    unsigned long mode;
 
    mode = DWMCI_CMD_DATA_EXP;
    if (data->flags & MMC_DATA_WRITE)
        mode |= DWMCI_CMD_RW;

    return mode;
}

static int dwmci_data_transfer(struct dwmci_host *host, struct mmc_data *data)
{
    int ret = 0;
    uint32_t timeout, mask, size, i, len = 0;
    uint32_t *buf = 0;
    uint32_t fifo_depth = (((host->fifoth_val & RX_WMARK_MASK) >>
                RX_WMARK_SHIFT) + 1) * 2;

    size = data->blocksize * data->blocks;
    if (data->flags == MMC_DATA_READ)
        buf = (uint32_t *)data->un.dest;
    else
        buf = (uint32_t *)data->un.src;

    timeout = 500;

    size /= 4;

    for (;;) {
        mask = dwmci_readl(host, DWMCI_RINTSTS);
        /* Error during data transfer. */
        if (mask & (DWMCI_DATA_ERR | DWMCI_DATA_TOUT)) {
            printf("%s: DATA ERROR %08lx!\n", __func__, mask);
            ret = -EINVAL;
            break;
        }

        if (host->fifo_mode && size) {
            len = 0;
            if (data->flags == MMC_DATA_READ &&
                (mask & (DWMCI_INTMSK_RXDR | DWMCI_INTMSK_DTO))) {
                dwmci_writel(host, DWMCI_RINTSTS,
                         DWMCI_INTMSK_RXDR | DWMCI_INTMSK_DTO);
                while (size) {
                    ret = dwmci_fifo_ready(host,
                            DWMCI_FIFO_EMPTY,
                            &len);
                    if (ret < 0)
                        break;

                    len = (len >> DWMCI_FIFO_SHIFT) &
                            DWMCI_FIFO_MASK;
                    len = min(size, len);
                    for (i = 0; i < len; i++)
                        *buf++ =
                        dwmci_readl(host, DWMCI_DATA);
                    size = size > len ? (size - len) : 0;
                                    }
            } else if (data->flags == MMC_DATA_WRITE &&
                   (mask & DWMCI_INTMSK_TXDR)) {
                while (size) {
                    ret = dwmci_fifo_ready(host,
                            DWMCI_FIFO_FULL,
                            &len);
                    if (ret < 0)
                        break;

                    len = fifo_depth - ((len >>
                           DWMCI_FIFO_SHIFT) &
                           DWMCI_FIFO_MASK);
                    len = min(size, len);
                    for (i = 0; i < len; i++)
                        dwmci_writel(host, DWMCI_DATA,
                                 *buf++);
                    size = size > len ? (size - len) : 0;
                }
                dwmci_writel(host, DWMCI_RINTSTS,
                         DWMCI_INTMSK_TXDR);
            }
        }

        /* Data arrived correctly. */
        if (mask & DWMCI_INTMSK_DTO) {
            ret = 0;
            break;
        }

        /* Check for timeout. */
        if (!timeout--) {
            printf("%s: Timeout waiting for data!\n",
                  __func__);
            ret = -ETIMEDOUT;
            break;
        }
        DWMMC_DELAY(200);
    }

    dwmci_writel(host, DWMCI_RINTSTS, mask);

    return ret;
}

static struct dwmci_host dwc_host = {
    .ioaddr = (void*) (MMIO_BASE + 0x214000),
    .fifoth_val = 0,
    .fifo_mode = true,
};

static int dwmci_send_cmd(struct mmc_cmd *cmd, struct mmc_data *data)
{

    TRACE();
    struct dwmci_host *host = &dwc_host;

    int ret = 0, flags = 0, i;
    int timeout = 500;
    int retry = 1000;
    uint32_t mask;

    TRACE();
    while (dwmci_readl(host, DWMCI_STATUS) & DWMCI_BUSY) {
        if (!timeout--) {
            printf("%s: Timeout on data busy\n", __func__);
            return -ETIMEDOUT;
        }
        TRACE();
        DWMMC_DELAY(100);
    }

    TRACE();
    dwmci_writel(host, DWMCI_RINTSTS, DWMCI_INTMSK_ALL);

    TRACE();
    if (data) {
        if (host->fifo_mode) {
            dwmci_writel(host, DWMCI_BLKSIZ, data->blocksize);
            dwmci_writel(host, DWMCI_BYTCNT,
                     data->blocksize * data->blocks);
            dwmci_wait_reset(host, DWMCI_CTRL_FIFO_RESET);
        } 
    }
    TRACE();
    dwmci_writel(host, DWMCI_CMDARG, cmd->cmdarg);

    if (data)
        flags = dwmci_set_transfer_mode(host, data);

    if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY))
        return -EBUSY;

    if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
        flags |= DWMCI_CMD_ABORT_STOP;
    else
        flags |= DWMCI_CMD_PRV_DAT_WAIT;

    if (cmd->resp_type & MMC_RSP_PRESENT) {
        flags |= DWMCI_CMD_RESP_EXP;
        if (cmd->resp_type & MMC_RSP_136)
            flags |= DWMCI_CMD_RESP_LENGTH;
    }

    if (cmd->resp_type & MMC_RSP_CRC)
        flags |= DWMCI_CMD_CHECK_CRC;

    flags |= (cmd->cmdidx | DWMCI_CMD_START | DWMCI_CMD_USE_HOLD_REG);

    //printf("MMC CMD%d ",cmd->cmdidx);

    dwmci_writel(host, DWMCI_CMD, flags);

    for (i = 0; i < retry; i++) {
        mask = dwmci_readl(host, DWMCI_RINTSTS);
        if (mask & DWMCI_INTMSK_CDONE) {
            if (!data)
                dwmci_writel(host, DWMCI_RINTSTS, mask);
            break;
        }
    }

    if (i == retry) {
        printf("%s: Timeout.\n", __func__);
        return -ETIMEDOUT;
    }

    if (mask & DWMCI_INTMSK_RTO) {
        /*
         * Timeout here is not necessarily fatal. (e)MMC cards
         * will splat here when they receive CMD55 as they do
         * not support this command and that is exactly the way
         * to tell them apart from SD cards. Thus, this output
         * below shall be printf(). eMMC cards also do not favor
         * CMD8, please keep that in mind.
         */
        printf("%s: Response Timeout.\n", __func__);
        return -ETIMEDOUT;
    } else if (mask & DWMCI_INTMSK_RE) {
        printf("%s: Response Error.\n", __func__);
        return -EIO;
    } else if ((cmd->resp_type & MMC_RSP_CRC) &&
           (mask & DWMCI_INTMSK_RCRC)) {
        printf("%s: Response CRC Error.\n", __func__);
        return -EIO;
    }


    if (cmd->resp_type & MMC_RSP_PRESENT) {
        if (cmd->resp_type & MMC_RSP_136) {
            cmd->response[0] = dwmci_readl(host, DWMCI_RESP3);
            cmd->response[1] = dwmci_readl(host, DWMCI_RESP2);
            cmd->response[2] = dwmci_readl(host, DWMCI_RESP1);
            cmd->response[3] = dwmci_readl(host, DWMCI_RESP0);
        } else {
            cmd->response[0] = dwmci_readl(host, DWMCI_RESP0);
        }
    }

    if (data) {
        ret = dwmci_data_transfer(host, data);
    }

    DWMMC_DELAY(100);
    return ret;
}

int mmc_set_blocklen(int len)
{
    struct mmc_cmd cmd;

    cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = len;


    TRACE();
    if(dwmci_send_cmd(&cmd, 0)){

        TRACE();
        return -1;
    }
    return 0;
}

int mmc_read_blocks(void *dst, uint32_t sector)
{
    struct mmc_cmd cmd;
    struct mmc_data data;
    TRACE();

    cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;
    cmd.cmdarg = sector;

    cmd.resp_type = MMC_RSP_R1;

    data.un.dest = dst;
    data.blocks = 1;
    data.blocksize = 512;
    data.flags = MMC_DATA_READ;

    return dwmci_send_cmd(&cmd, &data);
}

static int dwmci_setup_bus(struct dwmci_host *host)
{
    uint32_t status;
    int timeout = 10000;

    dwmci_writel(host, DWMCI_CLKENA, 0);
    dwmci_writel(host, DWMCI_CLKSRC, 0);

    dwmci_writel(host, DWMCI_CLKDIV, 0);
    dwmci_writel(host, DWMCI_CMD, DWMCI_CMD_PRV_DAT_WAIT |
            DWMCI_CMD_UPD_CLK | DWMCI_CMD_START);

    do {
        status = dwmci_readl(host, DWMCI_CMD);
        if (timeout-- < 0) {
            printf("%s: Timeout!\n", __func__);
            return -ETIMEDOUT;
        }
    } while (status & DWMCI_CMD_START);

    dwmci_writel(host, DWMCI_CLKENA, DWMCI_CLKEN_ENABLE |
            DWMCI_CLKEN_LOW_PWR);

    dwmci_writel(host, DWMCI_CMD, DWMCI_CMD_PRV_DAT_WAIT |
            DWMCI_CMD_UPD_CLK | DWMCI_CMD_START);

    timeout = 10000;
    do {
        status = dwmci_readl(host, DWMCI_CMD);
        if (timeout-- < 0) {
            printf("%s: Timeout!\n", __func__);
            return -ETIMEDOUT;
        }
    } while (status & DWMCI_CMD_START);

    return 0;
}

int dwmci_init(void)
{
    return 0;
    struct dwmci_host *host = &dwc_host;

    // if (host->board_init)
    //     host->board_init(host);

    dwmci_writel(host, DWMCI_PWREN, 1);

    if (!dwmci_wait_reset(host, DWMCI_RESET_ALL)) {
        printf("%s[%d] Fail-reset!!\n", __func__, __LINE__);
        return -EIO;
    }

    /* Enumerate at 400KHz */
    dwmci_setup_bus(host);

    dwmci_writel(host, DWMCI_RINTSTS, 0xFFFFFFFF);
    dwmci_writel(host, DWMCI_INTMASK, 0);

    dwmci_writel(host, DWMCI_TMOUT, 0xFFFFFFFF);

    dwmci_writel(host, DWMCI_IDINTEN, 0);
    dwmci_writel(host, DWMCI_BMOD, 1);

    if (!host->fifoth_val) {
        uint32_t fifo_size;

        fifo_size = dwmci_readl(host, DWMCI_FIFOTH);
        fifo_size = ((fifo_size & RX_WMARK_MASK) >> RX_WMARK_SHIFT) + 1;
        host->fifoth_val = MSIZE(0x2) | RX_WMARK(fifo_size / 2 - 1) |
                TX_WMARK(fifo_size / 2);
    }
    dwmci_writel(host, DWMCI_FIFOTH, host->fifoth_val);

    dwmci_writel(host, DWMCI_CLKENA, 0);
    dwmci_writel(host, DWMCI_CLKSRC, 0);

    if (!host->fifo_mode)
        dwmci_writel(host, DWMCI_IDINTEN, DWMCI_IDINTEN_MASK);

    dwmci_setup_bus(host);
    uint32_t ctype = DWMCI_CTYPE_4BIT;

    dwmci_writel(host, DWMCI_CTYPE, ctype);

    uint32_t regs = dwmci_readl(host, DWMCI_UHS_REG);
    regs &= ~DWMCI_DDR_MODE;

    dwmci_writel(host, DWMCI_UHS_REG, regs);
    
    mmc_set_blocklen(512);
    return 0;
}