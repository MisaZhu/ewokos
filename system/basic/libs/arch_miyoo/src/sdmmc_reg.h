#ifndef __SD_MMC_REGS_H
#define __SD_MMC_REGS_H


#define FALSE	0
#define TRUE	1

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


//============================================
//MIE_EVENT: offset 0x00
//============================================
#define R_DATA_END              BIT00_T
#define R_CMD_END               BIT01_T
#define R_ERR_STS               BIT02_T
#define R_SDIO_INT              BIT03_T
#define R_BUSY_END_INT          BIT04_T
#define R_R2N_RDY_INT           BIT05_T
#define R_CARD_CHANGE           BIT06_T
#define R_CARD2_CHANGE          BIT07_T


//============================================
//MIE_INT_EN: offset 0x01
//============================================
#define R_DATA_END_IEN          BIT00_T
#define R_CMD_END_IEN           BIT01_T
#define R_ERR_STS_IEN           BIT02_T
#define R_SDIO_INT_IEN          BIT03_T
#define R_BUSY_END_IEN          BIT04_T
#define R_R2N_RDY_INT_IEN       BIT05_T
#define R_CARD_CHANGE_IEN       BIT06_T
#define R_CARD2_CHANGE_IEN      BIT07_T

//============================================
//MMA_PRI_REG: offset 0x02
//============================================
#define R_MIU_R_PRIORITY        BIT00_T
#define R_MIU_W_PRIORITY        BIT01_T

#define R_MIU1_SELECT           BIT02_T
#define R_MIU2_SELECT           BIT03_T
#define R_MIU3_SELECT           (BIT03_T|BIT02_T)

#define R_MIU_BUS_BURST2        BIT04_T
#define R_MIU_BUS_BURST4        BIT05_T
#define R_MIU_BUS_BURST8        (BIT05_T|BIT04_T)

//============================================
//MIE_FUNC_CTL: offset 0x07
//============================================
#define R_EMMC_EN               BIT00_T
#define R_SD_EN                 BIT01_T
#define R_SDIO_MODE             BIT02_T


//============================================
//SD_MODE: offset 0x0B
//============================================
#define R_CLK_EN                BIT00_T
#define R_BUS_WIDTH_4           BIT01_T
#define R_BUS_WIDTH_8           BIT02_T
#define R_DEST_R2N              BIT04_T
#define	R_DATASYNC              BIT05_T
#define	R_DMA_RD_CLK_STOP       BIT07_T
#define R_DIS_WR_BUSY_CHK       BIT08_T


//============================================
//SD_CTL: offset 0x0C
//============================================
#define R_RSPR2_EN              BIT00_T
#define R_RSP_EN                BIT01_T
#define R_CMD_EN                BIT02_T
#define R_DTRX_EN               BIT03_T
#define R_JOB_DIR               BIT04_T
#define R_ADMA_EN               BIT05_T
#define R_JOB_START             BIT06_T
#define R_CHK_CMD               BIT07_T
#define R_BUSY_DET_ON           BIT08_T
#define R_ERR_DET_ON            BIT09_T


//============================================
//SD_STS: offset 0x0D
//============================================
#define R_DAT_RD_CERR           BIT00_T
#define R_DAT_WR_CERR           BIT01_T
#define R_DAT_WR_TOUT           BIT02_T
#define R_CMD_NORSP             BIT03_T
#define R_CMDRSP_CERR           BIT04_T
#define R_DAT_RD_TOUT           BIT05_T
#define R_CARD_BUSY             BIT06_T
#define R_DAT0                  BIT08_T
#define R_DAT1                  BIT09_T
#define R_DAT2                  BIT10_T
#define R_DAT3                  BIT11_T
#define R_DAT4                  BIT12_T
#define R_DAT5                  BIT13_T
#define R_DAT6                  BIT14_T
#define R_DAT7                  BIT15_T


//============================================
//BOOT_MOD:offset 0x0E
//============================================
#define R_BOOT_MODE             BIT02_T


//============================================
//DDR_MOD: offset 0x0F
//============================================
#define R_PAD_IN_BYPASS         BIT00_T
#define R_PAD_IN_RDY_SEL        BIT01_T
#define R_PRE_FULL_SEL0         BIT02_T
#define R_PRE_FULL_SEL1         BIT03_T
#define R_DDR_MACRO_EN          BIT07_T
#define R_DDR_EN                BIT08_T
#define R_PAD_CLK_SEL           BIT10_T
#define R_PAD_IN_SEL_IP         BIT11_T
#define R_DDR_MACRO32_EN        BIT12_T
#define R_PAD_IN_SEL            BIT13_T
#define R_FALL_LATCH            BIT14_T
#define R_PAD_IN_MASK           BIT15_T


//============================================
//SDIO_MOD: offset 0x11
//============================================
#define R_SDIO_INT_MOD0         BIT00_T
#define R_SDIO_INT_MOD1         BIT01_T
#define R_SDIO_INT_MOD_SW_EN    BIT02_T
#define R_SDIO_DET_INT_SRC      BIT03_T
#define R_SDIO_INT_TUNE0        BIT04_T
#define R_SDIO_INT_TUNE1        BIT05_T
#define R_SDIO_INT_TUNE2        BIT06_T
#define R_SDIO_INT_TUNE_CLR0    BIT07_T
#define R_SDIO_INT_TUNE_CLR1    BIT08_T
#define R_SDIO_INT_TUNE_CLR2    BIT09_T
#define R_SDIO_RDWAIT_EN        BIT11_T
#define R_SDIO_BLK_GAP_DIS      BIT12_T
#define R_SDIO_INT_STOP_DMA     BIT13_T
#define R_SDIO_INT_TUNE_SW      BIT14_T
#define R_SDIO_INT_ASYN_EN      BIT15_T


//============================================
//TEST_MOD: offset 0x15
//============================================
#define R_SDDR1                 BIT00_T
#define R_SD_DEBUG_MOD0         BIT01_T
#define R_SD_DEBUG_MOD1         BIT02_T
#define R_SD_DEBUG_MOD2         BIT03_T
#define R_BIST_MODE             BIT04_T


//============================================
//WR_SBIT_TIMER: offset 0x17
//============================================
#define R_WR_SBIT_TIMER_EN      BIT15_T


//============================================
//RD_SBIT_TIMER: offset 0x18
//============================================
#define R_RD_SBIT_TIMER_EN      BIT15_T


//============================================
//SDIO_DET_ON: offset 0x2F
//============================================
#define R_SDIO_DET_ON           BIT00_T


//============================================
//CIFD_EVENT: offset 0x30
//============================================
#define R_WBUF_FULL             BIT00_T
#define R_WBUF_EMPTY_TRIG       BIT01_T
#define R_RBUF_FULL_TRIG        BIT02_T
#define R_RBUF_EMPTY            BIT03_T


//============================================
//CIFD_INT_EN: offset 0x31
//============================================
#define R_WBUF_FULL_IEN         BIT00_T
#define R_RBUF_EMPTY_IEN        BIT01_T
#define R_F_WBUF_FULL_INT       BIT08_T
#define R_F_RBUF_EMPTY_INT      BIT09_T


//============================================
//BOOT_MODE:offset 0x37
//============================================
#define R_NAND_BOOT_EN          BIT00_T
#define R_BOOTSRAM_ACCESS_SEL   BIT01_T
#define R_IMI_SEL               BIT02_T


//============================================
//CIFD_INT_EN: offset 0x39
//============================================
#define R_DEBUG_MOD0            BIT08_T
#define R_DEBUG_MOD1            BIT09_T
#define R_DEBUG_MOD2            BIT10_T
#define R_DEBUG_MOD3            BIT11_T


//============================================
//FCIE_RST:offset 0x3F
//============================================
#define R_FCIE_SOFT_RST         BIT00_T
#define R_RST_MIU_STS           BIT01_T
#define R_RST_MIE_STS           BIT02_T
#define	R_RST_MCU_STS           BIT03_T
#define	R_RST_ECC_STS           BIT04_T


#define A_RIU_BASE          (_mmio_base)
#define D_MIU_WIDTH                            8        // Special MIU WIDTH for FCIE4
#define REG_OFFSET_BITS                        2	// Register Offset Byte  (2= 4byte = 32bits)
#define GET_CARD_REG_ADDR(x, y)                ((x)+((y) << REG_OFFSET_BITS))


#define A_FCIE1_0_BANK      GET_CARD_REG_ADDR(A_RIU_BASE, 0xA0800)      //SDIO0_0_BANK 1410h
#define A_FCIE1_1_BANK      GET_CARD_REG_ADDR(A_RIU_BASE, 0xA0880)      //SDIO0_1_BANK 1411h
#define A_FCIE1_2_BANK      GET_CARD_REG_ADDR(A_RIU_BASE, 0xA0900)      //SDIO0_2_BANK 1412h

#define A_FCIE2_0_BANK      A_FCIE1_0_BANK
#define A_FCIE2_1_BANK      A_FCIE1_1_BANK
#define A_FCIE2_2_BANK      A_FCIE1_2_BANK

#define A_FCIE3_0_BANK      A_FCIE1_0_BANK
#define A_FCIE3_1_BANK      A_FCIE1_1_BANK
#define A_FCIE3_2_BANK      A_FCIE1_2_BANK


// Mask Range
//-----------------------------------------------------------------------------------------------------------
#define M_SD_ERRSTS			(R_DAT_RD_CERR|R_DAT_WR_CERR|R_DAT_WR_TOUT|R_CMD_NORSP|R_CMDRSP_CERR|R_DAT_RD_TOUT)
#define M_SD_MIEEVENT		(R_DATA_END|R_CMD_END|R_ERR_STS|R_BUSY_END_INT|R_R2N_RDY_INT)
#define M_RST_STS           (R_RST_MIU_STS|R_RST_MIE_STS|R_RST_MCU_STS)

#define IO_MAPADDR(Reg_Addr)	Reg_Addr
#define D_MIU_WIDTH                            8        // Special MIU WIDTH for FCIE4
#define REG_OFFSET_BITS                        2	// Register Offset Byte  (2= 4byte = 32bits)
#define GET_CARD_REG_ADDR(x, y)                ((x)+((y) << REG_OFFSET_BITS))

#define CARD_REG(Reg_Addr)                     (*(volatile uint16_t*)(IO_MAPADDR(Reg_Addr)) )
#define CARD_REG_L8(Reg_Addr)                  (*(volatile uint8_t*)(IO_MAPADDR(Reg_Addr)) )
#define CARD_REG_H8(Reg_Addr)                  (*( (volatile uint8_t*)(IO_MAPADDR(Reg_Addr))+1) )

#define CARD_REG_SETBIT(Reg_Addr, Value)       CARD_REG(Reg_Addr) |= (Value)
#define CARD_REG_CLRBIT(Reg_Addr, Value)       CARD_REG(Reg_Addr) &= (~(Value))

#define CARD_REG_L8_SETBIT(Reg_Addr, Value)    CARD_REG_L8(Reg_Addr) |= (Value)
#define CARD_REG_H8_SETBIT(Reg_Addr, Value)    CARD_REG_H8(Reg_Addr) |= (Value)
#define CARD_REG_L8_CLRBIT(Reg_Addr, Value)    CARD_REG_L8(Reg_Addr) &= (~(Value))
#define CARD_REG_H8_CLRBIT(Reg_Addr, Value)    CARD_REG_H8(Reg_Addr) &= (~(Value))

#define CARD_BANK(Bank_Addr)                   IO_MAPADDR(Bank_Addr)

#endif //End of __HAL_CARD_FCIE5_H