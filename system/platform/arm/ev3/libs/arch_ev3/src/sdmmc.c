/*
* hal_sdmmc_v5.c- Sigmastar
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

/***************************************************************************************************************
 *
 * FileName hal_sdmmc_v5.c  (Driver Version)
 *     @author jeremy.wang (2015/07/06)
 * Desc:
 *     HAL SD Driver will support basic SD function but not flow process.
 *     (1) It included Register and Buffer opertion code.
 *     (2) h file will has APIs for Driver, but just SD abstract, not register abstract.
 *
 ***************************************************************************************************************/
#include <ewoksys/mmio.h>
#include "sdmmc.h"


#define prtstring(s)	klog(s)
#define prtUInt(v) 		klog("%u", v)
#define prtU8(v)		klog("0x%02X", v)
#define prtU8Hex(v)		klog("0x%02X", v)
#define prtU16Hex(v)	klog("0x%04X", v)
#define prtU32Hex(v)	klog("0x%08X", v)

static inline void __attribute__((optimize("O0"))) delay(uint32_t count) {
	uint32_t c = count*100;
	while(c > 0) c--;
}

#define Hal_Timer_mDelay(x)     delay(x*1000)
#define Hal_Timer_uDelay(x)     delay(x)

#define TR_H_SDMMC(p) 
//***********************************************************************************************************
// Config Setting (Internal)
//***********************************************************************************************************

// Enable Setting
//-----------------------------------------------------------------------------------------------------------
#define EN_TRFUNC                   (FALSE)
#define EN_DUMPREG                  (TRUE)
#define EN_BYPASSMODE               (FALSE)    //BYPASS MODE or ADVANCE MODE(SDR/DDR)


// Retry Times
//-----------------------------------------------------------------------------------------------------------
#define RT_CLEAN_SDSTS				3
#define RT_CLEAN_MIEEVENT			3

// Wait Time
//-----------------------------------------------------------------------------------------------------------
#define WT_DAT0HI_END				1000	//(ms)
#define WT_EVENT_CIFD                500    //(ms)
#define WT_RESET                     100    //(ms)

//***********************************************************************************************************
//***********************************************************************************************************

// Reg Static Init Setting
//-----------------------------------------------------------------------------------------------------------
#define V_MIE_PATH_INIT		0
#define V_MMA_PRI_INIT      (R_MIU_R_PRIORITY|R_MIU_W_PRIORITY)
#define V_MIE_INT_EN_INIT   (R_DATA_END_IEN|R_CMD_END_IEN|R_SDIO_INT_IEN)
#define V_RSP_SIZE_INIT		0
#define V_CMD_SIZE_INIT		(5<<8)
#define V_SD_CTL_INIT		0
#define V_SD_MODE_INIT		(R_CLK_EN)	// Add
#define V_SDIO_MODE_INIT    0
#define V_DDR_MODE_INIT     0


// Mask Range
//-----------------------------------------------------------------------------------------------------------
#define M_SD_ERRSTS			(R_DAT_RD_CERR|R_DAT_WR_CERR|R_DAT_WR_TOUT|R_CMD_NORSP|R_CMDRSP_CERR|R_DAT_RD_TOUT)
#define M_SD_MIEEVENT		(R_DATA_END|R_CMD_END|R_ERR_STS|R_BUSY_END_INT|R_R2N_RDY_INT)
#define M_RST_STS           (R_RST_MIU_STS|R_RST_MIE_STS|R_RST_MCU_STS)


// Mask Reg Value
//-----------------------------------------------------------------------------------------------------------
#define M_REG_STSERR(IP)		(CARD_REG(A_SD_STS_REG(IP)) & M_SD_ERRSTS)
#define M_REG_SDMIEEvent(IP)    (CARD_REG(A_MIE_EVENT_REG(IP)) & M_SD_MIEEVENT)
#define M_REG_GETDAT0(IP)       (CARD_REG(A_SD_STS_REG(IP)) & R_DAT0)


//-----------------------------------------------------------------------------------------------------------
//IP_FCIE or IP_SDIO Register Basic Address
//-----------------------------------------------------------------------------------------------------------
#define A_SD_REG_POS(IP)		        GET_CARD_BANK(IP, 0)
#define A_SD_CFIFO_POS(IP)		        GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x20)  //Always at FCIE5
#define A_SD_CIFD_POS(IP)		        GET_CARD_BANK(IP, 1)

#define A_SD_CIFD_R_POS(IP)             GET_CARD_REG_ADDR(A_SD_CIFD_POS(IP), 0x00)
#define A_SD_CIFD_W_POS(IP)             GET_CARD_REG_ADDR(A_SD_CIFD_POS(IP), 0x20)

#define A_MIE_EVENT_REG(IP)    			GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x00)
#define A_MIE_INT_ENABLE_REG(IP)    	GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x01)
#define A_MMA_PRI_REG_REG(IP)			GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x02)
#define A_DMA_ADDR_15_0_REG(IP)         GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x03)
#define A_DMA_ADDR_31_16_REG(IP)        GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x04)
#define A_DMA_LEN_15_0_REG(IP)          GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x05)
#define A_DMA_LEN_31_16_REG(IP)         GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x06)
#define A_MIE_FUNC_CTL_REG(IP)         	GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x07)
#define A_JOB_BLK_CNT_REG(IP)			GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x08)
#define A_BLK_SIZE_REG(IP)              GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x09)
#define A_CMD_RSP_SIZE_REG(IP)          GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x0A)
#define A_SD_MODE_REG(IP)          		GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x0B)
#define A_SD_CTL_REG(IP)           		GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x0C)
#define A_SD_STS_REG(IP)           		GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x0D)
#define A_BOOT_MOD_REG(IP)              GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x0E)
#define A_DDR_MOD_REG(IP)               GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x0F)
#define A_DDR_TOGGLE_CNT_REG(IP)        GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x10)
#define A_SDIO_MODE_REG(IP)           	GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x11)
#define A_TEST_MODE_REG(IP)             GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x15)

#define A_WR_SBIT_TIMER_REG(IP)         GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x17)
#define A_RD_SBIT_TIMER_REG(IP)         GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x18)

#define A_SDIO_DET_ON(IP)               GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x2F)

#define A_CIFD_EVENT_REG(IP)			GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x30)
#define A_CIFD_INT_EN_REG(IP)			GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x31)

#define A_BOOT_REG(IP)                  GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x37)

#define A_DBG_BUS0_REG(IP)              GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x38)
#define A_DBG_BUS1_REG(IP)              GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x39)
#define A_FCIE_RST_REG(IP)       	    GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x3F)

#define A_CFIFO_OFFSET(IP, OFFSET)      GET_CARD_REG_ADDR(A_SD_CFIFO_POS(IP), OFFSET)
#define A_CIFD_R_OFFSET(IP, OFFSET)     GET_CARD_REG_ADDR(A_SD_CIFD_R_POS(IP), OFFSET)
#define A_CIFD_W_OFFSET(IP, OFFSET)     GET_CARD_REG_ADDR(A_SD_CIFD_W_POS(IP), OFFSET)


// Reg Dynamic Variable
//-----------------------------------------------------------------------------------------------------------
static RspStruct gst_RspStruct[3];
static volatile bool  gb_StopWProc[3] = {0};
static volatile uint16_t   gu16_WT_NRC[3] = {0};             //Waiting Time for Nrc (us)

static volatile uint16_t	gu16_SD_MODE_DatLine[3] = {0};

static volatile uint16_t	gu16_DDR_MODE_REG[3] = {0};
static volatile uint16_t   gu16_DDR_MODE_REG_ForR2N[3] = {0};

static volatile bool  gb_SDIODevice[3] = {0};


static volatile uint8_t*   gpu8Buf[3];

#define CLEAN(x) 	memset(x, 0, sizeof(x));
void sdmmc_init(void){
	CLEAN(gst_RspStruct);
	CLEAN(gb_StopWProc);
	CLEAN(gu16_WT_NRC);
	CLEAN(gu16_SD_MODE_DatLine);
	CLEAN(gu16_SD_MODE_DatLine);
	CLEAN(gu16_DDR_MODE_REG);
	CLEAN(gu16_DDR_MODE_REG_ForR2N);
	CLEAN(gb_SDIODevice);
	CLEAN(gpu8Buf);
}

volatile void* GET_CARD_BANK(IPEmType eIP, uint8_t u8Bank)
{
    void* pIPBANKArr[3][3] =
    {
      { (void*) (A_FCIE1_0_BANK), (void*) (A_FCIE1_1_BANK), (void*) (A_FCIE1_2_BANK) },
      { (void*) (A_FCIE2_0_BANK), (void*) (A_FCIE2_1_BANK), (void*) (A_FCIE2_2_BANK) },
      { (void*) (A_FCIE3_0_BANK), (void*) (A_FCIE3_1_BANK), (void*) (A_FCIE3_2_BANK) }
    };

	return pIPBANKArr[eIP][u8Bank];

}


// Register Opertation Define ==> For FCIE5 Special FCIE/SDIO Function Ctrl Setting
//-----------------------------------------------------------------------------------------------------------
static uint16_t _REG_GetMIEFunCtlSetting(IPEmType eIP)
{
	return R_SDIO_MODE;
}


// Register Operation Define ==> For Clean Reg and Special Case
//-----------------------------------------------------------------------------------------------------------
static RetEmType _REG_ClearSDSTS(IPEmType eIP, uint8_t u8Retry)
{
    do
    {
        CARD_REG_SETBIT(A_SD_STS_REG(eIP), M_SD_ERRSTS);

        if ( gb_StopWProc[eIP] )
            break;

        if ( !M_REG_STSERR(eIP) )
            return EV_OK;
        else if(!u8Retry)
            break;

    } while(u8Retry--);

    return EV_FAIL; //mark for coverity scan
}

#if(EN_BIND_CARD_INT)
void Hal_CARD_INT_ClearMIEEvent(IPEmType eIP)
{
    //klog("MIE EVENT: 0x%08x\r\n", CARD_REG(A_MIE_EVENT_REG(eIP)));
    if(CARD_REG(A_MIE_EVENT_REG(eIP)) & (1<<3))
    {
        klog("[MIE] SDIO INT\r\n");
        CARD_REG_SETBIT(A_MIE_EVENT_REG(eIP), (1<<0));
    }
}
#endif

// Register Operation Define ==> For Clear MIE Event
//-----------------------------------------------------------------------------------------------------------
static RetEmType _REG_ClearMIEEvent(IPEmType eIP, uint8_t u8Retry)
{
    /****** Clean global MIEEvent for Interrupt ******/
#if(EN_BIND_CARD_INT)
    Hal_CARD_INT_ClearMIEEvent(eIP);
#endif

    /****** Clean MIEEvent Reg *******/
    do
    {
        CARD_REG(A_MIE_EVENT_REG(eIP)) = M_SD_MIEEVENT;

        if ( gb_StopWProc[eIP] )
            break;

        if ( !M_REG_SDMIEEvent(eIP) )
            return EV_OK;
        else if( !u8Retry )
            break;

    }while( u8Retry-- );
    return EV_FAIL; //mark for coverity scan

}


// Register Operation Define ==> For Wait DAT0 High
//-----------------------------------------------------------------------------------------------------------
static RetEmType _REG_WaitDat0HI(IPEmType eIP, uint32_t u32WaitMs)
{
    uint32_t u32DiffTime = 0;

	do
	{

		if ( gb_StopWProc[eIP] )
			return EV_FAIL;

		if ( M_REG_GETDAT0(eIP) )
			return EV_OK;

		Hal_Timer_uDelay(1);
		u32DiffTime++;
	}while( u32DiffTime <= (u32WaitMs*1000) );

	return EV_FAIL;

}


// Register Operation Define ==> For Wait MIE Event or CIFD Event
//-----------------------------------------------------------------------------------------------------------
static RetEmType _REG_WaitEvent(IPEmType eIP, IPEventEmType eEvent, uint16_t u16ReqEvent, uint32_t u32WaitMs)
{
	uint32_t u32DiffTime = 0;

	do
	{

		if ( gb_StopWProc[eIP] )
			return EV_FAIL;

		if(eEvent == EV_MIE)
		{
			if ( (CARD_REG(A_MIE_EVENT_REG(eIP))&u16ReqEvent) == u16ReqEvent )
				return EV_OK;
		}
		else if(eEvent == EV_CIFD )
		{
			if ( (CARD_REG(A_CIFD_EVENT_REG(eIP))&u16ReqEvent) == u16ReqEvent )
				return EV_OK;
		}

		Hal_Timer_mDelay(1);
		u32DiffTime++;
	}while(u32DiffTime <= u32WaitMs);

	return EV_FAIL;
}


// Register Operation Define ==> For Software Reset
//-----------------------------------------------------------------------------------------------------------
void _REG_ResetIP(IPEmType eIP)
{
	uint32_t u32DiffTime = 0;

	CARD_REG_CLRBIT(A_SD_CTL_REG(eIP), R_JOB_START);  //Clear For Safe ?

	CARD_REG_CLRBIT(A_FCIE_RST_REG(eIP), R_FCIE_SOFT_RST);

	do
	{
		if( (CARD_REG(A_FCIE_RST_REG(eIP)) & M_RST_STS) == M_RST_STS )
			break;
		Hal_Timer_uDelay(1);
		u32DiffTime++;
	}while ( u32DiffTime <= (1000*WT_RESET) );

	if ( u32DiffTime > (1000*WT_RESET) )
		prtstring("[HSD] IP Reset Switch Low Fail !!\r\n");


	u32DiffTime = 0;
	CARD_REG_SETBIT(A_FCIE_RST_REG(eIP), R_FCIE_SOFT_RST);

	do
	{
		if( (CARD_REG(A_FCIE_RST_REG(eIP)) & M_RST_STS) == 0 )
			break;
		Hal_Timer_uDelay(1);
		u32DiffTime++;

	}while ( u32DiffTime <= (1000* WT_RESET) );

    if ( u32DiffTime > (1000*WT_RESET) )
        prtstring("[HSD] IP Reset Switch High Fail !!\r\n");
}


// IP Buffer Operation => Get Byte Value form Register
//-----------------------------------------------------------------------------------------------------------
static uint8_t _BUF_GetByteFromRegAddr(volatile void *pBuf, uint16_t u16Pos)
{
    if(u16Pos & 0x1)
        return CARD_REG_H8(GET_CARD_REG_ADDR(pBuf, u16Pos>>1));
    else
        return CARD_REG_L8(GET_CARD_REG_ADDR(pBuf, u16Pos>>1));

}


// IP Buffer Operation => CIFD FIFO Buffer Operation Define
//-----------------------------------------------------------------------------------------------------------
void _BUF_CIFD_DATA_IO(IPEmType eIP, CmdEmType eCmdType, volatile uint16_t *pu16Buf, uint8_t u8WordCnt)
{
	uint8_t u8Pos = 0;

	for ( u8Pos = 0; u8Pos < u8WordCnt; u8Pos++ )
	{
		if ( eCmdType==EV_CMDREAD )
			pu16Buf[u8Pos] = CARD_REG(A_CIFD_R_OFFSET(eIP, u8Pos));
		else
			CARD_REG(A_CIFD_W_OFFSET(eIP, u8Pos)) = pu16Buf[u8Pos];
	}

}


// IP Buffer Operation => CIFD FIFO Buffer Operation for waiting FCIE5 special Event
//-----------------------------------------------------------------------------------------------------------
static RetEmType _BUF_CIFD_WaitEvent(IPEmType eIP,  CmdEmType eCmdType, volatile uint8_t *pu8R2NBuf)
{
	uint8_t u8RegionNo = 0, u8RegionMax = 0, u8RemainByte = 0;
	uint32_t uint32_tranLen  =  CARD_REG(A_DMA_LEN_15_0_REG(eIP)) + ( CARD_REG(A_DMA_LEN_31_16_REG(eIP)) << 16 );

	u8RemainByte = uint32_tranLen & (64-1);  // uint32_tranLen % 64
    u8RegionMax = (uint32_tranLen>>6) + (u8RemainByte ? 1: 0);

	for(u8RegionNo=0; u8RegionNo<u8RegionMax; u8RegionNo++)
	{

		if ( eCmdType==EV_CMDREAD )
		{
			if ( _REG_WaitEvent(eIP, EV_CIFD, R_WBUF_FULL, WT_EVENT_CIFD) )
			   return EV_FAIL;

		   if ( (u8RegionNo == (u8RegionMax-1)) && (u8RemainByte>0) )
				_BUF_CIFD_DATA_IO(eIP, eCmdType, (volatile uint16_t *)(pu8R2NBuf + (u8RegionNo << 6)), u8RemainByte/2);
		   else
			   _BUF_CIFD_DATA_IO(eIP, eCmdType, (volatile uint16_t *)(pu8R2NBuf + (u8RegionNo << 6)), 32);

		   CARD_REG(A_CIFD_EVENT_REG(eIP)) = R_WBUF_FULL;
		   CARD_REG(A_CIFD_EVENT_REG(eIP)) = R_WBUF_EMPTY_TRIG;
		}
		else // Write
		{
			if ( (u8RegionNo == (u8RegionMax-1)) && (u8RemainByte>0) )
				 _BUF_CIFD_DATA_IO(eIP, eCmdType, (volatile uint16_t *)(pu8R2NBuf + (u8RegionNo << 6)), u8RemainByte/2);
			else
				_BUF_CIFD_DATA_IO(eIP, eCmdType, (volatile uint16_t *)(pu8R2NBuf + (u8RegionNo << 6)), 32);

			 CARD_REG(A_CIFD_EVENT_REG(eIP)) = R_RBUF_FULL_TRIG;

			if ( _REG_WaitEvent(eIP, EV_CIFD, R_RBUF_EMPTY, WT_EVENT_CIFD) )
			   return EV_FAIL;

			 CARD_REG(A_CIFD_EVENT_REG(eIP)) = R_RBUF_EMPTY;

		}

	}

	return EV_OK;

}


// SDMMC Internel Logic Function
//-----------------------------------------------------------------------------------------------------------
static void _SDMMC_REG_Dump(IPEmType eIP)
{

#if (EN_DUMPREG)

	uint8_t u8Pos, u8DGMode;

	prtstring("\n----------------------------------------------------\r\n");
    prtU32Hex(((uint32_t)A_SD_REG_POS(eIP) & 0x01FFFE00) >> 9);
	prtstring("  CMD_");
	prtUInt(gst_RspStruct[eIP].u8Cmd);
	prtstring(" (Arg: ");
	prtU32Hex(gst_RspStruct[eIP].u32Arg);
	prtstring(") [Line: ");
	prtUInt(gst_RspStruct[eIP].u32ErrLine);
	prtstring("]\r\n");
	prtstring("----------------------------------------------------\r\n");

	for(u8Pos = 0; u8Pos < gst_RspStruct[eIP].u8RspSize; u8Pos++)
	{
		if( (u8Pos == 0) || (u8Pos == 8) )
			prtstring("[");

		prtU8Hex(_BUF_GetByteFromRegAddr((volatile void *)A_SD_CFIFO_POS(eIP), u8Pos));
		prtstring(",");

		if( (u8Pos == 7) || (u8Pos == (gst_RspStruct[eIP].u8RspSize-1)) )
			prtstring("]\n");
	}

	prtstring("---------------DumpReg------------------------------\r\n");
	prtstring("[0x07][MIE_FUNC_CTL_REG]=    ");
	prtU16Hex(CARD_REG(A_MIE_FUNC_CTL_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x0B][SD_MODE_REG]=         ");
	prtU16Hex(CARD_REG(A_SD_MODE_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x0C][SD_CTL_REG]=          ");
	prtU16Hex(CARD_REG(A_SD_CTL_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x0F][DDR_MOD_REG]=         ");
	prtU16Hex(CARD_REG(A_DDR_MOD_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x0D][SD_STS_REG]=          ");
	prtU16Hex(CARD_REG(A_SD_STS_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x00][MIE_EVENT_REG]=       ");
	prtU16Hex(CARD_REG(A_MIE_EVENT_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x03][MMA_ADDR_15_0_REG]=   ");
	prtU16Hex(CARD_REG(A_DMA_ADDR_15_0_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x04][MMA_ADDR_31_16_REG]=  ");
	prtU16Hex(CARD_REG(A_DMA_ADDR_31_16_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x05][MMA_LEN_15_0_REG]=    ");
	prtU16Hex(CARD_REG(A_DMA_LEN_15_0_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x06][MMA_LEN_31_16_REG]=   ");
	prtU16Hex(CARD_REG(A_DMA_LEN_31_16_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x08][JOB_BLK_CNT]=         ");
	prtU16Hex(CARD_REG(A_JOB_BLK_CNT_REG(eIP)));
	prtstring("\r\n");

	prtstring("[0x09][BLK_SIZE]=            ");
	prtU16Hex(CARD_REG(A_BLK_SIZE_REG(eIP)));
	prtstring("\r\n");


	CARD_REG_CLRBIT(A_DBG_BUS1_REG(eIP), R_DEBUG_MOD0 | R_DEBUG_MOD1 | R_DEBUG_MOD2 | R_DEBUG_MOD3 );
	CARD_REG_SETBIT(A_DBG_BUS1_REG(eIP), R_DEBUG_MOD0 | R_DEBUG_MOD2); //Mode 5

	prtstring("[0x38][DEBUG_BUS0]=          ");
	for(u8DGMode = 1; u8DGMode <=4; u8DGMode++)
	{
		CARD_REG_CLRBIT(A_TEST_MODE_REG(eIP), R_SD_DEBUG_MOD0 | R_SD_DEBUG_MOD1 | R_SD_DEBUG_MOD2);
		CARD_REG_SETBIT(A_TEST_MODE_REG(eIP), (u8DGMode<<1));
		prtU16Hex(CARD_REG(A_DBG_BUS0_REG(eIP)));
		prtstring(", ");
	}
	prtstring("\r\n");
	prtstring("[0x39][DEBUG_BUS1]=          ");

	for(u8DGMode = 1; u8DGMode <=4; u8DGMode++)
	{
		CARD_REG_CLRBIT(A_TEST_MODE_REG(eIP), R_SD_DEBUG_MOD0 | R_SD_DEBUG_MOD1 | R_SD_DEBUG_MOD2);
		CARD_REG_SETBIT(A_TEST_MODE_REG(eIP), (u8DGMode<<1));
		prtU16Hex(CARD_REG(A_DBG_BUS1_REG(eIP)));
		prtstring(", ");

	}

	prtstring("\r\n");
	prtstring("----------------------------------------------------\r\n");

#endif  //End #if(EN_DUMPREG)


}


static RspErrEmType _SDMMC_EndProcess(IPEmType eIP, CmdEmType eCmdType, RspErrEmType eRspErr, bool bCloseClk, int Line)
{
	uint16_t u16RspErr = (uint16_t)eRspErr;
	uint16_t u16IPErr = EV_STS_RIU_ERR | EV_STS_MIE_TOUT | EV_STS_DAT0_BUSY;

	/****** (1) Record Information *******/
	gst_RspStruct[eIP].u32ErrLine = (uint32_t)Line;
	gst_RspStruct[eIP].u8RspSize = (uint8_t)CARD_REG(A_CMD_RSP_SIZE_REG(eIP));
	gst_RspStruct[eIP].eErrCode = eRspErr;

	/****** (2) Dump and the Reg Info + Reset IP *******/
	if ( u16RspErr && gb_StopWProc[eIP] )
	{
		eRspErr = EV_SWPROC_ERR;
		_REG_ResetIP(eIP);
	}
	else if( u16RspErr & u16IPErr ) //SD IP Error
	{
		_SDMMC_REG_Dump(eIP);
		_REG_ResetIP(eIP);
	}
	else if( u16RspErr & M_SD_ERRSTS ) //SD_STS Reg Error
	{
		//Do Nothing
	}

	/****** (3) Close clock and DMA Stop function ******/
	if(bCloseClk && !gb_SDIODevice[eIP])
		CARD_REG_CLRBIT(A_SD_MODE_REG(eIP), R_CLK_EN | R_DMA_RD_CLK_STOP);

	return eRspErr;


}


//***********************************************************************************************************
// SDMMC HAL Function
//***********************************************************************************************************

/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_SetSDIODevice
*     @author jeremy.wang (2015/10/7)
* Desc:
*
* @param eIP : FCIE1/FCIE2/...
* @param bEnable :
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_SetSDIODevice(IPEmType eIP, bool bEnable)
{
	if(bEnable)
		gb_SDIODevice[eIP] = TRUE;
	else
		gb_SDIODevice[eIP] = FALSE;

}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_SetSDIOIntDet
*     @author jeremy.wang (2015/10/7)
* Desc:
*
* @param eIP : FCIE1/FCIE2/...
* @param bEnable :
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_SetSDIOIntDet(IPEmType eIP, bool bEnable)
{
	if(gb_SDIODevice[eIP])
	{
		if(bEnable)
		{
            //CARD_REG_SETBIT(A_TEST_MODE_REG(eIP), (u8DGMode<<1));
#if(EN_BIND_CARD_INT)
            //klog("[MIE] ENABLE SDIO INT\r\n");
            CARD_REG_SETBIT(A_SDIO_DET_ON(eIP), (1<<0));
#endif
		}

	}

}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_SetDataWidth
*     @author jeremy.wang (2015/7/9)
* Desc: According as Data Bus Width to Set IP DataWidth
*
* @param eIP : FCIE1/FCIE2/...
* @param eBusWidth : 1BIT/4BITs/8BITs
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_SetDataWidth(IPEmType eIP, SDMMCBusWidthEmType eBusWidth)
{
	gu16_SD_MODE_DatLine[eIP] = (uint16_t)eBusWidth;
}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_SetBusTiming
*     @author jeremy.wang (2015/7/29)
* Desc:
*
* @param eIP : FCIE1/FCIE2/...
* @param eBusTiming : LOW/DEF/HS/SDR12/DDR...
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_SetBusTiming(IPEmType eIP, BusTimingEmType eBusTiming)
{

	switch ( eBusTiming )
	{

#if (EN_BYPASSMODE)

		case EV_BUS_LOW:
		case EV_BUS_DEF:
			gu16_DDR_MODE_REG[eIP] = 0;
			gu16_DDR_MODE_REG_ForR2N[eIP] = gu16_DDR_MODE_REG[eIP] | R_PAD_IN_BYPASS;
			break;
		case EV_BUS_HS:
			gu16_DDR_MODE_REG[eIP] = 0;
			gu16_DDR_MODE_REG_ForR2N[eIP] = gu16_DDR_MODE_REG[eIP] | 0;
			break;
        default:
            break;
#else
        //ADVANCE MODE(SDR/DDR) ==> Other can't run bypass mode

		case EV_BUS_LOW:
		case EV_BUS_DEF:
		case EV_BUS_HS:
			gu16_DDR_MODE_REG[eIP] = (R_PAD_CLK_SEL|R_PAD_IN_SEL|R_FALL_LATCH);
			gu16_DDR_MODE_REG_ForR2N[eIP] = gu16_DDR_MODE_REG[eIP] | R_PAD_IN_RDY_SEL | R_PRE_FULL_SEL0 | R_PRE_FULL_SEL1;
            break;
		case EV_BUS_SDR12:
			break;
		case EV_BUS_SDR25:
			break;
		case EV_BUS_SDR50:
			break;
		case EV_BUS_SDR104:
			break;
		case EV_BUS_DDR50:
			break;
		case EV_BUS_HS200:
			break;
        default:
            break;
#endif


	}


}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_SetNrcDelay
*     @author jeremy.wang (2015/7/9)
* Desc: According as Current Clock to Set Nrc Delay
*
* @param eIP : FCIE1/FCIE2/...
* @param u32RealClk : Real Clock
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_SetNrcDelay(IPEmType eIP, uint32_t u32RealClk)
{

	if( u32RealClk >= 8000000 )			//>=8MHz
		gu16_WT_NRC[eIP] = 1;
	else if( u32RealClk >= 4000000 )	//>=4MHz
		gu16_WT_NRC[eIP] = 2;
	else if( u32RealClk >= 2000000 )	//>=2MHz
		gu16_WT_NRC[eIP] = 4;
	else if( u32RealClk >= 1000000 )	//>=1MHz
		gu16_WT_NRC[eIP] = 8;
	else if( u32RealClk >= 400000 )     //>=400KHz
		gu16_WT_NRC[eIP] = 20;
	else if( u32RealClk >= 300000 )	    //>=300KHz
		gu16_WT_NRC[eIP] = 27;
	else if( u32RealClk >= 100000 )     //>=100KHz
		gu16_WT_NRC[eIP] = 81;
	else if(u32RealClk==0)
		gu16_WT_NRC[eIP] = 100;          //Default

}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_SetCmdToken
*     @author jeremy.wang (2015/7/8)
* Desc: Set Cmd Token
*
* @param eIP : FCIE1/FCIE2/...
* @param u8Cmd : SD Command
* @param u32Arg : SD Argument
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_SetCmdToken(IPEmType eIP, uint8_t u8Cmd, uint32_t u32Arg)
{
	gst_RspStruct[eIP].u8Cmd		= u8Cmd;
	gst_RspStruct[eIP].u32Arg       = u32Arg;

	CARD_REG(A_CFIFO_OFFSET(eIP, 0)) = (((uint8_t)(u32Arg>>24))<<8) | (0x40 + u8Cmd);
	CARD_REG(A_CFIFO_OFFSET(eIP, 1)) = (((uint8_t)(u32Arg>>8))<<8) | ((uint8_t)(u32Arg>>16));
	CARD_REG(A_CFIFO_OFFSET(eIP, 2)) = (uint8_t)u32Arg;

	TR_H_SDMMC(prtstring("[S_")); TR_H_SDMMC(prtUInt(eIP));
	TR_H_SDMMC(prtstring("] CMD_")); TR_H_SDMMC(prtUInt(u8Cmd));

	TR_H_SDMMC(prtstring(" ("));
	TR_H_SDMMC(prtU32Hex(u32Arg));
	TR_H_SDMMC(prtstring(")"));


}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_GetRspToken
*     @author jeremy.wang (2015/7/9)
* Desc: Get Command Response Info
*
* @param eIP : FCIE1/FCIE2/...
*
* @return RspStruct*  : Response Struct
----------------------------------------------------------------------------------------------------------*/
RspStruct* Hal_SDMMC_GetRspToken(IPEmType eIP)
{
	uint8_t u8Pos;

	TR_H_SDMMC(prtstring(" =>RSP: ("));
	TR_H_SDMMC(prtU16Hex((uint16_t)gst_RspStruct[eIP].eErrCode));
	TR_H_SDMMC(prtstring(")\r\n"));

	for(u8Pos=0; u8Pos<0x10; u8Pos++ )
		gst_RspStruct[eIP].u8ArrRspToken[u8Pos] = 0;

	TR_H_SDMMC(prtstring("["));

	for(u8Pos=0; u8Pos< gst_RspStruct[eIP].u8RspSize; u8Pos++)
	{
		gst_RspStruct[eIP].u8ArrRspToken[u8Pos] = _BUF_GetByteFromRegAddr((volatile void *)A_SD_CFIFO_POS(eIP), u8Pos);
		TR_H_SDMMC(prtU8Hex(gst_RspStruct[eIP].u8ArrRspToken[u8Pos]));
		TR_H_SDMMC(prtstring(", "));

	}

	TR_H_SDMMC(prtstring("]\r\n\r\n"));

	return &gst_RspStruct[eIP];

}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_TransCmdSetting
*     @author jeremy.wang (2015/7/15)
* Desc: For Data Transfer Setting
*
* @param eIP : FCIE1/FCIE2/...
* @param eTransType : CIFD/DMA/ADMA/NONE
* @param u16BCnt : Block Cnt
* @param u16BlkSize : Block Size
* @param u32BufAddr : Memory Address or DMA Table Address (32bits)
* @param pu8Buf : If run CIFD, it neet the buf pointer to do io between CIFD and Buf
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_TransCmdSetting(IPEmType eIP, TransEmType eTransType, uint16_t u16BlkCnt, uint16_t u16BlkSize, volatile uint32_t u32BufAddr, volatile uint8_t *pu8Buf)
{
	uint32_t uint32_tranLen = u16BlkCnt * u16BlkSize;

	CARD_REG(A_BLK_SIZE_REG(eIP)) = u16BlkSize;

	if (eTransType==EV_ADMA)
	{
		CARD_REG(A_JOB_BLK_CNT_REG(eIP)) = 1;        //ADMA BLK_CNT = 1
		CARD_REG(A_DMA_LEN_15_0_REG(eIP))  = 0x10;   //ADMA Fixed Value = 0x10
		CARD_REG(A_DMA_LEN_31_16_REG(eIP)) = 0;
	}
	else //R2N or DMA
	{
		CARD_REG(A_JOB_BLK_CNT_REG(eIP)) = u16BlkCnt;
		CARD_REG(A_DMA_LEN_15_0_REG(eIP))  = (uint16_t)(uint32_tranLen & 0xFFFF);
		CARD_REG(A_DMA_LEN_31_16_REG(eIP)) = (uint16_t)(uint32_tranLen >> 16);
	}

	if( (eTransType== EV_DMA) || (eTransType==EV_ADMA) )
	{
		CARD_REG(A_DMA_ADDR_15_0_REG(eIP))  = (uint16_t)(u32BufAddr & 0xFFFF);
		CARD_REG(A_DMA_ADDR_31_16_REG(eIP)) = (uint16_t)(u32BufAddr >> 16);
	}
	else //CIFD (R2N)
	{
		gpu8Buf[eIP] = pu8Buf;
	}

}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_SendCmdAndWaitProcess
*     @author jeremy.wang (2015/7/14)
* Desc: Send SD Command and Waiting for Process
*
* @param eIP :  FCIE1/FCIE2/...
* @param eTransType : CIFD/DMA/ADMA/NONE
* @param eCmdType : CMDRSP/READ/WRITIE
* @param eRspType : R1/R2/R3/...
* @param bCloseClk : Close Clock or not
*
* @return RspErrEmType  : Response Error Code
----------------------------------------------------------------------------------------------------------*/
RspErrEmType Hal_SDMMC_SendCmdAndWaitProcess(IPEmType eIP, TransEmType eTransType, CmdEmType eCmdType, SDMMCRspEmType eRspType, bool bCloseClk)
{
	uint32_t u32WaitMS	= WT_EVENT_RSP;
	uint16_t u16WaitMIEEvent = R_CMD_END;

	CARD_REG(A_CMD_RSP_SIZE_REG(eIP)) = V_CMD_SIZE_INIT | ((uint8_t)eRspType);
	CARD_REG(A_MIE_FUNC_CTL_REG(eIP)) = V_MIE_PATH_INIT | _REG_GetMIEFunCtlSetting(eIP);
	CARD_REG(A_SD_MODE_REG(eIP)) = V_SD_MODE_INIT | (eTransType>>8) | gu16_SD_MODE_DatLine[eIP] | ((uint8_t)(eTransType & R_DMA_RD_CLK_STOP));
	CARD_REG(A_SD_CTL_REG(eIP))  = V_SD_CTL_INIT | (eRspType>>12) | (eCmdType>>4) | ((uint8_t)(eTransType & R_ADMA_EN));
	CARD_REG(A_MMA_PRI_REG_REG(eIP)) = V_MMA_PRI_INIT;
	CARD_REG(A_DDR_MOD_REG(eIP)) = V_DDR_MODE_INIT | (eTransType==EV_CIF ? gu16_DDR_MODE_REG_ForR2N[eIP] : gu16_DDR_MODE_REG[eIP]);

    CARD_REG_CLRBIT(A_BOOT_MOD_REG(eIP), R_BOOT_MODE);
    CARD_REG_CLRBIT(A_BOOT_REG(eIP), (R_NAND_BOOT_EN | R_BOOTSRAM_ACCESS_SEL | R_IMI_SEL));


	Hal_Timer_uDelay(gu16_WT_NRC[eIP]);

	if( _REG_ClearSDSTS(eIP, RT_CLEAN_SDSTS) || _REG_ClearMIEEvent(eIP, RT_CLEAN_MIEEVENT) )
		return _SDMMC_EndProcess(eIP, eCmdType, EV_STS_RIU_ERR, bCloseClk, __LINE__);;

#if(EN_BIND_CARD_INT)
	//Hal_CARD_INT_SetMIEIntEn(eIP, EV_INT_SD, V_MIE_INT_EN_INIT | u16MIE_TRANS_END);
    Hal_SDMMC_SetSDIOIntDet(eIP, true);
#endif

	if(eCmdType==EV_CMDREAD)
	{
		if (eTransType!=EV_CIF)
		{
			u16WaitMIEEvent |= R_DATA_END;
			u32WaitMS += WT_EVENT_READ;
		}
        CARD_REG_SETBIT(A_SD_CTL_REG(eIP), (R_CMD_EN | R_DTRX_EN) );
		CARD_REG_SETBIT(A_SD_CTL_REG(eIP), (R_CMD_EN | R_DTRX_EN | R_JOB_START) );
	}
	else {
        CARD_REG_SETBIT(A_SD_CTL_REG(eIP), (R_CMD_EN) );
		CARD_REG_SETBIT(A_SD_CTL_REG(eIP), (R_CMD_EN | R_JOB_START) );
    }

	if(_REG_WaitEvent(eIP, EV_MIE, u16WaitMIEEvent, u32WaitMS))
		return _SDMMC_EndProcess(eIP, eCmdType, EV_STS_MIE_TOUT, bCloseClk, __LINE__);

    //====== Special Case for R2N CIFD Read Transfer ======
	if( (eCmdType==EV_CMDREAD) && (eTransType==EV_CIF) )
	{
		if(_BUF_CIFD_WaitEvent(eIP, eCmdType, gpu8Buf[eIP]))
		   return _SDMMC_EndProcess(eIP, eCmdType, EV_STS_MIE_TOUT, bCloseClk, __LINE__);

		if(_REG_WaitEvent(eIP, EV_MIE, R_DATA_END, WT_EVENT_READ))
			return _SDMMC_EndProcess(eIP, eCmdType, EV_STS_MIE_TOUT, bCloseClk, __LINE__);
	}

	if( (eRspType == EV_R1B) && _REG_WaitDat0HI(eIP, WT_DAT0HI_END) )
		return _SDMMC_EndProcess(eIP, eCmdType, EV_STS_DAT0_BUSY, bCloseClk, __LINE__);
	else if( (eRspType == EV_R3) || (eRspType == EV_R4) )  // For IP CRC bug
		CARD_REG(A_SD_STS_REG(eIP)) = R_CMDRSP_CERR; //Clear CMD CRC Error

	if( (eCmdType == EV_CMDWRITE) && (!M_REG_STSERR(eIP)) )
	{
		if( _REG_ClearSDSTS(eIP, RT_CLEAN_SDSTS) || _REG_ClearMIEEvent(eIP, RT_CLEAN_MIEEVENT) )
            return _SDMMC_EndProcess(eIP, eCmdType, EV_STS_RIU_ERR, bCloseClk, __LINE__);

		CARD_REG(A_SD_CTL_REG(eIP)) = V_SD_CTL_INIT |(eCmdType>>4) | ((uint8_t)(eTransType & R_ADMA_EN));
        CARD_REG_SETBIT(A_SD_CTL_REG(eIP), (R_DTRX_EN) );
		CARD_REG_SETBIT(A_SD_CTL_REG(eIP), (R_DTRX_EN | R_JOB_START) );

		//====== Special Case for R2N CIFD Write Transfer ======
		if ( (eTransType==EV_CIF) && _BUF_CIFD_WaitEvent(eIP, eCmdType, gpu8Buf[eIP]) )
			return _SDMMC_EndProcess(eIP, eCmdType, EV_STS_MIE_TOUT, bCloseClk, __LINE__);

		if(_REG_WaitEvent(eIP, EV_MIE, R_DATA_END, WT_EVENT_WRITE))
			return _SDMMC_EndProcess(eIP, eCmdType, EV_STS_MIE_TOUT, bCloseClk, __LINE__);

	}

	return _SDMMC_EndProcess(eIP, eCmdType, (RspErrEmType)M_REG_STSERR(eIP), bCloseClk, __LINE__);

}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_RunBrokenDmaAndWaitProcess
*     @author jeremy.wang (2015/7/29)
* Desc: For Broken DMA Data Transfer
*
* @param eIP : FCIE1/FCIE2/...
* @param eCmdType : READ/WRITE
*
* @return RspErrEmType  : Response Error Code
----------------------------------------------------------------------------------------------------------*/
RspErrEmType Hal_SDMMC_RunBrokenDmaAndWaitProcess(IPEmType eIP, CmdEmType eCmdType)
{
	uint32_t u32WaitMS	= 0;

	if(eCmdType==EV_CMDREAD)
		u32WaitMS = WT_EVENT_READ;
	else if(eCmdType==EV_CMDWRITE)
		u32WaitMS = WT_EVENT_WRITE;

	if ( _REG_ClearMIEEvent(eIP, RT_CLEAN_MIEEVENT) )
		return _SDMMC_EndProcess(eIP, eCmdType, EV_STS_RIU_ERR, FALSE, __LINE__);

    CARD_REG_CLRBIT(A_SD_CTL_REG(eIP), R_CMD_EN );
	CARD_REG_SETBIT(A_SD_CTL_REG(eIP), (R_DTRX_EN | R_JOB_START) );

	if ( _REG_WaitEvent(eIP, EV_MIE, R_DATA_END, u32WaitMS) )
		return _SDMMC_EndProcess(eIP, eCmdType, EV_STS_MIE_TOUT, FALSE, __LINE__);

	return _SDMMC_EndProcess(eIP, eCmdType, (RspErrEmType)M_REG_STSERR(eIP), FALSE, __LINE__);

}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_ADMASetting
*     @author jeremy.wang (2015/7/13)
* Desc: For ADMA Data Transfer Settings
*
* @param eIP : FCIE1/FCIE2/...
* @param pDMATable : DMA Table memory address pointer
* @param u8Item : DMA Table Item 0 ~
* @param u32SubDMALen :  DMA Table DMA Len
* @param u32SubDMAAddr : DMA Table DMA Addr
* @param u8MIUSel : MIU Select
* @param bEnd : End Flag
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_ADMASetting(IPEmType eIP, volatile void *pDMATable, uint8_t u8Item, uint32_t u32SubDMALen, uint32_t u32SubDMAAddr, uint8_t u8MIUSel, bool bEnd)
{
	//uint16_t u16Pos;

	AdmaDescStruct* pst_AdmaDescStruct = (AdmaDescStruct*) pDMATable;

	pst_AdmaDescStruct = (pst_AdmaDescStruct+u8Item);
	memset(pst_AdmaDescStruct, 0, sizeof(AdmaDescStruct));

	pst_AdmaDescStruct->u32_DmaLen = u32SubDMALen;
	pst_AdmaDescStruct->u32_Address = u32SubDMAAddr;
	pst_AdmaDescStruct->u32_JobCnt = (u32SubDMALen>>9);
	pst_AdmaDescStruct->u32_MiuSel = u8MIUSel;
	pst_AdmaDescStruct->u32_End    = bEnd;

	/*prtstring("\r\n");
	prtstring("gpst_AdmaDescStruct Pos=(");
	prtU32Hex(pst_AdmaDescStruct);
	prtstring(")\r\n");

    for(u16Pos=0; u16Pos<192 ; u16Pos++)
    {
		if( (u16Pos%12)==0)
		   (prtstring("\r\n"));

		(prtstring("["));
		(prtU8Hex( *(pu8Buf+u16Pos)));
		(prtstring("]"));

    }*/


}


ErrGrpEmType Hal_SDMMC_ErrGroup(RspErrEmType eErrType)
{

	switch((uint16_t)eErrType)
	{
		case EV_STS_OK:
			return EV_EGRP_OK;

		case EV_STS_WR_TOUT:
		case EV_STS_NORSP:
		case EV_STS_RD_TOUT:
		case EV_STS_RIU_ERR:
		case EV_STS_DAT0_BUSY:
		case EV_STS_MIE_TOUT:
			return EV_EGRP_TOUT;

		case EV_STS_RD_CERR:
		case EV_STS_WD_CERR:
		case EV_STS_RSP_CERR:
			return EV_EGRP_COMM;

		default:
			return EV_EGRP_OTHER;
	}

}



/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_ClkCtrl
*     @author jeremy.wang (2015/7/15)
* Desc: OpenCard Clk for Special Request (We always Open/Close Clk by Every Cmd)
*
* @param eIP : FCIE1/FCIE2/...
* @param bOpen : Open Clk or not
* @param u16DelayMs : Delay ms to Specail Purpose
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_ClkCtrl(IPEmType eIP, bool bOpen, uint16_t u16DelayMs)
{
	CARD_REG(A_MIE_FUNC_CTL_REG(eIP)) = V_MIE_PATH_INIT | _REG_GetMIEFunCtlSetting(eIP);

	if( bOpen )
		CARD_REG_SETBIT(A_SD_MODE_REG(eIP), R_CLK_EN);
	else
		CARD_REG_CLRBIT(A_SD_MODE_REG(eIP), R_CLK_EN);

	Hal_Timer_mDelay(u16DelayMs);
}

/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_Reset
*     @author jeremy.wang (2015/7/17)
* Desc:  Reset IP to avoid IP dead + Touch CIFD first time to avoid latch issue
*
* @param eIP : FCIE1/FCIE2/...
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_Reset(IPEmType eIP)
{
	_REG_ResetIP(eIP);
}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_WaitProcessCtrl
*     @author jeremy.wang (2015/7/17)
* Desc: Stop process to avoid Long time waiting
*
* @param eIP : FCIE1/FCIE2/...
* @param bStop : Stop Process or not
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_WaitProcessCtrl(IPEmType eIP, bool bStop)
{
	gb_StopWProc[eIP] = bStop;

#if(EN_BIND_CARD_INT)
	if ( gb_StopWProc[eIP] )
		Hal_CARD_INT_WaitMIEEventCtrl(eIP, TRUE);
	else
		Hal_CARD_INT_WaitMIEEventCtrl(eIP, FALSE);
#endif

}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_OtherPreUse
*     @author jeremy.wang (2015/7/17)
* Desc: Use this for avoid SD/EMMC to use FCIE at the same time
*
* @param eIP : FCIE1/FCIE2/...
*
* @return bool  :  (TRUE: Other driver usnig)
----------------------------------------------------------------------------------------------------------*/
bool Hal_SDMMC_OtherPreUse(IPEmType eIP)
{

	if ( CARD_REG(A_MIE_FUNC_CTL_REG(eIP)) & R_EMMC_EN )
		return (TRUE);

	if ( !(CARD_REG(A_MIE_FUNC_CTL_REG(eIP)) & (R_SD_EN | R_SDIO_MODE)) ) //Not SD Path
		return (TRUE);

	return (FALSE);
}



//***********************************************************************************************************
// SDMMC Information
//***********************************************************************************************************

/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_DumpMemTool
*     @author jeremy.wang (2015/7/29)
* Desc: Help us to dump memory
*
* @param u8ListNum : Each List Num include 16 bytes
* @param pu8Buf : Buffer Pointer
----------------------------------------------------------------------------------------------------------*/
void Hal_SDMMC_DumpMemTool(uint8_t u8ListNum, volatile uint8_t *pu8Buf)
{
	uint16_t u16Pos=0;
	uint8_t u8ListPos;
	uint32_t u32BufAddr = (uint32_t)pu8Buf;
	prtstring("\r\n $[Prt MEM_DATA: ");
	prtU32Hex(u32BufAddr);
	prtstring(" ]\r\n");

	for(u8ListPos=0 ; u8ListPos <u8ListNum; u8ListPos++ )
	{
		u16Pos= u8ListPos*16;
		for(; u16Pos< (u8ListPos+1)*16; u16Pos++)
		{
			prtstring("[");
			prtU8Hex(pu8Buf[u16Pos]);
			prtstring("]");
		}
		prtstring("\r\n");
	}

	prtstring("\r\n\r\n");

}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_GetDATBusLevel
*     @author jeremy.wang (2015/7/17)
* Desc: Monitor bus level for debug
*
* @param eIP : FCIE1/FCIE2/...
*
* @return uint8_t  : Return DAT Bus Level (4Bits)
----------------------------------------------------------------------------------------------------------*/
uint8_t Hal_SDMMC_GetDATBusLevel(IPEmType eIP)
{
	uint16_t u16Temp = 0;

	u16Temp = CARD_REG(A_SD_STS_REG(eIP)) & 0x0F00;

	return (uint8_t)(u16Temp>>8);

}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_SDMMC_GetMIEEvent
*     @author jeremy.wang (2015/7/31)
* Desc: Monitor MIE Event for debug
*
* @param eIP : FCIE1/FCIE2/...
*
* @return uint16_t  : Return MIE Event
----------------------------------------------------------------------------------------------------------*/
uint16_t Hal_SDMMC_GetMIEEvent(IPEmType eIP)
{
	return CARD_REG(A_MIE_EVENT_REG(eIP));
}


/*----------------------------------------------------------------------------------------------------------
*
* Function: Hal_CARD_TransMIUAddr
*     @author jeremy.wang (2015/7/31)
* Desc: Transfer original address to HW special dma address (MIU0/MIU1)
*
* @param u32Addr : Original address
*
* @return U32_T  : DMA address
----------------------------------------------------------------------------------------------------------*/
uint32_t Hal_CARD_TransMIUAddr(uint32_t u32Addr)
{
    if (u32Addr > 0x20000000)
    {
        return u32Addr - 0x20000000;
    }
    klog("Invalid addr :%X\n", u32Addr);
    return u32Addr;
}
