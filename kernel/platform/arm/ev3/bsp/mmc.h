/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Davinci MMC Controller Defines - Based on Linux davinci_mmc.c
 *
 * Copyright (C) 2010 Texas Instruments Incorporated
 */

#ifndef _SDMMC_DEFS_H_
#define _SDMMC_DEFS_H_

#define MMC_BASE    (MMIO_BASE + 0x01C40000)
//#define MMC_BASE    (MMIO_BASE + 0x01E1B000)

/* SD/MMC version bits; 8 flags, 8 major, 8 minor, 8 change */
#define SD_VERSION_SD	(1U << 31)
#define MMC_VERSION_MMC	(1U << 30)

#define MAKE_SDMMC_VERSION(a, b, c)	\
	((((u32)(a)) << 16) | ((u32)(b) << 8) | (u32)(c))
#define MAKE_SD_VERSION(a, b, c)	\
	(SD_VERSION_SD | MAKE_SDMMC_VERSION(a, b, c))
#define MAKE_MMC_VERSION(a, b, c)	\
	(MMC_VERSION_MMC | MAKE_SDMMC_VERSION(a, b, c))

#define EXTRACT_SDMMC_MAJOR_VERSION(x)	\
	(((u32)(x) >> 16) & 0xff)
#define EXTRACT_SDMMC_MINOR_VERSION(x)	\
	(((u32)(x) >> 8) & 0xff)
#define EXTRACT_SDMMC_CHANGE_VERSION(x)	\
	((u32)(x) & 0xff)

#define SD_VERSION_3		MAKE_SD_VERSION(3, 0, 0)
#define SD_VERSION_2		MAKE_SD_VERSION(2, 0, 0)
#define SD_VERSION_1_0		MAKE_SD_VERSION(1, 0, 0)
#define SD_VERSION_1_10		MAKE_SD_VERSION(1, 10, 0)

#define MMC_VERSION_UNKNOWN	MAKE_MMC_VERSION(0, 0, 0)
#define MMC_VERSION_1_2		MAKE_MMC_VERSION(1, 2, 0)
#define MMC_VERSION_1_4		MAKE_MMC_VERSION(1, 4, 0)
#define MMC_VERSION_2_2		MAKE_MMC_VERSION(2, 2, 0)
#define MMC_VERSION_3		MAKE_MMC_VERSION(3, 0, 0)
#define MMC_VERSION_4		MAKE_MMC_VERSION(4, 0, 0)
#define MMC_VERSION_4_1		MAKE_MMC_VERSION(4, 1, 0)
#define MMC_VERSION_4_2		MAKE_MMC_VERSION(4, 2, 0)
#define MMC_VERSION_4_3		MAKE_MMC_VERSION(4, 3, 0)
#define MMC_VERSION_4_4		MAKE_MMC_VERSION(4, 4, 0)
#define MMC_VERSION_4_41	MAKE_MMC_VERSION(4, 4, 1)
#define MMC_VERSION_4_5		MAKE_MMC_VERSION(4, 5, 0)
#define MMC_VERSION_5_0		MAKE_MMC_VERSION(5, 0, 0)
#define MMC_VERSION_5_1		MAKE_MMC_VERSION(5, 1, 0)

#define MMC_CAP(mode)		(1 << mode)
#define MMC_MODE_HS		(MMC_CAP(MMC_HS) | MMC_CAP(SD_HS))
#define MMC_MODE_HS_52MHz	MMC_CAP(MMC_HS_52)
#define MMC_MODE_DDR_52MHz	MMC_CAP(MMC_DDR_52)
#define MMC_MODE_HS200		MMC_CAP(MMC_HS_200)
#define MMC_MODE_HS400		MMC_CAP(MMC_HS_400)
#define MMC_MODE_HS400_ES	MMC_CAP(MMC_HS_400_ES)

#define MMC_CAP_NONREMOVABLE	BIT(14)
#define MMC_CAP_NEEDS_POLL	BIT(15)
#define MMC_CAP_CD_ACTIVE_HIGH  BIT(16)

#define MMC_MODE_8BIT		BIT(30)
#define MMC_MODE_4BIT		BIT(29)
#define MMC_MODE_1BIT		BIT(28)
#define MMC_MODE_SPI		BIT(27)

#define SD_DATA_4BIT	0x00040000

#define IS_SD(x)	((x)->version & SD_VERSION_SD)
#define IS_MMC(x)	((x)->version & MMC_VERSION_MMC)

#define MMC_DATA_READ		1
#define MMC_DATA_WRITE		2

#define MMC_CMD_GO_IDLE_STATE		0
#define MMC_CMD_SEND_OP_COND		1
#define MMC_CMD_ALL_SEND_CID		2
#define MMC_CMD_SET_RELATIVE_ADDR	3
#define MMC_CMD_SET_DSR			4
#define MMC_CMD_SWITCH			6
#define MMC_CMD_SELECT_CARD		7
#define MMC_CMD_SEND_EXT_CSD		8
#define MMC_CMD_SEND_CSD		9
#define MMC_CMD_SEND_CID		10
#define MMC_CMD_STOP_TRANSMISSION	12
#define MMC_CMD_SEND_STATUS		13
#define MMC_CMD_SET_BLOCKLEN		16
#define MMC_CMD_READ_SINGLE_BLOCK	17
#define MMC_CMD_READ_MULTIPLE_BLOCK	18
#define MMC_CMD_SEND_TUNING_BLOCK		19
#define MMC_CMD_SEND_TUNING_BLOCK_HS200	21
#define MMC_CMD_SET_BLOCK_COUNT         23
#define MMC_CMD_WRITE_SINGLE_BLOCK	24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK	25
#define MMC_CMD_ERASE_GROUP_START	35
#define MMC_CMD_ERASE_GROUP_END		36
#define MMC_CMD_ERASE			38
#define MMC_CMD_APP_CMD			55
#define MMC_CMD_SPI_READ_OCR		58
#define MMC_CMD_SPI_CRC_ON_OFF		59
#define MMC_CMD_RES_MAN			62

#define MMC_CMD62_ARG1			0xefac62ec
#define MMC_CMD62_ARG2			0xcbaea7

#define SD_CMD_SEND_RELATIVE_ADDR	3
#define SD_CMD_SWITCH_FUNC		6
#define SD_CMD_SEND_IF_COND		8
#define SD_CMD_SWITCH_UHS18V		11

#define SD_CMD_APP_SET_BUS_WIDTH	6
#define SD_CMD_APP_SD_STATUS		13
#define SD_CMD_ERASE_WR_BLK_START	32
#define SD_CMD_ERASE_WR_BLK_END		33
#define SD_CMD_APP_SEND_OP_COND		41
#define SD_CMD_APP_SEND_SCR		51

static inline bool mmc_is_tuning_cmd(unsigned int cmdidx)
{
	if ((cmdidx == MMC_CMD_SEND_TUNING_BLOCK_HS200) ||
	    (cmdidx == MMC_CMD_SEND_TUNING_BLOCK))
		return true;
	return false;
}

/* SCR definitions in different words */
#define SD_HIGHSPEED_BUSY	0x00020000
#define SD_HIGHSPEED_SUPPORTED	0x00020000

#define UHS_SDR12_BUS_SPEED	0
#define HIGH_SPEED_BUS_SPEED	1
#define UHS_SDR25_BUS_SPEED	1
#define UHS_SDR50_BUS_SPEED	2
#define UHS_SDR104_BUS_SPEED	3
#define UHS_DDR50_BUS_SPEED	4

#define SD_MODE_UHS_SDR12	BIT(UHS_SDR12_BUS_SPEED)
#define SD_MODE_UHS_SDR25	BIT(UHS_SDR25_BUS_SPEED)
#define SD_MODE_UHS_SDR50	BIT(UHS_SDR50_BUS_SPEED)
#define SD_MODE_UHS_SDR104	BIT(UHS_SDR104_BUS_SPEED)
#define SD_MODE_UHS_DDR50	BIT(UHS_DDR50_BUS_SPEED)

#define OCR_BUSY		0x80000000
#define OCR_HCS			0x40000000
#define OCR_S18R		0x1000000
#define OCR_VOLTAGE_MASK	0x007FFF80
#define OCR_ACCESS_MODE		0x60000000

#define MMC_ERASE_ARG		0x00000000
#define MMC_SECURE_ERASE_ARG	0x80000000
#define MMC_TRIM_ARG		0x00000001
#define MMC_DISCARD_ARG		0x00000003
#define MMC_SECURE_TRIM1_ARG	0x80000001
#define MMC_SECURE_TRIM2_ARG	0x80008000

#define MMC_STATUS_MASK		(~0x0206BF7F)
#define MMC_STATUS_SWITCH_ERROR	(1 << 7)
#define MMC_STATUS_RDY_FOR_DATA (1 << 8)
#define MMC_STATUS_CURR_STATE	(0xf << 9)
#define MMC_STATUS_ERROR	(1 << 19)

#define MMC_STATE_PRG		(7 << 9)
#define MMC_STATE_TRANS		(4 << 9)

#define MMC_VDD_165_195		0x00000080	/* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21		0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22		0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23		0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24		0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25		0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26		0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27		0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28		0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29		0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30		0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31		0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32		0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33		0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34		0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35		0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36		0x00800000	/* VDD voltage 3.5 ~ 3.6 */

#define MMC_SWITCH_MODE_CMD_SET		0x00 /* Change the command set */
#define MMC_SWITCH_MODE_SET_BITS	0x01 /* Set bits in EXT_CSD byte
						addressed by index which are
						1 in value field */
#define MMC_SWITCH_MODE_CLEAR_BITS	0x02 /* Clear bits in EXT_CSD byte
						addressed by index, which are
						1 in value field */
#define MMC_SWITCH_MODE_WRITE_BYTE	0x03 /* Set target byte to value */

#define SD_SWITCH_CHECK		0
#define SD_SWITCH_SWITCH	1

/*
 * EXT_CSD fields
 */
#define EXT_CSD_ENH_START_ADDR		136	/* R/W */
#define EXT_CSD_ENH_SIZE_MULT		140	/* R/W */
#define EXT_CSD_GP_SIZE_MULT		143	/* R/W */
#define EXT_CSD_PARTITION_SETTING	155	/* R/W */
#define EXT_CSD_PARTITIONS_ATTRIBUTE	156	/* R/W */
#define EXT_CSD_MAX_ENH_SIZE_MULT	157	/* R */
#define EXT_CSD_PARTITIONING_SUPPORT	160	/* RO */
#define EXT_CSD_RST_N_FUNCTION		162	/* R/W */
#define EXT_CSD_BKOPS_EN		163	/* R/W & R/W/E */
#define EXT_CSD_WR_REL_PARAM		166	/* R */
#define EXT_CSD_WR_REL_SET		167	/* R/W */
#define EXT_CSD_RPMB_MULT		168	/* RO */
#define EXT_CSD_USER_WP			171	/* R/W & R/W/C_P & R/W/E_P */
#define EXT_CSD_BOOT_WP			173	/* R/W & R/W/C_P */
#define EXT_CSD_BOOT_WP_STATUS		174	/* R */
#define EXT_CSD_ERASE_GROUP_DEF		175	/* R/W */
#define EXT_CSD_BOOT_BUS_WIDTH		177
#define EXT_CSD_PART_CONF		179	/* R/W */
#define EXT_CSD_BUS_WIDTH		183	/* R/W */
#define EXT_CSD_STROBE_SUPPORT		184	/* R/W */
#define EXT_CSD_HS_TIMING		185	/* R/W */
#define EXT_CSD_REV			192	/* RO */
#define EXT_CSD_CARD_TYPE		196	/* RO */
#define EXT_CSD_PART_SWITCH_TIME	199	/* RO */
#define EXT_CSD_SEC_CNT			212	/* RO, 4 bytes */
#define EXT_CSD_HC_WP_GRP_SIZE		221	/* RO */
#define EXT_CSD_HC_ERASE_GRP_SIZE	224	/* RO */
#define EXT_CSD_BOOT_MULT		226	/* RO */
#define EXT_CSD_SEC_FEATURE		231	/* RO */
#define EXT_CSD_GENERIC_CMD6_TIME       248     /* RO */
#define EXT_CSD_BKOPS_SUPPORT		502	/* RO */

/*
 * EXT_CSD field definitions
 */

#define EXT_CSD_CMD_SET_NORMAL		(1 << 0)
#define EXT_CSD_CMD_SET_SECURE		(1 << 1)
#define EXT_CSD_CMD_SET_CPSECURE	(1 << 2)

#define EXT_CSD_CARD_TYPE_26	(1 << 0)	/* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52	(1 << 1)	/* Card can run at 52MHz */
#define EXT_CSD_CARD_TYPE_DDR_1_8V	(1 << 2)
#define EXT_CSD_CARD_TYPE_DDR_1_2V	(1 << 3)
#define EXT_CSD_CARD_TYPE_DDR_52	(EXT_CSD_CARD_TYPE_DDR_1_8V \
					| EXT_CSD_CARD_TYPE_DDR_1_2V)

#define EXT_CSD_CARD_TYPE_HS200_1_8V	BIT(4)	/* Card can run at 200MHz */
						/* SDR mode @1.8V I/O */
#define EXT_CSD_CARD_TYPE_HS200_1_2V	BIT(5)	/* Card can run at 200MHz */
						/* SDR mode @1.2V I/O */
#define EXT_CSD_CARD_TYPE_HS200		(EXT_CSD_CARD_TYPE_HS200_1_8V | \
					 EXT_CSD_CARD_TYPE_HS200_1_2V)
#define EXT_CSD_CARD_TYPE_HS400_1_8V	BIT(6)
#define EXT_CSD_CARD_TYPE_HS400_1_2V	BIT(7)
#define EXT_CSD_CARD_TYPE_HS400		(EXT_CSD_CARD_TYPE_HS400_1_8V | \
					 EXT_CSD_CARD_TYPE_HS400_1_2V)

#define EXT_CSD_BUS_WIDTH_1	0	/* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4	1	/* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8	2	/* Card is in 8 bit mode */
#define EXT_CSD_DDR_BUS_WIDTH_4	5	/* Card is in 4 bit DDR mode */
#define EXT_CSD_DDR_BUS_WIDTH_8	6	/* Card is in 8 bit DDR mode */
#define EXT_CSD_DDR_FLAG	BIT(2)	/* Flag for DDR mode */
#define EXT_CSD_BUS_WIDTH_STROBE BIT(7)	/* Enhanced strobe mode */

#define EXT_CSD_TIMING_LEGACY	0	/* no high speed */
#define EXT_CSD_TIMING_HS	1	/* HS */
#define EXT_CSD_TIMING_HS200	2	/* HS200 */
#define EXT_CSD_TIMING_HS400	3	/* HS400 */
#define EXT_CSD_DRV_STR_SHIFT	4	/* Driver Strength shift */

#define EXT_CSD_BOOT_ACK_ENABLE			(1 << 6)
#define EXT_CSD_BOOT_PARTITION_ENABLE		(1 << 3)
#define EXT_CSD_PARTITION_ACCESS_ENABLE		(1 << 0)
#define EXT_CSD_PARTITION_ACCESS_DISABLE	(0 << 0)

#define EXT_CSD_BOOT_ACK(x)		(x << 6)
#define EXT_CSD_BOOT_PART_NUM(x)	(x << 3)
#define EXT_CSD_PARTITION_ACCESS(x)	(x << 0)

#define EXT_CSD_EXTRACT_BOOT_ACK(x)		(((x) >> 6) & 0x1)
#define EXT_CSD_EXTRACT_BOOT_PART(x)		(((x) >> 3) & 0x7)
#define EXT_CSD_EXTRACT_PARTITION_ACCESS(x)	((x) & 0x7)

#define EXT_CSD_BOOT_BUS_WIDTH_MODE(x)	(x << 3)
#define EXT_CSD_BOOT_BUS_WIDTH_RESET(x)	(x << 2)
#define EXT_CSD_BOOT_BUS_WIDTH_WIDTH(x)	(x)

#define EXT_CSD_PARTITION_SETTING_COMPLETED	(1 << 0)

#define EXT_CSD_ENH_USR		(1 << 0)	/* user data area is enhanced */
#define EXT_CSD_ENH_GP(x)	(1 << ((x)+1))	/* GP part (x+1) is enhanced */

#define EXT_CSD_HS_CTRL_REL	(1 << 0)	/* host controlled WR_REL_SET */

#define EXT_CSD_BOOT_WP_B_SEC_WP_SEL	(0x80)	/* enable partition selector */
#define EXT_CSD_BOOT_WP_B_PWR_WP_SEC_SEL (0x02)	/* partition selector to protect */
#define EXT_CSD_BOOT_WP_B_PWR_WP_EN	(0x01)	/* power-on write-protect */

#define EXT_CSD_WR_DATA_REL_USR		(1 << 0)	/* user data area WR_REL */
#define EXT_CSD_WR_DATA_REL_GP(x)	(1 << ((x)+1))	/* GP part (x+1) WR_REL */

#define EXT_CSD_SEC_FEATURE_TRIM_EN	(1 << 4) /* Support secure & insecure trim */

#define R1_ILLEGAL_COMMAND		(1 << 22)
#define R1_APP_CMD			(1 << 5)

#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136	(1 << 1)		/* 136 bit response */
#define MMC_RSP_CRC	(1 << 2)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)		/* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE| \
			MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3	(MMC_RSP_PRESENT)
#define MMC_RSP_R4	(MMC_RSP_PRESENT)
#define MMC_RSP_R5	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define MMCPART_NOAVAILABLE	(0xff)
#define PART_ACCESS_MASK	(0x7)
#define PART_SUPPORT		(0x1)
#define ENHNCD_SUPPORT		(0x2)
#define PART_ENH_ATTRIB		(0x1f)

#define MMC_QUIRK_RETRY_SEND_CID	BIT(0)
#define MMC_QUIRK_RETRY_SET_BLOCKLEN	BIT(1)
#define MMC_QUIRK_RETRY_APP_CMD	BIT(2)

enum mmc_voltage {
	MMC_SIGNAL_VOLTAGE_000 = 0,
	MMC_SIGNAL_VOLTAGE_120 = 1,
	MMC_SIGNAL_VOLTAGE_180 = 2,
	MMC_SIGNAL_VOLTAGE_330 = 4,
};

#define MMC_ALL_SIGNAL_VOLTAGE (MMC_SIGNAL_VOLTAGE_120 |\
				MMC_SIGNAL_VOLTAGE_180 |\
				MMC_SIGNAL_VOLTAGE_330)

/* Maximum block size for MMC */
#define MMC_MAX_BLOCK_LEN	512

/* The number of MMC physical partitions.  These consist of:
 * boot partitions (2), general purpose partitions (4) in MMC v4.4.
 */
#define MMC_NUM_BOOT_PARTITION	2
#define MMC_PART_RPMB           3       /* RPMB partition number */

/* timing specification used */
#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2
#define MMC_TIMING_UHS_SDR12	3
#define MMC_TIMING_UHS_SDR25	4
#define MMC_TIMING_UHS_SDR50	5
#define MMC_TIMING_UHS_SDR104	6
#define MMC_TIMING_UHS_DDR50	7
#define MMC_TIMING_MMC_DDR52	8
#define MMC_TIMING_MMC_HS200	9
#define MMC_TIMING_MMC_HS400	10

/* emmc PARTITION_CONFIG BOOT_PARTITION_ENABLE values */
enum emmc_boot_part {
	EMMC_BOOT_PART_DEFAULT = 0,
	EMMC_BOOT_PART_BOOT1 = 1,
	EMMC_BOOT_PART_BOOT2 = 2,
	EMMC_BOOT_PART_USER = 7,
};

/* emmc PARTITION_CONFIG BOOT_PARTITION_ENABLE names */
extern const char *emmc_boot_part_names[8];

/* emmc PARTITION_CONFIG ACCESS_ENABLE values */
enum emmc_hwpart {
	EMMC_HWPART_DEFAULT = 0, /* user */
	EMMC_HWPART_BOOT1 = 1,
	EMMC_HWPART_BOOT2 = 2,
	EMMC_HWPART_RPMB = 3,
	EMMC_HWPART_GP1 = 4,
	EMMC_HWPART_GP2 = 5,
	EMMC_HWPART_GP3 = 6,
	EMMC_HWPART_GP4 = 7,
};

/* emmc PARTITION_CONFIG ACCESS_ENABLE names */
extern const char *emmc_hwpart_names[8];

/* Driver model support */

/**
 * struct mmc_uclass_priv - Holds information about a device used by the uclass
 */
struct mmc_uclass_priv {
	struct mmc *mmc;
};

/**
 * mmc_get_mmc_dev() - get the MMC struct pointer for a device
 *
 * Provided that the device is already probed and ready for use, this value
 * will be available.
 *
 * @dev:	Device
 * Return: associated mmc struct pointer if available, else NULL
 */

/* End of driver model support */

struct mmc_cid {
	unsigned long psn;
	unsigned short oid;
	unsigned char mid;
	unsigned char prv;
	unsigned char mdt;
	char pnm[7];
};

struct mmc_cmd {
	unsigned int cmdidx;
	unsigned int resp_type;
	unsigned int cmdarg;
	unsigned int response[4];
};

struct mmc_data {
	union {
		char *dest;
		const char *src; /* src buffers don't get written to */
	}un;
	unsigned int flags;
	unsigned int blocks;
	unsigned int blocksize;
};

/* MMC Control Reg fields */
#define MMCCTL_DATRST		(1 << 0)
#define MMCCTL_CMDRST		(1 << 1)
#define MMCCTL_WIDTH_4_BIT	(1 << 2)
#define MMCCTL_DATEG_DISABLED	(0 << 6)
#define MMCCTL_DATEG_RISING	(1 << 6)
#define MMCCTL_DATEG_FALLING	(2 << 6)
#define MMCCTL_DATEG_BOTH	(3 << 6)
#define MMCCTL_PERMDR_LE	(0 << 9)
#define MMCCTL_PERMDR_BE	(1 << 9)
#define MMCCTL_PERMDX_LE	(0 << 10)
#define MMCCTL_PERMDX_BE	(1 << 10)

/* MMC Clock Control Reg fields */
#define MMCCLK_CLKEN		(1 << 8)
#define MMCCLK_CLKRT_MASK	(0xFF << 0)

/* MMC Status Reg0 fields */
#define MMCST0_DATDNE		(1 << 0)
#define MMCST0_BSYDNE		(1 << 1)
#define MMCST0_RSPDNE		(1 << 2)
#define MMCST0_TOUTRD		(1 << 3)
#define MMCST0_TOUTRS		(1 << 4)
#define MMCST0_CRCWR		(1 << 5)
#define MMCST0_CRCRD		(1 << 6)
#define MMCST0_CRCRS		(1 << 7)
#define MMCST0_DXRDY		(1 << 9)
#define MMCST0_DRRDY		(1 << 10)
#define MMCST0_DATED		(1 << 11)
#define MMCST0_TRNDNE		(1 << 12)

#define MMCST0_ERR_MASK		(0x00F8)

/* MMC Status Reg1 fields */
#define MMCST1_BUSY		(1 << 0)
#define MMCST1_CLKSTP		(1 << 1)
#define MMCST1_DXEMP		(1 << 2)
#define MMCST1_DRFUL		(1 << 3)
#define MMCST1_DAT3ST		(1 << 4)
#define MMCST1_FIFOEMP		(1 << 5)
#define MMCST1_FIFOFUL		(1 << 6)

/* MMC INT Mask Reg fields */
#define MMCIM_EDATDNE		(1 << 0)
#define MMCIM_EBSYDNE		(1 << 1)
#define MMCIM_ERSPDNE		(1 << 2)
#define MMCIM_ETOUTRD		(1 << 3)
#define MMCIM_ETOUTRS		(1 << 4)
#define MMCIM_ECRCWR		(1 << 5)
#define MMCIM_ECRCRD		(1 << 6)
#define MMCIM_ECRCRS		(1 << 7)
#define MMCIM_EDXRDY		(1 << 9)
#define MMCIM_EDRRDY		(1 << 10)
#define MMCIM_EDATED		(1 << 11)
#define MMCIM_ETRNDNE		(1 << 12)

#define MMCIM_MASKALL		(0xFFFFFFFF)

/* MMC Resp Tout Reg fields */
#define MMCTOR_TOR_MASK		(0xFF)		/* dont write to reg, | it */
#define MMCTOR_TOD_20_16_SHIFT  (8)

/* MMC Data Read Tout Reg fields */
#define MMCTOD_TOD_0_15_MASK	(0xFFFF)

/* MMC Block len Reg fields */
#define MMCBLEN_BLEN_MASK	(0xFFF)

/* MMC Num Blocks Reg fields */
#define MMCNBLK_NBLK_MASK	(0xFFFF)
#define MMCNBLK_NBLK_MAX	(0xFFFF)

/* MMC Num Blocks Counter Reg fields */
#define MMCNBLC_NBLC_MASK	(0xFFFF)

/* MMC Cmd Reg fields */
#define MMCCMD_CMD_MASK		(0x3F)
#define MMCCMD_PPLEN		(1 << 7)
#define MMCCMD_BSYEXP		(1 << 8)
#define MMCCMD_RSPFMT_NONE	(0 << 9)
#define MMCCMD_RSPFMT_R1567	(1 << 9)
#define MMCCMD_RSPFMT_R2	(2 << 9)
#define MMCCMD_RSPFMT_R3	(3 << 9)
#define MMCCMD_DTRW		(1 << 11)
#define MMCCMD_STRMTP		(1 << 12)
#define MMCCMD_WDATX		(1 << 13)
#define MMCCMD_INITCK		(1 << 14)
#define MMCCMD_DCLR		(1 << 15)
#define MMCCMD_DMATRIG		(1 << 16)

/* FIFO control Reg fields */
#define MMCFIFOCTL_FIFORST	(1 << 0)
#define MMCFIFOCTL_FIFODIR	(1 << 1)
#define MMCFIFOCTL_FIFOLEV	(1 << 2)
#define MMCFIFOCTL_ACCWD_4	(0 << 3)	/* access width of 4 bytes */
#define MMCFIFOCTL_ACCWD_3	(1 << 3)	/* access width of 3 bytes */
#define MMCFIFOCTL_ACCWD_2	(2 << 3)	/* access width of 2 bytes */
#define MMCFIFOCTL_ACCWD_1	(3 << 3)	/* access width of 1 byte */

/* Davinci MMC Register definitions */
struct davinci_mmc_regs {
	volatile unsigned int mmcctl;
	volatile unsigned int mmcclk;
	volatile unsigned int mmcst0;
	volatile unsigned int mmcst1;
	volatile unsigned int mmcim;
	volatile unsigned int mmctor;
	volatile unsigned int mmctod;
	volatile unsigned int mmcblen;
	volatile unsigned int mmcnblk;
	volatile unsigned int mmcnblc;
	volatile unsigned int mmcdrr;
	volatile unsigned int mmcdxr;
	volatile unsigned int mmccmd;
	volatile unsigned int mmcarghl;
	volatile unsigned int mmcrsp01;
	volatile unsigned int mmcrsp23;
	volatile unsigned int mmcrsp45;
	volatile unsigned int mmcrsp67;
	volatile unsigned int mmcdrsp;
	volatile unsigned int mmcetok;
	volatile unsigned int mmccidx;
	volatile unsigned int mmcckc;
	volatile unsigned int mmctorc;
	volatile unsigned int mmctodc;
	volatile unsigned int mmcblnc;
	volatile unsigned int sdioctl;
	volatile unsigned int sdiost0;
	volatile unsigned int sdioien;
	volatile unsigned int sdioist;
	volatile unsigned int mmcfifoctl;
};

/* Davinci MMC board definitions */
struct davinci_mmc {
	struct davinci_mmc_regs *reg_base;	/* Register base address */
	unsigned int input_clk;		/* Input clock to MMC controller */
	unsigned int host_caps;		/* Host capabilities */
	unsigned int voltages;		/* Host supported voltages */
};

#define DAVINCI_MAX_BLOCKS	(32)
#endif /* _SDMMC_DEFS_H */
