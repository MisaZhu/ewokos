#include <ewoksys/mmio.h>
#include <ewoksys/klog.h>
#include "dwmmc.h"
#include "mmc.h"
#define PAGE_SIZE 4096
#define ARCH_DMA_MINALIGN 8
#define roundup(x, y)		((((x) + ((y) - 1)) / (y)) * (y))

#define min(a,b) ((a>b)?b:a)

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
        klog("%s: FIFO underflow timeout\n", __func__);
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
        buf = (uint32_t *)data->dest;
    else
        buf = (uint32_t *)data->src;

    timeout = 500;

    size /= 4;

    for (;;) {
        mask = dwmci_readl(host, DWMCI_RINTSTS);
        /* Error during data transfer. */
        if (mask & (DWMCI_DATA_ERR | DWMCI_DATA_TOUT)) {
            klog("%s: DATA ERROR %08x!\n", __func__, mask);
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
            //klog("%s: Timeout waiting for data!\n",
            //      __func__);
            ret = -ETIMEDOUT;
            break;
        }
        DWMMC_DELAY(100);
    }

    dwmci_writel(host, DWMCI_RINTSTS, mask);

    return ret;
}

static int dwmci_send_cmd(struct dwmci_host *host, struct mmc_cmd *cmd, struct mmc_data *data)
{

    TRACE();
    int ret = 0, flags = 0, i;
    int timeout = 500;
    int retry = 1000;
    uint32_t mask;

    TRACE();
    while (dwmci_readl(host, DWMCI_STATUS) & DWMCI_BUSY) {
        if (!timeout--) {
            klog("%s: Timeout on data busy\n", __func__);
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

    //klog("MMC CMD%d ",cmd->cmdidx);

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
        klog("%s: Timeout.\n", __func__);
        return -ETIMEDOUT;
    }

    if (mask & DWMCI_INTMSK_RTO) {
        /*
         * Timeout here is not necessarily fatal. (e)MMC cards
         * will splat here when they receive CMD55 as they do
         * not support this command and that is exactly the way
         * to tell them apart from SD cards. Thus, this output
         * below shall be klog(). eMMC cards also do not favor
         * CMD8, please keep that in mind.
         */
        klog("%s: Response Timeout.\n", __func__);
        return -ETIMEDOUT;
    } else if (mask & DWMCI_INTMSK_RE) {
        klog("%s: Response Error.\n", __func__);
        return -EIO;
    } else if ((cmd->resp_type & MMC_RSP_CRC) &&
           (mask & DWMCI_INTMSK_RCRC)) {
        klog("%s: Response CRC Error.\n", __func__);
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

int mmc_read_blocks(struct dwmci_host *host, void *dst, uint32_t sector)
{
    struct mmc_cmd cmd;
    struct mmc_data data;
    TRACE();

    cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;
    cmd.cmdarg = sector;

    cmd.resp_type = MMC_RSP_R1;

    data.dest = dst;
    data.blocks = 1;
    data.blocksize = 512;
    data.flags = MMC_DATA_READ;

    return dwmci_send_cmd(host, &cmd, &data);
}
