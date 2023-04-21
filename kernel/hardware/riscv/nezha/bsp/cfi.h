#include <stdint.h>

#ifndef __CFI_FLASH_H__
#define __CFI_FLASH_H__

#define FLASH_CMD_CFI			0x98
#define FLASH_CMD_READ_ID		0x90
#define FLASH_CMD_RESET			0xff
#define FLASH_CMD_BLOCK_ERASE		0x20
#define FLASH_CMD_ERASE_CONFIRM		0xD0
#define FLASH_CMD_WRITE			0x40
#define FLASH_CMD_PROTECT		0x60
#define FLASH_CMD_SETUP			0x60
#define FLASH_CMD_SET_CR_CONFIRM	0x03
#define FLASH_CMD_PROTECT_SET		0x01
#define FLASH_CMD_PROTECT_CLEAR		0xD0
#define FLASH_CMD_CLEAR_STATUS		0x50
#define FLASH_CMD_READ_STATUS		0x70
#define FLASH_CMD_WRITE_TO_BUFFER	0xE8
#define FLASH_CMD_WRITE_BUFFER_PROG	0xE9
#define FLASH_CMD_WRITE_BUFFER_CONFIRM	0xD0

#define FLASH_STATUS_DONE		0x80
#define FLASH_STATUS_ESS		0x40
#define FLASH_STATUS_ECLBS		0x20
#define FLASH_STATUS_PSLBS		0x10
#define FLASH_STATUS_VPENS		0x08
#define FLASH_STATUS_PSS		0x04
#define FLASH_STATUS_DPS		0x02
#define FLASH_STATUS_R			0x01
#define FLASH_STATUS_PROTECT		0x01

#define AMD_CMD_RESET			0xF0
#define AMD_CMD_WRITE			0xA0
#define AMD_CMD_ERASE_START		0x80
#define AMD_CMD_ERASE_SECTOR		0x30
#define AMD_CMD_UNLOCK_START		0xAA
#define AMD_CMD_UNLOCK_ACK		0x55
#define AMD_CMD_WRITE_TO_BUFFER		0x25
#define AMD_CMD_WRITE_BUFFER_CONFIRM	0x29
#define AMD_CMD_SET_PPB_ENTRY		0xC0
#define AMD_CMD_SET_PPB_EXIT_BC1	0x90
#define AMD_CMD_SET_PPB_EXIT_BC2	0x00
#define AMD_CMD_PPB_UNLOCK_BC1		0x80
#define AMD_CMD_PPB_UNLOCK_BC2		0x30
#define AMD_CMD_PPB_LOCK_BC1		0xA0
#define AMD_CMD_PPB_LOCK_BC2		0x00

#define AMD_STATUS_TOGGLE		0x40
#define AMD_STATUS_ERROR		0x20

#define ATM_CMD_UNLOCK_SECT		0x70
#define ATM_CMD_SOFTLOCK_START		0x80
#define ATM_CMD_LOCK_SECT		0x40

#define FLASH_CONTINUATION_CODE		0x7F

#define FLASH_OFFSET_MANUFACTURER_ID	0x00
#define FLASH_OFFSET_DEVICE_ID		0x01
#define FLASH_OFFSET_LOWER_SW_BITS	0x0C
#define FLASH_OFFSET_DEVICE_ID2		0x0E
#define FLASH_OFFSET_DEVICE_ID3		0x0F
#define FLASH_OFFSET_CFI		0x55
#define FLASH_OFFSET_CFI_ALT		0x555
#define FLASH_OFFSET_CFI_RESP		0x10
#define FLASH_OFFSET_PRIMARY_VENDOR	0x13
/* extended query table primary address */
#define FLASH_OFFSET_EXT_QUERY_T_P_ADDR	0x15
#define FLASH_OFFSET_WTOUT		0x1F
#define FLASH_OFFSET_WBTOUT		0x20
#define FLASH_OFFSET_ETOUT		0x21
#define FLASH_OFFSET_CETOUT		0x22
#define FLASH_OFFSET_WMAX_TOUT		0x23
#define FLASH_OFFSET_WBMAX_TOUT		0x24
#define FLASH_OFFSET_EMAX_TOUT		0x25
#define FLASH_OFFSET_CEMAX_TOUT		0x26
#define FLASH_OFFSET_SIZE		0x27
#define FLASH_OFFSET_INTERFACE		0x28
#define FLASH_OFFSET_BUFFER_SIZE	0x2A
#define FLASH_OFFSET_NUM_ERASE_REGIONS	0x2C
#define FLASH_OFFSET_ERASE_REGIONS	0x2D
#define FLASH_OFFSET_PROTECT		0x02
#define FLASH_OFFSET_USER_PROTECTION	0x85
#define FLASH_OFFSET_INTEL_PROTECTION	0x81

#define CFI_CMDSET_NONE			0
#define CFI_CMDSET_INTEL_EXTENDED	1
#define CFI_CMDSET_AMD_STANDARD		2
#define CFI_CMDSET_INTEL_STANDARD	3
#define CFI_CMDSET_AMD_EXTENDED		4
#define CFI_CMDSET_MITSU_STANDARD	256
#define CFI_CMDSET_MITSU_EXTENDED	257
#define CFI_CMDSET_SST			258
#define CFI_CMDSET_INTEL_PROG_REGIONS	512


#define FLASH_CFI_8BIT		0x01
#define FLASH_CFI_16BIT		0x02
#define FLASH_CFI_32BIT		0x04
#define FLASH_CFI_64BIT		0x08

#define FLASH_CFI_BY8		0x01
#define FLASH_CFI_BY16		0x02
#define FLASH_CFI_BY32		0x04
#define FLASH_CFI_BY64		0x08

#define FLASH_CFI_X8		0x00
#define FLASH_CFI_X16		0x01
#define FLASH_CFI_X8X16		0x02
#define FLASH_CFI_X16X32	0x05

#define CFI_FLASH_SHIFT_WIDTH	3

typedef struct{
    char* name;
    uint32_t base;
    uint8_t portwidth;
    uint8_t chipwidth;
    uint32_t start[512];
    uint8_t	 chip_lsb;
    uint16_t interface;
    uint16_t cfi_offset;
    uint16_t addr_unlock1;
    uint16_t addr_unlock2;
}flash_info_t;

typedef union {
	uint8_t w8;
	uint16_t w16;
	uint32_t w32;
	uint64_t w64;
} cfiword_t;


struct cfi_qry {
	uint8_t	qry[3];
	uint16_t	p_id;			/* unaligned */
	uint16_t	p_adr;			/* unaligned */
	uint16_t	a_id;			/* unaligned */
	uint16_t	a_adr;			/* unaligned */
	uint8_t	vcc_min;
	uint8_t	vcc_max;
	uint8_t	vpp_min;
	uint8_t	vpp_max;
	uint8_t	word_write_timeout_typ;
	uint8_t	buf_write_timeout_typ;
	uint8_t	block_erase_timeout_typ;
	uint8_t	chip_erase_timeout_typ;
	uint8_t	word_write_timeout_max;
	uint8_t	buf_write_timeout_max;
	uint8_t	block_erase_timeout_max;
	uint8_t	chip_erase_timeout_max;
	uint8_t	dev_size;
	uint16_t	interface_desc;		/* aligned */
	uint16_t	max_buf_write_size;	/* aligned */
	uint8_t	num_erase_regions;
	uint32_t	erase_region_info[4];	/* unaligned */
} __attribute__((packed));


typedef unsigned long flash_sect_t;
typedef unsigned int uint;
typedef unsigned char uchar;


int flash_detect_cfi(flash_info_t *info, struct cfi_qry *qry);

#endif