#include <types.h>
#include <utils/log.h>

#include "mmc.h"
#include "sdio.h"
#include "sdhci.h"


#define MMC_DEBUG	0
#define mmc_host_is_spi(mmc)	(0)

static struct mmc _mmc;

int mmc_io_rw_direct_host(int write, unsigned fn,
	unsigned addr, uint8_t in, uint8_t *out)
{
	struct mmc_cmd cmd = {};
	int err;

	/* sanity check */
	if (addr & ~0x1FFFF)
		return -EINVAL;

	cmd.cmdidx = SD_IO_RW_DIRECT;
	cmd.cmdarg = write ? 0x80000000 : 0x00000000;
	cmd.cmdarg |= fn << 28;
	cmd.cmdarg |= (write && out) ? 0x08000000 : 0x00000000;
	cmd.cmdarg |= addr << 9;
	cmd.cmdarg |= in;
	cmd.resp_type = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_AC;

	err = sdhci_send_command(&cmd, 0);

#if MMC_DEBUG
	if(out)
        brcm_log("%s w:%d f:%d a:%x in:%x out:%x\n", __func__, write, fn, addr, in, cmd.response[0] & 0xFF);
    else
        brcm_log("%s w:%d f:%d a:%x in:%x\n", __func__, write, fn, addr, in);
#endif

	if (err){
		return err;
	}

	if (cmd.response[0] & R5_ERROR){
		return -EIO;
	}if (cmd.response[0] & R5_FUNCTION_NUMBER){
		return -EINVAL;
	}if (cmd.response[0] & R5_OUT_OF_RANGE){
		return -ERANGE;
	}

	if (out) {
		if (mmc_host_is_spi(host))
			*out = (cmd.response[0] >> 8) & 0xFF;
		else
			*out = cmd.response[0] & 0xFF;
	}

	return 0;
}

int mmc_io_rw_direct( int write, unsigned fn,
	unsigned addr, uint8_t in, uint8_t *out)
{
	return mmc_io_rw_direct_host(write, fn, addr, in, out);
}

int mmc_io_rw_extended(int write, int fn,
	unsigned addr, int incr_addr, uint8_t *buf, unsigned blocks, unsigned blksz)
{
	struct mmc_cmd cmd = {};
	struct mmc_data data = {};
	int err;

	WARN_ON(blksz == 0);

	/* sanity check */
	if (addr & ~0x1FFFF)
		return -EINVAL;

	cmd.cmdidx = SD_IO_RW_EXTENDED;
	cmd.cmdarg = write ? 0x80000000 : 0x00000000;
	cmd.cmdarg |= fn << 28;
	cmd.cmdarg |= incr_addr ? 0x04000000 : 0x00000000;
	cmd.cmdarg |= addr << 9;
	if (blocks == 0)
		cmd.cmdarg |= (blksz == 512) ? 0 : blksz;	/* byte mode */
	else
		cmd.cmdarg |= 0x08000000 | blocks;		/* block mode */
	cmd.resp_type = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;

	data.blocksize = blksz;
	/* Code in host drivers/fwk assumes that "blocks" always is >=1 */
	data.blocks = blocks ? blocks : 1;
	data.flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
    data.src = buf;

    err = sdhci_send_command(&cmd, &data);

#if MMC_DEBUG
	if(fn == 2){ //dont dump interrupt and console data
		brcm_log("%s w:%d f:%d a:%x%s b:%d s:%d r:%d ",
			__func__, write, fn, addr, incr_addr?"+":" ", blocks, blksz, err);
		if(blksz <= 4 || fn != 2)
			brcm_log("[%02x %02x %02x %02x]\n", buf[0], buf[1], buf[2], buf[3]);
		else
			hexdump("", buf, min(blksz, 256));
	}
#endif
	if (err){
		return err;
	}

    if (cmd.response[0] & R5_ERROR)
		err = -EIO;
	else if (cmd.response[0] & R5_FUNCTION_NUMBER)
		err = -EINVAL;
	else if (cmd.response[0] & R5_OUT_OF_RANGE)
		err = -ERANGE;
	else
		err = 0;

	return err;
}

static int mmc_go_idle(void)
{
	struct mmc_cmd cmd;
	int err;

	usleep(1000);

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;

	err = sdhci_send_command(&cmd, NULL);

	if (err)
		return err;

	return 0;
}

/*
* dump form linux kernel
*/
static int brcm_init(void)
{
	struct mmc_cmd cmd;
	int err;

	usleep(1000); 

	cmd.cmdidx = 5;
	cmd.cmdarg = 0x0;
	cmd.resp_type = 0x2e1;

	err = sdhci_send_command(&cmd, NULL);

	if (err)
		return err;

	cmd.cmdidx = 5;
	cmd.cmdarg = 0x300000;
	cmd.resp_type = 0x2e1;

	err = sdhci_send_command(&cmd, NULL);

	if (err)
		return err;

	cmd.cmdidx = 3;
	cmd.cmdarg = 0x0;
	cmd.resp_type = 0x75;

	err = sdhci_send_command(&cmd, NULL);

	if (err)
		return err;

	cmd.cmdidx = 7;
	cmd.cmdarg = 0x10000;
	cmd.resp_type = 0x15;

	err = sdhci_send_command(&cmd, NULL);

	if (err)
		return err;
	return 0;
}


static int mmc_init_card(void)
{
	mmc_go_idle();
	usleep(200000);

	brcm_init();
	return 0;
}

int mmc_hw_reset(void)
{
    sdhci_init();
    _mmc.bus_width = 1;
    _mmc.clock = 400000;
    _mmc.ocr = 1;
	_mmc.voltages =  MMC_VDD_32_33 | MMC_VDD_33_34|MMC_VDD_165_195;

	return mmc_init_card();
}