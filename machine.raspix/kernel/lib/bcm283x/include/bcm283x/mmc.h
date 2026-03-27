#ifndef __MMC_H__
#define __MMC_H__

#include <stdint.h>
#include <stdbool.h>

/* SD/MMC version bits; 8 flags, 8 major, 8 minor, 8 change */
#define SD_VERSION_SD	(1U << 31)
#define MMC_VERSION_MMC	(1U << 30)
#define BIT(x)  (0x1<<(x))

#define MAKE_SDMMC_VERSION(a, b, c)	\
	((((uint32_t)(a)) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(c))
#define MAKE_SD_VERSION(a, b, c)	\
	(SD_VERSION_SD | MAKE_SDMMC_VERSION(a, b, c))
#define MAKE_MMC_VERSION(a, b, c)	\
	(MMC_VERSION_MMC | MAKE_SDMMC_VERSION(a, b, c))

#define EXTRACT_SDMMC_MAJOR_VERSION(x)	\
	(((uint32_t)(x) >> 16) & 0xff)
#define EXTRACT_SDMMC_MINOR_VERSION(x)	\
	(((uint32_t)(x) >> 8) & 0xff)
#define EXTRACT_SDMMC_CHANGE_VERSION(x)	\
	((uint32_t)(x) & 0xff)

enum bus_mode {
	MMC_LEGACY,
	MMC_HS,
	SD_HS,
	MMC_HS_52,
	MMC_DDR_52,
	UHS_SDR12,
	UHS_SDR25,
	UHS_SDR50,
	UHS_DDR50,
	UHS_SDR104,
	MMC_HS_200,
	MMC_HS_400,
	MMC_HS_400_ES,
	MMC_MODES_END
};

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

#define IS_SD(x)	(!!((x)->version & SD_VERSION_SD))
#define IS_MMC(x)	(!!((x)->version & MMC_VERSION_MMC))

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

#define MVEBU_MMC_CLOCKRATE_MAX			50000000
#define MVEBU_MMC_BASE_DIV_MAX			0x7ff
#define MVEBU_MMC_BASE_FAST_CLOCK		CFG_SYS_TCLK
#define MVEBU_MMC_BASE_FAST_CLK_100		100000000
#define MVEBU_MMC_BASE_FAST_CLK_200		200000000

/* SDIO register */
#define SDIO_SYS_ADDR_LOW			0x000
#define SDIO_SYS_ADDR_HI			0x004
#define SDIO_BLK_SIZE				0x008
#define SDIO_BLK_COUNT				0x00c
#define SDIO_ARG_LOW				0x010
#define SDIO_ARG_HI				0x014
#define SDIO_XFER_MODE				0x018
#define SDIO_CMD				0x01c
#define SDIO_RSP(i)				(0x020 + ((i)<<2))
#define SDIO_RSP0				0x020
#define SDIO_RSP1				0x024
#define SDIO_RSP2				0x028
#define SDIO_RSP3				0x02c
#define SDIO_RSP4				0x030
#define SDIO_RSP5				0x034
#define SDIO_RSP6				0x038
#define SDIO_RSP7				0x03c
#define SDIO_BUF_DATA_PORT			0x040
#define SDIO_RSVED				0x044
#define SDIO_HW_STATE				0x048
#define SDIO_PRESENT_STATE0			0x048
#define SDIO_PRESENT_STATE1			0x04c
#define SDIO_HOST_CTRL				0x050
#define SDIO_BLK_GAP_CTRL			0x054
#define SDIO_CLK_CTRL				0x058
#define SDIO_SW_RESET				0x05c
#define SDIO_NOR_INTR_STATUS			0x060
#define SDIO_ERR_INTR_STATUS			0x064
#define SDIO_NOR_STATUS_EN			0x068
#define SDIO_ERR_STATUS_EN			0x06c
#define SDIO_NOR_INTR_EN			0x070
#define SDIO_ERR_INTR_EN			0x074
#define SDIO_AUTOCMD12_ERR_STATUS		0x078
#define SDIO_CURR_BYTE_LEFT			0x07c
#define SDIO_CURR_BLK_LEFT			0x080
#define SDIO_AUTOCMD12_ARG_LOW			0x084
#define SDIO_AUTOCMD12_ARG_HI			0x088
#define SDIO_AUTOCMD12_INDEX			0x08c
#define SDIO_AUTO_RSP(i)			(0x090 + ((i)<<2))
#define SDIO_AUTO_RSP0				0x090
#define SDIO_AUTO_RSP1				0x094
#define SDIO_AUTO_RSP2				0x098
#define SDIO_CLK_DIV				0x128

#define WINDOW_CTRL(i)				(0x108 + ((i) << 3))
#define WINDOW_BASE(i)				(0x10c + ((i) << 3))

/* SDIO_PRESENT_STATE */
#define CARD_BUSY				(1 << 1)
#define CMD_INHIBIT				(1 << 0)
#define CMD_TXACTIVE				(1 << 8)
#define CMD_RXACTIVE				(1 << 9)
#define CMD_FIFO_EMPTY				(1 << 13)
#define CMD_AUTOCMD12ACTIVE			(1 << 14)
#define CMD_BUS_BUSY				(CMD_AUTOCMD12ACTIVE |	\
						CMD_RXACTIVE |	\
						CMD_TXACTIVE |	\
						CMD_INHIBIT |	\
						CARD_BUSY)

/*
 * SDIO_CMD
 */

#define SDIO_CMD_RSP_NONE			(0 << 0)
#define SDIO_CMD_RSP_136			(1 << 0)
#define SDIO_CMD_RSP_48				(2 << 0)
#define SDIO_CMD_RSP_48BUSY			(3 << 0)

#define SDIO_CMD_CHECK_DATACRC16		(1 << 2)
#define SDIO_CMD_CHECK_CMDCRC			(1 << 3)
#define SDIO_CMD_INDX_CHECK			(1 << 4)
#define SDIO_CMD_DATA_PRESENT			(1 << 5)
#define SDIO_UNEXPECTED_RESP			(1 << 7)

#define SDIO_CMD_INDEX(x)			((x) << 8)

/*
 * SDIO_XFER_MODE
 */

#define SDIO_XFER_MODE_STOP_CLK			(1 << 5)
#define SDIO_XFER_MODE_HW_WR_DATA_EN		(1 << 1)
#define SDIO_XFER_MODE_AUTO_CMD12		(1 << 2)
#define SDIO_XFER_MODE_INT_CHK_EN		(1 << 3)
#define SDIO_XFER_MODE_TO_HOST			(1 << 4)
#define SDIO_XFER_MODE_DMA			(0 << 6)

/*
 * SDIO_HOST_CTRL
 */

#define SDIO_HOST_CTRL_PUSH_PULL_EN		(1 << 0)

#define SDIO_HOST_CTRL_CARD_TYPE_MEM_ONLY	(0 << 1)
#define SDIO_HOST_CTRL_CARD_TYPE_IO_ONLY	(1 << 1)
#define SDIO_HOST_CTRL_CARD_TYPE_IO_MEM_COMBO	(2 << 1)
#define SDIO_HOST_CTRL_CARD_TYPE_IO_MMC		(3 << 1)
#define SDIO_HOST_CTRL_CARD_TYPE_MASK		(3 << 1)

#define SDIO_HOST_CTRL_BIG_ENDIAN		(1 << 3)
#define SDIO_HOST_CTRL_LSB_FIRST		(1 << 4)
#define SDIO_HOST_CTRL_DATA_WIDTH_1_BIT		(0 << 9)
#define SDIO_HOST_CTRL_DATA_WIDTH_4_BITS	(1 << 9)
#define SDIO_HOST_CTRL_HI_SPEED_EN		(1 << 10)

#define SDIO_HOST_CTRL_TMOUT_MAX		0xf
#define SDIO_HOST_CTRL_TMOUT_MASK		(0xf << 11)
#define SDIO_HOST_CTRL_TMOUT(x)			((x) << 11)
#define SDIO_HOST_CTRL_TMOUT_EN			(1 << 15)

/*
 * SDIO_SW_RESET
 */

#define SDIO_SW_RESET_NOW			(1 << 8)

/*
 * Normal interrupt status bits
 */

#define SDIO_NOR_ERROR				(1 << 15)
#define SDIO_NOR_UNEXP_RSP			(1 << 14)
#define SDIO_NOR_AUTOCMD12_DONE			(1 << 13)
#define SDIO_NOR_SUSPEND_ON			(1 << 12)
#define SDIO_NOR_LMB_FF_8W_AVAIL		(1 << 11)
#define SDIO_NOR_LMB_FF_8W_FILLED		(1 << 10)
#define SDIO_NOR_READ_WAIT_ON			(1 << 9)
#define SDIO_NOR_CARD_INT			(1 << 8)
#define SDIO_NOR_READ_READY			(1 << 5)
#define SDIO_NOR_WRITE_READY			(1 << 4)
#define SDIO_NOR_DMA_INI			(1 << 3)
#define SDIO_NOR_BLK_GAP_EVT			(1 << 2)
#define SDIO_NOR_XFER_DONE			(1 << 1)
#define SDIO_NOR_CMD_DONE			(1 << 0)

/*
 * Error status bits
 */

#define SDIO_ERR_CRC_STATUS			(1 << 14)
#define SDIO_ERR_CRC_STARTBIT			(1 << 13)
#define SDIO_ERR_CRC_ENDBIT			(1 << 12)
#define SDIO_ERR_RESP_TBIT			(1 << 11)
#define SDIO_ERR_XFER_SIZE			(1 << 10)
#define SDIO_ERR_CMD_STARTBIT			(1 << 9)
#define SDIO_ERR_AUTOCMD12			(1 << 8)
#define SDIO_ERR_DATA_ENDBIT			(1 << 6)
#define SDIO_ERR_DATA_CRC			(1 << 5)
#define SDIO_ERR_DATA_TIMEOUT			(1 << 4)
#define SDIO_ERR_CMD_INDEX			(1 << 3)
#define SDIO_ERR_CMD_ENDBIT			(1 << 2)
#define SDIO_ERR_CMD_CRC			(1 << 1)
#define SDIO_ERR_CMD_TIMEOUT			(1 << 0)
/* enable all for polling */
#define SDIO_POLL_MASK				0xffff

/*
 * CMD12 error status bits
 */

#define SDIO_AUTOCMD12_ERR_NOTEXE		(1 << 0)
#define SDIO_AUTOCMD12_ERR_TIMEOUT		(1 << 1)
#define SDIO_AUTOCMD12_ERR_CRC			(1 << 2)
#define SDIO_AUTOCMD12_ERR_ENDBIT		(1 << 3)
#define SDIO_AUTOCMD12_ERR_INDEX		(1 << 4)
#define SDIO_AUTOCMD12_ERR_RESP_T_BIT		(1 << 5)
#define SDIO_AUTOCMD12_ERR_RESP_STARTBIT	(1 << 6)

#define MMC_RSP_PRESENT				(1 << 0)
/* 136 bit response */
#define MMC_RSP_136				(1 << 1)
/* expect valid crc */
#define MMC_RSP_CRC				(1 << 2)
/* card may send busy */
#define MMC_RSP_BUSY				(1 << 3)
/* response contains opcode */
#define MMC_RSP_OPCODE				(1 << 4)

#define MMC_BUSMODE_OPENDRAIN			1
#define MMC_BUSMODE_PUSHPULL			2

#define MMC_BUS_WIDTH_1				0
#define MMC_BUS_WIDTH_4				2
#define MMC_BUS_WIDTH_8				3

/* Can the host do 4 bit transfers */
#define MMC_CAP_4_BIT_DATA			(1 << 0)
/* Can do MMC high-speed timing */
#define MMC_CAP_MMC_HIGHSPEED			(1 << 1)
/* Can do SD high-speed timing */
#define MMC_CAP_SD_HIGHSPEED			(1 << 2)
/* Can signal pending SDIO IRQs */
#define MMC_CAP_SDIO_IRQ			(1 << 3)
/* Talks only SPI protocols */
#define MMC_CAP_SPI				(1 << 4)
/* Can the host do 8 bit transfers */
#define MMC_CAP_8_BIT_DATA			(1 << 6)

/* Waits while card is busy */
#define MMC_CAP_WAIT_WHILE_BUSY			(1 << 9)
/* Allow erase/trim commands */
#define MMC_CAP_ERASE				(1 << 10)
/* can support DDR mode at 1.8V */
#define MMC_CAP_1_8V_DDR			(1 << 11)
/* can support DDR mode at 1.2V */
#define MMC_CAP_1_2V_DDR			(1 << 12)
/* Can power off after boot */
#define MMC_CAP_POWER_OFF_CARD			(1 << 13)
/* CMD14/CMD19 bus width ok */
#define MMC_CAP_BUS_WIDTH_TEST			(1 << 14)
/* Host supports UHS SDR12 mode */
#define MMC_CAP_UHS_SDR12			(1 << 15)
/* Host supports UHS SDR25 mode */
#define MMC_CAP_UHS_SDR25			(1 << 16)
/* Host supports UHS SDR50 mode */
#define MMC_CAP_UHS_SDR50			(1 << 17)
/* Host supports UHS SDR104 mode */
#define MMC_CAP_UHS_SDR104			(1 << 18)
/* Host supports UHS DDR50 mode */
#define MMC_CAP_UHS_DDR50			(1 << 19)
/* Host supports Driver Type A */
#define MMC_CAP_DRIVER_TYPE_A			(1 << 23)
/* Host supports Driver Type C */
#define MMC_CAP_DRIVER_TYPE_C			(1 << 24)
/* Host supports Driver Type D */
#define MMC_CAP_DRIVER_TYPE_D			(1 << 25)
/* CMD23 supported. */
#define MMC_CAP_CMD23				(1 << 30)
/* Hardware reset */
#define MMC_CAP_HW_RESET			(1 << 31)


#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		 5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR		20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY		26	/* Text file busy */
#define	EFBIG		27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */

#define	EDEADLK		35	/* Resource deadlock would occur */
#define	ENAMETOOLONG	36	/* File name too long */
#define	ENOLCK		37	/* No record locks available */

#define	ENOSYS		38	/* Invalid system call number */

#define	ENOTEMPTY	39	/* Directory not empty */
#define	ELOOP		40	/* Too many symbolic links encountered */
#define	EWOULDBLOCK	EAGAIN	/* Operation would block */
#define	ENOMSG		42	/* No message of desired type */
#define	EIDRM		43	/* Identifier removed */
#define	ECHRNG		44	/* Channel number out of range */
#define	EL2NSYNC	45	/* Level 2 not synchronized */
#define	EL3HLT		46	/* Level 3 halted */
#define	EL3RST		47	/* Level 3 reset */
#define	ELNRNG		48	/* Link number out of range */
#define	EUNATCH		49	/* Protocol driver not attached */
#define	ENOCSI		50	/* No CSI structure available */
#define	EL2HLT		51	/* Level 2 halted */
#define	EBADE		52	/* Invalid exchange */
#define	EBADR		53	/* Invalid request descriptor */
#define	EXFULL		54	/* Exchange full */
#define	ENOANO		55	/* No anode */
#define	EBADRQC		56	/* Invalid request code */
#define	EBADSLT		57	/* Invalid slot */

#define	EDEADLOCK	EDEADLK

#define	EBFONT		59	/* Bad font file format */
#define	ENOSTR		60	/* Device not a stream */
#define	ENODATA		61	/* No data available */
#define	ETIME		62	/* Timer expired */
#define	ENOSR		63	/* Out of streams resources */
#define	ENONET		64	/* Machine is not on the network */
#define	ENOPKG		65	/* Package not installed */
#define	EREMOTE		66	/* Object is remote */
#define	ENOLINK		67	/* Link has been severed */
#define	EADV		68	/* Advertise error */
#define	ESRMNT		69	/* Srmount error */
#define	ECOMM		70	/* Communication error on send */
#define	EPROTO		71	/* Protocol error */
#define	EMULTIHOP	72	/* Multihop attempted */
#define	EDOTDOT		73	/* RFS specific error */
#define	EBADMSG		74	/* Not a data message */
#define	EOVERFLOW	75	/* Value too large for defined data type */
#define	ENOTUNIQ	76	/* Name not unique on network */
#define	EBADFD		77	/* File descriptor in bad state */
#define	EREMCHG		78	/* Remote address changed */
#define	ELIBACC		79	/* Can not access a needed shared library */
#define	ELIBBAD		80	/* Accessing a corrupted shared library */
#define	ELIBSCN		81	/* .lib section in a.out corrupted */
#define	ELIBMAX		82	/* Attempting to link in too many shared libraries */
#define	ELIBEXEC	83	/* Cannot exec a shared library directly */
#define	EILSEQ		84	/* Illegal byte sequence */
#define	ERESTART	85	/* Interrupted system call should be restarted */
#define	ESTRPIPE	86	/* Streams pipe error */
#define	EUSERS		87	/* Too many users */
#define	ENOTSOCK	88	/* Socket operation on non-socket */
#define	EDESTADDRREQ	89	/* Destination address required */
#define	EMSGSIZE	90	/* Message too long */
#define	EPROTOTYPE	91	/* Protocol wrong type for socket */
#define	ENOPROTOOPT	92	/* Protocol not available */
#define	EPROTONOSUPPORT	93	/* Protocol not supported */
#define	ESOCKTNOSUPPORT	94	/* Socket type not supported */
#define	EOPNOTSUPP	95	/* Operation not supported on transport endpoint */
#define	EPFNOSUPPORT	96	/* Protocol family not supported */
#define	EAFNOSUPPORT	97	/* Address family not supported by protocol */
#define	EADDRINUSE	98	/* Address already in use */
#define	EADDRNOTAVAIL	99	/* Cannot assign requested address */
#define	ENETDOWN	100	/* Network is down */
#define	ENETUNREACH	101	/* Network is unreachable */
#define	ENETRESET	102	/* Network dropped connection because of reset */
#define	ECONNABORTED	103	/* Software caused connection abort */
#define	ECONNRESET	104	/* Connection reset by peer */
#define	ENOBUFS		105	/* No buffer space available */
#define	EISCONN		106	/* Transport endpoint is already connected */
#define	ENOTCONN	107	/* Transport endpoint is not connected */
#define	ESHUTDOWN	108	/* Cannot send after transport endpoint shutdown */
#define	ETOOMANYREFS	109	/* Too many references: cannot splice */
#define	ETIMEDOUT	110	/* Connection timed out */
#define	ECONNREFUSED	111	/* Connection refused */
#define	EHOSTDOWN	112	/* Host is down */
#define	EHOSTUNREACH	113	/* No route to host */
#define	EALREADY	114	/* Operation already in progress */
#define	EINPROGRESS	115	/* Operation now in progress */
#define	ESTALE		116	/* Stale file handle */
#define	EUCLEAN		117	/* Structure needs cleaning */
#define	ENOTNAM		118	/* Not a XENIX named type file */
#define	ENAVAIL		119	/* No XENIX semaphores available */
#define	EISNAM		120	/* Is a named type file */
#define	EREMOTEIO	121	/* Remote I/O error */
#define	EDQUOT		122	/* Quota exceeded */

#define	ENOMEDIUM	123	/* No medium found */
#define	EMEDIUMTYPE	124	/* Wrong medium type */
#define	ECANCELED	125	/* Operation Canceled */
#define	ENOKEY		126	/* Required key not available */
#define	EKEYEXPIRED	127	/* Key has expired */
#define	EKEYREVOKED	128	/* Key has been revoked */
#define	EKEYREJECTED	129	/* Key was rejected by service */

/* for robust mutexes */
#define	EOWNERDEAD	130	/* Owner died */
#define	ENOTRECOVERABLE	131	/* State not recoverable */

#define ERFKILL		132	/* Operation not possible due to RF-kill */

#define EHWPOISON	133	/* Memory page has hardware error */

#define ERESTARTSYS	512
#define ERESTARTNOINTR	513
#define ERESTARTNOHAND	514	/* restart if no handler.. */
#define ENOIOCTLCMD	515	/* No ioctl command */
#define ERESTART_RESTARTBLOCK 516 /* restart by calling sys_restart_syscall */
#define EPROBE_DEFER	517	/* Driver requests probe retry */
#define EOPENSTALE	518	/* open found a stale dentry */

/* Defined for the NFSv3 protocol */
#define EBADHANDLE	521	/* Illegal NFS file handle */
#define ENOTSYNC	522	/* Update synchronization mismatch */
#define EBADCOOKIE	523	/* Cookie is stale */
#define ENOTSUPP	524	/* Operation is not supported */
#define ETOOSMALL	525	/* Buffer or request is too small */
#define ESERVERFAULT	526	/* An untranslatable error occurred */
#define EBADTYPE	527	/* Type not supported by server */
#define EJUKEBOX	528	/* Request initiated, but will not complete before timeout */
#define EIOCBQUEUED	529	/* iocb queued, will get completion event */
#define ERECALLCONFLICT	530	/* conflict with recalled state */

#define mmc_host_is_spi(mmc)	(0)

struct mmc_cmd {
	uint16_t cmdidx;
	uint32_t resp_type;
	uint32_t cmdarg;
	uint32_t response[4];
};

struct mmc_data {
	union {
		char *dest;
		const char *src; /* src buffers don't get written to */
	};
	uint32_t flags;
	uint32_t blocks;
	uint32_t blocksize;
};
struct mmc_config {
	const char *name;
	uint32_t host_caps;
	uint32_t voltages;
	uint32_t f_min;
	uint32_t f_max;
	uint32_t b_max;
	unsigned char part_type;
};

struct bus_ops{
	int (*set_ios)(struct mmc *mmc);
	int (*send_command)(struct mmc_cmd *cmd, struct mmc_data *data)
};

struct mmc{
    struct mmc_config *cfg;

	int high_capacity;
    int op_cond_pending;

	bool clk_disable; /* true if the clock can be turned off */

    uint8_t part_config;

    uint16_t rca;

    uint32_t version;
    uint32_t ocr;
    uint32_t dsr;
    uint32_t has_init;
	uint32_t bus_width;
	uint32_t clock;
    uint32_t card_caps;
    uint32_t csd[4];
    uint32_t cid[4];
    uint32_t legacy_speed; 
    uint32_t dsr_imp;
    uint32_t read_bl_len;
    uint32_t write_bl_len;
    uint32_t erase_grp_size;

    uint64_t capacity;
	uint64_t capacity_user;
	uint64_t capacity_boot;
	uint64_t capacity_rpmb;
	uint64_t capacity_gp[4];
    
    enum bus_mode selected_mode; /* mode currently used */
	enum bus_mode best_mode; /* best mode is the supported mode with the
				  * highest bandwidth. It may not always be the
				  * operating mode due to limitations when
				  * accessing the boot partitions
				  */
	struct bus_ops *ops;
};

int mmc_init(void);
int mmc_read_blocks(void *dst, uint32_t start, uint32_t blkcnt);
uint32_t mmc_write_blocks(uint32_t start,uint32_t blkcnt, const void *src);
#endif /* __MVEBU_MMC_H__ */
