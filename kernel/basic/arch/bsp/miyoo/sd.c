#include <dev/sd.h>

#include "sdmmc.h"
#include <mm/mmu.h>

uint8_t *_sector_buf = 0x87E00000;
// static IPEmType ge_IPSlot[3]     = {D_SDMMC1_IP, D_SDMMC2_IP, D_SDMMC3_IP};
// static PortEmType ge_PORTSlot[3] = {D_SDMMC1_PORT, D_SDMMC2_PORT, D_SDMMC3_PORT};
// static PADEmType  ge_PADSlot[3]  = {D_SDMMC1_PAD, D_SDMMC2_PAD, D_SDMMC3_PAD};
// static U32_T  gu32_MaxClkSlot[3] = {V_SDMMC1_MAX_CLK, V_SDMMC2_MAX_CLK, V_SDMMC3_MAX_CLK};

uint16_t SDMMC_Init(uint8_t u8Slot)
{
	IPEmType eIP  = EV_IP_FCIE1;
	RspStruct * eRspSt;

	//_SDMMC_InfoInit(u8Slot);

	// SDMMC_SwitchPAD(u8Slot);
	// SDMMC_SetPower(u8Slot, EV_POWER_OFF);
	// SDMMC_SetPower(u8Slot, EV_POWER_ON);
	// SDMMC_SetPower(u8Slot, EV_POWER_UP);

	// SDMMC_SetClock(u8Slot, 400000, 0);
	// SDMMC_SetBusTiming(u8Slot, EV_BUS_LOW);

	// Hal_SDMMC_SetDataWidth(eIP, EV_BUS_1BIT);
	//Hal_SDMMC_SetSDIOClk(eIP, TRUE); //For Measure Clock, Don't Stop Clock

    // //--------------------------------------------------------------------------------------------------------
    // eRspSt = _SDMMC_Identification(u8Slot);
    // //--------------------------------------------------------------------------------------------------------
    // if(eRspSt->eErrCode)
    //     pr_sd_info(" errCmd:%d:x%x\n", eRspSt->u8Cmd, eRspSt->eErrCode);
    // if(eRspSt->eErrCode)
	// 	return (uint16_t)eRspSt->eErrCode;

    // //--------------------------------------------------------------------------------------------------------
    // eRspSt = _SDMMC_CMDReq(u8Slot, 9, 0, EV_R2);  //CMD9
    // //--------------------------------------------------------------------------------------------------------
    // if(eRspSt->eErrCode)
	// 	return (uint16_t)eRspSt->eErrCode;
	// else
	// 	_SDMMC_GetCSDInfo(u8Slot, eRspSt->u8ArrRspToken+1);

    // //--------------------------------------------------------------------------------------------------------
    // eRspSt = _SDMMC_SEND_STAUS(u8Slot); //CMD13
    // //--------------------------------------------------------------------------------------------------------
    // if(eRspSt->eErrCode)
	// 	return (uint16_t)eRspSt->eErrCode;

    // //--------------------------------------------------------------------------------------------------------
    // eRspSt = _SDMMC_CMDReq(u8Slot,  7, 0, EV_R1B); //CMD7;
    // //--------------------------------------------------------------------------------------------------------
    // if(eRspSt->eErrCode)
	// 	return (uint16_t)eRspSt->eErrCode;

	// eRspSt = _SDMMC_SEND_SCR(u8Slot, _sector_buf);


    // return (uint16_t)eRspSt->eErrCode;
	return 0;

}

int32_t sd_init(void) {
	return SDMMC_Init(0);
}

static RspStruct *_SDMMC_DATAReq(uint8_t u8Slot, uint8_t u8Cmd, uint32_t u32Arg, uint16_t u16BlkCnt, uint16_t u16BlkSize, TransEmType eTransType, volatile uint8_t *pu8Buf)
{
	IPEmType eIP = EV_IP_FCIE1;
	//RspErrEmType eErr  = EV_STS_OK;
	CmdEmType eCmdType = EV_CMDREAD;
	RspStruct * eRspSt;

	bool bCloseClock = FALSE;

	//printf("_[sdmmc_%u] CMD_%u (0x%08X)__(TB: %u)(BSz: %u)", u8Slot, u8Cmd, u32Arg, u16BlkCnt, u16BlkSize);

	if( (u8Cmd == 24) || (u8Cmd==25))
		eCmdType = EV_CMDWRITE;

	if(u16BlkCnt>1)
		bCloseClock = FALSE;
	else
		bCloseClock = TRUE;

	Hal_SDMMC_SetCmdToken(eIP, u8Cmd, u32Arg);
	Hal_SDMMC_TransCmdSetting(eIP, eTransType, u16BlkCnt, u16BlkSize, Hal_CARD_TransMIUAddr(V2P(pu8Buf)), pu8Buf);
	Hal_SDMMC_SendCmdAndWaitProcess(eIP, eTransType, eCmdType, EV_R1, bCloseClock);
	eRspSt = Hal_SDMMC_GetRspToken(eIP);

	//printf("=> (Err: 0x%04X)\n", (uint16_t)eRspSt->eErrCode);
	return eRspSt;


}

int32_t sd_dev_read(int32_t sector) {
	static RspStruct * _pstRsp;
	//------------------------------------------------------------------------------------------------------------
	_pstRsp = _SDMMC_DATAReq(0, 17, sector, 1, 512, EV_DMA, _sector_buf);  //CMD17
	//------------------------------------------------------------------------------------------------------------
	return 0;//_pstRsp->eErrCode;
}

int32_t sd_dev_read_done(void* buf) {
	memcpy(buf, _sector_buf, 512);
	return 0;
}

int32_t sd_dev_write(int32_t sector, const void* buf) {
	return -1;
}

int32_t sd_dev_write_done(void) {
	return -1;
}
