#ifndef __SDMMC_H__
#define __SDMMC_H__

#include <stdint.h>

#define SDMMC_SECTOR_SIZE		((uint16_t) 512)
#define SDMMC_DEFAULT_TIMEOUT    (1000)

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


struct mmc_dev
{
    void *priv;
	int (*send_command)(void *priv, struct mmc_cmd *cmd, struct mmc_data *data);
};

/***********************************************************/
#define SDMMC_CMD0		0x40	// software reset
#define SDMMC_CMD1		0x41	// initiate initialisation process
#define SDMMC_CMD8		0x48	// check voltage range (SDC v2 only)
#define SDMMC_CMD9		0x49	// read CSD register
#define SDMMC_CMD12		0x4C	// force stop transmission
#define SDMMC_CMD16		0x50	// set read/write block size
#define SDMMC_CMD17		0x51	// read a block
#define SDMMC_CMD18		0x52	// read multiple blocks
#define SDMMC_CMD24		0x58	// write a block
#define SDMMC_CMD25		0x59	// write multiple blocks
#define SDMMC_CMD41		0x69
#define SDMMC_CMD55		0x77	// leading command for ACMD commands
#define SDMMC_CMD58		0x7A	// read OCR
#define SDMMC_ACMD41	0xE9	// ACMDx = CMDx + 0x80
// (note: all commands are ORed with 64 [e.g. CMD16 = 16+64 = 0x50])
/***********************************************************/
/* SDMMC Card Type Definitions */
#define SDMMC_TYPE_MMC		0x01
#define SDMMC_TYPE_SD1		0x02								// SD v1
#define SDMMC_TYPE_SD2		0x04								// SD v2
#define SDMMC_TYPE_SDC		(SDMMC_TYPE_SD1|SDMMC_TYPE_SD2)		// SD
#define SDMMC_TYPE_BLOCK	0x08								// block addressing
/***********************************************************/
/* Error Codes */
#define SDMMC_ERR_NONE								+0
#define SDMMC_ERR_INIT								-1
#define SDMMC_ERR_READ_BLOCK_DATA_TOKEN_MISSING		-5
#define SDMMC_ERR_BAD_REPLY							-6
#define SDMMC_ERR_ACCESS_DENIED						-7
#define SDMMC_ERR_IDLE_STATE_TIMEOUT				-10
#define SDMMC_ERR_OP_COND_TIMEOUT					-11
#define SDMMC_ERR_SET_BLOCKLEN_TIMEOUT				-12
#define SDMMC_ERR_READ_BLOCK_TIMEOUT				-13
#define SDMMC_ERR_APP_CMD_TIMEOUT					-14
#define SDMMC_ERR_APP_SEND_IF_COND_TIMEOUT			-15

#define SDMMC_WAIT_MS								1
#define SDMMC_CMD8_ACCEPTED							2
#define SDMMC_CMD8_REJECTED							3
#define SDMMC_MAX_TIMEOUT							1024		// TODO arbitrary value; might need revising


#define MMC_CMD_GO_IDLE_STATE       0
#define MMC_CMD_SEND_OP_COND        1
#define MMC_CMD_ALL_SEND_CID        2
#define MMC_CMD_SET_RELATIVE_ADDR   3
#define MMC_CMD_SET_DSR         4
#define MMC_CMD_SWITCH          6
#define MMC_CMD_SELECT_CARD     7
#define MMC_CMD_SEND_EXT_CSD        8
#define MMC_CMD_SEND_CSD        9
#define MMC_CMD_SEND_CID        10
#define MMC_CMD_STOP_TRANSMISSION   12
#define MMC_CMD_SEND_STATUS     13
#define MMC_CMD_SET_BLOCKLEN        16
#define MMC_CMD_READ_SINGLE_BLOCK   17
#define MMC_CMD_READ_MULTIPLE_BLOCK 18
#define MMC_CMD_SEND_TUNING_BLOCK   19
#define MMC_CMD_SEND_TUNING_BLOCK_HS200 21
#define MMC_CMD_SET_BLOCK_COUNT         23
#define MMC_CMD_WRITE_SINGLE_BLOCK  24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK    25
#define MMC_CMD_SET_WRITE_PROT          28
#define MMC_CMD_CLR_WRITE_PROT          29
#define MMC_CMD_SEND_WRITE_PROT         30
#define MMC_CMD_SEND_WRITE_PROT_TYPE    31
#define MMC_CMD_ERASE_GROUP_START   35
#define MMC_CMD_ERASE_GROUP_END     36
#define MMC_CMD_ERASE           38
#define MMC_CMD_APP_CMD         55
#define MMC_CMD_SPI_READ_OCR        58
#define MMC_CMD_SPI_CRC_ON_OFF      59
#define MMC_CMD_RES_MAN         62

#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136 (1 << 1)        /* 136 bit response */
#define MMC_RSP_CRC (1 << 2)        /* expect valid crc */
#define MMC_RSP_BUSY    (1 << 3)        /* card may send busy */
#define MMC_RSP_OPCODE  (1 << 4)        /* response contains opcode */

#define MMC_RSP_NONE    (0)
#define MMC_RSP_R1  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE| \
            MMC_RSP_BUSY)
#define MMC_RSP_R2  (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3  (MMC_RSP_PRESENT)
#define MMC_RSP_R4  (MMC_RSP_PRESENT)
#define MMC_RSP_R5  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

/*
 * EXT_CSD fields
 */
#define EXT_CSD_ENH_START_ADDR      136 /* R/W */
#define EXT_CSD_ENH_SIZE_MULT       140 /* R/W */
#define EXT_CSD_GP_SIZE_MULT        143 /* R/W */
#define EXT_CSD_PARTITION_SETTING   155 /* R/W */
#define EXT_CSD_PARTITIONS_ATTRIBUTE    156 /* R/W */
#define EXT_CSD_MAX_ENH_SIZE_MULT   157 /* R */
#define EXT_CSD_PARTITIONING_SUPPORT    160 /* RO */
#define EXT_CSD_RST_N_FUNCTION      162 /* R/W */
#define EXT_CSD_BKOPS_EN        163 /* R/W & R/W/E */
#define EXT_CSD_SANITIZE_START          165     /* W */
#define EXT_CSD_WR_REL_PARAM        166 /* R */
#define EXT_CSD_WR_REL_SET      167 /* R/W */
#define EXT_CSD_RPMB_MULT       168 /* RO */
#define EXT_CSD_USER_WP         171 /* R/W */
#define EXT_CSD_ERASE_GROUP_DEF     175 /* R/W */
#define EXT_CSD_BOOT_BUS_WIDTH      177
#define EXT_CSD_PART_CONF       179 /* R/W */
#define EXT_CSD_ERASED_MEM_CONT         181     /* RO */
#define EXT_CSD_BUS_WIDTH       183 /* R/W */
#define EXT_CSD_HS_TIMING       185 /* R/W */
#define EXT_CSD_REV         192 /* RO */
#define EXT_CSD_STRUCTURE       194     /* RO */
#define EXT_CSD_CARD_TYPE       196 /* RO */
#define EXT_CSD_SEC_CNT         212 /* RO, 4 bytes */
#define EXT_CSD_HC_WP_GRP_SIZE      221 /* RO */
#define EXT_CSD_ERASE_TIMEOUT_MULT      223     /* RO */
#define EXT_CSD_HC_ERASE_GRP_SIZE   224 /* RO */
#define EXT_CSD_BOOT_MULT       226 /* RO */
#define EXT_CSD_SEC_TRIM_MULT           229     /* RO */
#define EXT_CSD_SEC_ERASE_MULT          230     /* RO */
#define EXT_CSD_SEC_FEATURE_SUPPORT     231     /* RO */
#define EXT_CSD_TRIM_MULT               232    /* RO */
#define EXT_CSD_POWER_OFF_LONG_TIME     247     /* RO */
#define EXT_CSD_GENERIC_CMD6_TIME       248     /* RO */
#define EXT_CSD_BKOPS_SUPPORT       502 /* RO */


#define MMC_DATA_READ       1
#define MMC_DATA_WRITE      2

#endif