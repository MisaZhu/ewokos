#include <dev/sdc.h>
#include <system.h>
#include <mm/mmu.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <scheduler.h>

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

#define SDC_RCA  0x45670000 // QEMU's hard-coded RCA
#define SDC_BASE (MMIO_BASE + 0x5000) // PL180 SDC_BASE address

// shared variables between SDC driver and interrupt handler
static int32_t _p_lock = 0;
static char *_rxbuf_index;
static char *_rxbuf;
static const char *_txbuf_index;
static uint32_t _rxcount, _txcount, _rxdone, _txdone;

static inline void do_command(int32_t cmd, int32_t arg, int32_t resp) {
	*(uint32_t *)(SDC_BASE + ARGUMENT) = (uint32_t)arg;
	*(uint32_t *)(SDC_BASE + COMMAND)  = 0x400 | (resp<<6) | cmd;
}

int32_t sdc_init() {
	_rxdone = 1;
	_txdone = 1;
	_rxbuf = km_alloc(SDC_BLOCK_SIZE);

	*(uint32_t *)(SDC_BASE + POWER) = (uint32_t)0xBF; // power on
	*(uint32_t *)(SDC_BASE + CLOCK) = (uint32_t)0xC6; // default CLK

	// send init command sequence
	do_command(0, 0, MMC_RSP_NONE);// idle state
	do_command(55, 0, MMC_RSP_R1);  // ready state  
	do_command(41, 1, MMC_RSP_R3);  // argument must not be zero
	do_command(2, 0, MMC_RSP_R2);  // ask card CID
	do_command(3, SDC_RCA, MMC_RSP_R1);  // assign RCA
	do_command(7, SDC_RCA, MMC_RSP_R1);  // transfer state: must use RCA
	do_command(16, 512, MMC_RSP_R1);  // set data block length

	// set interrupt MASK0 registers bits = RxFULL(17)|TxEmpty(18)
	*(uint32_t *)(SDC_BASE + MASK0) = (1<<17)|(1<<18);
	return 0;
}

static int32_t sdc_read_block(int32_t block) {
	uint32_t cmd, arg;
	CRIT_IN(_p_lock)
	if(_rxdone == 0) {
		CRIT_OUT(_p_lock)
		return -1;
	}

	//printk("getblock %d ", block);
	_rxbuf_index = _rxbuf; _rxcount = SDC_BLOCK_SIZE;
	_rxdone = 0;

	*(uint32_t *)(SDC_BASE + DATATIMER) = 0xFFFF0000;
	// write data_len to datalength reg
	*(uint32_t *)(SDC_BASE + DATALENGTH) = SDC_BLOCK_SIZE;

	cmd = 18; // CMD17 = READ single sector; 18=read block
	arg = ((block*2)*512); // absolute byte offset in disk
	do_command(cmd, arg, MMC_RSP_R1);

	//printk("dataControl=%x\n", 0x93);
	// 0x93=|9|0011|=|9|DMA=0,0=BLOCK,1=Host<-Card,1=Enable
	*(uint32_t *)(SDC_BASE + DATACTRL) = 0x93;
	//printk("getblock return\n");
	CRIT_OUT(_p_lock)
	return 0;
}

static inline int32_t sdc_read_done(char* buf) {
	CRIT_IN(_p_lock)
	if(_rxdone == 0) {
		CRIT_OUT(_p_lock)
		return -1;
	}

	memcpy(buf, _rxbuf, SDC_BLOCK_SIZE);
	CRIT_OUT(_p_lock)
	return 0;
}

static int32_t sdc_write_block(int32_t block, const char* buf) {
	uint32_t cmd, arg;
	CRIT_IN(_p_lock)
	if(_txdone == 0) {
		CRIT_OUT(_p_lock)
			return -1;
	}

	//printk("putblock %d %x\n", block, buf);
	_txbuf_index = buf; _txcount = SDC_BLOCK_SIZE;
	_txdone = 0;

	*(uint32_t *)(SDC_BASE + DATATIMER) = 0xFFFF0000;
	*(uint32_t *)(SDC_BASE + DATALENGTH) = SDC_BLOCK_SIZE;

	cmd = 25;                  // CMD24 = Write single sector; 25=read block
	arg = (uint32_t)((block*2)*512);  // absolute byte offset in diks
	do_command(cmd, arg, MMC_RSP_R1);

	//printk("dataControl=%x\n", 0x91);
	// write 0x91=|9|0001|=|9|DMA=0,BLOCK=0,0=Host->Card, Enable
	*(uint32_t *)(SDC_BASE + DATACTRL) = 0x91; // Host->card

	CRIT_OUT(_p_lock)
	return 0;
}

static inline int32_t sdc_write_done() {
	return _txdone;
}

void sdc_handle() {
	int32_t status, status_err;
	int32_t i; 
	uint32_t *up;

	// read status register to find out TXempty or RxAvail
	status = *(uint32_t *)(SDC_BASE + STATUS);

	if (status & (1<<17)){ // RxFull: read 16 uint32_t at a time;
		//printk("SDC RX interrupt: ");
		up = (uint32_t *)_rxbuf_index;
		status_err = status & (SDI_STA_DCRCFAIL|SDI_STA_DTIMEOUT|SDI_STA_RXOVERR);
		if (!status_err && _rxcount) {
			//printk("R%d ", _rxcount);
			for (i = 0; i < 16; i++)
				*(up + i) = *(uint32_t *)(SDC_BASE + FIFO);
			up += 16;
			_rxcount -= 64;
			_rxbuf_index += 64;
			status = *(uint32_t *)(SDC_BASE + STATUS); // read status to clear Rx interrupt
		}
		if (_rxcount == 0){
			do_command(12, 0, MMC_RSP_R1); // stop transmission
			_rxdone = 1;
			//printk("SDC handler done ");
		}
	}
	else if (status & (1<<18)){ // TXempty: write 16 uint32_t at a time
		//printk("TX interrupt: ");
		up = (uint32_t *)_txbuf_index;
		status_err = status & (SDI_STA_DCRCFAIL | SDI_STA_DTIMEOUT);

		if (!status_err && _txcount) {
			// printk("W%d ", _txcount);
			for (i = 0; i < 16; i++)
				*(uint32_t *)(SDC_BASE + FIFO) = *(up + i);
			up += 16;
			_txcount -= 64;
			_txbuf_index += 64;            // advance _txbuf_index for next write  
			status = *(uint32_t *)(SDC_BASE + STATUS); // read status to clear Tx interrupt
		}
		if (_txcount == 0){
			do_command(12, 0, MMC_RSP_R1); // stop transmission
			_txdone = 1;
		}
	}
	//printk("write to clear register\n");
	*(uint32_t *)(SDC_BASE + STATUS_CLEAR) = 0xFFFFFFFF;
	// printk("SDC interrupt handler done\n");
}

int32_t dev_sdc_read(int16_t id, uint32_t block) {
	(void)id;
	return sdc_read_block(block);
}

int32_t dev_sdc_read_done(int16_t id, void* buf) {
	(void)id;
	return sdc_read_done((char*)buf);
}

int32_t dev_sdc_write(int16_t id, uint32_t block, void* buf) {
	(void)id;
	return sdc_write_block(block, (const char*)buf);
}

int32_t dev_sdc_write_done(int16_t id) {
	(void)id;
	return -1;
}
