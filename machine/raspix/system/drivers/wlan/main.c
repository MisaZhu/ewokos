#include <ewoksys/vdevice.h>
#include <string.h>
#include <ewoksys/mmio.h>
#include <arch/bcm283x/mailbox.h>
#include <ewoksys/dma.h>
#include <ewoksys/mstr.h>
#include <sdio/sdhci.h>
#include <utils/log.h>
#include <types.h>
#include <brcm/brcm.h>
#include <brcm/command.h>

uint8_t buf[512];

/* All message buffers must start with this header */
struct bcm2835_mbox_hdr {
	uint32_t buf_size;
	uint32_t code;
};

struct bcm2835_mbox_tag_hdr {
	uint32_t tag;
	uint32_t val_buf_size;
	uint32_t val_len;
};

struct bcm2835_mbox_tag_set_power_state {
	struct bcm2835_mbox_tag_hdr tag_hdr;
	union {
		struct {
			uint32_t device_id;
			uint32_t state;
		} req;
		struct {
			uint32_t device_id;
			uint32_t state;
		} resp;
	} body;
};

struct bcm2835_mbox_tag_set_sdhost_clock {
	struct bcm2835_mbox_tag_hdr tag_hdr;
	union {
		struct {
			uint32_t rate_hz;
		} req;
		struct {
			uint32_t rate_hz;
			uint32_t rate_1;
			uint32_t rate_2;
		} resp;
	} body;
};

struct bcm2835_mbox_tag_get_clock_rate {
	struct bcm2835_mbox_tag_hdr tag_hdr;
	union {
		struct {
			uint32_t clock_id;
		} req;
		struct {
			uint32_t clock_id;
			uint32_t rate_hz;
		} resp;
	} body;
};

struct msg_set_power_state {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_set_power_state set_power_state;
	uint32_t end_tag;
};


struct msg_set_sdhost_clock {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_set_sdhost_clock set_sdhost_clock;
	uint32_t end_tag;
};


struct msg_get_clock_rate {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_clock_rate get_clock_rate;
	uint32_t end_tag;
};

#define BCM2835_MBOX_INIT_HDR(_m_) { \
		memset((_m_), 0, sizeof(*(_m_))); \
		(_m_)->hdr.buf_size = sizeof(*(_m_)); \
		(_m_)->hdr.code = 0; \
		(_m_)->end_tag = 0; \
	}

#define BCM2835_MBOX_INIT_TAG(_t_, _id_) { \
		(_t_)->tag_hdr.tag = BCM2835_MBOX_TAG_##_id_; \
		(_t_)->tag_hdr.val_buf_size = sizeof((_t_)->body); \
		(_t_)->tag_hdr.val_len = sizeof((_t_)->body.req); \
	}

#define BCM2835_MBOX_TAG_GET_CLOCK_RATE	0x00030002
#define BCM2835_MBOX_TAG_GET_MAX_CLOCK_RATE	0x00030004
#define BCM2835_MBOX_TAG_GET_MIN_CLOCK_RATE	0x00030007


#define BCM2835_MBOX_POWER_DEVID_SDHCI		0
#define BCM2835_MBOX_POWER_DEVID_UART0		1
#define BCM2835_MBOX_POWER_DEVID_UART1		2
#define BCM2835_MBOX_POWER_DEVID_USB_HCD	3
#define BCM2835_MBOX_POWER_DEVID_I2C0		4
#define BCM2835_MBOX_POWER_DEVID_I2C1		5
#define BCM2835_MBOX_POWER_DEVID_I2C2		6
#define BCM2835_MBOX_POWER_DEVID_SPI		7
#define BCM2835_MBOX_POWER_DEVID_CCP2TX		8

#define BCM2835_MBOX_TAG_SET_POWER_STATE	0x00028001
#define BCM2835_MBOX_TAG_SET_SDHOST_CLOCK	0x00038042
#define BCM2835_MBOX_SET_POWER_STATE_REQ_ON	(1 << 0)
#define BCM2835_MBOX_SET_POWER_STATE_REQ_WAIT	(1 << 1)
#define BCM2835_MBOX_PROP_CHAN		8

static int bcm2835_power_on_module(uint32_t module)
{
    mail_message_t msg;
    //struct msg_set_power_state* msg_pwr = (struct msg_set_power_state*)dma_phy_addr(0, dma_alloc(sizeof(struct msg_set_power_state)));
    struct msg_set_power_state* msg_pwr = (struct msg_set_power_state*)(dma_alloc(0, sizeof(struct msg_set_power_state)));

	BCM2835_MBOX_INIT_HDR(msg_pwr);
	BCM2835_MBOX_INIT_TAG(&msg_pwr->set_power_state,
			      SET_POWER_STATE);
	msg_pwr->set_power_state.body.req.device_id = module;
	msg_pwr->set_power_state.body.req.state =
		BCM2835_MBOX_SET_POWER_STATE_REQ_ON |
		BCM2835_MBOX_SET_POWER_STATE_REQ_WAIT;

    msg.data = ((uint32_t)msg_pwr + 0x40000000) >> 4;	
	msg.channel = PROPERTY_CHANNEL;
    bcm283x_mailbox_call(&msg);

	return 0;
}

void bcm283x_mbox_pin_ctrl(int idx, int dir, int on) {
	mail_message_t msg;
	/*message head + tag head + property*/
	uint32_t size = 12 + 12 + 24;
	//uint32_t* buf = (uint32_t*)dma_phy_addr(dma_alloc(size));
	uint32_t* buf = (uint32_t*)dma_alloc(0, size);

	/*message head*/
	buf[0] = size;
	buf[1] = 0;	//RPI_FIRMWARE_STATUS_REQUEST;
	/*tag head*/
	buf[2] = 0x00038043;								/*set gpio config*/
	buf[3] = 24;									/*buffer size*/
	buf[4] = 0;										/*respons size*/
	/*property package*/
	buf[5] =  128 + idx;								/*wifi power pin*/
	buf[6] =  dir ? 1 : 0;								/*direction*/
	buf[7] =  0;										/*polarity*/
	buf[8] = 0;										/*term_en*/	
	buf[9] = 0;										/*term_pull_up*/
	buf[10] = on ? 1 : 0;		
	/*message end*/
	buf[11] = 0;
	
	msg.data = ((uint32_t)buf + 0x40000000) >> 4;	
	msg.channel = PROPERTY_CHANNEL;
	bcm283x_mailbox_call(&msg);
}

#define CM_GP2DIV	(_mmio_base + 0x101084) 
#define CM_GP2CTL	(_mmio_base + 0x101080) 
#define CM_PASSWORD (0x5a000000)
#define CM_BUSY 	(0x1<<7)
#define CM_ENABLE 	(0x1<<4)
#define writel(val, addr) (*((volatile uint32_t *)(addr)) = (uint32_t)(val))

void clock_init(void){
	int timeout = 1000;

	//set 32.768 clock for wifi module
	writel(CM_PASSWORD|0x1, CM_GP2CTL);
	while(timeout--)  // Wait for clock to be !BUSY
  	{
		uint32_t reg = readl(CM_GP2CTL);
		if(!(reg&CM_BUSY))
			break;
    	usleep( 1000 );
  	}
	//32.768Hz = 19.2Mhz / (585 + 3840/4096)
	writel(CM_PASSWORD | (585<<12)|(3840), CM_GP2DIV); 
    writel((CM_PASSWORD | (1 << 9) | 1), CM_GP2CTL );
    writel(CM_PASSWORD | (1 << 9) | 1 | (1 << 4), CM_GP2CTL);

	timeout = 1000;
	while(timeout--){
		uint32_t reg = readl(CM_GP2CTL);
		if(reg&CM_BUSY)
			break;
		usleep(1000);
	}
}

static int net_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)p;
	(void)node;

	int len = brcm_recv(buf + offset, size);
	return (len > 0)?len:VFS_ERR_RETRY; 
}

static int net_write(int fd, int from_pid, fsinfo_t* node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)node;
	int len = brcm_send((uint8_t*)(buf + offset), size);
	return (len > 0)?len:VFS_ERR_RETRY; 
}

static int net_dcntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	char mac[6];
	switch(cmd){
		case 0:	{//get mac
			if(brcm_state() > 0){
				get_ethaddr(mac);
				PF->add(ret, mac, 6);
			}else{
				return VFS_ERR_RETRY;
			}
			break;
		}
		case 1:
		{//get buffer count
			PF->addi(ret, brcm_check_data());
			break;
		}	
		default:
			break;
	}
	return 0;
}

char* net_dev_cmd(int from_pid, int argc, char** argv, void* p) {
	if(strcmp(argv[0], "log") == 0) {
		return brcm_get_log();
	}
	return NULL;
}

int main(int argc, char** argv) {
	_mmio_base = mmio_map();
	log_init();	
	bcm2835_power_on_module(BCM2835_MBOX_POWER_DEVID_SDHCI);
	clock_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "wlan");
	bcm283x_mbox_pin_ctrl(1, 1, 0);
	bcm283x_mbox_pin_ctrl(2, 1, 0);
	usleep(100000);
	bcm283x_mbox_pin_ctrl(1, 1, 1);
	bcm283x_mbox_pin_ctrl(2, 1, 1);
	usleep(100000);
	brcm_init();


	const char* mnt_point = argc > 1 ? argv[1]: "/dev/eth0";
	strcpy(dev.name, "eth");
	dev.read = net_read;
	dev.write = net_write;
	dev.dev_cntl = net_dcntl;
	dev.cmd = net_dev_cmd;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);


	return 0;
}

