
#ifndef __BRCM_FMAC_H__
#define __BRCM_FMAC_H__

#include "types.h"


/*
 * Software-defined protocol header
 */

/* Current protocol version */
#define SDPCM_PROT_VERSION	4

/*
 * Shared structure between dongle and the host.
 * The structure contains pointers to trap or assert information.
 */
#define SDPCM_SHARED_VERSION       0x0003
#define SDPCM_SHARED_VERSION_MASK  0x00FF
#define SDPCM_SHARED_ASSERT_BUILT  0x0100
#define SDPCM_SHARED_ASSERT        0x0200
#define SDPCM_SHARED_TRAP          0x0400

/* Space for header read, limit for data packets */
#define MAX_HDR_READ	(1 << 6)
#define MAX_RX_DATASZ	2048

/* Bump up limit on waiting for HT to account for first startup;
 * if the image is doing a CRC calculation before programming the PMU
 * for HT availability, it could take a couple hundred ms more, so
 * max out at a 1 second (1000000us).
 */
#undef PMU_MAX_TRANSITION_DLY
#define PMU_MAX_TRANSITION_DLY 1000000

/* Value for ChipClockCSR during initial setup */
#define BRCMF_INIT_CLKCTL1	(SBSDIO_FORCE_HW_CLKREQ_OFF |	\
					SBSDIO_ALP_AVAIL_REQ)

/* Flags for SDH calls */
#define F2SYNC	(SDIO_REQ_4BYTE | SDIO_REQ_FIXED)

#define BRCMF_IDLE_ACTIVE	0	/* Do not request any SD clock change
					 /* when idle
					 */
#define BRCMF_IDLE_INTERVAL	1

#define KSO_WAIT_US 50
#define MAX_KSO_ATTEMPTS (PMU_MAX_TRANSITION_DLY/KSO_WAIT_US)
#define BRCMF_SDIO_MAX_ACCESS_ERRORS	5


#define SDIOD_FBR_SIZE		0x100

/* io_en */
#define SDIO_FUNC_ENABLE_1	0x02
#define SDIO_FUNC_ENABLE_2	0x04

/* io_rdys */
#define SDIO_FUNC_READY_1	0x02
#define SDIO_FUNC_READY_2	0x04

/* intr_status */
#define INTR_STATUS_FUNC1	0x2
#define INTR_STATUS_FUNC2	0x4

/* mask of register map */
#define REG_F0_REG_MASK		0x7FF
#define REG_F1_MISC_MASK	0x1FFFF

/* function 0 vendor specific CCCR registers */

#define SDIO_CCCR_BRCM_CARDCAP			0xf0
#define SDIO_CCCR_BRCM_CARDCAP_CMD14_SUPPORT	BIT(1)
#define SDIO_CCCR_BRCM_CARDCAP_CMD14_EXT	BIT(2)
#define SDIO_CCCR_BRCM_CARDCAP_CMD_NODEC	BIT(3)

/* Interrupt enable bits for each function */
#define SDIO_CCCR_IEN_FUNC0			BIT(0)
#define SDIO_CCCR_IEN_FUNC1			BIT(1)
#define SDIO_CCCR_IEN_FUNC2			BIT(2)

#define SDIO_CCCR_BRCM_CARDCTRL			0xf1
#define SDIO_CCCR_BRCM_CARDCTRL_WLANRESET	BIT(1)

#define SDIO_CCCR_BRCM_SEPINT			0xf2
#define SDIO_CCCR_BRCM_SEPINT_MASK		BIT(0)
#define SDIO_CCCR_BRCM_SEPINT_OE		BIT(1)
#define SDIO_CCCR_BRCM_SEPINT_ACT_HI		BIT(2)

/* function 1 miscellaneous registers */

/* sprom command and status */
#define SBSDIO_SPROM_CS			0x10000
/* sprom info register */
#define SBSDIO_SPROM_INFO		0x10001
/* sprom indirect access data byte 0 */
#define SBSDIO_SPROM_DATA_LOW		0x10002
/* sprom indirect access data byte 1 */
#define SBSDIO_SPROM_DATA_HIGH		0x10003
/* sprom indirect access addr byte 0 */
#define SBSDIO_SPROM_ADDR_LOW		0x10004
/* gpio select */
#define SBSDIO_GPIO_SELECT		0x10005
/* gpio output */
#define SBSDIO_GPIO_OUT			0x10006
/* gpio enable */
#define SBSDIO_GPIO_EN			0x10007
/* rev < 7, watermark for sdio device TX path */
#define SBSDIO_WATERMARK		0x10008
/* control busy signal generation */
#define SBSDIO_DEVICE_CTL		0x10009

/* SB Address Window Low (b15) */
#define SBSDIO_FUNC1_SBADDRLOW		0x1000A
/* SB Address Window Mid (b23:b16) */
#define SBSDIO_FUNC1_SBADDRMID		0x1000B
/* SB Address Window High (b31:b24)    */
#define SBSDIO_FUNC1_SBADDRHIGH		0x1000C
/* Frame Control (frame term/abort) */
#define SBSDIO_FUNC1_FRAMECTRL		0x1000D
/* ChipClockCSR (ALP/HT ctl/status) */
#define SBSDIO_FUNC1_CHIPCLKCSR		0x1000E
/* SdioPullUp (on cmd, d0-d2) */
#define SBSDIO_FUNC1_SDIOPULLUP		0x1000F
/* Write Frame Byte Count Low */
#define SBSDIO_FUNC1_WFRAMEBCLO		0x10019
/* Write Frame Byte Count High */
#define SBSDIO_FUNC1_WFRAMEBCHI		0x1001A
/* Read Frame Byte Count Low */
#define SBSDIO_FUNC1_RFRAMEBCLO		0x1001B
/* Read Frame Byte Count High */
#define SBSDIO_FUNC1_RFRAMEBCHI		0x1001C
/* MesBusyCtl (rev 11) */
#define SBSDIO_FUNC1_MESBUSYCTRL	0x1001D
/* Watermark for sdio device RX path */
#define SBSDIO_MESBUSY_RXFIFO_WM_MASK	0x7F
#define SBSDIO_MESBUSY_RXFIFO_WM_SHIFT	0
/* Enable busy capability for MES access */
#define SBSDIO_MESBUSYCTRL_ENAB		0x80
#define SBSDIO_MESBUSYCTRL_ENAB_SHIFT	7

/* Sdio Core Rev 12 */
#define SBSDIO_FUNC1_WAKEUPCTRL		0x1001E
#define SBSDIO_FUNC1_WCTRL_ALPWAIT_MASK		0x1
#define SBSDIO_FUNC1_WCTRL_ALPWAIT_SHIFT	0
#define SBSDIO_FUNC1_WCTRL_HTWAIT_MASK		0x2
#define SBSDIO_FUNC1_WCTRL_HTWAIT_SHIFT		1
#define SBSDIO_FUNC1_SLEEPCSR		0x1001F
#define SBSDIO_FUNC1_SLEEPCSR_KSO_MASK		0x1
#define SBSDIO_FUNC1_SLEEPCSR_KSO_SHIFT		0
#define SBSDIO_FUNC1_SLEEPCSR_KSO_EN		1
#define SBSDIO_FUNC1_SLEEPCSR_DEVON_MASK	0x2
#define SBSDIO_FUNC1_SLEEPCSR_DEVON_SHIFT	1

#define SBSDIO_FUNC1_MISC_REG_START	0x10000	/* f1 misc register start */
#define SBSDIO_FUNC1_MISC_REG_LIMIT	0x1001F	/* f1 misc register end */

/* function 1 OCP space */

/* sb offset addr is <= 15 bits, 32k */
#define SBSDIO_SB_OFT_ADDR_MASK		0x07FFF
#define SBSDIO_SB_OFT_ADDR_LIMIT	0x08000
/* with b15, maps to 32-bit SB access */
#define SBSDIO_SB_ACCESS_2_4B_FLAG	0x08000

/* Address bits from SBADDR regs */
#define SBSDIO_SBWINDOW_MASK		0xffff8000

#define SDIOH_READ              0	/* Read request */
#define SDIOH_WRITE             1	/* Write request */

#define SDIOH_DATA_FIX          0	/* Fixed addressing */
#define SDIOH_DATA_INC          1	/* Incremental addressing */

/* internal return code */
#define SUCCESS	0
#define ERROR	1

/* Packet alignment for most efficient SDIO (can change based on platform) */
#define BRCMF_SDALIGN	(1 << 6)

/* watchdog polling interval */
#define BRCMF_WD_POLL	msecs_to_jiffies(10)

#define	BRCMF_FW_NAME_LEN		320


enum brcmf_sdiod_state {
	BRCMF_SDIOD_DOWN,
	BRCMF_SDIOD_DATA,
	BRCMF_SDIOD_NOMEDIUM
};

/* sdio core registers */
struct sdpcmd_regs {
    u32 corecontrol;        /* 0x00, rev8 */
    u32 corestatus;         /* rev8 */
    u32 PAD0[1];
    u32 biststatus;         /* rev8 */

    /* PCMCIA access */
    u16 pcmciamesportaladdr;    /* 0x010, rev8 */
    u16 PAD1[1];
    u16 pcmciamesportalmask;    /* rev8 */
    u16 PAD2[1];
    u16 pcmciawrframebc;        /* rev8 */
    u16 PAD3[1];
    u16 pcmciaunderflowtimer;   /* rev8 */
    u16 PAD4[1];

    /* interrupt */
    u32 intstatus;          /* 0x020, rev8 */
    u32 hostintmask;        /* rev8 */
    u32 intmask;            /* rev8 */
    u32 sbintstatus;        /* rev8 */
    u32 sbintmask;          /* rev8 */
    u32 funcintmask;        /* rev4 */
    u32 PAD5[2];
    u32 tosbmailbox;        /* 0x040, rev8 */
    u32 tohostmailbox;      /* rev8 */
    u32 tosbmailboxdata;        /* rev8 */
    u32 tohostmailboxdata;      /* rev8 */

    /* synchronized access to registers in SDIO clock domain */
    u32 sdioaccess;         /* 0x050, rev8 */
    u32 PAD6[3];

    /* PCMCIA frame control */
    u8 pcmciaframectrl;     /* 0x060, rev8 */
    u8 PAD7[3];
    u8 pcmciawatermark;     /* rev8 */
    u8 PAD8[155];

    /* interrupt batching control */
    u32 intrcvlazy;         /* 0x100, rev8 */
    u32 PA9[3];

    /* counters */
    u32 cmd52rd;            /* 0x110, rev8 */
    u32 cmd52wr;            /* rev8 */
    u32 cmd53rd;            /* rev8 */
    u32 cmd53wr;            /* rev8 */
    u32 abort;          /* rev8 */
    u32 datacrcerror;       /* rev8 */
    u32 rdoutofsync;        /* rev8 */
    u32 wroutofsync;        /* rev8 */
    u32 writebusy;          /* rev8 */
    u32 readwait;           /* rev8 */
    u32 readterm;           /* rev8 */
    u32 writeterm;          /* rev8 */
    u32 PAD10[40];
    u32 clockctlstatus;     /* rev8 */
    u32 PAD11[7];

    u32 PAD12[128];           /* DMA engines */

    /* SDIO/PCMCIA CIS region */
    char cis[512];          /* 0x400-0x5ff, rev6 */

    /* PCMCIA function control registers */
    char pcmciafcr[256];        /* 0x600-6ff, rev6 */
    u16 PAD13[55];

    /* PCMCIA backplane access */
    u16 backplanecsr;       /* 0x76E, rev6 */
    u16 backplaneaddr0;     /* rev6 */
    u16 backplaneaddr1;     /* rev6 */
    u16 backplaneaddr2;     /* rev6 */
    u16 backplaneaddr3;     /* rev6 */
    u16 backplanedata0;     /* rev6 */
    u16 backplanedata1;     /* rev6 */
    u16 backplanedata2;     /* rev6 */
    u16 backplanedata3;     /* rev6 */
    u16 PAD14[31];

    /* sprom "size" & "blank" info */
    u16 spromstatus;        /* 0x7BE, rev2 */
    u32 PAD15[464];

    u16 PAD16[0x80];
};

#define SD_REG(field) \
        (offsetof(struct sdpcmd_regs, field))
		
#define CONSOLE_LINE_MAX    192

struct rte_log_le {
    uint32_t buf;     /* Can't be pointer on (64-bit) hosts */
    uint32_t buf_size;
    uint32_t idx;
    char *_buf_compat;  /* Redundant pointer for backward compat. */
};


struct rte_console {
    /* Virtual UART
     * When there is no UART (e.g. Quickturn),
     * the host should write a complete
     * input line directly into cbuf and then write
     * the length into vcons_in.
     * This may also be used when there is a real UART
     * (at risk of conflicting with
     * the real UART).  vcons_out is currently unused.
     */
    uint vcons_in;
    uint vcons_out;

    /* Output (logging) buffer
     * Console output is written to a ring buffer log_buf at index log_idx.
     * The host may read the output when it sees log_idx advance.
     * Output will be lost if the output wraps around faster than the host
     * polls.
     */
    struct rte_log_le log_le;

    /* Console input line buffer
     * Characters are read one at a time into cbuf
     * until <CR> is received, then
     * the buffer is processed as a command line.
     * Also used for virtual UART.
     */
    uint cbuf_idx;
    char cbuf[128];
};

/* Device console log buffer state */
struct brcmf_console {
    uint32_t console_addr;
	uint count;		/* Poll interval msec counter */
	uint log_addr;		/* Log struct address (fixed) */
	struct rte_log_le log_le;	/* Log struct (host copy) */
	uint bufsize;		/* Size of log buffer */
	u8 *buf;		/* Log buffer (host copy) */
	uint last;		/* Last buffer read index */
};

struct brcmf_trap_info {
	uint32_t		type;
	uint32_t		epc;
	uint32_t		cpsr;
	uint32_t		spsr;
	uint32_t		r0;	/* a1 */
	uint32_t		r1;	/* a2 */
	uint32_t		r2;	/* a3 */
	uint32_t		r3;	/* a4 */
	uint32_t		r4;	/* v1 */
	uint32_t		r5;	/* v2 */
	uint32_t		r6;	/* v3 */
	uint32_t		r7;	/* v4 */
	uint32_t		r8;	/* v5 */
	uint32_t		r9;	/* sb/v6 */
	uint32_t		r10;	/* sl/v7 */
	uint32_t		r11;	/* fp/v8 */
	uint32_t		r12;	/* ip */
	uint32_t		r13;	/* sp */
	uint32_t		r14;	/* lr */
	uint32_t		pc;	/* r15 */
};

struct sdpcm_shared {
	u32 flags;
	u32 trap_addr;
	u32 assert_exp_addr;
	u32 assert_file_addr;
	u32 assert_line;
	u32 console_addr;	/* Address of struct rte_console */
	u32 msgtrace_addr;
	u8 tag[32];
	u32 brpt_addr;
};

struct sdpcm_shared_le {
	uint32_t flags;
	uint32_t trap_addr;
	uint32_t assert_exp_addr;
	uint32_t assert_file_addr;
	uint32_t assert_line;
	uint32_t console_addr;	/* Address of struct rte_console */
	uint32_t msgtrace_addr;
	u8 tag[32];
	uint32_t brpt_addr;
};

#endif
