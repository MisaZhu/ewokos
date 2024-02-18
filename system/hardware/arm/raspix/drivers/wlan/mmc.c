#include "types.h"
#include "mmc.h"
#include "sdio.h"
#include "sdhci.h"

#define MMC_BLOCK_SIZE  512
static struct mmc _mmc;

int mmc_io_rw_direct_host(int write, unsigned fn,
	unsigned addr, u8 in, u8 *out)
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
	
	// if(out)
    //     klog("%s w:%d f:%d a:%x in:%x out:%x\n", __func__, write, fn, addr, in, cmd.response[0] & 0xFF);
    // else
    //     klog("%s w:%d f:%d a:%x in:%x\n", __func__, write, fn, addr, in);

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
	unsigned addr, u8 in, u8 *out)
{
	return mmc_io_rw_direct_host(write, fn, addr, in, out);
}

int mmc_io_rw_extended(int write, int fn,
	unsigned addr, int incr_addr, u8 *buf, unsigned blocks, unsigned blksz)
{
	struct mmc_cmd cmd = {};
	struct mmc_data data = {};
	int err;

	WARN_ON(blksz == 0);
 	//klog("%s w:%d f:%d a:%x%s b:%d s:%d\n", __func__, write, fn, addr, incr_addr?"+":" ", blocks, blksz);
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

    sdhci_send_command(&cmd, &data);

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


static int mmc_go_idle(struct mmc *mmc)
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

void mmc_set_bus_mode(struct mmc* mmc, unsigned int mode)
{
	mmc->selected_mode = mode;
    sdhci_set_ios(mmc);;
}

// static int mmc_send_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr)
// {
// 	struct mmc_cmd cmd = {};
// 	int err = 0;

// 	cmd.cmdidx = MMC_SEND_OP_COND;
// 	cmd.cmdarg = mmc_host_is_spi(host) ? 0 : ocr;
// 	cmd.resp_type = MMC_RSP_SPI_R1 | MMC_RSP_R3 | MMC_CMD_BCR;

//     sdhci_send_command(&cmd, NULL);
// 	if (err)
// 		return err;

// 	if (rocr && !mmc_host_is_spi(host))
// 		*rocr = cmd.response[0];

// 	return err;
// }

int mmc_set_relative_addr(struct mmc *mmc)
{
	struct mmc_cmd cmd = {};

	cmd.cmdidx = MMC_SET_RELATIVE_ADDR;
	cmd.cmdarg = mmc->rca << 16;
	cmd.resp_type = MMC_RSP_R1 | MMC_CMD_AC;

	return sdhci_send_command(&cmd, NULL);
}


static int mmc_send_cxd_native(struct mmc *mmc, u32 arg, u32 *cxd, int opcode)
{
	int err;
	struct mmc_cmd cmd = {};

	cmd.cmdidx = opcode;
	cmd.cmdarg = arg;
	cmd.resp_type = MMC_RSP_R2 | MMC_CMD_AC;

	err = sdhci_send_command(&cmd, NULL);
	if (err)
		return err;

	memcpy(cxd, cmd.response, sizeof(u32) * 4);

	return 0;
}

int mmc_send_cid(struct mmc *mmc, u32 *cid)
{
    return mmc_send_cxd_native(mmc, 0, cid, MMC_ALL_SEND_CID);
}

int mmc_send_if_cond(struct mmc_host *host, u32 ocr)
{
	struct mmc_cmd cmd = {0};
	int err;
	static const u8 test_pattern = 0xAA;
	u8 result_pattern;

	/*
	 * To support SD 2.0 cards, we must always invoke SD_SEND_IF_COND
	 * before SD_APP_OP_COND. This command will harmlessly fail for
	 * SD 1.0 cards.
	 */
	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	cmd.cmdarg = ((ocr & 0xFF8000) != 0) << 8 | test_pattern;
	cmd.resp_type = MMC_RSP_R7;

	err = sdhci_send_command(&cmd, 0);
	if (err)
		return err;

	result_pattern = cmd.response[0] & 0xFF;

	if (result_pattern != test_pattern)
		return -EIO;

	return 0;
}

int sd_send_op_cond(struct mmc *mmc)
{
	struct mmc_cmd cmd = {0};
	int i, err = 0;

	for (i = 100; i; i--) {

		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;

		err = sdhci_send_command(&cmd, NULL);

		if (err)
			return err;

		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.cmdarg = (mmc->version & 0xff8000);
		cmd.resp_type = MMC_RSP_R3;



		err =sdhci_send_command(&cmd, NULL);
		if (err)
			break;

		if (cmd.response[0] & OCR_BUSY)
			break;

		err = -ETIMEDOUT;

		usleep(1000);
	}

	mmc->ocr = cmd.response[0];

	return err;
}

static const int fbase[] = {
        10000,
        100000,
        1000000,
        10000000,
    };

static const uint8_t multipliers[] = {
	0,	/* reserved */
	10,
	12,
	13,
	15,
	20,
	25,
	30,
	35,
	40,
	45,
	50,
	55,
	60,
	70,
	80,
};

static int mmc_send_op_cond_iter(struct mmc *mmc, int use_arg)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = MMC_CMD_SEND_OP_COND;
	cmd.resp_type = MMC_RSP_R3;
	cmd.cmdarg = 0;
	if (use_arg && !mmc_host_is_spi(mmc))
		cmd.cmdarg = OCR_HCS |
			(mmc->voltages &
			(mmc->ocr & OCR_VOLTAGE_MASK)) |
			(mmc->ocr & OCR_ACCESS_MODE);

	err = sdhci_send_command(&cmd, NULL);
	if (err)
		return err;
	mmc->ocr = cmd.response[0];
	return 0;
}


static int mmc_send_op_cond(struct mmc *mmc)
{
	int err, i;
	int timeout = 1000;
	uint start;

	/* Some cards seem to need this */
	mmc_go_idle(mmc);

	start = get_timer(0);
	/* Asking to the card its capabilities */
	for (i = 0; ; i++) {
		err = mmc_send_op_cond_iter(mmc, i != 0);
		if (err)
			return err;

		/* exit if not busy (flag seems to be inverted) */
		if (mmc->ocr & OCR_BUSY)
			break;

		if (get_timer(start) > timeout)
			return -ETIMEDOUT;
		usleep(100);
	}
	mmc->op_cond_pending = 1;
	return 0;
}


static int mmc_startup(struct mmc *mmc)
{
	int err, i;
	uint32_t mult, freq;
	uint64_t cmult, csize;
	struct mmc_cmd cmd;

	/* Put the Card in Identify Mode */
	cmd.cmdidx = MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;

	err = mmc_send_cmd(&cmd, NULL);
	if (err)
		return err;

	memcpy(mmc->cid, cmd.response, 16);

	cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
	cmd.cmdarg = mmc->rca << 16;
	cmd.resp_type = MMC_RSP_R6;

	err = sdhci_send_command(&cmd, NULL);

	if (err)
		return err;

	if (IS_SD(mmc))
		mmc->rca = (cmd.response[0] >> 16) & 0xffff;


	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = mmc->rca << 16;

	err = sdhci_send_command(&cmd, NULL);

	if (err)
		return err;

	mmc->csd[0] = cmd.response[0];
	mmc->csd[1] = cmd.response[1];
	mmc->csd[2] = cmd.response[2];
	mmc->csd[3] = cmd.response[3];

	if (mmc->version == MMC_VERSION_UNKNOWN) {
		int version = (cmd.response[0] >> 26) & 0xf;

		switch (version) {
		case 0:
			mmc->version = MMC_VERSION_1_2;
			break;
		case 1:
			mmc->version = MMC_VERSION_1_4;
			break;
		case 2:
			mmc->version = MMC_VERSION_2_2;
			break;
		case 3:
			mmc->version = MMC_VERSION_3;
			break;
		case 4:
			mmc->version = MMC_VERSION_4;
			break;
		default:
			mmc->version = MMC_VERSION_1_2;
			break;
		}
	}

	/* divide frequency by 10, since the mults are 10x bigger */
	freq = fbase[(cmd.response[0] & 0x7)];
	mult = multipliers[((cmd.response[0] >> 3) & 0xf)];

	mmc->legacy_speed = freq * mult;
	//mmc_select_mode(mmc, MMC_LEGACY);

	mmc->dsr_imp = ((cmd.response[1] >> 12) & 0x1);
	mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);
	if (IS_SD(mmc))
		mmc->write_bl_len = mmc->read_bl_len;
	else
		mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);

	if (mmc->high_capacity) {
		csize = (mmc->csd[1] & 0x3f) << 16
			| (mmc->csd[2] & 0xffff0000) >> 16;
		cmult = 8;
	} else {
		csize = (mmc->csd[1] & 0x3ff) << 2
			| (mmc->csd[2] & 0xc0000000) >> 30;
		cmult = (mmc->csd[2] & 0x00038000) >> 15;
	}

	mmc->capacity_user = (csize + 1) << (cmult + 2);
	mmc->capacity_user *= mmc->read_bl_len;
	mmc->capacity_boot = 0;
	mmc->capacity_rpmb = 0;
	for (i = 0; i < 4; i++)
		mmc->capacity_gp[i] = 0;

	if (mmc->read_bl_len > MMC_MAX_BLOCK_LEN)
		mmc->read_bl_len = MMC_MAX_BLOCK_LEN;

	if (mmc->write_bl_len > MMC_MAX_BLOCK_LEN)
		mmc->write_bl_len = MMC_MAX_BLOCK_LEN;

	if ((mmc->dsr_imp) && (0xffffffff != mmc->dsr)) {
		cmd.cmdidx = MMC_CMD_SET_DSR;
		cmd.cmdarg = (mmc->dsr & 0xffff) << 16;
		cmd.resp_type = MMC_RSP_NONE;
		if (sdhci_send_command(&cmd, NULL))
			klog("MMC: SET_DSR failed\n");
	}

	/* Select the card, and put it into Transfer Mode */
	if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = MMC_CMD_SELECT_CARD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = mmc->rca << 16;
		err = sdhci_send_command(&cmd, NULL);

		if (err)
			return err;
	}

	mmc->bus_width = 4;
	mmc->clock = 50000000;
	sdhci_set_ios(mmc);
	/*
	 * For SD, its erase group is always one sector
	 */
	mmc->erase_grp_size = 1;
	mmc->part_config = MMCPART_NOAVAILABLE;
	mmc->has_init = 1;
	return 0;
}



void sdio_claim_host(int func){

}

void sdio_release_host(int func){

}

/*********************************************/

static int brcm_init(struct mmc *mmc)
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


static int mmc_init_card(struct mmc *mmc, u32 ocr)
{
	int err;
	u32 cid[4];
	u32 rocr;
	
	mmc_go_idle(mmc);
	usleep(200000);

	// for(int i = 0; i < 50; i++){

	// 	usleep(100000);	
	// }
	//mmc_send_if_cond(mmc, MMC_VDD_165_195|MMC_VDD_32_33|MMC_VDD_33_34);
	brcm_init(mmc);
	// if(sd_send_op_cond(mmc) < 0){
	//mmc_send_op_cond(mmc);
	// }
	// mmc_startup(mmc);
	return 0;
}

int mmc_hw_reset()
{
    sdhci_init();
    _mmc.bus_width = 1;
    _mmc.clock = 400000;
    _mmc.ocr = 1;
	_mmc.voltages =  MMC_VDD_32_33 | MMC_VDD_33_34|MMC_VDD_165_195;

	return mmc_init_card(&_mmc, _mmc.ocr);
}