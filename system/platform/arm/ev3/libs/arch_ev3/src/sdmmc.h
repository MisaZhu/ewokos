/*
* hal_sdmmc_v5.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: truman.yang <truman.yang@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#ifndef __HAL_SDMMC_V5_H
#define __HAL_SDMMC_V5_H

#include <stdint.h>
#include <stdbool.h>
#include "sdmmc_reg.h"

//***********************************************************************************************************
// Config Setting (Externel)
//***********************************************************************************************************

#define WT_EVENT_RSP                10      //(ms)
#define WT_EVENT_READ               2000    //(ms)
#define WT_EVENT_WRITE              3000    //(ms)
#define EN_BIND_CARD_INT            (FALSE)

#define BIT00_T 0x0001
#define BIT01_T 0x0002
#define BIT02_T 0x0004
#define BIT03_T 0x0008
#define BIT04_T 0x0010
#define BIT05_T 0x0020
#define BIT06_T 0x0040
#define BIT07_T 0x0080
#define BIT08_T 0x0100
#define BIT09_T 0x0200
#define BIT10_T 0x0400
#define BIT11_T 0x0800
#define BIT12_T 0x1000
#define BIT13_T 0x2000
#define BIT14_T 0x4000
#define BIT15_T 0x8000
#define BIT16_T 0x00010000
#define BIT17_T 0x00020000
#define BIT18_T 0x00040000
#define BIT19_T 0x00080000
#define BIT20_T 0x00100000
#define BIT21_T 0x00200000
#define BIT22_T 0x00400000
#define BIT23_T 0x00800000
#define BIT24_T 0x01000000
#define BIT25_T 0x02000000
#define BIT26_T 0x04000000
#define BIT27_T 0x08000000
#define BIT28_T 0x10000000
#define BIT29_T 0x20000000
#define BIT30_T 0x40000000
#define BIT31_T 0x80000000

typedef enum
{
	//SD_STS Reg Error
	EV_STS_OK				= 0x0000,
    EV_STS_RD_CERR 		    = BIT00_T,
    EV_STS_WD_CERR 		    = BIT01_T,
    EV_STS_WR_TOUT 		    = BIT02_T,
    EV_STS_NORSP 			= BIT03_T,
    EV_STS_RSP_CERR 		= BIT04_T,
	EV_STS_RD_TOUT 		    = BIT05_T,

	//SD IP Error
	EV_STS_RIU_ERR 			= BIT06_T,
	EV_STS_DAT0_BUSY		= BIT07_T,
	EV_STS_MIE_TOUT			= BIT08_T,

	//Stop Wait Process Error
	EV_SWPROC_ERR           = BIT09_T,

	//SD Check Error
	EV_CMD8_PERR			= BIT10_T,
	EV_OCR_BERR				= BIT11_T,
	EV_OUT_VOL_RANGE		= BIT12_T,
	EV_STATE_ERR			= BIT13_T,

	//Other Error
	EV_OTHER_ERR            = BIT15_T,


} RspErrEmType;

typedef enum
{
	EV_CMDRSP	=0x000,
	EV_CMDREAD	=0x001,
	EV_CMDWRITE	=0x101,

} CmdEmType;

typedef enum
{
	EV_EMP	= 0x0000,
	EV_ADMA = 0x0020,  //Add at FCIE5
	EV_DMA	= 0x0080,  //Change at FCIE5
	EV_CIF	= 0x1000,  //Change at FCIE5

} TransEmType;

//(2bits: Rsp Mapping to SD_CTL) (4bits: Identity) (8bits: RspSize)
typedef enum
{
	EV_NO	= 0x0000,	//No response type
	EV_R1	= 0x2105,
	EV_R1B 	= 0x2205,
	EV_R2	= 0x3310,
	EV_R3	= 0x2405,
	EV_R4	= 0x2505,
	EV_R5	= 0x2605,
	EV_R6	= 0x2705,
	EV_R7	= 0x2805,

} SDMMCRspEmType;

typedef enum
{
	EV_BUS_1BIT	 = 0x00,
	EV_BUS_4BITS = 0x02,
	EV_BUS_8BITS = 0x04,

} SDMMCBusWidthEmType;

typedef enum
{
	EV_MIE  = 0x0,
	EV_CIFD = 0x1,

} IPEventEmType;


typedef enum
{
	EV_EGRP_OK       = 0x0,
	EV_EGRP_TOUT     = 0x1,
	EV_EGRP_COMM     = 0x2,
	EV_EGRP_OTHER    = 0x3,

} ErrGrpEmType;

typedef struct
{
	uint8_t u8Cmd;
	uint32_t u32Arg;              //Mark for ROM
	uint32_t u32ErrLine;          //Mark for ROM
	RspErrEmType eErrCode;
	uint8_t u8RspSize;            //Mark for ROM
	uint8_t u8ArrRspToken[0x10];    //uint8_t u8ArrRspToken[0x10];  //Mark for ROM

} RspStruct;


typedef struct
{
	uint32_t u32_End     : 1;
	uint32_t u32_MiuSel  : 2;
	uint32_t             : 13;
	uint32_t u32_JobCnt  : 16;
	uint32_t u32_Address;
	uint32_t u32_DmaLen;
	uint32_t u32_Dummy;

} AdmaDescStruct;

typedef enum
{
	EV_IP_FCIE1     = 0,
	EV_IP_FCIE2     = 1,
	EV_IP_FCIE3     = 2,

} IPEmType;


typedef enum
{
	EV_BUS_LOW     = 0,
	EV_BUS_DEF     = 1,
	EV_BUS_HS      = 2,
	EV_BUS_SDR12   = 3,
	EV_BUS_SDR25   = 4,
	EV_BUS_SDR50   = 5,
	EV_BUS_SDR104  = 6,
	EV_BUS_DDR50   = 7,
	EV_BUS_HS200   = 8,

} BusTimingEmType;

typedef enum
{
	EV_OK	= 0,
	EV_FAIL	= 1,

} RetEmType;

typedef enum
{
    EV_POWER_OFF = 0,
    EV_POWER_ON  = 1,
    EV_POWER_UP  = 2,

} PowerEmType;
// SDMMC Function
//----------------------------------------------------------------------------------------------------------
 void _REG_ResetIP(IPEmType eIP);
void Hal_SDMMC_SetSDIODevice(IPEmType eIP, bool bEnable);
void Hal_SDMMC_SetSDIOIntDet(IPEmType eIP, bool bEnable);

void Hal_SDMMC_SetDataWidth(IPEmType eIP, SDMMCBusWidthEmType eBusWidth);
void Hal_SDMMC_SetBusTiming(IPEmType eIP, BusTimingEmType eBusTiming);
void Hal_SDMMC_SetNrcDelay(IPEmType eIP, uint32_t u32RealClk);

void Hal_SDMMC_SetCmdToken(IPEmType eIP, uint8_t u8Cmd, uint32_t u32Arg);
RspStruct* Hal_SDMMC_GetRspToken(IPEmType eIP);
void Hal_SDMMC_TransCmdSetting(IPEmType eIP, TransEmType eTransType, uint16_t u16BlkCnt, uint16_t u16BlkSize, volatile uint32_t u32BufAddr, volatile uint8_t *pu8Buf);
RspErrEmType Hal_SDMMC_SendCmdAndWaitProcess(IPEmType eIP, TransEmType eTransType, CmdEmType eCmdType, SDMMCRspEmType eRspType, bool bCloseClk);

RspErrEmType Hal_SDMMC_RunBrokenDmaAndWaitProcess(IPEmType eIP, CmdEmType eCmdType);
void Hal_SDMMC_ADMASetting(IPEmType eIP, volatile void *pDMATable, uint8_t u8Item, uint32_t u32SubDMALen, uint32_t u32SubDMAAddr, uint8_t u8MIUSel, bool bEnd);

ErrGrpEmType Hal_SDMMC_ErrGroup(RspErrEmType eErrType);

void Hal_SDMMC_ClkCtrl(IPEmType eIP, bool bOpen, uint16_t u16DelayMs);
void Hal_SDMMC_Reset(IPEmType eIP);
void Hal_SDMMC_WaitProcessCtrl(IPEmType eIP, bool bStop);
bool Hal_SDMMC_OtherPreUse(IPEmType eIP);

void Hal_SDMMC_DumpMemTool(uint8_t u8ListNum, volatile uint8_t *pu8Buf);
uint8_t Hal_SDMMC_GetDATBusLevel(IPEmType eIP);
uint16_t Hal_SDMMC_GetMIEEvent(IPEmType eIP);

uint32_t Hal_CARD_TransMIUAddr(uint32_t u32Addr);

#endif //End of __HAL_SDMMC_V5_H





