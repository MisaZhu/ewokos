#include <kernel/system.h>
#include <mm/mmu.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <kernel/proc.h>
#include <kernel/hw_info.h>
#include <dev/sd.h>

#define CONFIG_ARM_PL180_MMCI_CLOCK_FREQ 6250000
#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136 (1 << 1)    /* 136 bit response */
#define MMC_RSP_CRC (1 << 2)    /* expect valid crc */
#define MMC_RSP_BUSY  (1 << 3)    /* card may send busy */
#define MMC_RSP_OPCODE  (1 << 4)    /* response contains opcode */

#define MMC_RSP_NONE  (0)
#define MMC_RSP_R1  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE| \
		MMC_RSP_BUSY)
#define MMC_RSP_R2  (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3  (MMC_RSP_PRESENT)
#define MMC_RSP_R4  (MMC_RSP_PRESENT)
#define MMC_RSP_R5  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

/* SDI Status register bits */
#define SDI_STA_CCRCFAIL  0x00000001
#define SDI_STA_DCRCFAIL  0x00000002
#define SDI_STA_CTIMEOUT  0x00000004
#define SDI_STA_DTIMEOUT  0x00000008
#define SDI_STA_TXUNDERR  0x00000010
#define SDI_STA_RXOVERR   0x00000020
#define SDI_STA_CMDREND   0x00000040
#define SDI_STA_CMDSENT   0x00000080
#define SDI_STA_DATAEND   0x00000100
#define SDI_STA_STBITERR  0x00000200
#define SDI_STA_DBCKEND   0x00000400
#define SDI_STA_CMDACT    0x00000800
#define SDI_STA_TXACT   0x00001000
#define SDI_STA_RXACT   0x00002000
#define SDI_STA_TXFIFOBW  0x00004000
#define SDI_STA_RXFIFOBR  0x00008000
#define SDI_STA_TXFIFOF   0x00010000
#define SDI_STA_RXFIFOF   0x00020000
#define SDI_STA_TXFIFOE   0x00040000
#define SDI_STA_RXFIFOE   0x00080000
#define SDI_STA_TXDAVL    0x00100000
#define SDI_STA_RXDAVL    0x00200000
#define SDI_STA_SDIOIT    0x00400000
#define SDI_STA_CEATAEND  0x00800000
#define SDI_STA_CARDBUSY  0x01000000
#define SDI_STA_BOOTMODE  0x02000000
#define SDI_STA_BOOTACKERR  0x04000000
#define SDI_STA_BOOTACKTIMEOUT  0x08000000
#define SDI_STA_RSTNEND   0x10000000

/******* PL180 Register offsets from BASE *******/
#define POWER         0x00
#define CLOCK         0x04
#define ARGUMENT      0x08
#define COMMAND       0x0c
#define RESPCOMMAND   0x10
#define RESPONSE0     0x14
#define RESPONSE1     0x18
#define RESPONSE2     0x1c
#define RESPONSE3     0x20
#define DATATIMER     0x24
#define DATALENGTH    0x28
#define DATACTRL      0x2c
#define DATACOUNT     0x30
#define STATUS        0x34
#define STATUS_CLEAR  0x38
#define MASK0         0x3c
#define MASK1         0x40
#define CARD_SELECT   0x44
#define FIFO_COUNT    0x48
#define FIFO          0x80

#define SD_RCA  0x45670000 // QEMU's hard-coded RCA
#define SD_BASE (_sys_info.mmio.v_base + 0x5000) // PL180 SD_BASE address

#define SECTOR_SIZE   512

// shared variables between SDC driver and interrupt handler
typedef struct {
	char rxbuf[SECTOR_SIZE];
	char txbuf[SECTOR_SIZE];
	char *rxbuf_index;
	const char *txbuf_index;
	uint32_t rxcount, txcount, rxdone, txdone;
} sd_t;

static sd_t _sdc;

static inline void do_command(int32_t cmd, int32_t arg, int32_t resp) {
	put32(SD_BASE + ARGUMENT, arg);
	put32(SD_BASE + COMMAND, 0x400 | (resp<<6) | cmd);
}

int32_t sd_init(void) {
	sd_t* sdc = &_sdc;
	memset(sdc, 0, sizeof(sd_t));
	sdc->rxdone = 1;
	sdc->txdone = 1;

	put32(SD_BASE + POWER, 0xBF); // power on
	put32(SD_BASE + CLOCK, 0xC6); // default CLK

	// send init command sequence
	do_command(0, 0, MMC_RSP_NONE);// idle state
	do_command(55, 0, MMC_RSP_R1);  // ready state  
	do_command(41, 0x10000, MMC_RSP_R3);  // argument must not be zero
	do_command(2, 0, MMC_RSP_R2);  // ask card CID
	do_command(3, SD_RCA, MMC_RSP_R1);  // assign RCA
	do_command(7, SD_RCA, MMC_RSP_R1);  // transfer state: must use RCA
	do_command(16, 512, MMC_RSP_R1);  // set data sector length

	// set interrupt MASK0 registers bits = RxFULL(17)|TxEmpty(18)
	put32(SD_BASE + MASK0, (1<<17)|(1<<18));
	return 0;
}

static int32_t sd_read_sector(int32_t sector) {
	uint32_t cmd, arg;
	sd_t* sdc = (sd_t*)&_sdc;
	if(sdc->rxdone == 0) {
		return -1;
	}

	//printf("getsector %d ", sector);
	sdc->rxbuf_index = sdc->rxbuf; 
	sdc->rxcount = SECTOR_SIZE;
	sdc->rxdone = 0;

	put32(SD_BASE + DATATIMER, 0xFFFF0000);
	// write data_len to datalength reg
	put32(SD_BASE + DATALENGTH, SECTOR_SIZE);

	cmd = 18; // CMD17 = READ single sector; 18=read sector
	arg = (uint32_t)(sector*SECTOR_SIZE);  // absolute byte offset in diks
	do_command(cmd, arg, MMC_RSP_R1);

	//printf("dataControl=%x\n", 0x93);
	// 0x93=|9|0011|=|9|DMA=0,0=BLOCK,1=Host<-Card,1=Enable
	put32(SD_BASE + DATACTRL, 0x93);
	//printf("getsector return\n");
	return 0;
}

static inline int32_t sd_read_done(void* buf) {
	sd_t* sdc = (sd_t*)&_sdc;
	if(sdc->rxdone == 0) {
		return -1;
	}
	memcpy(buf, (void*)sdc->rxbuf, SECTOR_SIZE);
	return 0;
}

static int32_t sd_write_sector(int32_t sector, const void* buf) {
	sd_t* sdc = (sd_t*)&_sdc;
	uint32_t cmd, arg;
	if(sdc->txdone == 0) {
		return -1;
	}
	memcpy(sdc->txbuf, buf, SECTOR_SIZE);
	sdc->txbuf_index = sdc->txbuf; sdc->txcount = SECTOR_SIZE;
	sdc->txdone = 0;

	put32(SD_BASE + DATATIMER, 0xFFFF0000);
	put32(SD_BASE + DATALENGTH, SECTOR_SIZE);

	cmd = 25;                  // CMD24 = Write single sector; 25=write sector
	arg = (uint32_t)(sector*SECTOR_SIZE);  // absolute byte offset in diks
	do_command(cmd, arg, MMC_RSP_R1);

	// write 0x91=|9|0001|=|9|DMA=0,BLOCK=0,0=Host->Card, Enable
	put32(SD_BASE + DATACTRL, 0x91); // Host->card
	return 0;
}

static inline int32_t sd_write_done(void) {
	sd_t* sdc = (sd_t*)&_sdc;
	if(sdc->txdone == 0) {
		return -1;
	}
	return 0;
}

void sd_dev_handle(void) {
	sd_t* sdc = (sd_t*)&_sdc;
	int32_t status, status_err;
	int32_t i; 
	uint32_t *up;

	// read status register to find out TXempty or RxAvail
	status = get32(SD_BASE + STATUS);

	if (status & (1<<17)){ // RxFull: read 16 uint32_t at a time;
		//printf("SDC RX interrupt: ");
		up = (uint32_t *)sdc->rxbuf_index;
		status_err = status & (SDI_STA_DCRCFAIL|SDI_STA_DTIMEOUT|SDI_STA_RXOVERR);
		if (!status_err && sdc->rxcount) {
			//printf("R%d ", sdc->rxcount);
			for (i = 0; i < 16; i++)
				*(up + i) = get32(SD_BASE + FIFO);
			up += 16;
			sdc->rxcount -= 64;
			sdc->rxbuf_index += 64;
			status = get32(SD_BASE + STATUS); // read status to clear Rx interrupt
		}
		if (sdc->rxcount == 0){
			do_command(12, 0, MMC_RSP_R1); // stop transmission
			sdc->rxdone = 1;
			//printf("SDC handler done ");
		}
	}
	else if (status & (1<<18)){ // TXempty: write 16 uint32_t at a time
		//printf("TX interrupt: ");
		up = (uint32_t *)sdc->txbuf_index;
		status_err = status & (SDI_STA_DCRCFAIL | SDI_STA_DTIMEOUT);

		if (!status_err && sdc->txcount) {
			// printf("W%d ", sdc->txcount);
			for (i = 0; i < 16; i++)
				put32(SD_BASE + FIFO, *(up + i));
			up += 16;
			sdc->txcount -= 64;
			sdc->txbuf_index += 64;            // advance sdc->txbuf_index for next write  
			status = get32(SD_BASE + STATUS); // read status to clear Tx interrupt
		}
		if (sdc->txcount == 0){
			do_command(12, 0, MMC_RSP_R1); // stop transmission
			sdc->txdone = 1;
		}
	}
	//printf("write to clear register\n");
	put32(SD_BASE + STATUS_CLEAR, 0xFFFFFFFF);
	// printf("SDC interrupt handler done\n");
}

int32_t sd_dev_read(int32_t sector) {
	return sd_read_sector(sector);
}

int32_t sd_dev_read_done(void* buf) {
	sd_dev_handle();
	return sd_read_done(buf);
}

int32_t sd_dev_write(int32_t sector, const void* buf) {
	return sd_write_sector(sector, buf);
}

int32_t sd_dev_write_done(void) {
	sd_dev_handle();
	return sd_write_done();
}
