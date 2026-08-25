#ifndef __ARCH_VIRT_XHCI_H
#define __ARCH_VIRT_XHCI_H

/*
 * Polled xHCI host controller driver (ported from the raspi5 RP1 driver).
 *
 * On the QEMU virt machine the controller is a plain PCI xHCI device
 * (-device qemu-xhci): the arch/virt pci layer locates it via ECAM,
 * enables its BAR and hands the MMIO base to xhci_init(). No firmware
 * or DWC3 machinery is involved.
 *
 * The driver is fully polled: no interrupts are enabled, the caller
 * drains the event ring from its main loop. Interrupt-IN endpoints do
 * not need software NAK pacing - the controller schedules the periodic
 * transactions in hardware and only posts a transfer event when a TD
 * actually completes.
 *
 * DMA: the virt PCIe host has no dma-ranges, so bus addresses are CPU
 * physical addresses 1:1. All rings, contexts and data buffers come
 * from the sys_dma pool, which the kernel maps uncached into this
 * process, so no cache maintenance is required.
 */

#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/ewokdef.h>
#include <usb/usb_defs.h>

#define XHCI_MAX_PORTS      8
#define XHCI_MAX_SLOTS      16
#define XHCI_MAX_EPS        32   /* device context entries (DCI 0..31) */

/* xHCI/USB speed IDs (PORTSC.PS and slot context speed field) */
#define XHCI_SPEED_FULL     1
#define XHCI_SPEED_LOW      2
#define XHCI_SPEED_HIGH     3
#define XHCI_SPEED_SUPER    4

typedef struct xhci_hc xhci_hc_t;

/* per-endpoint transfer state (one in-flight TD at a time) */
typedef struct {
	bool     open;
	bool     in_flight;
	bool     done;
	uint8_t  comp_code;
	uint32_t xfer_len;      /* bytes actually transferred when done */
	uint16_t mps;
	uint32_t buf_len;       /* posted TD length */
	uint64_t td_last_phys;  /* bus addr of the event-generating TRB */
	uint32_t short_left;    /* residue latched from a mid-TD short packet */
	/* transfer ring */
	uint64_t ring_phys;
	uint32_t* ring;
	uint32_t enq;
	uint32_t cycle;
	/* DMA data buffer for interrupt IN */
	uint64_t data_phys;
	uint8_t* data;
} xhci_ep_t;

typedef struct xhci_dev {
	bool     used;
	xhci_hc_t* hc;
	uint8_t  slot_id;
	uint8_t  speed;         /* XHCI_SPEED_* */
	uint8_t  root_port;     /* 1-based root hub port */
	uint32_t route;         /* xHCI route string */
	uint8_t  depth;         /* 0 = attached to root port */
	uint16_t mps0;          /* current EP0 max packet size, in bytes */
	/* TT info for LS/FS devices behind a HS hub */
	uint8_t  tt_slot;
	uint8_t  tt_port;
	void*    arena;         /* per-device DMA arena (xhci.c internal) */
	/* contexts */
	uint64_t dev_ctx_phys;
	uint8_t* dev_ctx;
	uint64_t in_ctx_phys;
	uint8_t* in_ctx;
	xhci_ep_t eps[XHCI_MAX_EPS]; /* indexed by DCI, [1] = EP0 */
} xhci_dev_t;

struct xhci_hc {
	bool     present;
	/*
	 * Sticky: USBSTS reported HSE/HCE, the controller is dead until the
	 * whole daemon restarts. Every entry point turns into a no-op so the
	 * poll loop stops burning time on a controller that will never answer.
	 */
	bool     failed;
	int      id;            /* 0 or 1 */
	ewokos_addr_t base;     /* capability register base (virtual) */
	ewokos_addr_t op;       /* operational registers */
	ewokos_addr_t rt;       /* runtime registers */
	ewokos_addr_t db;       /* doorbell array */
	uint32_t csz;           /* context stride: 32 or 64 bytes */
	uint32_t max_slots;
	uint32_t num_ports;
	/* DCBAA */
	uint64_t dcbaa_phys;
	uint64_t* dcbaa;
	/* command ring */
	uint64_t cmd_phys;
	uint32_t* cmd;
	uint32_t cmd_enq;
	uint32_t cmd_cycle;
	/* event ring: one segment */
	uint64_t evt_phys;
	uint32_t* evt;
	uint32_t evt_deq;
	uint32_t evt_cycle;
	uint64_t erst_phys;
	/* pending command completion */
	bool     cmd_done;
	uint64_t cmd_trb_phys;
	uint8_t  cmd_code;
	uint8_t  cmd_slot;
	/* root port connect-status-change latch, bit(port-1) */
	uint32_t port_change;
	/* slot id -> device backref for event routing */
	xhci_dev_t* slot_dev[XHCI_MAX_SLOTS + 1];
};

/* USB setup packet comes from the shared usb_defs.h (usb_setup_pkt_t) */

/*
 * DMA pool. All "phys" values in this driver are PCIe bus addresses;
 * the virt host decodes them 1:1 with CPU physical addresses.
 */
int   xhci_dma_init(void);
void* xhci_dma_alloc(uint32_t size, uint32_t align, uint64_t* bus);

/* controller */
int  xhci_init(xhci_hc_t* hc, int id, ewokos_addr_t cap_base);
void xhci_process_events(xhci_hc_t* hc);

/* root ports (1-based) */
uint32_t xhci_port_status(xhci_hc_t* hc, int port);
bool xhci_port_connected(xhci_hc_t* hc, int port);
/* reset+enable; returns XHCI_SPEED_* or <0 */
int  xhci_port_reset(xhci_hc_t* hc, int port);
/* fetch-and-clear the connect change latch (CSC bits seen by event/poll) */
uint32_t xhci_port_take_changes(xhci_hc_t* hc);

/* device lifecycle */
int  xhci_device_attach(xhci_hc_t* hc, int root_port, int speed,
		const xhci_dev_t* parent_hub, int hub_port, xhci_dev_t* dev);
void xhci_device_detach(xhci_dev_t* dev);
/* update EP0 MPS after reading the device descriptor */
int  xhci_update_mps0(xhci_dev_t* dev, uint8_t mps0);
/* mark the slot as a hub (needed before addressing children behind it) */
int  xhci_configure_hub(xhci_dev_t* dev, int num_ports);

/* transfers */
int  xhci_control_xfer(xhci_dev_t* dev, const usb_setup_pkt_t* setup,
		void* data, bool dir_in);
/* open an interrupt-IN endpoint (bEndpointAddress with 0x80 set) */
int  xhci_int_in_open(xhci_dev_t* dev, uint8_t ep_addr, uint16_t mps,
		uint8_t bInterval);
/* non-blocking: >0 data bytes copied, 0 pending, <0 error */
int  xhci_int_in_poll(xhci_dev_t* dev, uint8_t ep_addr, void* buf, int size);

/* bulk endpoints (mass storage): open once per direction, then run
   synchronous transfers through a shared DMA bounce buffer */
int  xhci_bulk_open(xhci_dev_t* dev, uint8_t ep_addr, uint16_t mps);
/* returns bytes moved, -2 endpoint stalled, -1 error/timeout */
int  xhci_bulk_xfer(xhci_dev_t* dev, uint8_t ep_addr, void* data,
		int len, bool dir_in, uint32_t timeout_ms);

#endif
