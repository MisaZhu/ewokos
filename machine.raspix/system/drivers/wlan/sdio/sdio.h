#ifndef __SDIO_H__
#define __SDIO_H__

#include <types.h>



/* SDIO commands                         type  argument     response */
#define SD_IO_SEND_OP_COND          5 /* bcr  [23:0] OCR         R4  */
#define SD_IO_RW_DIRECT            52 /* ac   [31:0] See below   R5  */
#define SD_IO_RW_EXTENDED          53 /* adtc [31:0] See below   R5  */

#define R4_18V_PRESENT (1<<24)
#define R4_MEMORY_PRESENT (1 << 27)

#define R5_COM_CRC_ERROR	(1 << 15)	/* er, b */
#define R5_ILLEGAL_COMMAND	(1 << 14)	/* er, b */
#define R5_ERROR		(1 << 11)	/* erx, c */
#define R5_FUNCTION_NUMBER	(1 << 9)	/* er, c */
#define R5_OUT_OF_RANGE		(1 << 8)	/* er, c */
#define R5_STATUS(x)		(x & 0xCB00)
#define R5_IO_CURRENT_STATE(x)	((x & 0x3000) >> 12) /* s, b */

#define SDIO_CCCR_CCCR		0x00

#define  SDIO_CCCR_REV_1_00	0	/* CCCR/FBR Version 1.00 */
#define  SDIO_CCCR_REV_1_10	1	/* CCCR/FBR Version 1.10 */
#define  SDIO_CCCR_REV_1_20	2	/* CCCR/FBR Version 1.20 */
#define  SDIO_CCCR_REV_3_00	3	/* CCCR/FBR Version 3.00 */

#define  SDIO_SDIO_REV_1_00	0	/* SDIO Spec Version 1.00 */
#define  SDIO_SDIO_REV_1_10	1	/* SDIO Spec Version 1.10 */
#define  SDIO_SDIO_REV_1_20	2	/* SDIO Spec Version 1.20 */
#define  SDIO_SDIO_REV_2_00	3	/* SDIO Spec Version 2.00 */
#define  SDIO_SDIO_REV_3_00	4	/* SDIO Spec Version 3.00 */

#define SDIO_CCCR_SD		0x01

#define  SDIO_SD_REV_1_01	0	/* SD Physical Spec Version 1.01 */
#define  SDIO_SD_REV_1_10	1	/* SD Physical Spec Version 1.10 */
#define  SDIO_SD_REV_2_00	2	/* SD Physical Spec Version 2.00 */
#define  SDIO_SD_REV_3_00	3	/* SD Physical Spec Version 3.00 */

#define SDIO_CCCR_IOEx		0x02
#define SDIO_CCCR_IORx		0x03

#define SDIO_CCCR_IENx		0x04	/* Function/Master Interrupt Enable */
#define SDIO_CCCR_INTx		0x05	/* Function Interrupt Pending */

#define SDIO_CCCR_ABORT		0x06	/* function abort/card reset */

#define SDIO_CCCR_IF		0x07	/* bus interface controls */

#define  SDIO_BUS_WIDTH_MASK	0x03	/* data bus width setting */
#define  SDIO_BUS_WIDTH_1BIT	0x00
#define  SDIO_BUS_WIDTH_RESERVED 0x01
#define  SDIO_BUS_WIDTH_4BIT	0x02
#define  SDIO_BUS_ECSI		0x20	/* Enable continuous SPI interrupt */
#define  SDIO_BUS_SCSI		0x40	/* Support continuous SPI interrupt */

#define  SDIO_BUS_ASYNC_INT	0x20

#define  SDIO_BUS_CD_DISABLE     0x80	/* disable pull-up on DAT3 (pin 1) */

#define SDIO_CCCR_CAPS		0x08

#define  SDIO_CCCR_CAP_SDC	0x01	/* can do CMD52 while data transfer */
#define  SDIO_CCCR_CAP_SMB	0x02	/* can do multi-block xfers (CMD53) */
#define  SDIO_CCCR_CAP_SRW	0x04	/* supports read-wait protocol */
#define  SDIO_CCCR_CAP_SBS	0x08	/* supports suspend/resume */
#define  SDIO_CCCR_CAP_S4MI	0x10	/* interrupt during 4-bit CMD53 */
#define  SDIO_CCCR_CAP_E4MI	0x20	/* enable ints during 4-bit CMD53 */
#define  SDIO_CCCR_CAP_LSC	0x40	/* low speed card */
#define  SDIO_CCCR_CAP_4BLS	0x80	/* 4 bit low speed card */

#define SDIO_CCCR_CIS		0x09	/* common CIS pointer (3 bytes) */

#define SDIO_CCCR_SUSPEND	0x0c
#define SDIO_CCCR_SELx		0x0d
#define SDIO_CCCR_EXECx		0x0e
#define SDIO_CCCR_READYx	0x0f

#define SDIO_CCCR_BLKSIZE	0x10

#define SDIO_CCCR_POWER		0x12

#define  SDIO_POWER_SMPC	0x01	/* Supports Master Power Control */
#define  SDIO_POWER_EMPC	0x02	/* Enable Master Power Control */

#define SDIO_CCCR_SPEED		0x13

#define  SDIO_SPEED_SHS		0x01	/* Supports High-Speed mode */
#define  SDIO_SPEED_BSS_SHIFT	1
#define  SDIO_SPEED_BSS_MASK	(7<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_SDR12	(0<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_SDR25	(1<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_SDR50	(2<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_SDR104	(3<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_DDR50	(4<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_EHS		SDIO_SPEED_SDR25	/* Enable High-Speed */

#define SDIO_CCCR_UHS		0x14
#define  SDIO_UHS_SDR50		0x01
#define  SDIO_UHS_SDR104	0x02
#define  SDIO_UHS_DDR50		0x04

#define SDIO_CCCR_DRIVE_STRENGTH 0x15
#define  SDIO_SDTx_MASK		0x07
#define  SDIO_DRIVE_SDTA	(1<<0)
#define  SDIO_DRIVE_SDTC	(1<<1)
#define  SDIO_DRIVE_SDTD	(1<<2)
#define  SDIO_DRIVE_DTSx_MASK	0x03
#define  SDIO_DRIVE_DTSx_SHIFT	4
#define  SDIO_DTSx_SET_TYPE_B	(0 << SDIO_DRIVE_DTSx_SHIFT)
#define  SDIO_DTSx_SET_TYPE_A	(1 << SDIO_DRIVE_DTSx_SHIFT)
#define  SDIO_DTSx_SET_TYPE_C	(2 << SDIO_DRIVE_DTSx_SHIFT)
#define  SDIO_DTSx_SET_TYPE_D	(3 << SDIO_DRIVE_DTSx_SHIFT)

#define SDIO_CCCR_INTERRUPT_EXT	0x16
#define SDIO_INTERRUPT_EXT_SAI	(1 << 0)
#define SDIO_INTERRUPT_EXT_EAI	(1 << 1)
#define SDIO_FBR_BASE(f)	((f) * 0x100) /* base of function f's FBRs */
#define SDIO_FBR_STD_IF		0x00
#define  SDIO_FBR_SUPPORTS_CSA	0x40	/* supports Code Storage Area */
#define  SDIO_FBR_ENABLE_CSA	0x80	/* enable Code Storage Area */
#define SDIO_FBR_STD_IF_EXT	0x01
#define SDIO_FBR_POWER		0x02
#define  SDIO_FBR_POWER_SPS	0x01	/* Supports Power Selection */
#define  SDIO_FBR_POWER_EPS	0x02	/* Enable (low) Power Selection */
#define SDIO_FBR_CIS		0x09	/* CIS pointer (3 bytes) */
#define SDIO_FBR_CSA		0x0C	/* CSA pointer (3 bytes) */
#define SDIO_FBR_CSA_DATA	0x0F
#define SDIO_FBR_BLKSIZE	0x10	/* block size (2 bytes) */

int sdio_memcpy_fromio(int func, void *dst, unsigned int addr, int count);
int sdio_memcpy_toio(int func, unsigned int addr, void *src, int count);
uint8_t sdio_readb(int func, unsigned int addr, int *err_ret);
void sdio_writeb(int func, uint8_t b, unsigned int addr, int *err_ret);
int sdio_readsb(int func, void *dst, unsigned int addr,int count);
int sdio_writesb(int func, unsigned int addr, void *src,int count);
uint16_t sdio_readw(int func, unsigned int addr, int *err_ret);
void sdio_writew(int func, uint16_t b, unsigned int addr, int *err_ret);
uint32_t sdio_readl(int func, unsigned int addr, int *err_ret);
void sdio_writel(int func, uint32_t b, unsigned int addr, int *err_ret);

int sdio_reset(void);
int sdio_set_block_size(unsigned int func, unsigned int blksz);
int sdio_enable_func(int func);
int sdio_disable_func(int func);
int sdio_claim_irq(int func);

#endif
