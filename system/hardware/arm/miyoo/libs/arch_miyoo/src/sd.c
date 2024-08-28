#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <sysinfo.h>

#include "sdmmc.h"
static uint8_t *_sector_buf;

static RspStruct *_SDMMC_DATAReq(uint8_t u8Slot, uint8_t u8Cmd, uint32_t u32Arg, uint16_t u16BlkCnt, uint16_t u16BlkSize, TransEmType eTransType, volatile uint8_t *pu8Buf)
{
	IPEmType eIP = EV_IP_FCIE1;
	//RspErrEmType eErr  = EV_STS_OK;
	CmdEmType eCmdType = EV_CMDREAD;
	RspStruct * eRspSt;

	bool bCloseClock = FALSE;

	//klog("_[sdmmc_%u] CMD_%u (0x%08X)__(TB: %u)(BSz: %u)", u8Slot, u8Cmd, u32Arg, u16BlkCnt, u16BlkSize);

	if( (u8Cmd == 24) || (u8Cmd==25))
		eCmdType = EV_CMDWRITE;

	if(u16BlkCnt>1)
		bCloseClock = FALSE;
	else
		bCloseClock = TRUE;

	Hal_SDMMC_SetCmdToken(eIP, u8Cmd, u32Arg);
	Hal_SDMMC_TransCmdSetting(eIP, eTransType, u16BlkCnt, u16BlkSize, Hal_CARD_TransMIUAddr(pu8Buf - 0x60000000), pu8Buf);
	Hal_SDMMC_SendCmdAndWaitProcess(eIP, eTransType, eCmdType, EV_R1, bCloseClock);
	eRspSt = Hal_SDMMC_GetRspToken(eIP);

	//klog("=> (Err: 0x%04X)\n", (uint16_t)eRspSt->eErrCode);
	return eRspSt;


}

/**
 * initialize EMMC to read SDHC card
 */
int32_t miyoo_sd_init(void) {
	_mmio_base = mmio_map();
	_sector_buf = 0x87e00000;
	syscall3(SYS_MEM_MAP, _sector_buf, 0x27e00000, 4096);
	sdmmc_init();
	return 0;
}


int32_t miyoo_sd_read_sector(int32_t sector, void* buf) {
	static RspStruct * pstRsp;
	pstRsp = _SDMMC_DATAReq(0, 17, sector, 1, 512, EV_DMA, _sector_buf);  //CMD17
	memcpy(buf, _sector_buf, 512);
	return pstRsp->eErrCode;
}

int32_t miyoo_sd_write_sector(int32_t sector, const void* buf) {

	return 0;
}

