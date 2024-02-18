#include <stdint.h>
#include <stdbool.h>

#include "types.h"
#include "skb.h"
#include "brcmfmac.h"
#include "chip.h"
#include "chipcommon.h"
#include "soc.h"
#include "brcm_hw_ids.h"
#include "bcma.h"
#include "bcma_driver_chipcommon.h"

#define brcmf_dbg(x, fmt, ...)  klog(fmt,__VA_ARGS__) 

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
#define SBSDIO_AVBITS       (SBSDIO_HT_AVAIL | SBSDIO_ALP_AVAIL)
#define SBSDIO_ALPAV(regval)    ((regval) & SBSDIO_AVBITS)
#define SBSDIO_HTAV(regval) (((regval) & SBSDIO_AVBITS) == SBSDIO_AVBITS)
#define SBSDIO_ALPONLY(regval)  (SBSDIO_ALPAV(regval) && !SBSDIO_HTAV(regval))
#define SBSDIO_CLKAV(regval, alponly) \
    (SBSDIO_ALPAV(regval) && (alponly ? 1 : SBSDIO_HTAV(regval)))


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

struct brcmf_chip *ci;
struct brcmf_core *sdio_core;
struct brcmf_core *cc_core;
static int clkstate;

static int brcmf_sdiod_set_backplane_window(u32 addr)
{
    u32 v, bar0 = addr & SBSDIO_SBWINDOW_MASK;
    int err = 0, i;

    v = bar0 >> 8;

    for (i = 0 ; i < 3 && !err ; i++, v >>= 8)
        brcmf_sdiod_writeb(SBSDIO_FUNC1_SBADDRLOW + i,
                   v & 0xff, &err);

    return err;
}

static int brcmf_sdiod_skbuff_read(
				   int func, u32 addr,
				   struct sk_buff *skb)
{
	unsigned int req_sz;
	int err;

	/* Single skb use the standard mmc interface */
	req_sz = skb->len + 3;
	req_sz &= (uint)~3;

	switch (func) {
	case 1:
		err = sdio_memcpy_fromio(func, ((u8 *)(skb->data)), addr,
					 req_sz);
		break;
	case 2:
		err = sdio_readsb(func, ((u8 *)(skb->data)), addr, req_sz);
		break;
	default:
		/* bail out as things are really fishy here */
		klog("invalid sdio function number: %d\n", func);
		err = -ENOMEDIUM;
	}

	return err;
}

static int brcmf_sdiod_skbuff_write(
                    int func, u32 addr,
                    struct sk_buff *skb)
{
    unsigned int req_sz;
    int err;

    /* Single skb use the standard mmc interface */
    req_sz = skb->len + 3;
    req_sz &= (uint)~3;

    err = sdio_memcpy_toio(func, addr, ((u8 *)(skb->data)), req_sz);
    return err;
}

int
brcmf_sdiod_ramrw(bool write, u32 address,
          u8 *data, uint size)
{
    int err = 0;
    struct sk_buff *pkt;
    u32 sdaddr;
    uint dsize;
    dsize = min_t(uint, SBSDIO_SB_OFT_ADDR_LIMIT, size);

    pkt = dev_alloc_skb(dsize);
    if (!pkt) {
        klog("dev_alloc_skb failed: len %d\n", dsize);
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

        // klog("%s %d bytes at offset 0x%08x in window 0x%08x\n",
        //       write ? "write" : "read", dsize,
        //       sdaddr, address & SBSDIO_SBWINDOW_MASK);

        sdaddr &= SBSDIO_SB_OFT_ADDR_MASK;
        sdaddr |= SBSDIO_SB_ACCESS_2_4B_FLAG;

        if (write) {
            memcpy(pkt->data, data, dsize);
            err = brcmf_sdiod_skbuff_write(1, sdaddr, pkt);
        } else {
            err = brcmf_sdiod_skbuff_read(1, sdaddr, pkt);
        }

        if (err) {
            klog("membytes transfer failed\n");
            break;
        }
        if (!write)
            memcpy(data, pkt->data, dsize);

        /* Adjust for next transfer (if any) */
        size -= dsize;
        if (size) {
            data += dsize;
            address += dsize;
            sdaddr = 0;
            dsize = min_t(uint, SBSDIO_SB_OFT_ADDR_LIMIT, size);
        }
    }

    dev_kfree_skb(pkt);
    return err;
}

u32 brcmf_sdiod_readl(u32 addr, int *ret)
{
	u32 data = 0;
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

void brcmf_sdiod_writel(u32 addr,
			u32 data, int *ret)
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

void brcm_read(uint32_t addr, int len){
    int ret = 0;
    for(int i= 0; i < len; i++){
        u8 val = sdio_readb(0, addr + i, &ret);
        if (ret) {
            klog("Failed %d\n", ret);
        }
    }
}
/*****************************************************************/

static int brcmf_sdio_buscoreprep()
{
    int err = 0;
    u8 clkval, clkset;

    /* Try forcing SDIO core to do ALPAvail request only */
    clkset = SBSDIO_FORCE_HW_CLKREQ_OFF | SBSDIO_ALP_AVAIL_REQ;
    brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR, clkset, &err);
    if (err) {
        klog("error writing for HT off\n");
        return err;
    }

    /* If register supported, wait for ALPAvail and then force ALP */
    /* This may take up to 15 milliseconds */
    clkval = brcmf_sdiod_readb(SBSDIO_FUNC1_CHIPCLKCSR, NULL);

    if ((clkval & ~SBSDIO_AVBITS) != clkset) {
        klog("ChipClkCSR access: wrote 0x%02x read 0x%02x\n",
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
        klog("timeout on ALPAV wait, clkval 0x%02x\n",
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
    u8 strength;    /* Pad Drive Strength in mA */
    u8 sel;     /* Chip-specific select value */
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
brcmf_sdio_drivestrengthinit(u32 drivestrength)
{
    const struct sdiod_drive_str *str_tab = NULL;
    u32 str_mask;
    u32 str_shift;
    u32 i;
    u32 drivestrength_sel = 0;
    u32 cc_data_temp;
    u32 addr;

    if (!(ci->cc_caps & CC_CAP_PMU))
        return;

    switch (SDIOD_DRVSTR_KEY(ci->chip, ci->pmurev)) {
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
            klog("Invalid SDIO Drive strength for chip %s, strength=%d\n",
                  ci->name, drivestrength);
        break;
    case SDIOD_DRVSTR_KEY(BRCM_CC_43362_CHIP_ID, 13):
        str_tab = sdiod_drive_strength_tab5_1v8;
        str_mask = 0x00003800;
        str_shift = 11;
        break;
    default:
        klog("No SDIO driver strength init needed for chip %s rev %d pmurev %d\n",
              ci->name, ci->chiprev, ci->pmurev);
        break;
    }

    if (str_tab != NULL) {
        struct brcmf_core *pmu = brcmf_chip_get_pmu(ci);

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

        klog("SDIO: %d mA (req=%d mA) drive strength selected, set to 0x%08x\n",
              str_tab[i].strength, drivestrength, cc_data_temp);
    }
}

/* enable KSO bit */
static int brcmf_sdio_kso_init()
{
    struct brcmf_core *core = sdio_core;
    u8 val;
    int err = 0;

    /* KSO bit added in SDIO core rev 12 */
    if (core->rev < 12)
        return 0;

    val = brcmf_sdiod_readb(SBSDIO_FUNC1_SLEEPCSR, &err);
    if (err) {
        klog("error reading SBSDIO_FUNC1_SLEEPCSR\n");
        return err;
    }

    if (!(val & SBSDIO_FUNC1_SLEEPCSR_KSO_MASK)) {
        val |= (SBSDIO_FUNC1_SLEEPCSR_KSO_EN <<
            SBSDIO_FUNC1_SLEEPCSR_KSO_SHIFT);
        brcmf_sdiod_writeb(SBSDIO_FUNC1_SLEEPCSR,
                   val, &err);
        if (err) {
            klog("error writing SBSDIO_FUNC1_SLEEPCSR\n");
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
        clkstate = CLK_SDONLY;
    else
        clkstate = CLK_NONE;

    return 0;
}


/* Turn backplane clock on or off */
static int brcmf_sdio_htclk(bool on, bool pendok)
{
    int err;
    u8 clkctl, clkreq, devctl;
    unsigned long timeout;

    klog("brcmf_sdio_htclk %d %d\n", on, pendok);

    clkctl = 0;

    if (on) {
        /* Request HT Avail */
        clkreq = (0) ? SBSDIO_ALP_AVAIL_REQ : SBSDIO_HT_AVAIL_REQ;

        brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR,
                   clkreq, &err);
        if (err) {
            klog("HT Avail request error: %d\n", err);
            return -EBADE;
        }

        /* Check current status */
        clkctl = brcmf_sdiod_readb(
                       SBSDIO_FUNC1_CHIPCLKCSR, &err);
        if (err) {
            klog("HT Avail read error: %d\n", err);
            return -EBADE;
        }

        /* Go to pending and await interrupt if appropriate */
        if (!SBSDIO_CLKAV(clkctl, 0) && pendok) {
            /* Allow only clock-available interrupt */
            devctl = brcmf_sdiod_readb(
                           SBSDIO_DEVICE_CTL, &err);
            if (err) {
                klog("Devctl error setting CA: %d\n", err);
                return -EBADE;
            }

            devctl |= SBSDIO_DEVCTL_CA_INT_ONLY;
            brcmf_sdiod_writeb(SBSDIO_DEVICE_CTL,
                       devctl, &err);
            klog("CLKCTL: set PENDING\n");
            clkstate = CLK_PENDING;

            return 0;
        } else if (clkstate == CLK_PENDING) {
            /* Cancel CA-only interrupt filter */
            devctl = brcmf_sdiod_readb(
                           SBSDIO_DEVICE_CTL, &err);
            devctl &= ~SBSDIO_DEVCTL_CA_INT_ONLY;
            brcmf_sdiod_writeb( SBSDIO_DEVICE_CTL,
                       devctl, &err);
        }

        /* Otherwise, wait here (polling) for HT Avail */
        timeout = get_timer(0) + PMU_MAX_TRANSITION_DLY;
        while (!SBSDIO_CLKAV(clkctl, 0)) {
            clkctl = brcmf_sdiod_readb(
                           SBSDIO_FUNC1_CHIPCLKCSR,
                           &err);
            if (get_timer(timeout) > 0)
                break;
            else
                usleep(1000);
        }
        if (err) {
            klog("HT Avail request error: %d\n", err);
            return -EBADE;
        }
        if (!SBSDIO_CLKAV(clkctl, 0)) {
            klog("HT Avail timeout (%d): clkctl 0x%02x\n",
                  PMU_MAX_TRANSITION_DLY, clkctl);
            return -EBADE;
        }

        /* Mark clock available */
        clkstate = CLK_AVAIL;
        klog("CLKCTL: turned ON\n");

#if defined(DEBUG)
        if (!bus->alp_only) {
            if (SBSDIO_ALPONLY(clkctl))
                brcmf_err("HT Clock should be on\n");
        }
#endif              /* defined (DEBUG) */

    } else {
        clkreq = 0;

        if (clkstate == CLK_PENDING) {
            /* Cancel CA-only interrupt filter */
            devctl = brcmf_sdiod_readb(
                           SBSDIO_DEVICE_CTL, &err);
            devctl &= ~SBSDIO_DEVCTL_CA_INT_ONLY;
            brcmf_sdiod_writeb(SBSDIO_DEVICE_CTL,
                       devctl, &err);
        }

        clkstate = CLK_SDONLY;
        brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR,
                   clkreq, &err);
        klog("CLKCTL: turned OFF\n");
        if (err) {
            klog("Failed access turning clock off: %d\n",
                  err);
            return -EBADE;
        }
    }
    return 0;
}


/* Transition SD and backplane clock readiness */
static int brcmf_sdio_clkctl( uint target, bool pendok)
{
    /* Early exit if we're already there */
    if (clkstate == target)
        return 0;

    switch (target) {
    case CLK_AVAIL:
        /* Make sure SD clock is available */
        if (clkstate == CLK_NONE)
            brcmf_sdio_sdclk(true);
        /* Now request HT Avail on the backplane */
        brcmf_sdio_htclk(true, pendok);
        break;

    case CLK_SDONLY:
        /* Remove HT request, or bring up SD clock */
        if (clkstate == CLK_NONE)
            brcmf_sdio_sdclk(true);
        else if (clkstate == CLK_AVAIL)
            brcmf_sdio_htclk(false, false);
        else
            klog("request for %d -> %d\n",
                  clkstate, target);
        break;

    case CLK_NONE:
        /* Make sure to remove HT request */
        if (clkstate == CLK_AVAIL)
            brcmf_sdio_htclk(false, false);
        /* Now remove the SD clock */
        brcmf_sdio_sdclk(false);
        break;
    }
    return 0;
}


static bool
brcmf_sdio_verifymemory(u32 ram_addr,
            u8 *ram_data, uint ram_sz)
{
    char *ram_cmp;
    int err;
    bool ret = true;
    int address;
    int offset;
    int len;

    /* read back and verify */
    klog("Compare RAM dl & ul at 0x%08x; size=%d\n", ram_addr,
          ram_sz);
    ram_cmp = malloc(2048);
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
            klog("error %d on reading %d membytes at 0x%08x\n",
                  err, len, address);
            ret = false;
            break;
        } else if (memcmp(ram_cmp, &ram_data[offset], len)) {
            klog("Downloaded RAM image is corrupted, block offset is %d, len is %d\n",
                  offset, len);
            ret = false;
            break;
        }
        offset += len;
        address += len;
    }
    if(offset == ram_sz)
        klog("Verify success!\n");

    klog("");
    free(ram_cmp);

    return ret;
}

static int brcmf_sdio_download_code_file(const uint8_t *fw, int len)
{
    int err;

    err = brcmf_sdiod_ramrw(true, ci->rambase,
                (u8 *)fw, len);
    if (err)
        klog("error %d on writing %d membytes at 0x%08x\n",
              err, (int)fw, ci->rambase);
    else if (!brcmf_sdio_verifymemory(ci->rambase,
                      fw, len))
        err = -EIO;

    return err;
}

static int brcmf_sdio_download_nvram(const uint8_t  *vars, int varsz)
{
    int address;
    int err;

    address = ci->ramsize - varsz + ci->rambase;
    err = brcmf_sdiod_ramrw(true, address, vars, varsz);
    if (err)
        klog("error %d on writing %d nvram bytes at 0x%08x\n",
              err, varsz, address);
    else if (!brcmf_sdio_verifymemory(address, vars, varsz))
        err = -EIO;

    return err;
}

static int brcmf_sdio_download_firmware(uint8_t *fw, uint32_t len,  uint8_t *nvram, uint32_t nvlen)
{
    int bcmerror;
    u32 rstvec;

    klog("download firmware rstvec: %x\n", *((uint32_t*)fw)); 

    bcmerror = brcmf_sdio_download_code_file(fw, len);
    if (bcmerror) {
        klog("dongle image file download failed\n");
        goto err;
    }

    klog("download nvram\n"); 
    bcmerror = brcmf_sdio_download_nvram(fw, nvlen);
    if (bcmerror) {
        klog("dongle nvram file download failed\n");
        goto err;
    }

    /* Take arm out of reset */
    if (!brcmf_chip_set_active(rstvec)) {
        klog("error getting out of ARM core reset\n");
        bcmerror = -EIO;
        goto err;
    }

err:
    return bcmerror;
}

#include "firmware.h"

void brcmf_sdiod_probe(void){
    int ret = 0, err = 0;
    u8 clkctl, clkreq, devctl;

    klog("%08x\n", fmac43455_fw);
    klog("%08x\n", *(uint32_t*)fmac43455_fw);
#if 0
    brcm_read(0x0, 1);
    brcm_read(0x8, 1);
    brcm_read(0x12, 2);
    brcm_read(0x9, 3);
    brcm_read(0x10ac, 0xd5);
#endif
    u8 val = sdio_readb(0, 0x13, &ret);
    if (ret) {
        klog("Failed %d\n", ret);
    }
    klog("%x\n", val);

    sdio_writeb(0, 0x3, 0x13, &ret);
    if (ret) {
        klog("Failed %d\n", ret);
    }

    val = sdio_readb(0, 0x7, &ret);
    if (ret) {
        klog("Failed %d\n", ret);
    }
    klog("%x\n", val);

    sdio_writeb(0, 0x42, 0x7, &ret);
    if (ret) {
        klog("Failed %d\n", ret);
    }
#if 0
        brcm_read(0x100, 1);
        brcm_read(0x109, 3);
        brcm_read(0x1000, 0x37);

        brcm_read(0x200, 1);
        brcm_read(0x209, 3);
        brcm_read(0x1038, 0x37);

        brcm_read(0x300, 1);
        brcm_read(0x309, 3);
        brcm_read(0x1070, 0x3c);
#endif
    ret = sdio_set_block_size(1, 64);
    if (ret) {
        klog("Failed to set F1 blocksize\n");
        return ret;
    }

    ret = sdio_set_block_size(2, 512);
    if (ret) {
        klog("Failed to set F2 blocksize\n");
        return ret;
    } else {
        klog("set F2 blocksize to %d\n", 512);
    }

    ret = sdio_set_block_size(1, 64);
    if (ret) {
        klog("Failed to set F1 blocksize\n");
        return ret;
    }

    ret = sdio_set_block_size(2, 512);
    if (ret) {
        klog("Failed to set F2 blocksize\n");
        return ret;
    } else {
        klog("set F2 blocksize to %d\n", 512);
    }

    ret = sdio_enable_func(1);
    if (ret) {
        klog("Failed to enable F1: err=%d\n", ret);
    }

    uint32_t enum_base = 0x18000000;
    klog("F1 signature read @0x%08x=0x%4x\n", enum_base,
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
        klog("ChipClkCSR access: err %d wrote 0x%02x read 0x%02x\n",
              err, BRCMF_INIT_CLKCTL1, clkctl);
        return -1;
    }

    brcmf_sdio_buscoreprep();
    ci = brcmf_chip_attach();
    sdio_core   = brcmf_chip_get_core(BCMA_CORE_SDIO_DEV);
    cc_core = brcmf_chip_get_core(BCMA_CORE_CHIPCOMMON);

    if (brcmf_sdio_kso_init()) {
        klog("error enabling KSO\n");
        return -1;
    }

    brcmf_sdio_drivestrengthinit(6);

    klog("Reset WLAN backplane...\n");
    uint32_t reg_val = brcmf_sdiod_func0_rb(SDIO_CCCR_BRCM_CARDCTRL, &err);
    if (err){
        klog("WLAN backplane reset error\n");
        return;
    }
    
    reg_val |= SDIO_CCCR_BRCM_CARDCTRL_WLANRESET;

    brcmf_sdiod_func0_wb(SDIO_CCCR_BRCM_CARDCTRL, reg_val, &err);
    if (err){
        klog("WLAN backplane reset error\n");
        return;
    }

    klog("Reset PMU...\n");
    struct brcmf_core* pmu = brcmf_chip_get_pmu();
    uint32_t reg_addr = CORE_CC_REG(pmu->base, pmucontrol);
    reg_val = brcmf_sdiod_readl(reg_addr, &err);
    if (err){
        klog("PMU read error\n");
        return;
    }

    reg_val |= (BCMA_CC_PMU_CTL_RES_RELOAD << BCMA_CC_PMU_CTL_RES_SHIFT);

    brcmf_sdiod_writel(reg_addr, reg_val, &err);
    if (err){
        klog("PMU write error\n");
        return;
    }

    /* Disable F2 to clear any intermediate frame state on the dongle */
    sdio_disable_func(2);

    /* Done with backplane-dependent accesses, can drop clock... */
    brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR, 0, NULL);



    brcmf_sdio_download_firmware(fmac43455_fw, fmac43455_fw_len, fmac43455_nvram, fmac43455_nvram_len);

    /* Make sure backplane clock is on, needed to generate F2 interrupt */
    brcmf_sdio_clkctl(CLK_AVAIL, false);
    if (clkstate != CLK_AVAIL){
        klog("failed\n");
        return;
    }

    /* Force clocks on backplane to be sure F2 interrupt propagates */
    u8 saveclk = brcmf_sdiod_readb(SBSDIO_FUNC1_CHIPCLKCSR, &err);
    if (!err) {
        u8 bpreq = saveclk;
        bpreq |= SBSDIO_FORCE_HT;
        brcmf_sdiod_writeb(SBSDIO_FUNC1_CHIPCLKCSR,
                   bpreq, &err);
    }
    if (err) {
        klog("Failed to force clock for F2: err %d\n", err);
        return;
    }

    /* Enable function 2 (frame transfers) */
    brcmf_sdiod_writel(sdio_core->base + SD_REG(tosbmailboxdata),
               SDPCM_PROT_VERSION << SMB_DATA_VERSION_SHIFT, NULL);

    err = sdio_enable_func(2);

    brcmf_dbg(INFO, "enable F2: err=%d\n", err);
}

void brcm_init(void){
    mmc_hw_reset();
    brcmf_sdiod_probe();
}