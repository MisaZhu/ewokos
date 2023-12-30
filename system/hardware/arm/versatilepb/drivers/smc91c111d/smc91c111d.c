#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/mmio.h>

#include "smc91c111d.h"

#define SMC_BASE  (_mmio_base+0x10000)
#define MII_DELAY		1

void udelay(volatile int x){
    x*=50;
    while(x--){
        __asm("nop");
    }
}

uint32_t phy_type;
uint32_t phy_id;
uint32_t rx_errors;
uint32_t rx_frame_errors;
uint32_t rx_length_errors;
uint32_t rx_crc_errors;
uint32_t rx_packets;
uint32_t rx_bytes;
uint32_t tx_errors;
uint32_t tx_fifo_errors;
uint32_t tx_packets;
uint32_t tx_bytes;

uint32_t multicast;
uint8_t rx_buf[2048];

static void dump_hex(char* lable, char* buf, int size){
	int i;
	klog("%s:\n", lable);
	for(int i = 0; i < size; i++){
		klog("%02x ", buf[i]);
		if(i%4 == 3)
			klog(" ");
		if(i%16 == 15)
			klog("\n");
	}
	klog("\n");
}

static void smc_reset()
{
	unsigned int ctl, cfg;
	struct sk_buff *pending_skb;

	SMC_SELECT_BANK(2);
	SMC_SET_INT_MASK(0);
	SMC_SELECT_BANK(0);
	SMC_SET_RCR(RCR_SOFTRST);
	SMC_SELECT_BANK(1);

	cfg = CONFIG_DEFAULT;
	cfg |= CONFIG_NO_WAIT;
	cfg |= CONFIG_EPH_POWER_EN;
	SMC_SET_CONFIG(cfg);

	udelay(1);

	SMC_SELECT_BANK(0);
	SMC_SET_RCR(RCR_CLEAR);
	SMC_SET_TCR(TCR_CLEAR);

	SMC_SELECT_BANK(1);
	ctl = SMC_GET_CTL() | CTL_LE_ENABLE;
	ctl |= CTL_AUTO_RELEASE;
	SMC_SET_CTL(ctl);

	/* Reset the MMU */
	SMC_SELECT_BANK(2);
	SMC_SET_MMU_CMD(MC_RESET);

	SMC_WAIT_MMU_BUSY();

	int mask;
	/* see the header file for options in TCR/RCR DEFAULT */
	SMC_SELECT_BANK(0);
	SMC_SET_TCR(TCR_DEFAULT);
	SMC_SET_RCR(RCR_DEFAULT);

	SMC_SELECT_BANK(1);
	//SMC_SET_MAC_ADDR(mac_addr);

	/* now, enable interrupts */
	mask = IM_EPH_INT|IM_RX_OVRN_INT|IM_RCV_INT;
	mask |= IM_MDINT;
	SMC_SELECT_BANK(2);
	SMC_SET_INT_MASK(mask);
}

static  int  smc_recv(char* buf, int size)
{
	unsigned int packet_number, status, packet_len;

	packet_number = SMC_GET_RXFIFO();
	if (packet_number & RXFIFO_REMPTY) {
		return 0;
	}

	/* read from start of packet */
	SMC_SET_PTR(PTR_READ | PTR_RCV | PTR_AUTOINC);

	/* First two words are status and packet length */
	SMC_GET_PKT_HDR(status, packet_len);
	packet_len &= 0x07ff;  /* mask off top bits */

	back:
	if (packet_len < 6 || status & RS_ERRORS) {
		if (status & RS_TOOLONG && packet_len <= (1514 + 4 + 6)) {
			/* accept VLAN packets */
			status &= ~RS_TOOLONG;
			goto back;
		}
		if (packet_len < 6) {
			/* bloody hardware */
			status |= RS_TOOSHORT;
		}
		SMC_WAIT_MMU_BUSY();
		SMC_SET_MMU_CMD(MC_RELEASE);
		rx_errors++;
		if (status & RS_ALGNERR)
			rx_frame_errors++;
		if (status & (RS_TOOSHORT | RS_TOOLONG))
			rx_length_errors++;
		if (status & RS_BADCRC)
			rx_crc_errors++;
		return -1;
	} else {
		unsigned char *data = buf;
		unsigned int data_len;

		/* set multicast stats */
		if (status & RS_MULTICAST)
			multicast++;

		/*
		 * If odd length: packet_len - 5,
		 * otherwise packet_len - 6.
		 * With the trailing ctrl byte it's packet_len - 4.
		 */
		data_len = packet_len - ((status & RS_ODDFRAME) ? 5 : 6);
		if(data_len > size)
			data_len = size;

		SMC_PULL_DATA(data, packet_len - 4);

		SMC_WAIT_MMU_BUSY();
		SMC_SET_MMU_CMD(MC_RELEASE);

		rx_packets++;
		rx_bytes += data_len;
		return data_len;
	}
}

/*
 * This is called to actually send a packet to the chip.
 */
static int smc_send(uint8_t *buf, int size)
{
	unsigned int packet_no;
	unsigned long flags;
	
	/* now, try to allocate the memory */
	SMC_SET_MMU_CMD( MC_ALLOC | (((size & ~1) + 5) >> 8));

	while(true){
		uint8_t status = SMC_GET_INT();
		if (status & IM_ALLOC_INT) {
			SMC_ACK_INT(IM_ALLOC_INT);
  			break;
		}
   	} 

	packet_no = SMC_GET_AR();

	if (packet_no & AR_FAILED) {
		tx_errors++;
		tx_fifo_errors++;
		return ERR_RETRY_NON_BLOCK;
	}

	/* point to the beginning of the packet */
	SMC_SET_PN(packet_no);
	SMC_SET_PTR(PTR_AUTOINC);

	/*
	 * Send the packet length (+6 for status words, length, and ctl.
	 * The card will pad to 64 bytes with zeroes if packet is too small.
	 */
	SMC_PUT_PKT_HDR(0, size + 6);

	/* send the actual data */
	SMC_PUSH_DATA(buf, size & ~1);

	/* Send final ctl word with the last byte if there is one */
	SMC_outw(((size & 1) ? (0x2000 | buf[size - 1]) : 0), SMC_BASE, DATA_REG);

	/* queue the packet for TX */
	SMC_SET_MMU_CMD(MC_ENQUEUE);

	tx_packets++;
	tx_bytes += size;

	SMC_ENABLE_INT(IM_TX_INT | IM_TX_EMPTY_INT);
	return size;
}

int32_t eth_init(void) {
	_mmio_base = mmio_map();
	smc_reset();

	return 0;
}

static int eth_read(int fd, int from_pid, uint32_t node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)p;
	(void)node;
	int len = smc_recv(buf + offset, size);

	return (len > 0) ? len : ERR_RETRY_NON_BLOCK;
}

static int eth_write(int fd, int from_pid, uint32_t node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)node;

	return smc_send(buf + offset, size);
}

static int eth_dcntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	uint8_t buf[6];
	switch(cmd){
		case 0:	{//get mac
			int bank = SMC_CURRENT_BANK();
			SMC_SELECT_BANK(1);	
			SMC_GET_MAC_ADDR(buf);
			SMC_SELECT_BANK(bank);
			PF->add(ret, buf, 6);
			break;
		}
		default:
			break;
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/eth0";
	eth_init();
	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "eth");
	dev.read = eth_read;
	dev.write = eth_write;
	dev.dev_cntl = eth_dcntl;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
