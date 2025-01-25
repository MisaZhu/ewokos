#ifndef _SMC91X_H_
#define _SMC91X_H_

#define writel(v,a) 	(*(volatile uint32_t *)(a) = (v))
#define readl(a) 	(*(volatile uint32_t *)(a))
#define writew(v,a) 	(*(volatile uint16_t *)(a) = (v))
#define readw(a) 	(*(volatile uint16_t *)(a))
#define writeb(v,a) 	(*(volatile uint8_t *)(a) = (v))
#define readb(a) 	(*(volatile uint8_t *)(a))


#define SMC_CAN_USE_8BIT	1
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	1
#define SMC_NOWAIT		1

#define SMC_IO_SHIFT	0	

#define SMC_inb(a, r)           readb((a) + (r))
#define SMC_inw(a, r)           readw((a) + (r))
#define SMC_inl(a, r)           readl((a) + (r))
#define SMC_outb(v, a, r)       writeb(v, (a) + (r))
#define SMC_outw(v, a, r)       writew(v, (a) + (r))
#define SMC_outl(v, a, r)       writel(v, (a) + (r))

#define RPC_LSA_DEFAULT         RPC_LED_100_10
#define RPC_LSB_DEFAULT         RPC_LED_TX_RX



#define SMC_CAN_USE_DATACS	0
#define	SMC_IRQ_FLAGS	    0	
#define SMC_INTERRUPT_PREAMBLE
#define SMC_IO_EXTENT	(16 << SMC_IO_SHIFT)
#define SMC_DATA_EXTENT (4)
#define BANK_SELECT		(14 << SMC_IO_SHIFT)


// Transmit Control Register
/* BANK 0  */
#define TCR_REG 	(0)
#define TCR_ENABLE	0x0001	// When 1 we can transmit
#define TCR_LOOP	0x0002	// Controls output pin LBK
#define TCR_FORCOL	0x0004	// When 1 will force a collision
#define TCR_PAD_EN	0x0080	// When 1 will pad tx frames < 64 bytes w/0
#define TCR_NOCRC	0x0100	// When 1 will not append CRC to tx frames
#define TCR_MON_CSN	0x0400	// When 1 tx monitors carrier
#define TCR_FDUPLX    	0x0800  // When 1 enables full duplex operation
#define TCR_STP_SQET	0x1000	// When 1 stops tx if Signal Quality Error
#define TCR_EPH_LOOP	0x2000	// When 1 enables EPH block loopback
#define TCR_SWFDUP	0x8000	// When 1 enables Switched Full Duplex mode

#define TCR_CLEAR	0	/* do NOTHING */
/* the default settings for the TCR register : */
#define TCR_DEFAULT	(TCR_ENABLE | TCR_PAD_EN)


// EPH Status Register
/* BANK 0  */
#define EPH_STATUS_REG	(0x0002)
#define ES_TX_SUC	0x0001	// Last TX was successful
#define ES_SNGL_COL	0x0002	// Single collision detected for last tx
#define ES_MUL_COL	0x0004	// Multiple collisions detected for last tx
#define ES_LTX_MULT	0x0008	// Last tx was a multicast
#define ES_16COL	0x0010	// 16 Collisions Reached
#define ES_SQET		0x0020	// Signal Quality Error Test
#define ES_LTXBRD	0x0040	// Last tx was a broadcast
#define ES_TXDEFR	0x0080	// Transmit Deferred
#define ES_LATCOL	0x0200	// Late collision detected on last tx
#define ES_LOSTCARR	0x0400	// Lost Carrier Sense
#define ES_EXC_DEF	0x0800	// Excessive Deferral
#define ES_CTR_ROL	0x1000	// Counter Roll Over indication
#define ES_LINK_OK	0x4000	// Driven by inverted value of nLNK pin
#define ES_TXUNRN	0x8000	// Tx Underrun


// Receive Control Register
/* BANK 0  */
#define RCR_REG         4
#define RCR_RX_ABORT	0x0001	// Set if a rx frame was aborted
#define RCR_PRMS	0x0002	// Enable promiscuous mode
#define RCR_ALMUL	0x0004	// When set accepts all multicast frames
#define RCR_RXEN	0x0100	// IFF this is set, we can receive packets
#define RCR_STRIP_CRC	0x0200	// When set strips CRC from rx packets
#define RCR_ABORT_ENB	0x0200	// When set will abort rx on collision
#define RCR_FILT_CAR	0x0400	// When set filters leading 12 bit s of carrier
#define RCR_SOFTRST	0x8000 	// resets the chip

/* the normal settings for the RCR register : */
#define RCR_DEFAULT	(RCR_STRIP_CRC | RCR_RXEN)
#define RCR_CLEAR	0x0	// set it to a base state


// Counter Register
/* BANK 0  */
#define COUNTER_REG 	6


// Memory Information Register
/* BANK 0  */
#define MIR_REG    		8


// Receive/Phy Control Register
/* BANK 0  */
#define RPC_REG     0xA
#define RPC_SPEED	0x2000	// When 1 PHY is in 100Mbps mode.
#define RPC_DPLX	0x1000	// When 1 PHY is in Full-Duplex Mode
#define RPC_ANEG	0x0800	// When 1 PHY is in Auto-Negotiate Mode
#define RPC_LSXA_SHFT	5	// Bits to shift LS2A,LS1A,LS0A to lsb
#define RPC_LSXB_SHFT	2	// Bits to get LS2B,LS1B,LS0B to lsb

#define RPC_DEFAULT (RPC_ANEG | RPC_SPEED | RPC_DPLX)


/* Bank 0 0x0C is reserved */

// Bank Select Register
/* All Banks */
#define BSR_REG		0x000E


// Configuration Reg
/* BANK 1 */
#define CONFIG_REG     0
#define CONFIG_EXT_PHY	0x0200	// 1=external MII, 0=internal Phy
#define CONFIG_GPCNTRL	0x0400	// Inverse value drives pin nCNTRL
#define CONFIG_NO_WAIT	0x1000	// When 1 no extra wait states on ISA bus
#define CONFIG_EPH_POWER_EN 0x8000 // When 0 EPH is placed into low power mode.

// Default is powered-up, Internal Phy, Wait States, and pin nCNTRL=low
#define CONFIG_DEFAULT	(CONFIG_EPH_POWER_EN)


// Base Address Register
/* BANK 1 */
#define BASE_REG        2


// Individual Address Registers
/* BANK 1 */
#define ADDR0_REG       4
#define ADDR1_REG       6
#define ADDR2_REG       8


// General Purpose Register
/* BANK 1 */
#define GP_REG          0xA


// Control Register
/* BANK 1 */
#define CTL_REG     0xC
#define CTL_RCV_BAD	0x4000 // When 1 bad CRC packets are received
#define CTL_AUTO_RELEASE 0x0800 // When 1 tx pages are released automatically
#define CTL_LE_ENABLE	0x0080 // When 1 enables Link Error interrupt
#define CTL_CR_ENABLE	0x0040 // When 1 enables Counter Rollover interrupt
#define CTL_TE_ENABLE	0x0020 // When 1 enables Transmit Error interrupt
#define CTL_EEPROM_SELECT 0x0004 // Controls EEPROM reload & store
#define CTL_RELOAD	0x0002 // When set reads EEPROM into registers
#define CTL_STORE	0x0001 // When set stores registers into EEPROM


// MMU Command Register
/* BANK 2 */
#define MMU_CMD_REG     0
#define MC_BUSY		1	// When 1 the last release has not completed
#define MC_NOP		(0<<5)	// No Op
#define MC_ALLOC	(1<<5) 	// OR with number of 256 byte packets
#define MC_RESET	(2<<5)	// Reset MMU to initial state
#define MC_REMOVE	(3<<5) 	// Remove the current rx packet
#define MC_RELEASE  	(4<<5) 	// Remove and release the current rx packet
#define MC_FREEPKT  	(5<<5) 	// Release packet in PNR register
#define MC_ENQUEUE	(6<<5)	// Enqueue the packet for transmit
#define MC_RSTTXFIFO	(7<<5)	// Reset the TX FIFOs


// Packet Number Register
/* BANK 2 */
#define PN_REG          2


// Allocation Result Register
/* BANK 2 */
#define AR_REG            3
#define AR_FAILED	0x80	// Alocation Failed


// TX FIFO Ports Register
/* BANK 2 */
#define TXFIFO_REG      4
#define TXFIFO_TEMPTY	0x80	// TX FIFO Empty

// RX FIFO Ports Register
/* BANK 2 */
#define RXFIFO_REG      5
#define RXFIFO_REMPTY	0x80	// RX FIFO Empty

#define FIFO_REG        4

// Pointer Register
/* BANK 2 */
#define PTR_REG         6
#define PTR_RCV		0x8000 // 1=Receive area, 0=Transmit area
#define PTR_AUTOINC 	0x4000 // Auto increment the pointer on each access
#define PTR_READ	0x2000 // When 1 the operation is a read


// Data Register
/* BANK 2 */
#define DATA_REG           8


// Interrupt Status/Acknowledge Register
/* BANK 2 */
#define INT_REG            0xC


// Interrupt Mask Register
/* BANK 2 */
#define IM_REG              0xD
#define IM_MDINT	0x80 // PHY MI Register 18 Interrupt
#define IM_ERCV_INT	0x40 // Early Receive Interrupt
#define IM_EPH_INT	0x20 // Set by Ethernet Protocol Handler section
#define IM_RX_OVRN_INT	0x10 // Set by Receiver Overruns
#define IM_ALLOC_INT	0x08 // Set when allocation request is completed
#define IM_TX_EMPTY_INT	0x04 // Set if the TX FIFO goes empty
#define IM_TX_INT	0x02 // Transmit Interrupt
#define IM_RCV_INT	0x01 // Receive Interrupt


// Multicast Table Registers
/* BANK 3 */
#define MCAST_REG1              0
#define MCAST_REG2              2
#define MCAST_REG3              4
#define MCAST_REG4              6


// Management Interface Register (MII)
/* BANK 3 */
#define MII_REG                 8
#define MII_MSK_CRS100	0x4000 // Disables CRS100 detection during tx half dup
#define MII_MDOE	0x0008 // MII Output Enable
#define MII_MCLK	0x0004 // MII Clock, pin MDCLK
#define MII_MDI		0x0002 // MII Input, pin MDI
#define MII_MDO		0x0001 // MII Output, pin MDO


// Revision Register
/* BANK 3 */
/* ( hi: chip id   low: rev # ) */
#define REV_REG             0xA


// Early RCV Register
/* BANK 3 */
/* this is NOT on SMC9192 */
#define ERCV_REG        0xC
#define ERCV_RCV_DISCRD	0x0080 // When 1 discards a packet being received
#define ERCV_THRESHOLD	0x001F // ERCV Threshold Mask

/*
 . Receive status bits
*/
#define RS_ALGNERR	0x8000
#define RS_BRODCAST	0x4000
#define RS_BADCRC	0x2000
#define RS_ODDFRAME	0x1000
#define RS_TOOLONG	0x0800
#define RS_TOOSHORT	0x0400
#define RS_MULTICAST	0x0001
#define RS_ERRORS	(RS_ALGNERR | RS_BADCRC | RS_TOOLONG | RS_TOOSHORT)


/*
 * PHY IDs
 *  LAN83C183 == LAN91C111 Internal PHY
 */
#define PHY_LAN83C183	0x0016f840
#define PHY_LAN83C180	0x02821c50

/*
 * PHY Register Addresses (LAN91C111 Internal PHY)
 *
 * Generic PHY registers can be found in <linux/mii.h>
 *
 * These phy registers are specific to our on-board phy.
 */

// PHY Configuration Register 1
#define PHY_CFG1_REG		0x10
#define PHY_CFG1_LNKDIS		0x8000	// 1=Rx Link Detect Function disabled
#define PHY_CFG1_XMTDIS		0x4000	// 1=TP Transmitter Disabled
#define PHY_CFG1_XMTPDN		0x2000	// 1=TP Transmitter Powered Down
#define PHY_CFG1_BYPSCR		0x0400	// 1=Bypass scrambler/descrambler
#define PHY_CFG1_UNSCDS		0x0200	// 1=Unscramble Idle Reception Disable
#define PHY_CFG1_EQLZR		0x0100	// 1=Rx Equalizer Disabled
#define PHY_CFG1_CABLE		0x0080	// 1=STP(150ohm), 0=UTP(100ohm)
#define PHY_CFG1_RLVL0		0x0040	// 1=Rx Squelch level reduced by 4.5db
#define PHY_CFG1_TLVL_SHIFT	2	// Transmit Output Level Adjust
#define PHY_CFG1_TLVL_MASK	0x003C
#define PHY_CFG1_TRF_MASK	0x0003	// Transmitter Rise/Fall time


// PHY Configuration Register 2
#define PHY_CFG2_REG		0x11
#define PHY_CFG2_APOLDIS	0x0020	// 1=Auto Polarity Correction disabled
#define PHY_CFG2_JABDIS		0x0010	// 1=Jabber disabled
#define PHY_CFG2_MREG		0x0008	// 1=Multiple register access (MII mgt)
#define PHY_CFG2_INTMDIO	0x0004	// 1=Interrupt signaled with MDIO pulseo

// PHY Status Output (and Interrupt status) Register
#define PHY_INT_REG		0x12	// Status Output (Interrupt Status)
#define PHY_INT_INT		0x8000	// 1=bits have changed since last read
#define PHY_INT_LNKFAIL		0x4000	// 1=Link Not detected
#define PHY_INT_LOSSSYNC	0x2000	// 1=Descrambler has lost sync
#define PHY_INT_CWRD		0x1000	// 1=Invalid 4B5B code detected on rx
#define PHY_INT_SSD		0x0800	// 1=No Start Of Stream detected on rx
#define PHY_INT_ESD		0x0400	// 1=No End Of Stream detected on rx
#define PHY_INT_RPOL		0x0200	// 1=Reverse Polarity detected
#define PHY_INT_JAB		0x0100	// 1=Jabber detected
#define PHY_INT_SPDDET		0x0080	// 1=100Base-TX mode, 0=10Base-T mode
#define PHY_INT_DPLXDET		0x0040	// 1=Device in Full Duplex

// PHY Interrupt/Status Mask Register
#define PHY_MASK_REG		0x13	// Interrupt Mask
// Uses the same bit definitions as PHY_INT_REG


#define SMC_GET_PN()	    (SMC_inb(SMC_BASE, PN_REG))
#define SMC_SET_PN(x)	    (SMC_outb(x, SMC_BASE, PN_REG))
#define SMC_GET_AR()	    (SMC_inb(SMC_BASE, AR_REG))
#define SMC_GET_TXFIFO()	(SMC_inb(SMC_BASE, TXFIFO_REG))
#define SMC_GET_RXFIFO()	(SMC_inb(SMC_BASE, RXFIFO_REG))
#define SMC_GET_INT()		(SMC_inb(SMC_BASE, INT_REG))
#define SMC_ACK_INT(x)	    (SMC_outb(x, SMC_BASE, INT_REG))
#define SMC_GET_INT_MASK()  (SMC_inb(SMC_BASE, IM_REG))

#define SMC_SET_INT_MASK(x)	(SMC_outb(x, SMC_BASE, IM_REG))
#define SMC_CURRENT_BANK()	(SMC_inw(SMC_BASE, BANK_SELECT))
#define SMC_SELECT_BANK(x)	(SMC_outw(x, SMC_BASE, BANK_SELECT))
#define SMC_GET_BASE()		(SMC_inw(SMC_BASE, BASE_REG))
#define SMC_SET_BASE(x)	    (SMC_outw(x, SMC_BASE, BASE_REG))
#define SMC_GET_CONFIG()	(SMC_inw(SMC_BASE, CONFIG_REG))
#define SMC_SET_CONFIG(x)	(SMC_outw(x, SMC_BASE, CONFIG_REG))
#define SMC_GET_COUNTER()	(SMC_inw(SMC_BASE, COUNTER_REG))
#define SMC_GET_CTL()		(SMC_inw(SMC_BASE, CTL_REG))
#define SMC_SET_CTL(x)	    (SMC_outw(x, SMC_BASE, CTL_REG))
#define SMC_GET_MII()		(SMC_inw(SMC_BASE, MII_REG))
#define SMC_GET_GP()		(SMC_inw(SMC_BASE, GP_REG))
#define SMC_SET_GP(x)	    (SMC_outw(x, SMC_BASE, GP_REG))

#define SMC_SET_MII(x)	    (SMC_outw(x, SMC_BASE, MII_REG))
#define SMC_GET_MIR()		(SMC_inw(SMC_BASE, MIR_REG))
#define SMC_SET_MIR(x)	    (SMC_outw(x, SMC_BASE, MIR_REG))
#define SMC_GET_MMU_CMD()	(SMC_inw(SMC_BASE, MMU_CMD_REG))
#define SMC_SET_MMU_CMD(x)	(SMC_outw(x, SMC_BASE, MMU_CMD_REG))
#define SMC_GET_FIFO()	    (SMC_inw(SMC_BASE, FIFO_REG))
#define SMC_GET_PTR()		(SMC_inw(SMC_BASE, PTR_REG))
#define SMC_SET_PTR(x)	    (SMC_outw(x, SMC_BASE, PTR_REG))
#define SMC_GET_EPH_STATUS() (SMC_inw(SMC_BASE, EPH_STATUS_REG))
#define SMC_GET_RCR()		(SMC_inw(SMC_BASE, RCR_REG))
#define SMC_SET_RCR(x)		(SMC_outw(x, SMC_BASE, RCR_REG))
#define SMC_GET_REV()		(SMC_inw(SMC_BASE, REV_REG))
#define SMC_GET_RPC()		(SMC_inw(SMC_BASE, RPC_REG))
#define SMC_SET_RPC(x)	    (SMC_outw(x, SMC_BASE, RPC_REG))
#define SMC_GET_TCR()		(SMC_inw(SMC_BASE, TCR_REG))
#define SMC_SET_TCR(x)	    (SMC_outw(x, SMC_BASE, TCR_REG))

#define SMC_GET_MAC_ADDR(addr)					\
	do {								\
		unsigned int __v;					\
		__v = SMC_inw(SMC_BASE, ADDR0_REG);			\
		addr[0] = __v; addr[1] = __v >> 8;			\
		__v = SMC_inw(SMC_BASE, ADDR1_REG);			\
		addr[2] = __v; addr[3] = __v >> 8;			\
		__v = SMC_inw(SMC_BASE, ADDR2_REG);			\
		addr[4] = __v; addr[5] = __v >> 8;			\
	} while (0)

#define SMC_SET_MAC_ADDR(addr)					\
	do {								\
		SMC_outw(addr[0] | (addr[1] << 8), SMC_BASE, ADDR0_REG); \
		SMC_outw(addr[2] | (addr[3] << 8), SMC_BASE, ADDR1_REG); \
		SMC_outw(addr[4] | (addr[5] << 8), SMC_BASE, ADDR2_REG); \
	} while (0)

#define SMC_SET_MCAST(x)						\
	do {								\
		const unsigned char *mt = (x);				\
		SMC_outw(mt[0] | (mt[1] << 8), SMC_BASE, MCAST_REG1); \
		SMC_outw(mt[2] | (mt[3] << 8), SMC_BASE, MCAST_REG2); \
		SMC_outw(mt[4] | (mt[5] << 8), SMC_BASE, MCAST_REG3); \
		SMC_outw(mt[6] | (mt[7] << 8), SMC_BASE, MCAST_REG4); \
	} while (0)

#define SMC_PUT_PKT_HDR(status, length)				\
	do {								\
			SMC_outw(status, SMC_BASE, DATA_REG);	\
			SMC_outw(length, SMC_BASE, DATA_REG);	\
	} while (0)

#define SMC_GET_PKT_HDR(status, length)				\
	do {								\
			(status) = SMC_inw(SMC_BASE, DATA_REG);	\
			(length) = SMC_inw(SMC_BASE, DATA_REG);	\
	} while (0)

#define SMC_PUSH_DATA(p, l)					\
	do {								\
        for(int _i = 0; _i < (l); _i++){    \
		    SMC_outb((p)[_i], SMC_BASE, DATA_REG);	\
        } \
	} while (0)

#define SMC_PULL_DATA(p, l)					\
	do {								\
        for(int _i = 0; _i < (l); _i++){    \
		    (p)[_i] = SMC_inb(SMC_BASE, DATA_REG);		\
        } \
	} while (0)


#define SMC_WAIT_MMU_BUSY() do {					\
	if ((SMC_GET_MMU_CMD() & MC_BUSY)) {		\
		while (SMC_GET_MMU_CMD() & MC_BUSY) {		\
		}							\
	}								\
} while (0)

/* this enables an interrupt in the interrupt mask register */
#define SMC_ENABLE_INT(x) (SMC_SET_INT_MASK(SMC_GET_INT_MASK()|(x)))

/* Generic MII registers. */
#define MII_BMCR		0x00	/* Basic mode control register */
#define MII_BMSR		0x01	/* Basic mode status register  */
#define MII_PHYSID1		0x02	/* PHYS ID 1                   */
#define MII_PHYSID2		0x03	/* PHYS ID 2                   */
#define MII_ADVERTISE		0x04	/* Advertisement control reg   */
#define MII_LPA			0x05	/* Link partner ability reg    */
#define MII_EXPANSION		0x06	/* Expansion register          */
#define MII_CTRL1000		0x09	/* 1000BASE-T control          */
#define MII_STAT1000		0x0a	/* 1000BASE-T status           */
#define	MII_MMD_CTRL		0x0d	/* MMD Access Control Register */
#define	MII_MMD_DATA		0x0e	/* MMD Access Data Register */
#define MII_ESTATUS		0x0f	/* Extended Status             */
#define MII_DCOUNTER		0x12	/* Disconnect counter          */
#define MII_FCSCOUNTER		0x13	/* False carrier counter       */
#define MII_NWAYTEST		0x14	/* N-way auto-neg test reg     */
#define MII_RERRCOUNTER		0x15	/* Receive error counter       */
#define MII_SREVISION		0x16	/* Silicon revision            */
#define MII_RESV1		0x17	/* Reserved...                 */
#define MII_LBRERROR		0x18	/* Lpback, rx, bypass error    */
#define MII_PHYADDR		0x19	/* PHY address                 */
#define MII_RESV2		0x1a	/* Reserved...                 */
#define MII_TPISTATUS		0x1b	/* TPI status for 10mbps       */
#define MII_NCONFIG		0x1c	/* Network interface config    */

#endif  /* _SMC91X_H_ */
