#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ewoksys/klog.h>
#include <pthread.h>
#include <string.h>

#include <types.h>
#include <utils/skb.h>
#include <utils/utils.h>
#include <utils/qbuf.h>
#include <utils/log.h>
#include <utils/config.h>
#include <utils/pbkdf2.h>
#include <sdio/sdio.h>
#include <sdio/mmc.h>

#include <ewoksys/ipc.h>

#include "brcm.h"
#include "chip.h"
#include "firmware.h"
#include "chipcommon.h"
#include "command.h"
#include "soc.h"
#include "brcm_hw_ids.h"
#include "bcma.h"
#include "bcma_driver_chipcommon.h"


#define ALIGNMENT   4
#define MAX_FRAME_SIZE  2048

#define TXQLEN      2048    /* bulk tx queue length */
#define TXHI        (TXQLEN - 256)  /* turn on flow control above TXHI */
#define TXLOW       (TXHI - 256)    /* turn off flow control below TXLOW */
#define PRIOMASK    7
#define TXRETRIES   2   /* # of retries for tx frames */
#define BRCMF_RXBOUND   50  /* Default for max rx frames in
                 one scheduling */
#define BRCMF_TXBOUND   20  /* Default for max tx frames in
                 one scheduling */
#define BRCMF_TXMINMAX  1   /* Max tx frames if rx still pending */
#define MEMBLOCK    2048    /* Block size used for downloading
                 of dongle image */
#define MAX_DATA_BUF    (32 * 1024) /* Must be large enough to hold
                 biggest possible glom */
#define BRCMF_FIRSTREAD (1 << 6)
#define BRCMF_CONSOLE   10  /* watchdog interval to poll console */

/* watermark expressed in number of words */
#define DEFAULT_F2_WATERMARK    0x8
#define CY_4373_F2_WATERMARK    0x40
#define CY_4373_F1_MESBUSYCTRL  (CY_4373_F2_WATERMARK | SBSDIO_MESBUSYCTRL_ENAB)
#define CY_43012_F2_WATERMARK    0x60
#define CY_43012_MES_WATERMARK  0x50
#define CY_43012_MESBUSYCTRL    (CY_43012_MES_WATERMARK | \
                 SBSDIO_MESBUSYCTRL_ENAB)
#define CY_4339_F2_WATERMARK    48
#define CY_4339_MES_WATERMARK   80
#define CY_4339_MESBUSYCTRL (CY_4339_MES_WATERMARK | \
                 SBSDIO_MESBUSYCTRL_ENAB)
#define CY_43455_F2_WATERMARK   0x60
#define CY_43455_MES_WATERMARK  0x50
#define CY_43455_MESBUSYCTRL    (CY_43455_MES_WATERMARK | \
                 SBSDIO_MESBUSYCTRL_ENAB)
#define CY_435X_F2_WATERMARK    0x40
#define CY_435X_F1_MESBUSYCTRL  (CY_435X_F2_WATERMARK | \
                 SBSDIO_MESBUSYCTRL_ENAB)

/* SOC Interconnect types (aka chip types) */
#define SOCI_SB     0
#define SOCI_AI     1

/* PL-368 DMP definitions */
#define DMP_DESC_TYPE_MSK   0x0000000F
#define  DMP_DESC_EMPTY     0x00000000
#define  DMP_DESC_VALID     0x00000001
#define  DMP_DESC_COMPONENT 0x00000001
#define  DMP_DESC_MASTER_PORT   0x00000003
#define  DMP_DESC_ADDRESS   0x00000005
#define  DMP_DESC_ADDRSIZE_GT32 0x00000008
#define  DMP_DESC_EOT       0x0000000F

#define DMP_COMP_DESIGNER   0xFFF00000
#define DMP_COMP_DESIGNER_S 20
#define DMP_COMP_PARTNUM    0x000FFF00
#define DMP_COMP_PARTNUM_S  8
#define DMP_COMP_CLASS      0x000000F0
#define DMP_COMP_CLASS_S    4
#define DMP_COMP_REVISION   0xFF000000
#define DMP_COMP_REVISION_S 24
#define DMP_COMP_NUM_SWRAP  0x00F80000
#define DMP_COMP_NUM_SWRAP_S    19
#define DMP_COMP_NUM_MWRAP  0x0007C000
#define DMP_COMP_NUM_MWRAP_S    14
#define DMP_COMP_NUM_SPORT  0x00003E00
#define DMP_COMP_NUM_SPORT_S    9
#define DMP_COMP_NUM_MPORT  0x000001F0
#define DMP_COMP_NUM_MPORT_S    4

#define DMP_MASTER_PORT_UID 0x0000FF00
#define DMP_MASTER_PORT_UID_S   8
#define DMP_MASTER_PORT_NUM 0x000000F0
#define DMP_MASTER_PORT_NUM_S   4

#define DMP_SLAVE_ADDR_BASE 0xFFFFF000
#define DMP_SLAVE_ADDR_BASE_S   12
#define DMP_SLAVE_PORT_NUM  0x00000F00
#define DMP_SLAVE_PORT_NUM_S    8
#define DMP_SLAVE_TYPE      0x000000C0
#define DMP_SLAVE_TYPE_S    6
#define  DMP_SLAVE_TYPE_SLAVE   0
#define  DMP_SLAVE_TYPE_BRIDGE  1
#define  DMP_SLAVE_TYPE_SWRAP   2
#define  DMP_SLAVE_TYPE_MWRAP   3
#define DMP_SLAVE_SIZE_TYPE 0x00000030
#define DMP_SLAVE_SIZE_TYPE_S   4
#define  DMP_SLAVE_SIZE_4K  0
#define  DMP_SLAVE_SIZE_8K  1
#define  DMP_SLAVE_SIZE_16K 2
#define  DMP_SLAVE_SIZE_DESC    3

/* EROM CompIdentB */
#define CIB_REV_MASK        0xff000000
#define CIB_REV_SHIFT       24

/* ARM CR4 core specific control flag bits */
#define ARMCR4_BCMA_IOCTL_CPUHALT   0x0020

/* D11 core specific control flag bits */
#define D11_BCMA_IOCTL_PHYCLOCKEN   0x0004
#define D11_BCMA_IOCTL_PHYRESET     0x0008

/* chip core base & ramsize */
/* bcm4329 */
/* SDIO device core, ID 0x829 */
#define BCM4329_CORE_BUS_BASE       0x18011000
/* internal memory core, ID 0x80e */
#define BCM4329_CORE_SOCRAM_BASE    0x18003000
/* ARM Cortex M3 core, ID 0x82a */
#define BCM4329_CORE_ARM_BASE       0x18002000

/* Max possibly supported memory size (limited by IO mapped memory) */
#define BRCMF_CHIP_MAX_MEMSIZE      (4 * 1024 * 1024)

#define CORE_SB(base, field) \
        (base + SBCONFIGOFF + offsetof(struct sbconfig, field))
#define SBCOREREV(sbidh) \
    ((((sbidh) & SSB_IDHIGH_RCHI) >> SSB_IDHIGH_RCHI_SHIFT) | \
      ((sbidh) & SSB_IDHIGH_RCLO))


/* SBSDIO_DEVICE_CTL */

/* 1: device will assert busy signal when receiving CMD53 */
#define SBSDIO_DEVCTL_SETBUSY       0x01
/* 1: assertion of sdio interrupt is synchronous to the sdio clock */
#define SBSDIO_DEVCTL_SPI_INTR_SYNC 0x02
/* 1: mask all interrupts to host except the chipActive (rev 8) */
#define SBSDIO_DEVCTL_CA_INT_ONLY   0x04
/* 1: isolate internal sdio signals, put external pads in tri-state; requires
 * sdio bus power cycle to clear (rev 9) */
#define SBSDIO_DEVCTL_PADS_ISO      0x08
/* 1: enable F2 Watermark */
#define SBSDIO_DEVCTL_F2WM_ENAB     0x10
/* Force SD->SB reset mapping (rev 11) */
#define SBSDIO_DEVCTL_SB_RST_CTL    0x30
/*   Determined by CoreControl bit */
#define SBSDIO_DEVCTL_RST_CORECTL   0x00
/*   Force backplane reset */
#define SBSDIO_DEVCTL_RST_BPRESET   0x10
/*   Force no backplane reset */
#define SBSDIO_DEVCTL_RST_NOBPRESET 0x20


/* SDIO function 1 register CHIPCLKCSR */
/* Force ALP request to backplane */
#define SBSDIO_FORCE_ALP        0x01
/* Force HT request to backplane */
#define SBSDIO_FORCE_HT         0x02
/* Force ILP request to backplane */
#define SBSDIO_FORCE_ILP        0x04
/* Make ALP ready (power up xtal) */
#define SBSDIO_ALP_AVAIL_REQ        0x08
/* Make HT ready (power up PLL) */
#define SBSDIO_HT_AVAIL_REQ     0x10
/* Squelch clock requests from HW */
#define SBSDIO_FORCE_HW_CLKREQ_OFF  0x20
/* Status: ALP is ready */
#define SBSDIO_ALP_AVAIL        0x40
/* Status: HT is ready */
#define SBSDIO_HT_AVAIL         0x80
#define SBSDIO_CSR_MASK         0x1F
#define BRCMF_INIT_CLKCTL1  (SBSDIO_FORCE_HW_CLKREQ_OFF |   \
                    SBSDIO_ALP_AVAIL_REQ)
#define SBSDIO_AVBITS       (SBSDIO_HT_AVAIL | SBSDIO_ALP_AVAIL)

#define SDIOD_DRVSTR_KEY(chip, pmu)     (((unsigned int)(chip) << 16) | (pmu))
#define SBSDIO_ALPAV(regval)    ((regval) & SBSDIO_AVBITS)
#define SBSDIO_HTAV(regval) (((regval) & SBSDIO_AVBITS) == SBSDIO_AVBITS)
#define SBSDIO_ALPONLY(regval)  (SBSDIO_ALPAV(regval) && !SBSDIO_HTAV(regval))
#define SBSDIO_CLKAV(regval, alponly) \
    (SBSDIO_ALPAV(regval) && (alponly ? 1 : SBSDIO_HTAV(regval)))


/* intstatus */
#define I_SMB_SW0	(1 << 0)	/* To SB Mail S/W interrupt 0 */
#define I_SMB_SW1	(1 << 1)	/* To SB Mail S/W interrupt 1 */
#define I_SMB_SW2	(1 << 2)	/* To SB Mail S/W interrupt 2 */
#define I_SMB_SW3	(1 << 3)	/* To SB Mail S/W interrupt 3 */
#define I_SMB_SW_MASK	0x0000000f	/* To SB Mail S/W interrupts mask */
#define I_SMB_SW_SHIFT	0	/* To SB Mail S/W interrupts shift */
#define I_HMB_SW0	(1 << 4)	/* To Host Mail S/W interrupt 0 */
#define I_HMB_SW1	(1 << 5)	/* To Host Mail S/W interrupt 1 */
#define I_HMB_SW2	(1 << 6)	/* To Host Mail S/W interrupt 2 */
#define I_HMB_SW3	(1 << 7)	/* To Host Mail S/W interrupt 3 */
#define I_HMB_SW_MASK	0x000000f0	/* To Host Mail S/W interrupts mask */
#define I_HMB_SW_SHIFT	4	/* To Host Mail S/W interrupts shift */
#define I_WR_OOSYNC	(1 << 8)	/* Write Frame Out Of Sync */
#define I_RD_OOSYNC	(1 << 9)	/* Read Frame Out Of Sync */
#define	I_PC		(1 << 10)	/* descriptor error */
#define	I_PD		(1 << 11)	/* data error */
#define	I_DE		(1 << 12)	/* Descriptor protocol Error */
#define	I_RU		(1 << 13)	/* Receive descriptor Underflow */
#define	I_RO		(1 << 14)	/* Receive fifo Overflow */
#define	I_XU		(1 << 15)	/* Transmit fifo Underflow */
#define	I_RI		(1 << 16)	/* Receive Interrupt */
#define I_BUSPWR	(1 << 17)	/* SDIO Bus Power Change (rev 9) */
#define I_XMTDATA_AVAIL (1 << 23)	/* bits in fifo */
#define	I_XI		(1 << 24)	/* Transmit Interrupt */
#define I_RF_TERM	(1 << 25)	/* Read Frame Terminate */
#define I_WF_TERM	(1 << 26)	/* Write Frame Terminate */
#define I_PCMCIA_XU	(1 << 27)	/* PCMCIA Transmit FIFO Underflow */
#define I_SBINT		(1 << 28)	/* sbintstatus Interrupt */
#define I_CHIPACTIVE	(1 << 29)	/* chip from doze to active state */
#define I_SRESET	(1 << 30)	/* CCCR RES interrupt */
#define I_IOE2		(1U << 31)	/* CCCR IOE2 Bit Changed */
#define	I_ERRORS	(I_PC | I_PD | I_DE | I_RU | I_RO | I_XU)
#define I_DMA		(I_RI | I_XI | I_ERRORS)

#define SDPCM_HWHDR_LEN         4
#define SDPCM_HWEXT_LEN         8
#define SDPCM_SWHDR_LEN         8
#define SDPCM_HDRLEN            (SDPCM_HWHDR_LEN + SDPCM_SWHDR_LEN)
/* software header */
#define SDPCM_SEQ_MASK          0x000000ff
#define SDPCM_SEQ_WRAP          256
#define SDPCM_CHANNEL_MASK      0x00000f00
#define SDPCM_CHANNEL_SHIFT     8
#define SDPCM_CONTROL_CHANNEL       0   /* Control */
#define SDPCM_EVENT_CHANNEL     1   /* Asyc Event Indication */
#define SDPCM_DATA_CHANNEL      2   /* Data Xmit/Recv */
#define SDPCM_GLOM_CHANNEL      3   /* Coalesced packets */
#define SDPCM_TEST_CHANNEL      15  /* Test/debug packets */
#define SDPCM_GLOMDESC(p)       (((uint8_t *)p)[1] & 0x80)
#define SDPCM_NEXTLEN_MASK      0x00ff0000
#define SDPCM_NEXTLEN_SHIFT     16
#define SDPCM_DOFFSET_MASK      0xff000000
#define SDPCM_DOFFSET_SHIFT     24
#define SDPCM_FCMASK_MASK       0x000000ff
#define SDPCM_WINDOW_MASK       0x0000ff00
#define SDPCM_WINDOW_SHIFT      8

#define RETRYCHAN(chan) ((chan) == SDPCM_EVENT_CHANNEL)

/* Current protocol version */
#define SDPCM_PROT_VERSION  4
/* tosbmailboxdata */
#define SMB_DATA_VERSION_SHIFT  16  /* host protocol version */

/* SDIO device register access interface */
/* Accessors for SDIO Function 0 */
#define brcmf_sdiod_func0_rb(addr, r) \
    sdio_readb(0, (addr), (r))

#define brcmf_sdiod_func0_wb(addr, v, ret) \
    sdio_writeb(0, (v), (addr), (ret))

/* Accessors for SDIO Function 1 */
#define brcmf_sdiod_readb(addr, r) \
    sdio_readb(1, (addr), (r))

#define brcmf_sdiod_writeb(addr, v, ret) \
    sdio_writeb(1, (v), (addr), (ret))

enum brcmf_sdio_frmtype {
    BRCMF_SDIO_FT_NORMAL,
    BRCMF_SDIO_FT_SUPER,
    BRCMF_SDIO_FT_SUB,
};

struct brcmf_sdio_hdrinfo {
	uint8_t seq_num;
	uint8_t channel;
	uint16_t len;
	uint16_t len_left;
	uint16_t len_nxtfrm;
	uint8_t dat_offset;
	bool lastfrm;
	uint16_t tail_pad;
};

enum WL_STATE{
    IDLE,
    SCANNING,
    CONNECTING,
    CONNECTED,
    DISCONNECTED
};
struct brcmf_dev{
    struct brcmf_chip *ci;
    struct brcmf_core *sdio_core;
    struct brcmf_core *cc_core;
    int clkstate;
    bool alp_only;
    uint32_t sbwad;
    uint32_t console_addr;

    uint8_t *hdrbuf;     /* buffer for handling rx frame */
    uint8_t *rxhdr;      /* Header of current rx frame (in hdrbuf) */
    uint32_t rx_seq;
    uint32_t rxskip;

    uint32_t hostintmask;
    uint32_t intstatus;
    uint32_t fcstate;
    bool dpc_triggered;
    bool rxpending;
    uint8_t *ctrl_frame_buf;
    uint32_t ctrl_frame_len;
    int ctrl_frame_stat;
    int ctrl_frame_err;
    uint8_t flowcontrol; /* per prio flow control bitmask */
    uint8_t tx_seq;      /* Transmit sequence number (next) */
    uint8_t tx_max;      /* Maximum transmit sequence allowed */
    uint rxbound;       /* Rx frames to read before resched */
    uint txbound;       /* Tx frames to send before resched */
    
    int blocksize;
    int roundup;
    struct brcmf_sdio_hdrinfo cur_read;
    struct sk_buff *glomd;
    uint16_t head_align;     /* buffer pointer alignment */
    uint16_t sgentry_align;  /* scatter-gather buffer alignment */

    int rxlen;
    int rxdoff;
    uint8_t* rxctl;

    queue_buffer_t *rx_queue;
    queue_buffer_t *tx_queue;


    uint64_t scan_update;
    int priority;
    char ssid[32];
    enum WL_STATE  state;
};

struct brcmf_dev *bus =  NULL;
static void brcmf_sdio_dpc(void);

static inline uint8_t brcmf_sdio_getdatoffset(uint8_t *swheader)
{
    uint32_t hdrvalue;
    hdrvalue = *(uint32_t *)swheader;
    return (uint8_t)((hdrvalue & SDPCM_DOFFSET_MASK) >> SDPCM_DOFFSET_SHIFT);
}


static int brcmf_sdiod_set_backplane_window(uint32_t addr)
{
    uint32_t v, bar0 = addr & SBSDIO_SBWINDOW_MASK;
    int err = 0, i;
    v = bar0 >> 8;

    if (bar0 == bus->sbwad)
        return 0;

    for (i = 0 ; i < 3 && !err ; i++, v >>= 8)
        brcmf_sdiod_writeb(SBSDIO_FUNC1_SBADDRLOW + i,
                   v & 0xff, &err);

    if (!err)
        bus->sbwad = bar0;
    return err;
}

static int brcmf_sdiod_skbuff_read(
				   int func, uint32_t addr,
				   struct sk_buff *skb)
{
	unsigned int req_sz;
	int err;

	/* Single skb use the standard mmc interface */
	req_sz = skb->len + 3;
	req_sz &= (uint)~3;

	switch (func) {
	case 1:
		err = sdio_memcpy_fromio(func, ((uint8_t *)(skb->data)), addr,
					 req_sz);
		break;
	case 2:
		err = sdio_readsb(func, ((uint8_t *)(skb->data)), addr, req_sz);
		break;
	default:
		/* bail out as things are really fishy here */
		brcm_log("invalid sdio function number: %d\n", func);
		err = -ENOMEDIUM;
	}

	return err;
}

static int brcmf_sdiod_skbuff_write(
                    int func, uint32_t addr,
                    struct sk_buff *skb)
{
    unsigned int req_sz;
    int err;

    /* Single skb use the standard mmc interface */
    req_sz = skb->len + 3;
    req_sz &= (uint)~3;

    err = sdio_memcpy_toio(func, addr, ((uint8_t *)(skb->data)), req_sz);
    return err;
}

int
brcmf_sdiod_ramrw(bool write, uint32_t address,
          uint8_t *data, uint size)
{
    int err = 0;
    struct sk_buff *pkt;
    uint32_t sdaddr;
    uint dsize;
    dsize = min_t(uint, SBSDIO_SB_OFT_ADDR_LIMIT, size);

    pkt = skb_alloc(dsize);
    if (!pkt) {
        brcm_log("skb_alloc failed: len %d\n", dsize);
        return -EIO;
    }
    pkt->priority = 0;

    /* Determine initial transfer parameters */
    sdaddr = address & SBSDIO_SB_OFT_ADDR_MASK;
    if ((sdaddr + size) & SBSDIO_SBWINDOW_MASK)
        dsize = (SBSDIO_SB_OFT_ADDR_LIMIT - sdaddr);
    else
        dsize = size;

    /* Do the transfer(s) */
    while (size) {
        /* Set the backplane window to include the start address */
        err = brcmf_sdiod_set_backplane_window(address);
        if (err)
            break;

        // brcm_log("%s %d bytes at offset 0x%08x in window 0x%08x\n",
        //       write ? "write" : "read", dsize,
        //       sdaddr, address & SBSDIO_SBWINDOW_MASK);

        sdaddr &= SBSDIO_SB_OFT_ADDR_MASK;
        sdaddr |= SBSDIO_SB_ACCESS_2_4B_FLAG;

        skb_put(pkt, dsize);
        if (write) {
            memcpy(pkt->data, data, dsize);
            err = brcmf_sdiod_skbuff_write(1, sdaddr, pkt);
        } else {
            err = brcmf_sdiod_skbuff_read(1, sdaddr, pkt);
        }

        if (err) {
            brcm_log("membytes transfer failed\n");
            break;
        }
        if (!write)
            memcpy(data, pkt->data, dsize);
        skb_trim(pkt, 0);

        /* Adjust for next transfer (if any) */
        size -= dsize;
        if (size) {
            data += dsize;
            address += dsize;
            sdaddr = 0;
            dsize = min_t(uint, SBSDIO_SB_OFT_ADDR_LIMIT, size);
        }
    }

    skb_free(pkt);
    return err;
}

uint32_t brcmf_sdiod_readl(uint32_t addr, int *ret)
{
	uint32_t data = 0;
	int retval;

	retval = brcmf_sdiod_set_backplane_window(addr);
	if (retval)
		goto out;

	addr &= SBSDIO_SB_OFT_ADDR_MASK;
	addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;

	data = sdio_readl(1, addr, &retval);

out:
	if (ret)
		*ret = retval;

	return data;
}

void brcmf_sdiod_writel(uint32_t addr,
			uint32_t data, int *ret)
{
	int retval;

	retval = brcmf_sdiod_set_backplane_window(addr);
	if (retval)
		goto out;

	addr &= SBSDIO_SB_OFT_ADDR_MASK;
	addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;

	sdio_writel(1, data, addr, &retval);

out:
	if (ret)
		*ret = retval;
}

void brcm_dummy_read(uint32_t addr, int len){
    int ret;
    for(int i= 0; i < len; i++){
        sdio_readb(0, addr + i, &ret);
        if (ret) {
            brcm_log("Failed %d\n", ret);
        }
    }
}
/*****************************************************************/

static int brcmf_sdio_buscoreprep(void)
{
    int err = 0;
    uint8_t clkval, clkset;

    /* Try forcing SDIO core to do ALPAvail request only */
    clkset = SBSDIO_FORCE_HW_CLKREQ_OFF | SBSDIO_ALP_AVAIL_REQ;
    brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR, clkset, &err);
    if (err) {
        brcm_log("error writing for HT off\n");
        return err;
    }

    /* If register supported, wait for ALPAvail and then force ALP */
    /* This may take up to 15 milliseconds */
    clkval = brcmf_sdiod_readb(SBSDIO_FUNC1_CHIPCLKCSR, NULL);

    if ((clkval & ~SBSDIO_AVBITS) != clkset) {
        brcm_log("ChipClkCSR access: wrote 0x%02x read 0x%02x\n",
              clkset, clkval);
        return -EACCES;
    }

    int timeout = 15000;
    while(timeout-- > 0){
        clkval = brcmf_sdiod_readb(SBSDIO_FUNC1_CHIPCLKCSR, NULL);
        if(SBSDIO_ALPAV(clkval))
            break;
    }
    
    if (!SBSDIO_ALPAV(clkval)) {
        brcm_log("timeout on ALPAV wait, clkval 0x%02x\n",
              clkval);
        return -EBUSY;
    }

    clkset = SBSDIO_FORCE_HW_CLKREQ_OFF | SBSDIO_FORCE_ALP;
    brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR, clkset, &err);
    usleep(65);

    /* Also, disable the extra SDIO pull-ups */
    brcmf_sdiod_writeb(SBSDIO_FUNC1_SDIOPULLUP, 0, NULL);

    return 0;
}

/* SDIO Pad drive strength to select value mappings */
struct sdiod_drive_str {
    uint8_t strength;    /* Pad Drive Strength in mA */
    uint8_t sel;     /* Chip-specific select value */
};

/* SDIO Drive Strength to sel value table for PMU Rev 11 (1.8V) */
static const struct sdiod_drive_str sdiod_drvstr_tab1_1v8[] = {
    {32, 0x6},
    {26, 0x7},
    {22, 0x4},
    {16, 0x5},
    {12, 0x2},
    {8, 0x3},
    {4, 0x0},
    {0, 0x1}
};

/* SDIO Drive Strength to sel value table for PMU Rev 13 (1.8v) */
static const struct sdiod_drive_str sdiod_drive_strength_tab5_1v8[] = {
    {6, 0x7},
    {5, 0x6},
    {4, 0x5},
    {3, 0x4},
    {2, 0x2},
    {1, 0x1},
    {0, 0x0}
};

/* SDIO Drive Strength to sel value table for PMU Rev 17 (1.8v) */
static const struct sdiod_drive_str sdiod_drvstr_tab6_1v8[] = {
    {3, 0x3},
    {2, 0x2},
    {1, 0x1},
    {0, 0x0} };

/* SDIO Drive Strength to sel value table for 43143 PMU Rev 17 (3.3V) */
static const struct sdiod_drive_str sdiod_drvstr_tab2_3v3[] = {
    {16, 0x7},
    {12, 0x5},
    {8,  0x3},
    {4,  0x1}
};


static void
brcmf_sdio_drivestrengthinit(uint32_t drivestrength)
{
    const struct sdiod_drive_str *str_tab = NULL;
    uint32_t str_mask;
    uint32_t str_shift;
    uint32_t i;
    uint32_t drivestrength_sel = 0;
    uint32_t cc_data_temp;
    uint32_t addr;

    if (!(bus->ci->cc_caps & CC_CAP_PMU))
        return;

    switch (SDIOD_DRVSTR_KEY(bus->ci->chip, bus->ci->pmurev)) {
    case SDIOD_DRVSTR_KEY(BRCM_CC_4330_CHIP_ID, 12):
        str_tab = sdiod_drvstr_tab1_1v8;
        str_mask = 0x00003800;
        str_shift = 11;
        break;
    case SDIOD_DRVSTR_KEY(BRCM_CC_4334_CHIP_ID, 17):
        str_tab = sdiod_drvstr_tab6_1v8;
        str_mask = 0x00001800;
        str_shift = 11;
        break;
    case SDIOD_DRVSTR_KEY(BRCM_CC_43143_CHIP_ID, 17):
        /* note: 43143 does not support tristate */
        i = ARRAY_SIZE(sdiod_drvstr_tab2_3v3) - 1;
        if (drivestrength >= sdiod_drvstr_tab2_3v3[i].strength) {
            str_tab = sdiod_drvstr_tab2_3v3;
            str_mask = 0x00000007;
            str_shift = 0;
        } else
            brcm_log("Invalid SDIO Drive strength for chip %s, strength=%d\n",
                  bus->ci->name, drivestrength);
        break;
    case SDIOD_DRVSTR_KEY(BRCM_CC_43362_CHIP_ID, 13):
        str_tab = sdiod_drive_strength_tab5_1v8;
        str_mask = 0x00003800;
        str_shift = 11;
        break;
    default:
        brcm_log("No SDIO driver strength init needed for chip %s rev %d pmurev %d\n",
              bus->ci->name, bus->ci->chiprev, bus->ci->pmurev);
        break;
    }

    if (str_tab != NULL) {
        struct brcmf_core *pmu = brcmf_chip_get_pmu();

        for (i = 0; str_tab[i].strength != 0; i++) {
            if (drivestrength >= str_tab[i].strength) {
                drivestrength_sel = str_tab[i].sel;
                break;
            }
        }
        addr = CORE_CC_REG(pmu->base, chipcontrol_addr);
        brcmf_sdiod_writel(addr, 1, NULL);
        cc_data_temp = brcmf_sdiod_readl(addr, NULL);
        cc_data_temp &= ~str_mask;
        drivestrength_sel <<= str_shift;
        cc_data_temp |= drivestrength_sel;
        brcmf_sdiod_writel(addr, cc_data_temp, NULL);

        brcm_log("SDIO: %d mA (req=%d mA) drive strength selected, set to 0x%08x\n",
              str_tab[i].strength, drivestrength, cc_data_temp);
    }
}

/* enable KSO bit */
static int brcmf_sdio_kso_init()
{
    struct brcmf_core *core = bus->sdio_core;
    uint8_t val;
    int err = 0;

    /* KSO bit added in SDIO core rev 12 */
    if (core->rev < 12)
        return 0;

    val = brcmf_sdiod_readb(SBSDIO_FUNC1_SLEEPCSR, &err);
    if (err) {
        brcm_log("error reading SBSDIO_FUNC1_SLEEPCSR\n");
        return err;
    }

    if (!(val & SBSDIO_FUNC1_SLEEPCSR_KSO_MASK)) {
        val |= (SBSDIO_FUNC1_SLEEPCSR_KSO_EN <<
            SBSDIO_FUNC1_SLEEPCSR_KSO_SHIFT);
        brcmf_sdiod_writeb(SBSDIO_FUNC1_SLEEPCSR,
                   val, &err);
        if (err) {
            brcm_log("error writing SBSDIO_FUNC1_SLEEPCSR\n");
            return err;
        }
    }

    return 0;
}

/* clkstate */
#define CLK_NONE    0
#define CLK_SDONLY  1
#define CLK_PENDING 2
#define CLK_AVAIL   3

#define HOSTINTMASK     (I_HMB_SW_MASK | I_CHIPACTIVE)

/* Change idle/active SD state */
static int brcmf_sdio_sdclk(bool on)
{

    if (on)
        bus->clkstate = CLK_SDONLY;
    else
        bus->clkstate = CLK_NONE;

    return 0;
}


/* Turn backplane clock on or off */
static int brcmf_sdio_htclk(bool on, bool pendok)
{
    int err;
    uint8_t clkctl, clkreq, devctl;
    unsigned long timeout;

    clkctl = 0;

    if (on) {
        /* Request HT Avail */
        clkreq = (bus->alp_only) ? SBSDIO_ALP_AVAIL_REQ : SBSDIO_HT_AVAIL_REQ;

        brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR,
                   clkreq, &err);
        if (err) {
            brcm_log("HT Avail request error: %d\n", err);
            return -EBADE;
        }

        /* Check current status */
        clkctl = brcmf_sdiod_readb(
                       SBSDIO_FUNC1_CHIPCLKCSR, &err);
        if (err) {
            brcm_log("HT Avail read error: %d\n", err);
            return -EBADE;
        }

        /* Go to pending and await interrupt if appropriate */
        if (!SBSDIO_CLKAV(clkctl, bus->alp_only) && pendok) {
            /* Allow only clock-available interrupt */
            devctl = brcmf_sdiod_readb(
                           SBSDIO_DEVICE_CTL, &err);
            if (err) {
                brcm_log("Devctl error setting CA: %d\n", err);
                return -EBADE;
            }

            devctl |= SBSDIO_DEVCTL_CA_INT_ONLY;
            brcmf_sdiod_writeb(SBSDIO_DEVICE_CTL,
                       devctl, &err);
            brcm_log("CLKCTL: set PENDING\n");
            bus->clkstate = CLK_PENDING;

            return 0;
        } else if (bus->clkstate == CLK_PENDING) {
            /* Cancel CA-only interrupt filter */
            devctl = brcmf_sdiod_readb(
                           SBSDIO_DEVICE_CTL, &err);
            devctl &= ~SBSDIO_DEVCTL_CA_INT_ONLY;
            brcmf_sdiod_writeb( SBSDIO_DEVICE_CTL,
                       devctl, &err);
        }

        /* Otherwise, wait here (polling) for HT Avail */
        timeout = 15;
        while (!SBSDIO_CLKAV(clkctl, bus->alp_only)) {
            clkctl = brcmf_sdiod_readb(
                           SBSDIO_FUNC1_CHIPCLKCSR,
                           &err);
            if (timeout-- == 0)
                break;
            else
                usleep(100000);
        }

        if (err) {
            brcm_log("HT Avail request error: %d\n", err);
            return -EBADE;
        }
        if (!SBSDIO_CLKAV(clkctl, bus->alp_only)) {
            brcm_log("HT Avail timeout (%d): clkctl 0x%02x\n",
                  PMU_MAX_TRANSITION_DLY, clkctl);
            return -EBADE;
        }

        /* Mark clock available */
        bus->clkstate = CLK_AVAIL;
        brcm_log("CLKCTL: turned ON\n");

        if (!bus->alp_only) {
            if (SBSDIO_ALPONLY(clkctl))
                brcm_log("HT Clock should be on\n");
        }

    } else {
        clkreq = 0;

        if (bus->clkstate == CLK_PENDING) {
            /* Cancel CA-only interrupt filter */
            devctl = brcmf_sdiod_readb(
                           SBSDIO_DEVICE_CTL, &err);
            devctl &= ~SBSDIO_DEVCTL_CA_INT_ONLY;
            brcmf_sdiod_writeb(SBSDIO_DEVICE_CTL,
                       devctl, &err);
        }

        bus->clkstate = CLK_SDONLY;
        brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR,
                   clkreq, &err);
        brcm_log("CLKCTL: turned OFF\n");
        if (err) {
            brcm_log("Failed access turning clock off: %d\n",
                  err);
            return -EBADE;
        }
    }
    return 0;
}


/* Transition SD and backplane clock readiness */
static int brcmf_sdio_clkctl(int target, bool pendok)
{
    /* Early exit if we're already there */
    if (bus->clkstate == target)
        return 0;

    switch (target) {
    case CLK_AVAIL:
        /* Make sure SD clock is available */
        if (bus->clkstate == CLK_NONE)
            brcmf_sdio_sdclk(true);
        /* Now request HT Avail on the backplane */
        brcmf_sdio_htclk(true, pendok);
        break;

    case CLK_SDONLY:
        /* Remove HT request, or bring up SD clock */
        if (bus->clkstate == CLK_NONE)
            brcmf_sdio_sdclk(true);
        else if (bus->clkstate == CLK_AVAIL)
            brcmf_sdio_htclk(false, false);
        else
            brcm_log("request for %d -> %d\n",
                  bus->clkstate, target);
        break;

    case CLK_NONE:
        /* Make sure to remove HT request */
        if (bus->clkstate == CLK_AVAIL)
            brcmf_sdio_htclk(false, false);
        /* Now remove the SD clock */
        brcmf_sdio_sdclk(false);
        break;
    }
    return 0;
}


static bool
brcmf_sdio_verifymemory(uint32_t ram_addr,
            const uint8_t *ram_data, uint ram_sz)
{
    uint8_t *ram_cmp;
    int err;
    bool ret = false;
    int address;
    unsigned int offset;
    unsigned int len;

    /* read back and verify */
    brcm_log("Compare RAM dl & ul at 0x%08x; size=%d\n", ram_addr,
          ram_sz);
    ram_cmp = (uint8_t *)malloc(2048);
    /* do not proceed while no memory but  */
    if (!ram_cmp)
        return true;

    address = ram_addr;
    offset = 0;
    while (offset < ram_sz) {
        len = ((offset + 2048) < ram_sz) ? 2048 :
              ram_sz - offset;
        err = brcmf_sdiod_ramrw(false, address, ram_cmp, len);
        if (err) {
            brcm_log("error %d on reading %d membytes at 0x%08x\n",
                  err, len, address);
            ret = false;
            break;
        } else if (memcmp(ram_cmp, &ram_data[offset], len)) {
            brcm_log("Downloaded RAM image is corrupted, block offset is %d, len is %d\n",
                  offset, len);
            ret = false;
            break;
        }
        offset += len;
        address += len;
    }
    if(offset == ram_sz){
        brcm_log("Verify success!\n");
        ret = true;
    }
    free(ram_cmp);

    return ret;
}

static int brcmf_sdio_download_code_file(const uint8_t *fw, int len)
{
    int err;

    err = brcmf_sdiod_ramrw(true, bus->ci->rambase,
                (uint8_t *)fw, len);
    if (err)
        brcm_log("error %d on writing %d membytes at 0x%08x\n",
              err, (int)fw, bus->ci->rambase);
    // else if (!brcmf_sdio_verifymemory(bus->ci->rambase,
    //                   fw, len))
    //     err = -EIO;

    return err;
}

static int brcmf_sdio_download_nvram(const uint8_t  *vars, int varsz)
{
    int address;
    int err;

    address = bus->ci->ramsize - varsz + bus->ci->rambase;
    err = brcmf_sdiod_ramrw(true, address, (uint8_t*)vars, varsz);
    if (err)
        brcm_log("error %d on writing %d nvram bytes at 0x%08x\n",
              err, varsz, address);
    else if (!brcmf_sdio_verifymemory(address, vars, varsz))
        err = -EIO;

    return err;
}


static int brcmf_sdio_download_firmware(uint8_t *fw, uint32_t len,  uint8_t *nvram, uint32_t nvlen)
{
    int bcmerror;
    uint32_t rstvec = *((uint32_t*)fw);

    brcmf_sdio_clkctl(CLK_AVAIL, false);
    brcm_log("download firmware to 0x%x rstvec: %x\n", bus->ci->rambase, rstvec); 

    bcmerror = brcmf_sdio_download_code_file(fw, len);
    if (bcmerror) {
        brcm_log("dongle image file download failed\n");
        goto err;
    }

    brcm_log("download nvram\n"); 
    bcmerror = brcmf_sdio_download_nvram(nvram, nvlen);
    if (bcmerror) {
        brcm_log("dongle nvram file download failed\n");
        goto err;
    }

    /* Take arm out of reset */
    if (!brcmf_chip_set_active(rstvec)) {
        brcm_log("error getting out of ARM core reset\n");
        bcmerror = -EIO;
        goto err;
    }

err:
    brcmf_sdio_clkctl(CLK_SDONLY, false);
    return bcmerror;
}

/*
 * Software allocation of To SB Mailbox resources
 */

/* tosbmailbox bits corresponding to intstatus bits */
#define SMB_NAK     (1 << 0)    /* Frame NAK */
#define SMB_INT_ACK (1 << 1)    /* Host Interrupt ACK */
#define SMB_USE_OOB (1 << 2)    /* Use OOB Wakeup */
#define SMB_DEV_INT (1 << 3)    /* Miscellaneous Interrupt */

/* tosbmailboxdata */
#define SMB_DATA_VERSION_SHIFT  16  /* host protocol version */

/*
 * Software allocation of To Host Mailbox resources
 */

/* intstatus bits */
#define I_HMB_FC_STATE  I_HMB_SW0   /* Flow Control State */
#define I_HMB_FC_CHANGE I_HMB_SW1   /* Flow Control State Changed */
#define I_HMB_FRAME_IND I_HMB_SW2   /* Frame Indication */
#define I_HMB_HOST_INT  I_HMB_SW3   /* Miscellaneous Interrupt */

/* tohostmailboxdata */
#define HMB_DATA_NAKHANDLED 0x0001  /* retransmit NAK'd frame */
#define HMB_DATA_DEVREADY   0x0002  /* talk to host after enable */
#define HMB_DATA_FC     0x0004  /* per prio flowcontrol update flag */
#define HMB_DATA_FWREADY    0x0008  /* fw ready for protocol activity */
#define HMB_DATA_FWHALT     0x0010  /* firmware halted */

#define HMB_DATA_FCDATA_MASK    0xff000000
#define HMB_DATA_FCDATA_SHIFT   24

#define HMB_DATA_VERSION_MASK   0x00ff0000
#define HMB_DATA_VERSION_SHIFT  16

static inline bool brcmf_sdio_valid_shared_address(uint32_t addr)
{
    return !(addr == 0 || ((~addr >> 16) & 0xffff) == (addr & 0xffff));
}

static bool brcmf_chip_sr_capable(){
    uint32_t pmu_cc3_mask = BIT(2);
    struct brcmf_core *pmu =  brcmf_chip_get_pmu();
    uint32_t addr = CORE_CC_REG(pmu->base, chipcontrol_addr);
    brcmf_sdiod_writel(addr, 3, NULL);
    addr = CORE_CC_REG(pmu->base, chipcontrol_data);
    uint32_t reg = brcmf_sdiod_readl(addr, NULL);
    return (reg & pmu_cc3_mask) != 0;
}

static int brcmf_sdio_readshared(struct sdpcm_shared *sh)
{
    uint32_t addr = 0;
    int rv;
    uint32_t shaddr = 0;
    struct sdpcm_shared_le sh_le;
    uint32_t addr_le;

    /*
     * Read last word in socram to determine
     * address of sdpcm_shared structure
     */
    shaddr = bus->ci->rambase + bus->ci->ramsize - 4;
    if (!bus->ci->rambase && brcmf_chip_sr_capable(bus->ci))
        shaddr -= bus->ci->srsize;
    rv = brcmf_sdiod_ramrw(false, shaddr, (uint8_t *)&addr_le, 4);
    if (rv < 0)
        goto fail;

    /*
     * Check if addr is valid.
     * NVRAM length at the end of memory should have been overwritten.
     */
    addr = le32_to_cpu(addr_le);
    if (!brcmf_sdio_valid_shared_address(addr)) {
        brcm_log("invalid sdpcm_shared address 0x%08X\n", addr);
        rv = -EINVAL;
        goto fail;
    }

    brcm_log("sdpcm_shared address 0x%08X\n", addr);

    /* Read hndrte_shared structure */
    rv = brcmf_sdiod_ramrw(false, addr, (uint8_t *)&sh_le, sizeof(struct sdpcm_shared_le));
    if (rv < 0)
        goto fail;

    /* Endianness */
    sh->flags = le32_to_cpu(sh_le.flags);
    sh->trap_addr = le32_to_cpu(sh_le.trap_addr);
    sh->assert_exp_addr = le32_to_cpu(sh_le.assert_exp_addr);
    sh->assert_file_addr = le32_to_cpu(sh_le.assert_file_addr);
    sh->assert_line = le32_to_cpu(sh_le.assert_line);
    sh->console_addr = le32_to_cpu(sh_le.console_addr);
    sh->msgtrace_addr = le32_to_cpu(sh_le.msgtrace_addr);

    if ((sh->flags & SDPCM_SHARED_VERSION_MASK) > SDPCM_SHARED_VERSION) {
        brcm_log("sdpcm shared version unsupported: dhd %d dongle %d\n",
              SDPCM_SHARED_VERSION,
              sh->flags & SDPCM_SHARED_VERSION_MASK);
        return -EPROTO;
    }
    return 0;

fail:
    brcm_log("unable to obtain sdpcm_shared info: rv=%d (addr=0x%x)\n",
          rv, addr);
    return rv;
}

static int brcmf_sdio_checkdied(void)
{
    int error;
    struct sdpcm_shared sh;

    error = brcmf_sdio_readshared(&sh);

    if (error < 0)
        return error;

    if ((sh.flags & SDPCM_SHARED_ASSERT_BUILT) == 0)
        brcm_log("firmware not built with -assert\n");
    else if (sh.flags & SDPCM_SHARED_ASSERT)
        brcm_log("assertion in dongle\n");

    if (sh.flags & SDPCM_SHARED_TRAP) {
        brcm_log("firmware trap in dongle\n");
        //brcmf_sdio_trap_info(NULL, bus, &sh);
    }

    return 0;
}

int brcmf_sdiod_recv_pkt(struct sk_buff *pkt)
{
    uint32_t addr = bus->cc_core->base;
    int err = 0;

    err = brcmf_sdiod_set_backplane_window(addr);
    if (err)
        return err;

    addr &= SBSDIO_SB_OFT_ADDR_MASK;
    addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;

    err = brcmf_sdiod_skbuff_read(2, addr, pkt);

    return  err;
}

int brcmf_sdiod_recv_buf(uint8_t *buf, uint nbytes)
{
    struct sk_buff *mypkt;
    int err;

    mypkt = skb_alloc(nbytes);
    if (!mypkt) {
        brcm_log("brcmu_pkt_buf_get_skb failed: len %d\n",
              nbytes);
        return -EIO;
    }
    skb_put(mypkt, nbytes);

    err = brcmf_sdiod_recv_pkt(mypkt);
    if (!err)
        memcpy(buf, mypkt->data, nbytes);

    skb_free(mypkt);
    return err;
}

static void brcmf_sdio_rxfail(bool abort, bool rtx)
{
    struct brcmf_core *core = bus->sdio_core;
    uint retries = 0;
    uint16_t lastrbc;
    uint8_t hi, lo;
    int err;

    brcm_log("%sterminate frame%s\n",
          abort ? "abort command, " : "",
          rtx ? ", send NAK" : "");

    if (abort)
        brcmf_sdiod_func0_wb(SDIO_CCCR_ABORT, 2, NULL);

    brcmf_sdiod_writeb(SBSDIO_FUNC1_FRAMECTRL, 1, &err);

    /* Wait until the packet has been flushed (device/FIFO stable) */
    for (lastrbc = retries = 0xffff; retries > 0; retries--) {
        hi = brcmf_sdiod_readb(SBSDIO_FUNC1_RFRAMEBCHI, &err);
        lo = brcmf_sdiod_readb(SBSDIO_FUNC1_RFRAMEBCLO, &err);

        if ((hi == 0) && (lo == 0))
            break;

        if ((hi > (lastrbc >> 8)) && (lo > (lastrbc & 0x00ff))) {
            brcm_log("count growing: last 0x%04x now 0x%04x\n",
                  lastrbc, (hi << 8) + lo);
        }
        lastrbc = (hi << 8) + lo;
    }

    if (!retries)
        brcm_log("count never zeroed: last 0x%04x\n", lastrbc);
    else
        brcm_log("flush took %d iterations\n", 0xffff - retries);

    if (rtx) {
        brcmf_sdiod_writel(core->base + SD_REG(tosbmailbox),
                   SMB_NAK, &err);

        if (err == 0)
            bus->rxskip = true;
    }

    /* Clear partial in any case */
    bus->cur_read.len = 0;
}

extern void
hexdump(const char* lable, const void *data, size_t size);

static int brcmf_sdio_hdparse(uint8_t *header,
                  struct brcmf_sdio_hdrinfo *rd,
                  enum brcmf_sdio_frmtype type)
{
    uint16_t len, checksum;
    uint8_t rx_seq, fc, tx_seq_max;
    uint32_t swheader;

    //hexdump("packet header", header, 64);

    /* hw header */
    len = get_unaligned_le16(header);
    checksum = get_unaligned_le16(header + sizeof(uint16_t));
    /* All zero means no more to read */
    if (!(len | checksum)) {
        bus->rxpending = false;
        return -ENODATA;
    }
    if ((uint16_t)(~(len ^ checksum))) {
        brcm_log("HW header checksum error\n");
        brcmf_sdio_rxfail(false, false);
        return -EIO;
    }
    if (len < SDPCM_HDRLEN) {
        brcm_log("HW header length error\n");
        return -EPROTO;
    }
    if (type == BRCMF_SDIO_FT_SUPER &&
        (roundup(len, bus->blocksize) != rd->len)) {
        brcm_log("HW superframe header length error\n");
        return -EPROTO;
    }
    if (type == BRCMF_SDIO_FT_SUB && len > rd->len) {
        brcm_log("HW subframe header length error\n");
        return -EPROTO;
    }
    rd->len = len;

    /* software header */
    header += SDPCM_HWHDR_LEN;
    swheader = le32_to_cpu(*(__le32 *)header);
    if (type == BRCMF_SDIO_FT_SUPER && SDPCM_GLOMDESC(header)) {
        brcm_log("Glom descriptor found in superframe head\n");
        rd->len = 0;
        return -EINVAL;
    }
    rx_seq = (uint8_t)(swheader & SDPCM_SEQ_MASK);
    rd->channel = (swheader & SDPCM_CHANNEL_MASK) >> SDPCM_CHANNEL_SHIFT;
    if (len > MAX_RX_DATASZ && rd->channel != SDPCM_CONTROL_CHANNEL &&
        type != BRCMF_SDIO_FT_SUPER) {
        brcm_log("HW header length too long\n");
        brcmf_sdio_rxfail(false, false);
        rd->len = 0;
        return -EPROTO;
    }
    if (type == BRCMF_SDIO_FT_SUPER && rd->channel != SDPCM_GLOM_CHANNEL) {
        brcm_log("Wrong channel for superframe\n");
        rd->len = 0;
        return -EINVAL;
    }
    if (type == BRCMF_SDIO_FT_SUB && rd->channel != SDPCM_DATA_CHANNEL &&
        rd->channel != SDPCM_EVENT_CHANNEL) {
        brcm_log("Wrong channel for subframe\n");
        rd->len = 0;
        return -EINVAL;
    }
    rd->dat_offset = brcmf_sdio_getdatoffset(header);
    if (rd->dat_offset < SDPCM_HDRLEN || rd->dat_offset > rd->len) {
        brcm_log("seq %d: bad data offset\n", rx_seq);
        brcmf_sdio_rxfail(false, false);
        rd->len = 0;
        return -ENXIO;
    }
    if (rd->seq_num != rx_seq) {
        brcm_log("seq %d, expected %d\n", rx_seq, rd->seq_num);
        rd->seq_num = rx_seq;
    }
    /* no need to check the reset for subframe */
    if (type == BRCMF_SDIO_FT_SUB)
        return 0;
    rd->len_nxtfrm = (swheader & SDPCM_NEXTLEN_MASK) >> SDPCM_NEXTLEN_SHIFT;
    if (rd->len_nxtfrm << 4 > MAX_RX_DATASZ) {
        /* only warm for NON glom packet */
        if (rd->channel != SDPCM_GLOM_CHANNEL)
            brcm_log("seq %d: next length error\n", rx_seq);
        rd->len_nxtfrm = 0;
    }
    swheader = le32_to_cpu(*(__le32 *)(header + 4));
    fc = swheader & SDPCM_FCMASK_MASK;
    if (bus->flowcontrol != fc) {
        if (~bus->flowcontrol & fc)
        if (bus->flowcontrol & ~fc)
        bus->flowcontrol = fc;
    }
    tx_seq_max = (swheader & SDPCM_WINDOW_MASK) >> SDPCM_WINDOW_SHIFT;
    if ((uint8_t)(tx_seq_max - bus->tx_seq) > 0x40) {
        brcm_log("seq %d: max tx seq number error\n", rx_seq);
        tx_seq_max = bus->tx_seq + 2;
    }
    bus->tx_max = tx_seq_max;
    //brcm_log("recv hdr: len:%d seq:%d ch:%d off:%d next:%d max:%d\n", len, rx_seq, rd->channel, rd->dat_offset, rd->len_nxtfrm, tx_seq_max);
    //recv hdr: len:12 seq:0 ch:1 off:12 next:0 max:25
    return 0;
}

/* Pad read to blocksize for efficiency */
static void brcmf_sdio_pad(uint16_t *pad, uint16_t *rdlen)
{
    if (bus->blocksize && *rdlen > bus->blocksize) {
        *pad = bus->blocksize - (*rdlen % bus->blocksize);
        if (*pad <= bus->blocksize && *pad < bus->blocksize &&
            *rdlen + *pad + BRCMF_FIRSTREAD < MAX_RX_DATASZ)
            *rdlen += *pad;
    } else if (*rdlen % bus->head_align) {
        *rdlen += bus->head_align - (*rdlen % bus->head_align);
    }
}

static void pkt_align(struct sk_buff *p, int len, int align)
{
    uint datalign;
    datalign = (unsigned long)(p->data);
    datalign = roundup(datalign, (align)) - datalign;
    if (datalign)
        skb_pull(p, datalign);
    skb_trim(p, len);
}

#define MAXCTL 1024*9

static void
brcmf_sdio_read_control(uint8_t *hdr, int len, int doff)
{
    int rdlen, pad, sdret;
    uint8_t *buf = bus->rxctl;

    //TODO: 
    //for debug
    memset(buf, 0, len);
    /* Copy the already-read portion over */
    memcpy(buf, hdr, BRCMF_FIRSTREAD);
    if (len <= BRCMF_FIRSTREAD)
        goto gotpkt;

    /* Raise rdlen to next SDIO block to avoid tail command */
    rdlen = len - BRCMF_FIRSTREAD;
    if (bus->roundup && bus->blocksize && (rdlen > bus->blocksize)) {
        pad = bus->blocksize - (rdlen % bus->blocksize);
        if ((pad <= bus->roundup) && (pad < bus->blocksize) &&
            ((len + pad) < MAXCTL))
            rdlen += pad;
    } else if (rdlen % bus->head_align) {
        rdlen += bus->head_align - (rdlen % bus->head_align);
    }

    /* Drop if the read is too big or it exceeds our maximum */
    if ((rdlen + BRCMF_FIRSTREAD) > MAXCTL) {
        brcm_log("%d-byte control read exceeds %d-byte buffer\n",
              rdlen, MAXCTL);
        brcmf_sdio_rxfail(false, false);
        goto done;
    }

    if ((len - doff) > MAXCTL) {
        brcm_log("%d-byte ctl frame (%d-byte ctl data) exceeds %d-byte limit\n",
              len, len - doff, MAXCTL);
        brcmf_sdio_rxfail(false, false);
        goto done;
    }

    /* Read remain of frame body */
    sdret = brcmf_sdiod_recv_buf(buf + BRCMF_FIRSTREAD, rdlen);

    /* Control frame failures need retransmission */
    if (sdret < 0) {
        brcm_log("read %d control bytes failed: %d\n",
              rdlen, sdret);
        brcmf_sdio_rxfail(true, true);
        goto done;
    } //else
        //memcpy(buf + BRCMF_FIRSTREAD, rbuf, rdlen);

gotpkt:
    /* Point to valid data and indicate its length */
    if (bus->rxlen) {
        brcm_log("last control frame is being processed.\n");
        goto done;
    }
    bus->rxlen = len;
    bus->rxdoff = doff;

    //brcm_log("read control pack len:%d off:%d\n", len, doff);
done:
    return;
}

struct brcmf_event_msg {
    uint16_t version;
    uint16_t flags;
    uint32_t event_code;
    uint32_t status;
    uint32_t reason;
    s32 auth_type;
    uint32_t datalen;
    uint8_t addr[6];
    char ifname[16];
    uint8_t ifidx;
    uint8_t bsscfgidx;
};

struct ethhdr {
    unsigned char   h_dest[6];   /* destination eth addr */
    unsigned char   h_source[6]; /* source ether addr    */
    __be16      h_proto;        /* packet type ID field */
} __attribute__((packed));

struct brcm_ethhdr {
    __be16 subtype;
    __be16 length;
    uint8_t version;
    uint8_t oui[3];
    __be16 usr_subtype;
} __attribute__((packed));

struct brcmf_event_msg_be {
    __be16 version;
    __be16 flags;
    __be32 event_type;
    __be32 status;
    __be32 reason;
    __be32 auth_type;
    __be32 datalen;
    uint8_t addr[6];
    char ifname[16];
    uint8_t ifidx;
    uint8_t bsscfgidx;
} __attribute__((packed));

/**
 * struct brcmf_event - contents of broadcom event packet.
 *
 * @eth: standard ether header.
 * @hdr: broadcom specific ether header.
 * @msg: common part of the actual event message.
 */
struct brcmf_event {
    struct ethhdr eth;
    struct brcm_ethhdr hdr;
    struct brcmf_event_msg_be msg;
} __attribute__((packed));

struct brcmf_bss_info_le {
    __le32 version;     /* version field */
    __le32 length;      /* byte length of data in this record,
                 * starting at version and including IEs
                 */
    uint8_t BSSID[6];
    __le16 beacon_period;   /* units are Kusec */
    __le16 capability;  /* Capability information */
    uint8_t SSID_len;
    const char SSID[32];
    struct {
        __le32 count;   /* # rates in this set */
        uint8_t rates[16]; /* rates in 500kbps units w/hi bit set if basic */
    } rateset;      /* supported rates */
    __le16 chanspec;    /* chanspec for bss */
    __le16 atim_window; /* units are Kusec */
    uint8_t dtim_period; /* DTIM period */
    __le16 RSSI;        /* receive signal strength (in dBm) */
    s8 phy_noise;       /* noise (in dBm) */

    uint8_t n_cap;       /* BSS is 802.11N Capable */
    /* 802.11N BSS Capabilities (based on HT_CAP_*): */
    __le32 nbss_cap;
    uint8_t ctl_ch;      /* 802.11N BSS control channel number */
    __le32 reserved32[1];   /* Reserved for expansion of BSS properties */
    uint8_t flags;       /* flags */
    uint8_t reserved[3]; /* Reserved for expansion of BSS properties */
    uint8_t basic_mcs[16]; /* 802.11N BSS required MCS set */

    __le16 ie_offset;   /* offset at which IEs start, from beginning */
    __le32 ie_length;   /* byte length of Information Elements */
    __le16 SNR;     /* average SNR of during frame reception */
    /* Add new fields here */
    /* variable length Information Elements */
};

struct brcmf_scan_results {
    uint32_t buflen;
    uint32_t version;
    uint32_t count;
    struct brcmf_bss_info_le bss_info_le[];
};

struct brcmf_escan_result_le {
    __le32 buflen;
    __le32 version;
    __le16 sync_id;
    __le16 bss_count;
    struct brcmf_bss_info_le bss_info_le;
};

static void scan_result(struct brcmf_bss_info_le *info){

    bus->scan_update = kernel_tic_ms(0);
    int idx = config_match_ssid(info->SSID);
        
    if(idx >= 0){
        if(strlen(bus->ssid) > 0){
            int old = config_match_ssid(bus->ssid);
            if(config_get_priority(idx) >= config_get_priority(old))
                return;
        }
        brcm_log("match ssid:%s\n", info->SSID);
        strncpy(bus->ssid, info->SSID, sizeof(bus->ssid) - 1);
    } 
}


void brcmf_rx_event( struct sk_buff *skb)
{
    struct brcmf_event *event;
    struct brcmf_event_msg emsg;

    event = (struct brcmf_event *)(skb->data + 4); 
    struct brcmf_event_msg *emsg_be = (struct brcmf_event_msg *)&(event->msg);

    emsg.version = be16_to_cpu(emsg_be->version);
    emsg.flags = be16_to_cpu(emsg_be->flags);
    emsg.event_code = be16_to_cpu(emsg_be->event_code);
    emsg.status = be32_to_cpu(emsg_be->status);
    emsg.reason = be32_to_cpu(emsg_be->reason);
    emsg.auth_type = be32_to_cpu(emsg_be->auth_type);
    emsg.datalen = be32_to_cpu(emsg_be->datalen);
    memcpy(emsg.addr, emsg_be->addr, 6);
    memcpy(emsg.ifname, emsg_be->ifname, sizeof(emsg.ifname));
    emsg.ifidx = emsg_be->ifidx;
    emsg.bsscfgidx = emsg_be->bsscfgidx;

    uint32_t event_type = be32_to_cpu(event->msg.event_type);
    uint32_t event_status = be32_to_cpu(event->msg.status); 
    if(event_type == 69){
        //parse scan result;
        struct brcmf_escan_result_le *result = (struct brcmf_escan_result_le *)(skb->data + 4 + sizeof( struct brcmf_event));
        for(int i = 0; i < result->bss_count; i++){
           struct brcmf_bss_info_le *info = &((struct brcmf_bss_info_le *)&(result->bss_info_le))[i];
           scan_result(info);
        }
    }else if(event_type == 0 && event_status == 0){
        brcm_log("Event: link up\n"); 
        bus->state = CONNECTED;
        proc_wakeup(1);
    }else{
        brcm_log("Event: %d\n", event_type);
    }
    skb_free(skb);
}

void brcmf_rx_frame(struct sk_buff *skb)
{
    //remove 4byte head
    skb_pull(skb, 4);
    ipc_disable();
    queue_buffer_push(bus->rx_queue, skb->data, skb->len);
    ipc_enable();
    proc_wakeup(1);
    skb_free(skb);
}

static uint brcmf_sdio_readframes(uint maxframes)
{
    struct sk_buff *pkt;        /* Packet for event or data frames */
    uint16_t pad;        /* Number of pad bytes to read */
    uint rxleft = 0;    /* Remaining number of frames allowed */
    int ret;        /* Return code from calls */
    uint rxcount = 0;   /* Total frames read */
    struct brcmf_sdio_hdrinfo *rd = &bus->cur_read, rd_new;
    uint8_t head_read = 0;



    /* Not finished unless we encounter no more frames indication */
    bus->rxpending = true;

    for (rd->seq_num = bus->rx_seq, rxleft = maxframes;
         !bus->rxskip && rxleft;
         rd->seq_num++, rxleft--) {

        rd->len_left = rd->len;
        /* read header first for unknow frame length */
        if (!rd->len) {
            ret = brcmf_sdiod_recv_buf(
                           bus->rxhdr, BRCMF_FIRSTREAD);
            if (ret < 0) {
                brcm_log("RXHEADER FAILED: %d\n", ret);
                brcmf_sdio_rxfail(true, true);
                continue;
            }


            if (brcmf_sdio_hdparse(bus->rxhdr, rd,
                           BRCMF_SDIO_FT_NORMAL)) {
                if (!bus->rxpending)
                    break;
                else
                    continue;
            }

            if (rd->channel == SDPCM_CONTROL_CHANNEL) {
                brcmf_sdio_read_control(bus->rxhdr,
                            rd->len,
                            rd->dat_offset);
                /* prepare the descriptor for the next read */
                rd->len = rd->len_nxtfrm << 4;
                rd->len_nxtfrm = 0;
                /* treat all packet as event if we don't know */
                rd->channel = SDPCM_EVENT_CHANNEL;
                continue;
            }
            rd->len_left = rd->len > BRCMF_FIRSTREAD ?
                       rd->len - BRCMF_FIRSTREAD : 0;
            head_read = BRCMF_FIRSTREAD;
        }

        brcmf_sdio_pad(&pad, &rd->len_left);

        pkt = skb_alloc(rd->len_left + head_read + ALIGNMENT);
        if (!pkt) {
            /* Give up on data, request rtx of events */
            brcm_log("brcmu_pkt_buf_get_skb failed\n");
            brcmf_sdio_rxfail(false,
                        RETRYCHAN(rd->channel));
            continue;
        }
        skb_put(pkt, rd->len_left + head_read + ALIGNMENT);
        skb_pull(pkt, head_read);
        pkt_align(pkt, rd->len_left, ALIGNMENT);
        ret = brcmf_sdiod_recv_pkt(pkt);

        if (ret < 0) {
            brcm_log("read %d bytes from channel %d failed: %d\n",
                  rd->len, rd->channel, ret);
            skb_free(pkt);
            brcmf_sdio_rxfail(true,
                        RETRYCHAN(rd->channel));
            continue;
        }

        if (head_read) {
            skb_push(pkt, head_read);
            memcpy(pkt->data, bus->rxhdr, head_read);
            head_read = 0;
        } else {
            memcpy(bus->rxhdr, pkt->data, SDPCM_HDRLEN);
            rd_new.seq_num = rd->seq_num;
            if (brcmf_sdio_hdparse(bus->rxhdr, &rd_new,
                           BRCMF_SDIO_FT_NORMAL)) {
                rd->len = 0;
                brcmf_sdio_rxfail(true, true);
                skb_free(pkt);
                continue;
            }
            if (rd->len != roundup(rd_new.len, 16)) {
                brcm_log("frame length mismatch:read %d, should be %d\n",
                      rd->len,
                      roundup(rd_new.len, 16) >> 4);
                rd->len = 0;
                brcmf_sdio_rxfail(true, true);
                skb_free(pkt);
                continue;
            }
            rd->len_nxtfrm = rd_new.len_nxtfrm;
            rd->channel = rd_new.channel;
            rd->dat_offset = rd_new.dat_offset;

            if (rd_new.channel == SDPCM_CONTROL_CHANNEL) {
                brcm_log("readahead on control packet %d?\n",
                      rd_new.seq_num);
                /* Force retry w/normal header read */
                rd->len = 0;
                brcmf_sdio_rxfail(false, true);
                skb_free(pkt);
                continue;
            }
        }

        skb_trim(pkt, rd->len);
        skb_pull(pkt, rd->dat_offset);
        if (pkt->len == 0)
            skb_free(pkt);
        else if (rd->channel == SDPCM_EVENT_CHANNEL)
            brcmf_rx_event(pkt);
        else
            brcmf_rx_frame(pkt);

        /* prepare the descriptor for the next read */
        rd->len = rd->len_nxtfrm << 4;
        rd->len_nxtfrm = 0;
        /* treat all packet as event if we don't know */
        rd->channel = SDPCM_EVENT_CHANNEL;
    }

    rxcount = maxframes - rxleft;
    /* Message if we hit the limit */
    if (!rxleft)
        brcm_log("hit rx limit of %d frames\n", maxframes);
    // else
    //     brcm_log("processed %d frames\n", rxcount);
    /* Back off rxseq if awaiting rtx, update rx_seq */
    if (bus->rxskip)
        rd->seq_num--;
    bus->rx_seq = rd->seq_num;

    return rxcount;
}


static uint32_t brcmf_sdio_hostmail(void)
{
    struct brcmf_core *core = bus->sdio_core;
    uint32_t intstatus = 0;
    uint32_t hmb_data;
    int ret;

    /* Read mailbox data and ack that we did so */
    hmb_data = brcmf_sdiod_readl(core->base + SD_REG(tohostmailboxdata),&ret);

    if (!ret)
        brcmf_sdiod_writel(core->base + SD_REG(tosbmailbox), SMB_INT_ACK, &ret);

    //bus->sdcnt.f1regdata += 2;

    /* dongle indicates the firmware has halted/crashed */
    if (hmb_data & HMB_DATA_FWHALT) {
        brcm_log("mailbox indicates firmware halted\n");
    }

    /* Dongle recomposed rx frames, accept them again */
    if (hmb_data & HMB_DATA_NAKHANDLED) {
        brcm_log("Dongle reports NAK handled, expect rtx of %d\n", bus->rx_seq);
        if (!bus->rxskip)
            brcm_log("unexpected NAKHANDLED!\n");

        bus->rxskip = false;
        intstatus |= I_HMB_FRAME_IND;
    }

    /*
     * DEVREADY does not occur with gSPI.
     */
    if (hmb_data & (HMB_DATA_DEVREADY)) {
        uint32_t sdpcm_ver = (hmb_data & HMB_DATA_VERSION_MASK) >> HMB_DATA_VERSION_SHIFT;
        brcm_log("Dongle ready, protocol version %d\n", sdpcm_ver);
        struct sdpcm_shared sh;
        if (brcmf_sdio_readshared(&sh) == 0){
            brcm_console_init(sh.console_addr);
        }
    }

    /* Shouldn't be any others */
    if (hmb_data & ~(HMB_DATA_DEVREADY |
             HMB_DATA_NAKHANDLED |
             HMB_DATA_FC |
             HMB_DATA_FWREADY |
             HMB_DATA_FWHALT |
             HMB_DATA_FCDATA_MASK | HMB_DATA_VERSION_MASK))
        brcm_log("Unknown mailbox data content: 0x%02x\n",
              hmb_data);

    return intstatus;
}

/* To check if there's window offered */
static bool txctl_ok(void)
{
    return true;
    // static int cnt = 0;
    // cnt++;
    // if(cnt % 20 == 0)
    //     return true;
    // else
    //     return false;
    // return (bus->tx_max - bus->tx_seq) != 0 &&
    //        ((bus->tx_max - bus->tx_seq) & 0x80) == 0;
}

static inline void brcmf_sdio_update_hwhdr(uint8_t *header, uint16_t frm_length)
{
    *(__le16 *)header = cpu_to_le16(frm_length);
    *(((__le16 *)header) + 1) = cpu_to_le16(~frm_length);
}

static void brcmf_sdio_hdpack(uint8_t *header,
                  struct brcmf_sdio_hdrinfo *hd_info)
{
    uint32_t hdrval;
    uint8_t hdr_offset;

    brcmf_sdio_update_hwhdr(header, hd_info->len);
    hdr_offset = SDPCM_HWHDR_LEN;
 
    hdrval = hd_info->seq_num;
    hdrval |= (hd_info->channel << SDPCM_CHANNEL_SHIFT) &
          SDPCM_CHANNEL_MASK;
    hdrval |= (hd_info->dat_offset << SDPCM_DOFFSET_SHIFT) &
          SDPCM_DOFFSET_MASK;
    *((__le32 *)(header + hdr_offset)) = cpu_to_le32(hdrval);
    *(((__le32 *)(header + hdr_offset)) + 1) = 0;
}

int brcmf_sdiod_send_buf(uint8_t *buf, uint nbytes)
{
    struct sk_buff *mypkt;
    uint32_t addr = bus->cc_core->base;
    int err;

    mypkt = skb_alloc(nbytes);
    if (!mypkt) {
        brcm_log("brcmu_pkt_buf_get_skb failed: len %d\n",
              nbytes);
        return -EIO;
    }

    skb_put(mypkt, nbytes);
    memcpy(mypkt->data, buf, nbytes);

    err = brcmf_sdiod_set_backplane_window(addr);
    if (err)
        goto out;

    addr &= SBSDIO_SB_OFT_ADDR_MASK;
    addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;

    err = brcmf_sdiod_skbuff_write(2, addr, mypkt);
out:
    skb_free(mypkt);

    return err;
}

static void brcmf_sdio_txfail(void)
{
    uint8_t i, hi, lo;

    /* On failure, abort the command and terminate the frame */
    brcm_log("sdio error, abort command and terminate frame\n");

    brcmf_sdiod_func0_wb(SDIO_CCCR_ABORT, 2, NULL);
    brcmf_sdiod_writeb(SBSDIO_FUNC1_FRAMECTRL, 0x2, NULL);

    for (i = 0; i < 3; i++) {
        hi = brcmf_sdiod_readb(SBSDIO_FUNC1_WFRAMEBCHI, NULL);
        lo = brcmf_sdiod_readb(SBSDIO_FUNC1_WFRAMEBCLO, NULL);
        if ((hi == 0) && (lo == 0))
            break;
    }
}

static int brcmf_sdio_tx_ctrlframe(uint8_t *frame, uint16_t len)
{
    uint8_t doff;
    uint16_t pad;
    uint retries = 0;
    struct brcmf_sdio_hdrinfo hd_info = {0};
    int ret;

    /* Back the pointer to make room for bus header */
    frame -= SDPCM_HWHDR_LEN + SDPCM_SWHDR_LEN;
    len += SDPCM_HWHDR_LEN + SDPCM_SWHDR_LEN;

    /* Add alignment padding (optional for ctl frames) */
    doff = ((unsigned long)frame % ALIGNMENT);
    if (doff) {
        frame -= doff;
        len += doff;
        memset(frame + SDPCM_HWHDR_LEN + SDPCM_SWHDR_LEN, 0, doff);
    }

    /* Round send length to next SDIO block */
    pad = 0;
    if (bus->roundup && bus->blocksize && (len > bus->blocksize)) {
        pad = bus->blocksize - (len % bus->blocksize);
        if ((pad > bus->roundup) || (pad >= bus->blocksize))
            pad = 0;
    } else if (len % ALIGNMENT) {
        pad = ALIGNMENT - (len % ALIGNMENT);
    }
    len += pad;

    hd_info.len = len - pad;
    hd_info.channel = SDPCM_CONTROL_CHANNEL;
    hd_info.dat_offset = doff + SDPCM_HWHDR_LEN + SDPCM_SWHDR_LEN;
    hd_info.seq_num = bus->tx_seq;
    hd_info.lastfrm = true;
    hd_info.tail_pad = pad;
    brcmf_sdio_hdpack(frame, &hd_info);

    do {
        ret = brcmf_sdiod_send_buf(frame, len);

        if (ret < 0)
            brcmf_sdio_txfail();
        else
            bus->tx_seq = (bus->tx_seq + 1) % SDPCM_SEQ_WRAP;
    } while (ret < 0 && retries++ < TXRETRIES);

    return ret;
}

static int
brcmf_sdio_txpkt_prep(struct sk_buff *pkt, uint chan)
{
    uint8_t txseq;

    struct brcmf_sdio_hdrinfo hd_info = {0};
    skb_push(pkt, 16);
    txseq = bus->tx_seq;
    hd_info.len = pkt->len;
    hd_info.lastfrm = true;
    hd_info.channel = chan;
    hd_info.dat_offset = 12;
    hd_info.seq_num = txseq++;
    ((uint32_t*)pkt->data)[3] = 0x20;
    brcmf_sdio_hdpack(pkt->data, &hd_info);
    return 0;
}

static int brcmf_sdio_intr_rstatus(void)
{
    struct brcmf_core *core = bus->sdio_core;
    uint32_t addr;
    unsigned long val;
    int ret = 0;

    addr = core->base + SD_REG(intstatus);

    val = brcmf_sdiod_readl(addr, &ret);
    if (ret != 0)
        return ret;

    val &= HOSTINTMASK;
    bus->fcstate = !!(val & I_HMB_FC_STATE);

    /* Clear interrupts */
    if (val) {
        brcmf_sdiod_writel(addr, val, &ret);
        bus->intstatus |= val;
    }

    return ret;
}

int brcmf_sdiod_send_pkt(struct sk_buff* pkt)
{
    uint32_t addr = bus->cc_core->base;
    int err;

    //brcm_log("send addr = 0x%x, size = %d\n", addr, pkt->len);

    err = brcmf_sdiod_set_backplane_window(addr);
    if (err)
        return err;

    addr &= SBSDIO_SB_OFT_ADDR_MASK;
    addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;


    return brcmf_sdiod_skbuff_write(2, addr, pkt);
    return err;
}


static void brcmf_sdio_dpc(void)
{
    uint32_t newstatus = 0;
    uint32_t intstat_addr = bus->sdio_core->base + SD_REG(intstatus);
    uint32_t intstatus;
    int err = 0;
    err = brcmf_sdio_intr_rstatus();
    intstatus = bus->intstatus;

    /* Handle flow-control change: read new state in case our ack
     * crossed another change interrupt.  If change still set, assume
     * FC ON for safety, let next loop through do the debounce.
     */
    if (intstatus & I_HMB_FC_CHANGE) {
        intstatus &= ~I_HMB_FC_CHANGE;
        brcmf_sdiod_writel(intstat_addr, I_HMB_FC_CHANGE, &err);

        newstatus = brcmf_sdiod_readl(intstat_addr, &err);

        bus->fcstate = !!(newstatus & (I_HMB_FC_STATE | I_HMB_FC_CHANGE));
        intstatus |= (newstatus & bus->hostintmask);
    }
    /* Handle host mailbox indication */
    if (intstatus & I_HMB_HOST_INT) {
        intstatus &= ~I_HMB_HOST_INT;
        intstatus |= brcmf_sdio_hostmail();
    }
    /* Generally don't ask for these, can get CRC errors... */
    if (intstatus & I_WR_OOSYNC) {
        brcm_log("Dongle reports WR_OOSYNC\n");
        intstatus &= ~I_WR_OOSYNC;
    }
    if (intstatus & I_RD_OOSYNC) {
        brcm_log("Dongle reports RD_OOSYNC\n");
        intstatus &= ~I_RD_OOSYNC;
    }
    if (intstatus & I_SBINT) {
        brcm_log("Dongle reports SBINT\n");
        intstatus &= ~I_SBINT;
    }
    /* Would be active due to wake-wlan in gSPI */
    if (intstatus & I_CHIPACTIVE) {
        brcm_log("Dongle reports CHIPACTIVE\n");
        intstatus &= ~I_CHIPACTIVE;
    }
    /* Ignore frame indications if rxskip is set */
    if (bus->rxskip)
        intstatus &= ~I_HMB_FRAME_IND;
    /* On frame indication, read available frames */
    if ((intstatus & I_HMB_FRAME_IND) && (bus->clkstate == CLK_AVAIL)) {
        brcmf_sdio_readframes(20);
        if (!bus->rxpending)
            intstatus &= ~I_HMB_FRAME_IND;
    }
    //brcmf_sdio_clrintr(bus);
    bus->intstatus = intstatus;
    //brcmf_sdiod_writel(intstat_addr, intstatus, &err);

    if (bus->ctrl_frame_stat && (bus->clkstate == CLK_AVAIL) && txctl_ok()) {
        if (bus->ctrl_frame_stat) {
            err = brcmf_sdio_tx_ctrlframe(bus->ctrl_frame_buf, bus->ctrl_frame_len);
            bus->ctrl_frame_err = err;
            bus->ctrl_frame_stat = false;
            if (err)
                brcm_log("sdio ctrlframe tx failed err=%d\n", err);
        }
    }
    /* Send queued frames */
    int max_frames = 20;
    while(bus->state == CONNECTED && queue_buffer_check(bus->tx_queue) && max_frames--){
        struct sk_buff *pkt = skb_alloc(MAX_FRAME_SIZE);
        ipc_disable();
        int len = queue_buffer_pop(bus->tx_queue, pkt->data, MAX_FRAME_SIZE);
        ipc_enable();
        proc_wakeup(1);
        skb_put(pkt, len);
        brcmf_sdio_txpkt_prep(pkt, 2);
        int ret = brcmf_sdiod_send_pkt(pkt);
        if (ret < 0)
            brcmf_sdio_txfail();
        bus->tx_seq++;
        skb_free(pkt);
    }
}

int brcmf_sdio_bus_txctl(unsigned char *msg, uint msglen)
{
	int ret = 0;

	/* Send from dpc */
	bus->ctrl_frame_buf = msg;
	bus->ctrl_frame_len = msglen;
	bus->ctrl_frame_stat = true;
    bus->rxlen = 0;

	int timeout = 50;
	while (bus->ctrl_frame_stat && timeout--){
        brcmf_sdio_dpc();
        usleep(1000);
    } 

    if(bus->ctrl_frame_stat){
		brcm_log("ctrl_frame timeout\n");
		bus->ctrl_frame_stat = false;
	}else{
		ret = bus->ctrl_frame_err;
	}
	return ret;
}

int brcmf_sdio_bus_rxctl(unsigned char *msg, uint msglen)
{
    int timeleft = 1000;
    uint rxlen = 0;
    /* Wait until control frame is available */
    //timeleft = brcmf_sdio_dcmd_resp_wait(bus, &bus->rxlen, &pending);
    while(!bus->rxlen || timeleft--){
        usleep(1000);
        brcmf_sdio_dpc();
    };

    if(bus->rxlen){
        rxlen = bus->rxlen - bus->rxdoff;
        memcpy(msg, bus->rxctl + bus->rxdoff, min(msglen, rxlen));
        bus->rxlen = 0;
    }

    if (timeleft == 0) {
        brcm_log("resumed on timeout\n");
        brcmf_sdio_checkdied();
    } else if(!rxlen) {
        brcm_log("resumed for unknown reason?\n");
        brcmf_sdio_checkdied();
    }

    return rxlen ? (int)rxlen : -ETIMEDOUT;
}



int brcmf_sdiod_probe(void){
    int ret = 0, err = 0;
    bus = malloc(sizeof(struct brcmf_dev));
    memset(bus, 0, sizeof(struct brcmf_dev));

#if 0
    brcm_dummy_read(0x0, 1);
    brcm_dummy_read(0x8, 1);
    brcm_dummy_read(0x12, 2);
    brcm_dummy_read(0x9, 3);
    brcm_dummy_read(0x10ac, 0xd5);
#endif
    uint8_t val = sdio_readb(0, 0x13, &ret);
    if (ret) {
        brcm_log("Failed %d\n", ret);
    }

    sdio_writeb(0, 0x3, 0x13, &ret);
    if (ret) {
        brcm_log("Failed %d\n", ret);
    }

    val = sdio_readb(0, 0x7, &ret);
    if (ret) {
        brcm_log("Failed %d\n", ret);
    }

    sdio_writeb(0, 0x42, 0x7, &ret);
    if (ret) {
        brcm_log("Failed %d\n", ret);
    }
#if 0
        brcm_dummy_read(0x100, 1);
        brcm_dummy_read(0x109, 3);
        brcm_dummy_read(0x1000, 0x37);

        brcm_dummy_read(0x200, 1);
        brcm_dummy_read(0x209, 3);
        brcm_dummy_read(0x1038, 0x37);

        brcm_dummy_read(0x300, 1);
        brcm_dummy_read(0x309, 3);
        brcm_dummy_read(0x1070, 0x3c);
#endif
    ret = sdio_set_block_size(1, 64);
    if (ret) {
        brcm_log("Failed to set F1 blocksize\n");
        return ret;
    }

    ret = sdio_set_block_size(2, 512);
    if (ret) {
        brcm_log("Failed to set blocksize\n");
        return ret;
    } 

    ret = sdio_enable_func(1);
    if (ret) {
        brcm_log("Failed to enable F1: err=%d\n", ret);
    }

    uint32_t enum_base = 0x18000000;
    uint32_t clkctl = 0;
    brcm_log("F1 signature read @0x%08x=0x%4x\n", enum_base,
         brcmf_sdiod_readl(enum_base, NULL));


    /*
     * Force PLL off until brcmf_chip_attach()
     * programs PLL control regs
     */

    brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR, BRCMF_INIT_CLKCTL1,
               &err);
    if (!err)
        clkctl = brcmf_sdiod_readb(SBSDIO_FUNC1_CHIPCLKCSR,
                       &err);

    if (err || ((clkctl & ~SBSDIO_AVBITS) != BRCMF_INIT_CLKCTL1)) {
        brcm_log("ChipClkCSR access: err %d wrote 0x%02x read 0x%02x\n",
              err, BRCMF_INIT_CLKCTL1, clkctl);
        return err;
    }

    brcmf_sdio_buscoreprep();
    bus->ci = brcmf_chip_attach();
    bus->sdio_core   = brcmf_chip_get_core(BCMA_CORE_SDIO_DEV);
    bus->cc_core = brcmf_chip_get_core(BCMA_CORE_CHIPCOMMON);

    if (brcmf_sdio_kso_init()) {
        brcm_log("error enabling KSO\n");
        return -1;
    }

    brcmf_sdio_drivestrengthinit(6);

    uint32_t reg_val = brcmf_sdiod_func0_rb(SDIO_CCCR_BRCM_CARDCTRL, &err);
    if (err){
        brcm_log("WLAN backplane reset error\n");
        return err;
    }
    
    reg_val |= SDIO_CCCR_BRCM_CARDCTRL_WLANRESET;

    brcmf_sdiod_func0_wb(SDIO_CCCR_BRCM_CARDCTRL, reg_val, &err);
    if (err){
        brcm_log("WLAN backplane reset error\n");
        return err;
    }

    struct brcmf_core* pmu = brcmf_chip_get_pmu();
    uint32_t reg_addr = CORE_CC_REG(pmu->base, pmucontrol);
    reg_val = brcmf_sdiod_readl(reg_addr, &err);
    if (err){
        brcm_log("PMU reset error\n");
        return err;
    }

    reg_val |= (BCMA_CC_PMU_CTL_RES_RELOAD << BCMA_CC_PMU_CTL_RES_SHIFT);

    brcmf_sdiod_writel(reg_addr, reg_val, &err);
    if (err){
        brcm_log("PMU write error\n");
        return err;
    }
    bus->tx_max = 0;
    bus->tx_seq = 255;
    bus->rxbound = 50;
    bus->txbound = 20;
    bus->blocksize = bus->roundup = 512;
    bus->head_align = ALIGNMENT;
    bus->sgentry_align = ALIGNMENT;

    bus->rxhdr = malloc(64);
    bus->rxctl = malloc(8192);

    bus->rx_queue = queue_buffer_alloc(32, MAX_FRAME_SIZE);
    bus->tx_queue = queue_buffer_alloc(32, MAX_FRAME_SIZE);
    /* Disable F2 to clear any intermediate frame state on the dongle */
    sdio_disable_func(2);

    /* Done with backplane-dependent accesses, can drop clock... */
    brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR, 0, NULL);
    bus->clkstate = CLK_SDONLY;

    ret = sdio_set_block_size(3, 512);
    if (ret) {
        brcm_log("Failed to set F3 blocksize\n");
        return ret;
    }

    uint32_t fw_len;
    uint32_t nvram_len;
    uint8_t *fw = brcmf_fw_get_firmware(&fw_len);
    uint8_t* nvram = brcmf_fw_get_nvram(&nvram_len);

    bus->alp_only = true;
    brcmf_sdio_download_firmware(fw, fw_len, nvram, nvram_len);
    bus->alp_only = false;

    /* Make sure backplane clock is on, needed to generate F2 interrupt */
    brcmf_sdio_clkctl(CLK_AVAIL, false);
    if (bus->clkstate != CLK_AVAIL){
        brcm_log("failed\n");
        return -1;
    }

    /* Force clocks on backplane to be sure F2 interrupt propagates */
    uint8_t saveclk = brcmf_sdiod_readb(SBSDIO_FUNC1_CHIPCLKCSR, &err);
    if (!err) {
        uint8_t bpreq = saveclk;
        bpreq |= SBSDIO_FORCE_HT;
        brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR,
                   bpreq, &err);
    }
    if (err) {
        brcm_log("Failed to force clock for F2: err %d\n", err);
        return err;
    }

    /* Enable function 2 (frame transfers) */
    brcmf_sdiod_writel(bus->sdio_core->base + SD_REG(tosbmailboxdata),
               SDPCM_PROT_VERSION << SMB_DATA_VERSION_SHIFT, NULL);


    err = sdio_enable_func(2);

    if (!err) {
        bus->hostintmask = HOSTINTMASK;
        brcmf_sdiod_writel(bus->sdio_core->base + SD_REG(hostintmask),
                   bus->hostintmask, NULL); 

        brcm_log("set F2 watermark to 0x%x*4 bytes\n",DEFAULT_F2_WATERMARK);
        brcmf_sdiod_writeb(SBSDIO_WATERMARK, DEFAULT_F2_WATERMARK, &err);

        brcmf_chip_sr_capable();
        /* Restore previous clock setting */
        brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR, saveclk, &err);
    }

    sdio_claim_irq(1);
    sdio_claim_irq(2);

    uint8_t devpend = brcmf_sdiod_func0_rb(SDIO_CCCR_INTx, NULL);
    bus->intstatus = devpend & (INTR_STATUS_FUNC1 |
                               INTR_STATUS_FUNC2);

    return 0;
}


void* brcm_thread(void* p) {
    (void)p;
    static int tick = 0;
    sleep(10);
    brcmf_sdiod_probe();

    for(int i = 0; i < 500; i++){
        brcmf_sdio_dpc();
        usleep(1000);
    }

    uint32_t value = 0; 
    int err  = brcmf_fil_iovar_data_set(0, "bus:txglom", &value, sizeof(uint32_t));
    if(err){
        brcm_log("disable glom failed %d\n", err);
    }

    err  = brcmf_fil_iovar_data_set(0, "bus:rxglom", &value, sizeof(uint32_t));
    if(err){
        brcm_log("disable glom failed %d\n", err);
    }

    brcmf_c_preinit_dcmds();

    while(1){
        brcmf_sdio_dpc();

        if(tick%1000 == 0){
            brcmf_sdio_readconsole();

            if(tick % 100000 == 0 && bus->state != CONNECTED){
                memset(bus->ssid, 0, sizeof(bus->ssid));
                //bus->scan_update = kernel_tic_ms(0);
                bus->scan_update = 0;
                bus->state = SCANNING;
                scan();
            } 

            if(bus->state == SCANNING && (bus->scan_update++) > 20000 && strlen(bus->ssid) > 0){
                int idx = config_match_ssid(bus->ssid);
                char*  pmk = (char*)config_get_pmk(idx);
                if(!pmk){

                    const char* passwd = config_get_passwd(idx);
                    if(passwd){
                        char pmkstr[65];
                        unsigned char pmk[32];
                        PKCS5_PBKDF2_HMAC((const unsigned char*)passwd, strlen(passwd), (const unsigned char*)bus->ssid, strlen(bus->ssid), 4096, 32, pmk);
                        to_str(pmkstr, pmk, 32);
                        brcm_log("connect ssid:%s\n", bus->ssid);
                        bus->state = CONNECTING;
                        connect(bus->ssid, pmkstr);
                    }else{
                        brcm_log("no passwd fond for ssid: %s\n", config_get_ssid(idx));
                    }
                }else{
                    brcm_log("connect ssid:%s\n", bus->ssid);
                    bus->state = CONNECTING;
                    connect(bus->ssid, pmk);
                }
            }
        }
        usleep(1000);
        tick++;
    }
}

int brcm_state(void){
    return bus->state;
}

void brcm_init(void){
    mmc_hw_reset();
    config_init(NULL);
    pthread_t tid;
	pthread_create(&tid, NULL, brcm_thread, NULL);
}

int brcm_recv(uint8_t *buf, int len){
    int ret = queue_buffer_pop(bus->rx_queue, buf, len);
    return ret;
}

int brcm_send(uint8_t *buf, int len){
    if(bus->state != CONNECTED)
        return 0;
    int ret = queue_buffer_push(bus->tx_queue, buf, len);
    return ret;
}

int brcm_check_data(void){
    return queue_buffer_check(bus->rx_queue);
}
