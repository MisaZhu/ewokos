#include <types.h>
#include <stdlib.h>
#include <string.h>
 #include <stdio.h>
#include <utils/skb.h>
#include <utils/log.h>

#include "brcm.h"
#include "chip.h"
#include "chipcommon.h"
#include "soc.h"
#include "brcm_hw_ids.h"
#include "bcma.h"
#include "bcma_regs.h"

/* SOC Interconnect types (aka chip types) */
#define SOCI_SB		0
#define SOCI_AI		1

/* PL-368 DMP definitions */
#define DMP_DESC_TYPE_MSK	0x0000000F
#define  DMP_DESC_EMPTY		0x00000000
#define  DMP_DESC_VALID		0x00000001
#define  DMP_DESC_COMPONENT	0x00000001
#define  DMP_DESC_MASTER_PORT	0x00000003
#define  DMP_DESC_ADDRESS	0x00000005
#define  DMP_DESC_ADDRSIZE_GT32	0x00000008
#define  DMP_DESC_EOT		0x0000000F

#define DMP_COMP_DESIGNER	0xFFF00000
#define DMP_COMP_DESIGNER_S	20
#define DMP_COMP_PARTNUM	0x000FFF00
#define DMP_COMP_PARTNUM_S	8
#define DMP_COMP_CLASS		0x000000F0
#define DMP_COMP_CLASS_S	4
#define DMP_COMP_REVISION	0xFF000000
#define DMP_COMP_REVISION_S	24
#define DMP_COMP_NUM_SWRAP	0x00F80000
#define DMP_COMP_NUM_SWRAP_S	19
#define DMP_COMP_NUM_MWRAP	0x0007C000
#define DMP_COMP_NUM_MWRAP_S	14
#define DMP_COMP_NUM_SPORT	0x00003E00
#define DMP_COMP_NUM_SPORT_S	9
#define DMP_COMP_NUM_MPORT	0x000001F0
#define DMP_COMP_NUM_MPORT_S	4

#define DMP_MASTER_PORT_UID	0x0000FF00
#define DMP_MASTER_PORT_UID_S	8
#define DMP_MASTER_PORT_NUM	0x000000F0
#define DMP_MASTER_PORT_NUM_S	4

#define DMP_SLAVE_ADDR_BASE	0xFFFFF000
#define DMP_SLAVE_ADDR_BASE_S	12
#define DMP_SLAVE_PORT_NUM	0x00000F00
#define DMP_SLAVE_PORT_NUM_S	8
#define DMP_SLAVE_TYPE		0x000000C0
#define DMP_SLAVE_TYPE_S	6
#define  DMP_SLAVE_TYPE_SLAVE	0
#define  DMP_SLAVE_TYPE_BRIDGE	1
#define  DMP_SLAVE_TYPE_SWRAP	2
#define  DMP_SLAVE_TYPE_MWRAP	3
#define DMP_SLAVE_SIZE_TYPE	0x00000030
#define DMP_SLAVE_SIZE_TYPE_S	4
#define  DMP_SLAVE_SIZE_4K	0
#define  DMP_SLAVE_SIZE_8K	1
#define  DMP_SLAVE_SIZE_16K	2
#define  DMP_SLAVE_SIZE_DESC	3

/* EROM CompIdentB */
#define CIB_REV_MASK		0xff000000
#define CIB_REV_SHIFT		24

/* ARM CR4 core specific control flag bits */
#define ARMCR4_BCMA_IOCTL_CPUHALT	0x0020

/* D11 core specific control flag bits */
#define D11_BCMA_IOCTL_PHYCLOCKEN	0x0004
#define D11_BCMA_IOCTL_PHYRESET		0x0008

/* chip core base & ramsize */
/* bcm4329 */
/* SDIO device core, ID 0x829 */
#define BCM4329_CORE_BUS_BASE		0x18011000
/* internal memory core, ID 0x80e */
#define BCM4329_CORE_SOCRAM_BASE	0x18003000
/* ARM Cortex M3 core, ID 0x82a */
#define BCM4329_CORE_ARM_BASE		0x18002000

/* Max possibly supported memory size (limited by IO mapped memory) */
#define BRCMF_CHIP_MAX_MEMSIZE		(4 * 1024 * 1024)

#define CORE_SB(base, field) \
		(base + SBCONFIGOFF + offsetof(struct sbconfig, field))
#define	SBCOREREV(sbidh) \
	((((sbidh) & SSB_IDHIGH_RCHI) >> SSB_IDHIGH_RCHI_SHIFT) | \
	  ((sbidh) & SSB_IDHIGH_RCLO))

struct sbconfig {
	uint32_t PAD[2];
	uint32_t sbipsflag;	/* initiator port ocp slave flag */
	uint32_t PAD[3];
	uint32_t sbtpsflag;	/* target port ocp slave flag */
	uint32_t PAD[11];
	uint32_t sbtmerrloga;	/* (sonics >= 2.3) */
	uint32_t PAD;
	uint32_t sbtmerrlog;	/* (sonics >= 2.3) */
	uint32_t PAD[3];
	uint32_t sbadmatch3;	/* address match3 */
	uint32_t PAD;
	uint32_t sbadmatch2;	/* address match2 */
	uint32_t PAD;
	uint32_t sbadmatch1;	/* address match1 */
	uint32_t PAD[7];
	uint32_t sbimstate;	/* initiator agent state */
	uint32_t sbintvec;	/* interrupt mask */
	uint32_t sbtmstatelow;	/* target state */
	uint32_t sbtmstatehigh;	/* target state */
	uint32_t sbbwa0;		/* bandwidth allocation table0 */
	uint32_t PAD;
	uint32_t sbimconfiglow;	/* initiator configuration */
	uint32_t sbimconfighigh;	/* initiator configuration */
	uint32_t sbadmatch0;	/* address match0 */
	uint32_t PAD;
	uint32_t sbtmconfiglow;	/* target configuration */
	uint32_t sbtmconfighigh;	/* target configuration */
	uint32_t sbbconfig;	/* broadcast configuration */
	uint32_t PAD;
	uint32_t sbbstate;	/* broadcast state */
	uint32_t PAD[3];
	uint32_t sbactcnfg;	/* activate configuration */
	uint32_t PAD[3];
	uint32_t sbflagst;	/* current sbflags */
	uint32_t PAD[3];
	uint32_t sbidlow;		/* identification */
	uint32_t sbidhigh;	/* identification */
};

#define INVALID_RAMBASE			((uint32_t)(~0))

/* bankidx and bankinfo reg defines corerev >= 8 */
#define SOCRAM_BANKINFO_RETNTRAM_MASK	0x00010000
#define SOCRAM_BANKINFO_SZMASK		0x0000007f
#define SOCRAM_BANKIDX_ROM_MASK		0x00000100

#define SOCRAM_BANKIDX_MEMTYPE_SHIFT	8
/* socram bankinfo memtype */
#define SOCRAM_MEMTYPE_RAM		0
#define SOCRAM_MEMTYPE_R0M		1
#define SOCRAM_MEMTYPE_DEVRAM		2

#define SOCRAM_BANKINFO_SZBASE		8192
#define SRCI_LSS_MASK		0x00f00000
#define SRCI_LSS_SHIFT		20
#define	SRCI_SRNB_MASK		0xf0
#define	SRCI_SRNB_MASK_EXT	0x100
#define	SRCI_SRNB_SHIFT		4
#define	SRCI_SRBSZ_MASK		0xf
#define	SRCI_SRBSZ_SHIFT	0
#define SR_BSZ_BASE		14

struct sbsocramregs {
	uint32_t coreinfo;
	uint32_t bwalloc;
	uint32_t extracoreinfo;
	uint32_t biststat;
	uint32_t bankidx;
	uint32_t standbyctrl;

	uint32_t errlogstatus;	/* rev 6 */
	uint32_t errlogaddr;	/* rev 6 */
	/* used for patching rev 3 & 5 */
	uint32_t cambankidx;
	uint32_t cambankstandbyctrl;
	uint32_t cambankpatchctrl;
	uint32_t cambankpatchtblbaseaddr;
	uint32_t cambankcmdreg;
	uint32_t cambankdatareg;
	uint32_t cambankmaskreg;
	uint32_t PAD[1];
	uint32_t bankinfo;	/* corev 8 */
	uint32_t bankpda;
	uint32_t PAD[14];
	uint32_t extmemconfig;
	uint32_t extmemparitycsr;
	uint32_t extmemparityerrdata;
	uint32_t extmemparityerrcnt;
	uint32_t extmemwrctrlandsize;
	uint32_t PAD[84];
	uint32_t workaround;
	uint32_t pwrctl;		/* corerev >= 2 */
	uint32_t PAD[133];
	uint32_t sr_control;     /* corerev >= 15 */
	uint32_t sr_status;      /* corerev >= 15 */
	uint32_t sr_address;     /* corerev >= 15 */
	uint32_t sr_data;        /* corerev >= 15 */
};

#define SOCRAMREGOFFS(_f)	offsetof(struct sbsocramregs, _f)
#define SYSMEMREGOFFS(_f)	offsetof(struct sbsocramregs, _f)

#define ARMCR4_CAP		(0x04)
#define ARMCR4_BANKIDX		(0x40)
#define ARMCR4_BANKINFO		(0x44)
#define ARMCR4_BANKPDA		(0x4C)

#define	ARMCR4_TCBBNB_MASK	0xf0
#define	ARMCR4_TCBBNB_SHIFT	4
#define	ARMCR4_TCBANB_MASK	0xf
#define	ARMCR4_TCBANB_SHIFT	0

#define	ARMCR4_BSZ_MASK		0x3f
#define	ARMCR4_BSZ_MULT		8192

struct brcmf_core_priv {
	struct brcmf_core pub;
	uint32_t wrapbase;
};

static struct brcmf_core_priv* core_list[16] = {0}; 
static struct brcmf_chip pub;


static struct brcmf_core *brcmf_chip_add_core(u16 coreid, 
    uint32_t base,uint32_t wrapbase)
{
    struct brcmf_core_priv *core;

    core = malloc(sizeof(*core));
    if (!core)
        return NULL;

    core->pub.id = coreid;
    core->pub.base = base;
    core->wrapbase = wrapbase;

    for(int i = 0; i < 16; i++){
        if(core_list[i] == NULL){
            core_list[i] = core;
            break;
        }
    }
    return &core->pub;
}

struct brcmf_core *brcmf_chip_get_core(u16 coreid)
{
    struct brcmf_core_priv *core;

    for(int i = 0; i < 16; i++){
        core = core_list[i];
        if (core && core->pub.id == coreid)
            return &core->pub;
    }
    return NULL;
}

struct brcmf_core *brcmf_chip_get_d11core(u8 unit)
{
    struct brcmf_core_priv *core;

    for(int i = 0; i < 16; i++){
        core = core_list[i];
        if (core && core->pub.id == BCMA_CORE_80211) {
            if (unit-- == 0)
                return &core->pub;
        }
    }
    return NULL;
}

struct brcmf_core *brcmf_chip_get_chipcommon(void)
{
    struct brcmf_core_priv *cc;

    cc = core_list[0];
    if (cc->pub.id != BCMA_CORE_CHIPCOMMON)
    return brcmf_chip_get_core(BCMA_CORE_CHIPCOMMON);
    return &cc->pub;
}

static void brcmf_sdio_buscore_write32(uint32_t addr, uint32_t val)
{
    int err;
    brcmf_sdiod_writel(addr, val, &err);
    if(err){
        brcm_log("%s error:%d\n", __func__, err);
    }
}

static void brcmf_sdio_buscore_activate(uint32_t rstvec)
{
    struct brcmf_core *core = brcmf_chip_get_core(BCMA_CORE_SDIO_DEV);
    uint32_t reg_addr;

    /* clear all interrupts */
    reg_addr = core->base + SD_REG(intstatus);
    brcmf_sdiod_writel(reg_addr, 0xFFFFFFFF, NULL);

    if (rstvec)
        /* Write reset vector to address 0 */
        brcmf_sdiod_ramrw(true, 0, (void *)&rstvec,
                  sizeof(rstvec));
}

static uint32_t brcmf_sdio_buscore_read32(uint32_t addr)
{
    uint32_t val;
    int err;

    val = brcmf_sdiod_readl(addr, &err);
    if(err){
        brcm_log("%s 0x%08x error:%d\n", __func__, addr, err);
    }

    return val;
}

static uint32_t brcmf_chip_core_read32(struct brcmf_core_priv *core, u16 reg)
{
    return brcmf_sdio_buscore_read32(core->pub.base + reg);
}

static void brcmf_chip_core_write32(struct brcmf_core_priv *core,
                    u16 reg, uint32_t val)
{
    brcmf_sdio_buscore_write32(core->pub.base + reg, val);
}


static uint32_t brcmf_chip_dmp_get_desc(uint32_t *eromaddr,
                   u8 *type)
{
    uint32_t val;

    /* read next descriptor */
    val = brcmf_sdio_buscore_read32(*eromaddr);
    *eromaddr += 4;

    if (!type)
        return val;

    /* determine descriptor type */
    *type = (val & DMP_DESC_TYPE_MSK);
    if ((*type & ~DMP_DESC_ADDRSIZE_GT32) == DMP_DESC_ADDRESS)
        *type = DMP_DESC_ADDRESS;

    return val;
}

static int brcmf_chip_dmp_get_regaddr(uint32_t *eromaddr,
                      uint32_t *regbase, uint32_t *wrapbase)
{
    u8 desc;
    uint32_t val, szdesc;
    u8 stype, sztype, wraptype;

    *regbase = 0;
    *wrapbase = 0;

    val = brcmf_chip_dmp_get_desc(eromaddr, &desc);
    if (desc == DMP_DESC_MASTER_PORT) {
        wraptype = DMP_SLAVE_TYPE_MWRAP;
    } else if (desc == DMP_DESC_ADDRESS) {
        /* revert erom address */
        *eromaddr -= 4;
        wraptype = DMP_SLAVE_TYPE_SWRAP;
    } else {
        *eromaddr -= 4;
        return -EILSEQ;
    }

    do {
        /* locate address descriptor */
        do {
            val = brcmf_chip_dmp_get_desc(eromaddr, &desc);
            /* unexpected table end */
            if (desc == DMP_DESC_EOT) {
                *eromaddr -= 4;
                return -EFAULT;
            }
        } while (desc != DMP_DESC_ADDRESS &&
             desc != DMP_DESC_COMPONENT);

        /* stop if we crossed current component border */
        if (desc == DMP_DESC_COMPONENT) {
            *eromaddr -= 4;
            return 0;
        }

        /* skip upper 32-bit address descriptor */
        if (val & DMP_DESC_ADDRSIZE_GT32)
            brcmf_chip_dmp_get_desc(eromaddr, NULL);

        sztype = (val & DMP_SLAVE_SIZE_TYPE) >> DMP_SLAVE_SIZE_TYPE_S;

        /* next size descriptor can be skipped */
        if (sztype == DMP_SLAVE_SIZE_DESC) {
            szdesc = brcmf_chip_dmp_get_desc(eromaddr, NULL);
            /* skip upper size descriptor if present */
            if (szdesc & DMP_DESC_ADDRSIZE_GT32)
                brcmf_chip_dmp_get_desc(eromaddr, NULL);
        }

        /* look for 4K or 8K register regions */
        if (sztype != DMP_SLAVE_SIZE_4K &&
            sztype != DMP_SLAVE_SIZE_8K)
            continue;

        stype = (val & DMP_SLAVE_TYPE) >> DMP_SLAVE_TYPE_S;

        /* only regular slave and wrapper */
        if (*regbase == 0 && stype == DMP_SLAVE_TYPE_SLAVE)
            *regbase = val & DMP_SLAVE_ADDR_BASE;
        if (*wrapbase == 0 && stype == wraptype)
            *wrapbase = val & DMP_SLAVE_ADDR_BASE;
    } while (*regbase == 0 || *wrapbase == 0);

    return 0;
}

static int brcmf_chip_dmp_erom_scan()
{
    struct brcmf_core *core;
    uint32_t eromaddr;
    u8 desc_type = 0;
    uint32_t val;
    u16 id;
    u8 nmw, nsw, rev;
    uint32_t base, wrap;
    int err;

    do{
        eromaddr = brcmf_sdio_buscore_read32(CORE_CC_REG(0x18000000, eromptr));
        brcm_log("eromaddr: 0x%08x\n", eromaddr);
    }while(eromaddr < 0x18000000);

    while (desc_type != DMP_DESC_EOT) {
        val = brcmf_chip_dmp_get_desc(&eromaddr, &desc_type);
        if (!(val & DMP_DESC_VALID))
            continue;

        if (desc_type == DMP_DESC_EMPTY)
            continue;

        /* need a component descriptor */
        if (desc_type != DMP_DESC_COMPONENT)
            continue;

        id = (val & DMP_COMP_PARTNUM) >> DMP_COMP_PARTNUM_S;

        /* next descriptor must be component as well */
        val = brcmf_chip_dmp_get_desc(&eromaddr, &desc_type);
        if((val & DMP_DESC_TYPE_MSK) != DMP_DESC_COMPONENT)
            return -EFAULT;

        /* only look at cores with master port(s) */
        nmw = (val & DMP_COMP_NUM_MWRAP) >> DMP_COMP_NUM_MWRAP_S;
        nsw = (val & DMP_COMP_NUM_SWRAP) >> DMP_COMP_NUM_SWRAP_S;
        rev = (val & DMP_COMP_REVISION) >> DMP_COMP_REVISION_S;

        /* need core with ports */
        if (nmw + nsw == 0 &&
            id != BCMA_CORE_PMU &&
            id != BCMA_CORE_GCI)
            continue;

        /* try to obtain register address info */
        err = brcmf_chip_dmp_get_regaddr(&eromaddr, &base, &wrap);
        if (err)
            continue;

        /* finally a core to be added */
        core = brcmf_chip_add_core(id, base, wrap);
        core->rev = rev;
    }

    return 0;
}


/* safety check for chipinfo */
static int brcmf_chip_cores_check()
{
    struct brcmf_core_priv *core;
    bool need_socram = false;
    bool has_socram = false;
    bool cpu_found = false;
    int idx = 1;

    for(int i = 0; i < 16; i++) {
        core = core_list[i];
        if(!core)
            continue;
            
        brcm_log(" [%-2d] core 0x%x:%-3d base 0x%08x wrap 0x%08x\n",
              idx++, core->pub.id, core->pub.rev, core->pub.base,
              core->wrapbase);

        switch (core->pub.id) {
        case BCMA_CORE_ARM_CM3:
            cpu_found = true;
            need_socram = true;
            break;
        case BCMA_CORE_INTERNAL_MEM:
            has_socram = true;
            break;
        case BCMA_CORE_ARM_CR4:
            cpu_found = true;
            break;
        case BCMA_CORE_ARM_CA7:
            cpu_found = true;
            break;
        default:
            break;
        }
    }

    if (!cpu_found) {
        brcm_log("CPU core not detected\n");
        return -ENXIO;
    }
    /* check RAM core presence for ARM CM3 core */
    if (need_socram && !has_socram) {
        brcm_log("RAM core not provided with ARM CM3 core\n");
        return -ENODEV;
    }
    return 0;
}

static bool brcmf_chip_ai_iscoreup(struct brcmf_core_priv *core)
{
    uint32_t regdata;
    bool ret;

    regdata = brcmf_sdio_buscore_read32(core->wrapbase + BCMA_IOCTL);
    ret = (regdata & (BCMA_IOCTL_FGC | BCMA_IOCTL_CLK)) == BCMA_IOCTL_CLK;

    regdata = brcmf_sdio_buscore_read32(core->wrapbase + BCMA_RESET_CTL);
    ret = ret && ((regdata & BCMA_RESET_CTL_RESET) == 0);

    return ret;
}

static void brcmf_chip_ai_coredisable(struct brcmf_core_priv *core,
                      uint32_t prereset, uint32_t reset)
{
    uint32_t regdata;


    /* if core is already in reset, skip reset */
    regdata = brcmf_sdio_buscore_read32(core->wrapbase + BCMA_RESET_CTL);
    if ((regdata & BCMA_RESET_CTL_RESET) != 0)
        goto in_reset_configure;

    /* configure reset */
   brcmf_sdio_buscore_write32(core->wrapbase + BCMA_IOCTL,
             prereset | BCMA_IOCTL_FGC | BCMA_IOCTL_CLK);
    brcmf_sdio_buscore_read32(core->wrapbase + BCMA_IOCTL);

    /* put in reset */
    brcmf_sdio_buscore_write32(core->wrapbase + BCMA_RESET_CTL,
             BCMA_RESET_CTL_RESET);
    usleep(20);

    /* wait till reset is 1 */
    int timeout = 30;
    while(timeout--){
        uint32_t reg = brcmf_sdio_buscore_read32(core->wrapbase + BCMA_RESET_CTL) ;
        if(reg == BCMA_RESET_CTL_RESET)
            break;
        usleep(1000);
    };

in_reset_configure:
    /* in-reset configure */
    brcmf_sdio_buscore_write32(core->wrapbase + BCMA_IOCTL,
             reset | BCMA_IOCTL_FGC | BCMA_IOCTL_CLK);
    brcmf_sdio_buscore_read32(core->wrapbase + BCMA_IOCTL);
}

static void brcmf_chip_ai_resetcore(struct brcmf_core_priv *core, uint32_t prereset,
                    uint32_t reset, uint32_t postreset)
{
    int count;

    /* must disable first to work for arbitrary current core state */
    brcmf_chip_ai_coredisable(core, prereset, reset);

    count = 0;
    while (brcmf_sdio_buscore_read32(core->wrapbase + BCMA_RESET_CTL) &
           BCMA_RESET_CTL_RESET) {
       brcmf_sdio_buscore_write32(core->wrapbase + BCMA_RESET_CTL, 0);
        count++;
        if (count > 50)
            break;
        usleep(60);
    }

    brcmf_sdio_buscore_write32(core->wrapbase + BCMA_IOCTL,
             postreset | BCMA_IOCTL_CLK);
    brcmf_sdio_buscore_read32(core->wrapbase + BCMA_IOCTL);
}


bool brcmf_chip_iscoreup(struct brcmf_core *pub)
{
    struct brcmf_core_priv *core;

    core = container_of(pub, struct brcmf_core_priv, pub);
    return brcmf_chip_ai_iscoreup(core);
}

void brcmf_chip_resetcore(struct brcmf_core *pub, uint32_t prereset, uint32_t reset,
              uint32_t postreset)
{
    struct brcmf_core_priv *core;

    core = container_of(pub, struct brcmf_core_priv, pub);
    brcmf_chip_ai_resetcore(core, prereset, reset, postreset);
}

void brcmf_chip_coredisable(struct brcmf_core *pub, uint32_t prereset, uint32_t reset)
{
    struct brcmf_core_priv *core;

    core = container_of(pub, struct brcmf_core_priv, pub);
    brcmf_chip_ai_coredisable(core, prereset, reset);
}

static void brcmf_chip_disable_arm(u16 id)
{
    struct brcmf_core *core;
    struct brcmf_core_priv *cpu;
    uint32_t val;


    core = brcmf_chip_get_core(id);
    if (!core)
        return;

    switch (id) {
    case BCMA_CORE_ARM_CM3:
        brcmf_chip_coredisable(core, 0, 0);
        break;
    case BCMA_CORE_ARM_CR4:
    case BCMA_CORE_ARM_CA7:
        cpu = container_of(core, struct brcmf_core_priv, pub);

        /* clear all IOCTL bits except HALT bit */
        val = brcmf_sdio_buscore_read32(cpu->wrapbase + BCMA_IOCTL);
        val &= ARMCR4_BCMA_IOCTL_CPUHALT;
        brcmf_chip_resetcore(core, val, ARMCR4_BCMA_IOCTL_CPUHALT,
                     ARMCR4_BCMA_IOCTL_CPUHALT);
        break;
    default:
        brcm_log("unknown id: %u\n", id);
        break;
    }
}

static inline void
brcmf_chip_cr4_set_passive(void)
{
    struct brcmf_core *core;

    brcmf_chip_disable_arm(BCMA_CORE_ARM_CR4);

    core = brcmf_chip_get_core(BCMA_CORE_80211);
    brcmf_chip_resetcore(core, D11_BCMA_IOCTL_PHYRESET |
                   D11_BCMA_IOCTL_PHYCLOCKEN,
                 D11_BCMA_IOCTL_PHYCLOCKEN,
                 D11_BCMA_IOCTL_PHYCLOCKEN);
}

static inline void
brcmf_chip_ca7_set_passive(void)
{
    struct brcmf_core *core;

    brcmf_chip_disable_arm(BCMA_CORE_ARM_CA7);

    core = brcmf_chip_get_core(BCMA_CORE_80211);
    brcmf_chip_resetcore(core, D11_BCMA_IOCTL_PHYRESET |
                   D11_BCMA_IOCTL_PHYCLOCKEN,
                 D11_BCMA_IOCTL_PHYCLOCKEN,
                 D11_BCMA_IOCTL_PHYCLOCKEN);
}

static void
brcmf_chip_cm3_set_passive(void)
{
    struct brcmf_core *core;
    struct brcmf_core_priv *sr;

    brcmf_chip_disable_arm(BCMA_CORE_ARM_CM3);
    core = brcmf_chip_get_core(BCMA_CORE_80211);
    brcmf_chip_resetcore(core, D11_BCMA_IOCTL_PHYRESET |
                   D11_BCMA_IOCTL_PHYCLOCKEN,
                 D11_BCMA_IOCTL_PHYCLOCKEN,
                 D11_BCMA_IOCTL_PHYCLOCKEN);
    core = brcmf_chip_get_core(BCMA_CORE_INTERNAL_MEM);
    brcmf_chip_resetcore(core, 0, 0, 0);

    /* disable bank #3 remap for this device */
    if (pub.chip == BRCM_CC_43430_CHIP_ID ||
        pub.chip == CY_CC_43439_CHIP_ID) {
        sr = container_of(core, struct brcmf_core_priv, pub);
        brcmf_chip_core_write32(sr, SOCRAMREGOFFS(bankidx), 3);
        brcmf_chip_core_write32(sr, SOCRAMREGOFFS(bankpda), 0);
    }
}

static bool brcmf_chip_cr4_set_active( uint32_t rstvec)
{
    struct brcmf_core *core;

   brcmf_sdio_buscore_activate(rstvec);

    /* restore ARM */
    core = brcmf_chip_get_core(BCMA_CORE_ARM_CR4);
    brcmf_chip_resetcore(core, ARMCR4_BCMA_IOCTL_CPUHALT, 0, 0);

    return true;
}

static bool brcmf_chip_ca7_set_active(uint32_t rstvec)
{
    struct brcmf_core *core;

    brcmf_sdio_buscore_activate(rstvec);

    /* restore ARM */
    core = brcmf_chip_get_core(BCMA_CORE_ARM_CA7);
    brcmf_chip_resetcore(core, ARMCR4_BCMA_IOCTL_CPUHALT, 0, 0);

    return true;
}

static bool brcmf_chip_cm3_set_active(void)
{
    struct brcmf_core *core;

    core = brcmf_chip_get_core(BCMA_CORE_INTERNAL_MEM);
    if (!brcmf_chip_iscoreup(core)) {
        brcm_log("SOCRAM core is down after reset?\n");
        return false;
    }

    brcmf_sdio_buscore_activate(0);

    core = brcmf_chip_get_core(BCMA_CORE_ARM_CM3);
    brcmf_chip_resetcore(core, 0, 0, 0);

    return true;
}


void brcmf_chip_set_passive(void)
{
    struct brcmf_core *arm;

    arm = brcmf_chip_get_core(BCMA_CORE_ARM_CR4);
    if (arm) {
        brcmf_chip_cr4_set_passive();
        return;
    }
    arm = brcmf_chip_get_core(BCMA_CORE_ARM_CA7);
    if (arm) {
        brcmf_chip_ca7_set_passive();
        return;
    }
    arm = brcmf_chip_get_core(BCMA_CORE_ARM_CM3);
    if (arm) {
        brcmf_chip_cm3_set_passive();
        return;
    }
}


char *brcmf_chip_name(uint32_t id, uint32_t rev, char *buf, uint32_t len)
{
    const char *fmt;

    fmt = ((id > 0xa000) || (id < 0x4000)) ? "BCM%d/%u" : "BCM%x/%u";
    snprintf(buf, len, fmt, id, rev);
    return buf;
}


static uint32_t brcmf_chip_tcm_rambase(void)
{
    switch (pub.chip) {
    case BRCM_CC_4345_CHIP_ID:
    case BRCM_CC_43454_CHIP_ID:
        return 0x198000;
    case BRCM_CC_4335_CHIP_ID:
    case BRCM_CC_4339_CHIP_ID:
    case BRCM_CC_4350_CHIP_ID:
    case BRCM_CC_4354_CHIP_ID:
    case BRCM_CC_4356_CHIP_ID:
    case BRCM_CC_43567_CHIP_ID:
    case BRCM_CC_43569_CHIP_ID:
    case BRCM_CC_43570_CHIP_ID:
    case BRCM_CC_4358_CHIP_ID:
    case BRCM_CC_43602_CHIP_ID:
    case BRCM_CC_4371_CHIP_ID:
        return 0x180000;
    case BRCM_CC_43465_CHIP_ID:
    case BRCM_CC_43525_CHIP_ID:
    case BRCM_CC_4365_CHIP_ID:
    case BRCM_CC_4366_CHIP_ID:
    case BRCM_CC_43664_CHIP_ID:
    case BRCM_CC_43666_CHIP_ID:
        return 0x200000;
    case BRCM_CC_4359_CHIP_ID:
        return (pub.chiprev < 9) ? 0x180000 : 0x160000;
    case BRCM_CC_4364_CHIP_ID:
    case CY_CC_4373_CHIP_ID:
        return 0x160000;
    case CY_CC_43752_CHIP_ID:
        return 0x170000;
    case BRCM_CC_4378_CHIP_ID:
        return 0x352000;
    case CY_CC_89459_CHIP_ID:
        return ((pub.chiprev < 9) ? 0x180000 : 0x160000);
    default:
        brcm_log("unknown chip: %s\n", pub.name);
        break;
    }
    return INVALID_RAMBASE;
}

static bool brcmf_chip_socram_banksize(struct brcmf_core_priv *core, u8 idx,
                       uint32_t *banksize)
{
    uint32_t bankinfo;
    uint32_t bankidx = (SOCRAM_MEMTYPE_RAM << SOCRAM_BANKIDX_MEMTYPE_SHIFT);

    bankidx |= idx;
    brcmf_chip_core_write32(core, SOCRAMREGOFFS(bankidx), bankidx);
    bankinfo = brcmf_chip_core_read32(core, SOCRAMREGOFFS(bankinfo));
    *banksize = (bankinfo & SOCRAM_BANKINFO_SZMASK) + 1;
    *banksize *= SOCRAM_BANKINFO_SZBASE;
    return !!(bankinfo & SOCRAM_BANKINFO_RETNTRAM_MASK);
}

static void brcmf_chip_socram_ramsize(struct brcmf_core_priv *sr, uint32_t *ramsize,
                      uint32_t *srsize)
{
    uint32_t coreinfo;
    uint32_t i, nb, banksize, lss;
    bool retent;

    *ramsize = 0;
    *srsize = 0;

    if (!brcmf_chip_iscoreup(&sr->pub))
        brcmf_chip_resetcore(&sr->pub, 0, 0, 0);

    /* Get info for determining size */
    coreinfo = brcmf_chip_core_read32(sr, SOCRAMREGOFFS(coreinfo));
    nb = (coreinfo & SRCI_SRNB_MASK) >> SRCI_SRNB_SHIFT;

    if ((sr->pub.rev <= 7) || (sr->pub.rev == 12)) {
        banksize = (coreinfo & SRCI_SRBSZ_MASK);
        lss = (coreinfo & SRCI_LSS_MASK) >> SRCI_LSS_SHIFT;
        if (lss != 0)
            nb--;
        *ramsize = nb * (1 << (banksize + SR_BSZ_BASE));
        if (lss != 0)
            *ramsize += (1 << ((lss - 1) + SR_BSZ_BASE));
    } else {
        /* length of SRAM Banks increased for corerev greater than 23 */
        if (sr->pub.rev >= 23) {
            nb = (coreinfo & (SRCI_SRNB_MASK | SRCI_SRNB_MASK_EXT))
                >> SRCI_SRNB_SHIFT;
        } else {
            nb = (coreinfo & SRCI_SRNB_MASK) >> SRCI_SRNB_SHIFT;
        }
        for (i = 0; i < nb; i++) {
            retent = brcmf_chip_socram_banksize(sr, i, &banksize);
            *ramsize += banksize;
            if (retent)
                *srsize += banksize;
        }
    }

    /* hardcoded save&restore memory sizes */
    switch (pub.chip) {
    case BRCM_CC_4334_CHIP_ID:
        if (pub.chiprev < 2)
            *srsize = (32 * 1024);
        break;
    case BRCM_CC_43430_CHIP_ID:
    case CY_CC_43439_CHIP_ID:
        /* assume sr for now as we can not check
         * firmware sr capability at this point.
         */
        *srsize = (64 * 1024);
        break;
    default:
        break;
    }
}

/** Return the SYS MEM size */
static uint32_t brcmf_chip_sysmem_ramsize(struct brcmf_core_priv *sysmem)
{
    uint32_t memsize = 0;
    uint32_t coreinfo;
    uint32_t idx;
    uint32_t nb;
    uint32_t banksize;

    if (!brcmf_chip_iscoreup(&sysmem->pub))
        brcmf_chip_resetcore(&sysmem->pub, 0, 0, 0);

    coreinfo = brcmf_chip_core_read32(sysmem, SYSMEMREGOFFS(coreinfo));
    nb = (coreinfo & SRCI_SRNB_MASK) >> SRCI_SRNB_SHIFT;

    for (idx = 0; idx < nb; idx++) {
        brcmf_chip_socram_banksize(sysmem, idx, &banksize);
        memsize += banksize;
    }

    return memsize;
}

/** Return the TCM-RAM size of the ARMCR4 core. */
static uint32_t brcmf_chip_tcm_ramsize(struct brcmf_core_priv *cr4)
{
    uint32_t corecap;
    uint32_t memsize = 0;
    uint32_t nab;
    uint32_t nbb;
    uint32_t totb;
    uint32_t bxinfo;
    uint32_t idx;

    corecap = brcmf_chip_core_read32(cr4, ARMCR4_CAP);

    nab = (corecap & ARMCR4_TCBANB_MASK) >> ARMCR4_TCBANB_SHIFT;
    nbb = (corecap & ARMCR4_TCBBNB_MASK) >> ARMCR4_TCBBNB_SHIFT;
    totb = nab + nbb;

    for (idx = 0; idx < totb; idx++) {
        brcmf_chip_core_write32(cr4, ARMCR4_BANKIDX, idx);
        bxinfo = brcmf_chip_core_read32(cr4, ARMCR4_BANKINFO);
        memsize += ((bxinfo & ARMCR4_BSZ_MASK) + 1) * ARMCR4_BSZ_MULT;
    }

    return memsize;
}

static int brcmf_chip_get_raminfo(void){
    struct brcmf_core_priv *mem_core;
    struct brcmf_core *mem;
    brcm_log("brcmf_chip_get_raminfo\n");

    mem = brcmf_chip_get_core(BCMA_CORE_ARM_CR4);
    if (mem) {
        mem_core = container_of(mem, struct brcmf_core_priv, pub);
        pub.ramsize = brcmf_chip_tcm_ramsize(mem_core);
        pub.rambase = brcmf_chip_tcm_rambase();
        if (pub.rambase == INVALID_RAMBASE) {
            brcm_log("RAM base not provided with ARM CR4 core\n");
            return -EINVAL;
        }
    } else {
        mem = brcmf_chip_get_core(BCMA_CORE_SYS_MEM);
        if (mem) {
            mem_core = container_of(mem, struct brcmf_core_priv,
                        pub);
            pub.ramsize = brcmf_chip_sysmem_ramsize(mem_core);
            pub.rambase = brcmf_chip_tcm_rambase();
            if (pub.rambase == INVALID_RAMBASE) {
                brcm_log("RAM base not provided with ARM CA7 core\n");
                return -EINVAL;
            }
        } else {
            mem = brcmf_chip_get_core(BCMA_CORE_INTERNAL_MEM);
            if (!mem) {
                brcm_log("No memory cores found\n");
                return -ENOMEM;
            }
            mem_core = container_of(mem, struct brcmf_core_priv,
                        pub);
            brcmf_chip_socram_ramsize(mem_core, &pub.ramsize,
                            &pub.srsize);
        }
    }
    
    brcm_log("RAM: base=0x%x size=%d (0x%x) sr=%d (0x%x)\n",
          pub.rambase, pub.ramsize, pub.ramsize,
          pub.srsize,  pub.srsize);

    if (!pub.ramsize) {
        brcm_log("RAM size is undetermined\n");
        return -ENOMEM;
        if (pub.ramsize > BRCMF_CHIP_MAX_MEMSIZE) {
            brcm_log("RAM size is incorrect\n");
            return -ENOMEM;
        }
    }

    return 0;
}

int brcmf_chip_recognition(void)
{
    uint32_t regdata;
    uint32_t socitype;
    int ret;

    memset(core_list, 0, sizeof(core_list));
    /* Get CC core rev
     * Chipid is assume to be at offset 0 from SI_ENUM_BASE
     * For different chiptypes or old sdio hosts w/o chipcommon,
     * other ways of recognition should be added here.
     */
    regdata = brcmf_sdio_buscore_read32(CORE_CC_REG(SI_ENUM_BASE_DEFAULT, chipid));
    pub.chip = regdata & CID_ID_MASK;
    pub.chiprev = (regdata & CID_REV_MASK) >> CID_REV_SHIFT;
    socitype = (regdata & CID_TYPE_MASK) >> CID_TYPE_SHIFT;

    brcmf_chip_name(pub.chip, pub.chiprev,
            pub.name, sizeof(pub.name));
    brcm_log("found %s chip: %s\n",
          socitype == SOCI_SB ? "SB" : "AXI", pub.name);


    brcmf_chip_dmp_erom_scan();


    ret = brcmf_chip_cores_check();
    if (ret)
        return ret;

    /* assure chip is passive for core access */
    brcmf_chip_set_passive();

    return brcmf_chip_get_raminfo();
}        



struct brcmf_core *brcmf_chip_get_pmu(void)
{
    struct brcmf_core *cc = brcmf_chip_get_chipcommon();
    struct brcmf_core *pmu;

    /* See if there is separated PMU core available */
    if (cc->rev >= 35 &&
        pub.cc_caps_ext & 0x00000040) {
        pmu = brcmf_chip_get_core(BCMA_CORE_PMU);
        if (pmu)
            return pmu;
    }

    /* Fallback to ChipCommon core for older hardware */
    return cc;
}

bool brcmf_chip_set_active(uint32_t rstvec)
{
    struct brcmf_core *arm;

    arm = brcmf_chip_get_core(BCMA_CORE_ARM_CR4);
    if (arm)
        return brcmf_chip_cr4_set_active(rstvec);
    arm = brcmf_chip_get_core(BCMA_CORE_ARM_CA7);
    if (arm)
        return brcmf_chip_ca7_set_active(rstvec);
    arm = brcmf_chip_get_core(BCMA_CORE_ARM_CM3);
    if (arm)
        return brcmf_chip_cm3_set_active();

    return false;
}

int brcmf_chip_setup(void)
{
    struct brcmf_core_priv *cc;
    struct brcmf_core *pmu;
    uint32_t base;
    uint32_t val;
    int ret = 0;

    cc = core_list[0];
    base = cc->pub.base;

    /* get chipcommon capabilites */
    pub.cc_caps = brcmf_sdio_buscore_read32(CORE_CC_REG(base, capabilities));
    pub.cc_caps_ext = brcmf_sdio_buscore_read32(
                         CORE_CC_REG(base,capabilities_ext));

    /* get pmu caps & rev */
    pmu = brcmf_chip_get_pmu(); /* after reading cc_caps_ext */
    if (pub.cc_caps & CC_CAP_PMU) {
        val = brcmf_sdio_buscore_read32(
                    CORE_CC_REG(pmu->base, pmucapabilities));
        pub.pmurev = val & PCAP_REV_MASK;
        pub.pmucaps = val;
    }

    brcm_log("ccrev=%d, pmurev=%d, pmucaps=0x%x\n",
          cc->pub.rev, pub.pmurev, pub.pmucaps);

    return ret;
}

struct brcmf_chip *brcmf_chip_attach(void)
{
    int err = 0;

    err = brcmf_chip_recognition();
    if (err < 0)
        goto fail;

    err = brcmf_chip_setup();
    if (err < 0)
        goto fail;

    return &pub;

fail:
    //brcmf_chip_detach(&chip->pub);
    return NULL;
}