/*
 * Polled xHCI host controller driver, ported from the raspi5 RP1 driver.
 * See xhci.h for the big picture. Everything here follows the xHCI 1.1
 * spec register/TRB layout; the controller is a plain PCI device found
 * through the ECAM window, no firmware involvement.
 *
 * Concurrency model: single threaded, one in-flight TD per endpoint.
 * TDs are only written while their ring is idle (previous TD completed,
 * doorbell not yet rung), so the deferred-cycle-bit trick from Linux is
 * not needed: the controller never fetches a partially written TD.
 *
 * Bus addresses are CPU physical addresses 1:1 (the virt PCIe host has
 * no dma-ranges); all DMA memory comes from the sys_dma pool.
 *
 * On top of control and interrupt-IN transfers this port adds bulk
 * endpoint support (xhci_bulk_open/xhci_bulk_xfer) for the mass-storage
 * path, using a shared DMA bounce buffer allocated at pool init.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/mmio.h>
#include <ewoksys/dma.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <ewoksys/sys.h>
#include <arch/virt/xhci.h>

/* full memory barrier, same as virtio.c: "dmb sy" is not accepted by
   the 32-bit ARM-mode assembler, __sync_synchronize() emits the right
   instruction on both arm and aarch64 */
#define xhci_dmb() __sync_synchronize()

/* capability registers (offsets from cap base) */
#define XHCI_CAPLENGTH   0x00 /* [7:0] length, [31:16] HCIVERSION */
#define XHCI_HCSPARAMS1  0x04
#define XHCI_HCSPARAMS2  0x08
#define XHCI_HCCPARAMS1  0x10
#define XHCI_DBOFF       0x14
#define XHCI_RTSOFF      0x18

/* operational registers (offsets from op base) */
#define XHCI_USBCMD      0x00
#define XHCI_USBSTS      0x04
#define XHCI_PAGESIZE    0x08
#define XHCI_CRCR        0x18
#define XHCI_DCBAAP      0x30
#define XHCI_CONFIG      0x38
#define XHCI_PORTSC(p)   (0x400 + ((p) - 1) * 0x10) /* p is 1-based */

#define USBCMD_RUN       (1u << 0)
#define USBCMD_HCRST     (1u << 1)

#define USBSTS_HCH       (1u << 0)
#define USBSTS_HSE       (1u << 2)  /* RW1C: host system error (bus fault) */
#define USBSTS_CNR       (1u << 11)
#define USBSTS_HCE       (1u << 12) /* RO: internal controller error */

/* CRCR: command ring control */
#define CRCR_CS          (1u << 1)  /* command stop */
#define CRCR_CA          (1u << 2)  /* command abort */
#define CRCR_CRR         (1u << 3)  /* RO: command ring running */

/* PORTSC bits */
#define PORTSC_CCS       (1u << 0)
#define PORTSC_PED       (1u << 1)  /* RW1C: writing 1 disables the port! */
#define PORTSC_PR        (1u << 4)
#define PORTSC_PP        (1u << 9)
#define PORTSC_SPEED(v)  (((v) >> 10) & 0xfu)
#define PORTSC_CSC       (1u << 17)
#define PORTSC_PEC       (1u << 18)
#define PORTSC_WRC       (1u << 19)
#define PORTSC_OCC       (1u << 20)
#define PORTSC_PRC       (1u << 21)
#define PORTSC_PLC       (1u << 22)
#define PORTSC_CEC       (1u << 23)
#define PORTSC_CHANGE_BITS (PORTSC_CSC | PORTSC_PEC | PORTSC_WRC | \
        PORTSC_OCC | PORTSC_PRC | PORTSC_PLC | PORTSC_CEC)
/* bits safe to write back unchanged (RW, not RW1C, not reserved-preserve) */
#define PORTSC_PRESERVE  (PORTSC_PP | (3u << 14) | (7u << 25))

/* runtime registers: interrupter 0 register set */
#define XHCI_IR0_IMAN    0x20
#define XHCI_IR0_ERSTSZ  0x28
#define XHCI_IR0_ERSTBA  0x30
#define XHCI_IR0_ERDP    0x38
#define ERDP_EHB         (1u << 3)

/* TRB types */
#define TRB_NORMAL       1
#define TRB_SETUP        2
#define TRB_DATA         3
#define TRB_STATUS       4
#define TRB_LINK         6
#define TRB_ENABLE_SLOT  9
#define TRB_DISABLE_SLOT 10
#define TRB_ADDR_DEV     11
#define TRB_CONFIG_EP    12
#define TRB_EVAL_CTX     13
#define TRB_RESET_EP     14
#define TRB_STOP_EP      15
#define TRB_SET_TR_DEQ   16
#define TRB_EVT_XFER     32
#define TRB_EVT_CMD      33
#define TRB_EVT_PORT     34

#define TRB_TYPE(t)      ((uint32_t)(t) << 10)
#define TRB_GET_TYPE(d3) (((d3) >> 10) & 0x3fu)

/* generic TRB dword3 flags */
#define TRB_TC           (1u << 1)  /* link: toggle cycle */
#define TRB_ISP          (1u << 2)
#define TRB_CH           (1u << 4)
#define TRB_IOC          (1u << 5)
#define TRB_IDT          (1u << 6)
#define TRB_BSR          (1u << 9)  /* address device: block set address */
#define TRB_DIR_IN       (1u << 16)
#define TRB_TRT_OUT      (2u << 16) /* setup stage transfer type */
#define TRB_TRT_IN       (3u << 16)

/* completion codes */
#define CC_SUCCESS       1
#define CC_BABBLE        3
#define CC_TX_ERR        4
#define CC_STALL         6
#define CC_SHORT_PKT     13
#define CC_CMD_RING_STOP 24
#define CC_CMD_ABORTED   25

/* ring geometry: 64 TRBs, last one reserved for the link TRB */
#define RING_TRBS        64
#define RING_BYTES       (RING_TRBS * 16)
#define EVT_TRBS         256
#define EVT_BYTES        (EVT_TRBS * 16)

/* timeouts */
#define XHCI_CMD_TIMEOUT_MS   1000
#define XHCI_CMD_ABORT_TIMEOUT_MS 100
#define XHCI_CTRL_TIMEOUT_MS  1000
#define XHCI_RESET_TIMEOUT_MS 500

/*
 * DMA pool: one big uncached sys_dma allocation carved into
 *  - a bump region for per-controller globals (never freed)
 *  - fixed 16KB per-device arenas (freed on detach)
 *
 * xHCI 1.2 4.11.5.1 requires that a TRB ring segment never cross a 64KB
 * boundary, and 6.1/6.2 that a device/input context never cross a page.
 * dma_alloc() only guarantees PAGE_SIZE alignment (the kernel rounds the
 * *size* up to a page and splits its free list), so the pool base is
 * realigned to 64KB here. With a 64KB-aligned base every block handed out
 * at its own natural (power-of-two) alignment is automatically inside a
 * single 64KB window, which is why the ring allocations below ask for
 * align == size rather than 64.
 */
#define XHCI_DMA_POOL_SIZE  (1024 * 1024)
#define XHCI_RING_BOUNDARY  0x10000u
#define XHCI_ARENA_SIZE     0x4000
#define XHCI_ARENA_COUNT    16

/* bulk bounce buffer (mass storage sector I/O); lives in the pool bump
   region, one transfer in flight at a time */
#define XHCI_BULK_BUF_SIZE  0x2000u

/* fixed offsets inside a device arena */
#define ARENA_OFF_DEVCTX    0x0000 /* <= 2048 (csz 64 * 32 entries) */
#define ARENA_OFF_INCTX     0x1000 /* <= 2112 (csz 64 * 33 entries) */
#define ARENA_OFF_EP0_RING  0x2000 /* 1024 */
#define ARENA_OFF_CTRL_BUF  0x2400 /* 1024: control data stage bounce */
#define ARENA_BUMP_START    0x2800 /* interrupt EP rings + buffers */
#define XHCI_CTRL_BUF_SIZE  1024

typedef struct {
    bool used;
    uint8_t* virt;
    uint64_t bus;
    uint32_t bump;
} xhci_arena_t;

static struct {
    uint8_t* virt;
    uint64_t bus;
    uint32_t used;
} _pool;

static xhci_arena_t _arenas[XHCI_ARENA_COUNT];

static uint8_t* _bulk_buf = NULL;
static uint64_t _bulk_buf_phys = 0;

static inline uint64_t now_ms(void) {
    return kernel_tic_ms(0);
}

static inline void w64(ewokos_addr_t addr, uint64_t val) {
    put32(addr, (uint32_t)val);
    put32(addr + 4, (uint32_t)(val >> 32));
}

static uint32_t xhci_pick_page_size(ewokos_addr_t op_base) {
    sys_info_t sysinfo;
    uint32_t page_bits = get32(op_base + XHCI_PAGESIZE);
    uint32_t preferred = 4096;

    if(sys_get_sys_info(&sysinfo) == 0 &&
            sysinfo.page_size >= 4096 &&
            (sysinfo.page_size & (sysinfo.page_size - 1u)) == 0) {
        preferred = sysinfo.page_size;
    }

    if((page_bits & (preferred >> 12)) != 0)
        return preferred;

    for(uint32_t shift = 12; shift < 32; shift++) {
        if((page_bits & (1u << (shift - 12))) != 0)
            return 1u << shift;
    }

    return 4096;
}

int xhci_dma_init(void) {
    /* over-allocate one boundary so the pool can start on a 64KB line */
    ewokos_addr_t raw = dma_alloc(0, XHCI_DMA_POOL_SIZE + XHCI_RING_BOUNDARY);
    if (raw == 0) {
        klog("xhci: dma pool alloc failed (%u bytes)\n",
                XHCI_DMA_POOL_SIZE + XHCI_RING_BOUNDARY);
        return -1;
    }
    ewokos_addr_t phy = dma_phy_addr(0, raw);
    if (phy == 0) {
        klog("xhci: dma pool phy addr failed\n");
        return -1;
    }
    /* the DMA window is mapped linearly, and the virt PCIe host decodes
       bus addresses 1:1, so bus == physical here */
    uint32_t pad = (uint32_t)((XHCI_RING_BOUNDARY - (phy & (XHCI_RING_BOUNDARY - 1u)))
            & (XHCI_RING_BOUNDARY - 1u));
    _pool.virt = (uint8_t*)(raw + pad);
    _pool.bus = (uint64_t)(phy + pad);
    _pool.used = 0;
    memset(_pool.virt, 0, XHCI_DMA_POOL_SIZE);
    memset(_arenas, 0, sizeof(_arenas));
    /* shared bounce buffer for bulk transfers (mass storage) */
    _bulk_buf = xhci_dma_alloc(XHCI_BULK_BUF_SIZE, 64, &_bulk_buf_phys);
    return 0;
}

void* xhci_dma_alloc(uint32_t size, uint32_t align, uint64_t* bus) {
    uint32_t off = (_pool.used + align - 1u) & ~(align - 1u);
    /* keep the arena strip at the pool tail intact */
    uint32_t limit = XHCI_DMA_POOL_SIZE - XHCI_ARENA_COUNT * XHCI_ARENA_SIZE;
    if (off + size > limit) {
        klog("xhci: dma pool exhausted (%u + %u > %u)\n", off, size, limit);
        return NULL;
    }
    _pool.used = off + size;
    if (bus != NULL) {
        *bus = _pool.bus + off;
    }
    return _pool.virt + off;
}

static xhci_arena_t* arena_take(void) {
    uint32_t strip = XHCI_DMA_POOL_SIZE - XHCI_ARENA_COUNT * XHCI_ARENA_SIZE;
    for (int i = 0; i < XHCI_ARENA_COUNT; ++i) {
        if (_arenas[i].used) {
            continue;
        }
        uint32_t off = strip + (uint32_t)i * XHCI_ARENA_SIZE;
        _arenas[i].used = true;
        _arenas[i].virt = _pool.virt + off;
        _arenas[i].bus = _pool.bus + off;
        _arenas[i].bump = ARENA_BUMP_START;
        memset(_arenas[i].virt, 0, XHCI_ARENA_SIZE);
        return &_arenas[i];
    }
    return NULL;
}

static void arena_free(xhci_arena_t* a) {
    if (a != NULL) {
        a->used = false;
    }
}

/* bump-allocate inside a device arena (freed all at once on detach) */
static void* arena_alloc(xhci_arena_t* a, uint32_t size, uint32_t align,
        uint64_t* bus) {
    uint32_t off = (a->bump + align - 1u) & ~(align - 1u);
    if (off + size > XHCI_ARENA_SIZE) {
        return NULL;
    }
    a->bump = off + size;
    if (bus != NULL) {
        *bus = a->bus + off;
    }
    return a->virt + off;
}

/*
 * Rings. The producer writes dwords 0-2 first and flips the cycle bit in
 * dword3 last; the link TRB in the final slot is (re)written on wrap with
 * the current cycle so the controller follows it back to the top.
 */
static uint64_t ring_push(uint32_t* ring, uint64_t bus, uint32_t* enq,
        uint32_t* cycle, bool chain_link,
        uint32_t d0, uint32_t d1, uint32_t d2, uint32_t d3) {
    uint32_t i = *enq;
    uint64_t trb_bus = bus + (uint64_t)i * 16u;

    ring[i * 4 + 0] = d0;
    ring[i * 4 + 1] = d1;
    ring[i * 4 + 2] = d2;
    xhci_dmb();
    ring[i * 4 + 3] = d3 | *cycle;

    (*enq)++;
    if (*enq == RING_TRBS - 1) {
        uint32_t* l = &ring[(RING_TRBS - 1) * 4];
        l[0] = (uint32_t)bus;
        l[1] = (uint32_t)(bus >> 32);
        l[2] = 0;
        xhci_dmb();
        l[3] = TRB_TYPE(TRB_LINK) | TRB_TC |
                (chain_link ? TRB_CH : 0) | *cycle;
        *enq = 0;
        *cycle ^= 1u;
    }
    return trb_bus;
}

static void ring_doorbell(xhci_hc_t* hc, uint32_t slot, uint32_t val) {
    xhci_dmb();
    put32(hc->db + slot * 4u, val);
}

/* ---------------- event handling ---------------- */

static void handle_xfer_event(xhci_hc_t* hc, uint32_t d0, uint32_t d1,
        uint32_t d2, uint32_t d3) {
    uint32_t slot = d3 >> 24;
    uint32_t dci = (d3 >> 16) & 0x1fu;
    uint8_t code = (uint8_t)(d2 >> 24);
    uint32_t residue = d2 & 0xffffffu;
    uint64_t trb = (uint64_t)d0 | ((uint64_t)d1 << 32);

    if (slot > XHCI_MAX_SLOTS || dci >= XHCI_MAX_EPS) {
        return;
    }
    xhci_dev_t* dev = hc->slot_dev[slot];
    if (dev == NULL || !dev->eps[dci].open) {
        return;
    }
    xhci_ep_t* ep = &dev->eps[dci];

    if (trb != ep->td_last_phys) {
        /* mid-TD event: a short data stage inside a control TD */
        if (code == CC_SHORT_PKT) {
            ep->short_left = residue;
        }
        else if (code != CC_SUCCESS) {
            /* error mid-TD halts the EP; no more events will come */
            ep->done = true;
            ep->in_flight = false;
            ep->comp_code = code;
            ep->xfer_len = 0;
            ep->short_left = 0;
        }
        return;
    }

    ep->done = true;
    ep->in_flight = false;
    ep->comp_code = code;
    if (code == CC_SHORT_PKT) {
        ep->xfer_len = ep->buf_len - residue;
    }
    else if (code == CC_SUCCESS) {
        ep->xfer_len = ep->buf_len - ep->short_left;
    }
    else {
        ep->xfer_len = 0;
    }
    ep->short_left = 0;
}

/*
 * HSE (a bus fault while the controller was mastering DMA) and HCE (an
 * internal controller error) both stop the controller dead: xHCI 1.2 4.10.2.6
 * and 5.4.2 say the only way out is a full HCRST + re-init. Re-init is not
 * possible here because the DMA pool is a bump allocator with no free path, so
 * latch the failure and stop poking the hardware. Without this check every
 * later command sits out its full timeout and the event ring reads back
 * garbage that would be mistaken for real completions.
 */
static bool hc_check_fatal(xhci_hc_t* hc) {
    uint32_t sts;

    if (hc->failed) {
        return true;
    }
    sts = get32(hc->op + XHCI_USBSTS);
    if ((sts & (USBSTS_HSE | USBSTS_HCE)) == 0) {
        return false;
    }
    if ((sts & USBSTS_HSE) != 0) {
        put32(hc->op + XHCI_USBSTS, USBSTS_HSE); /* RW1C, ack it once */
    }
    klog("xhci%d: fatal controller error usbsts=%08x, disabling\n",
            hc->id, sts);
    hc->failed = true;
    hc->present = false;
    return true;
}

void xhci_process_events(xhci_hc_t* hc) {
    int handled = 0;

    if (hc_check_fatal(hc)) {
        return;
    }

    while (handled < EVT_TRBS) {
        uint32_t* e = &hc->evt[hc->evt_deq * 4];
        uint32_t d3 = e[3];
        if ((d3 & 1u) != hc->evt_cycle) {
            break;
        }
        xhci_dmb();
        uint32_t d0 = e[0];
        uint32_t d1 = e[1];
        uint32_t d2 = e[2];
        uint32_t type = TRB_GET_TYPE(d3);

        if (type == TRB_EVT_CMD) {
            uint64_t trb = (uint64_t)d0 | ((uint64_t)d1 << 32);
            if (trb == hc->cmd_trb_phys) {
                hc->cmd_done = true;
                hc->cmd_code = (uint8_t)(d2 >> 24);
                hc->cmd_slot = (uint8_t)(d3 >> 24);
            }
        }
        else if (type == TRB_EVT_XFER) {
            handle_xfer_event(hc, d0, d1, d2, d3);
        }
        else if (type == TRB_EVT_PORT) {
            uint32_t port = d0 >> 24;
            if (port >= 1 && port <= hc->num_ports) {
                hc->port_change |= 1u << (port - 1);
            }
        }
        /* other event types (MFINDEX wrap etc.) are ignored */

        hc->evt_deq++;
        if (hc->evt_deq == EVT_TRBS) {
            hc->evt_deq = 0;
            hc->evt_cycle ^= 1u;
        }
        handled++;
    }

    if (handled > 0) {
        w64(hc->rt + XHCI_IR0_ERDP,
                (hc->evt_phys + (uint64_t)hc->evt_deq * 16u) | ERDP_EHB);
    }
}

/* ---------------- commands ---------------- */

/*
 * A timed-out command is still queued on the controller's command ring, so
 * simply returning would leave the ring and our enqueue pointer out of step
 * and every later command would be matched against a stale TRB address.
 * xHCI 1.2 4.6.1.2: write CRCR.CA, wait for CRR to drop, then restart the
 * ring from a known-good state.
 */
static void cmd_abort(xhci_hc_t* hc) {
    uint64_t deadline;

    w64(hc->op + XHCI_CRCR, CRCR_CA);
    deadline = now_ms() + XHCI_CMD_ABORT_TIMEOUT_MS;
    while ((get32(hc->op + XHCI_CRCR) & CRCR_CRR) != 0) {
        if (now_ms() > deadline) {
            klog("xhci%d: command abort timeout, controller wedged\n", hc->id);
            hc->failed = true;
            hc->present = false;
            return;
        }
        xhci_process_events(hc);
        usleep(100);
    }
    /* drain the Command Ring Stopped/Aborted events the abort generated */
    xhci_process_events(hc);
    memset(hc->cmd, 0, RING_BYTES);
    hc->cmd_enq = 0;
    hc->cmd_cycle = 1;
    hc->cmd_done = false;
    hc->cmd_trb_phys = 0;
    w64(hc->op + XHCI_CRCR, hc->cmd_phys | 1u); /* RCS = 1 */
}

/* submit one command TRB and poll for its completion event */
static int xhci_cmd(xhci_hc_t* hc, uint32_t d0, uint32_t d1, uint32_t d2,
        uint32_t d3, uint8_t* slot_out) {
    if (hc->failed) {
        return -1;
    }
    hc->cmd_done = false;
    hc->cmd_trb_phys = ring_push(hc->cmd, hc->cmd_phys,
            &hc->cmd_enq, &hc->cmd_cycle, false, d0, d1, d2, d3);
    ring_doorbell(hc, 0, 0);

    uint64_t deadline = now_ms() + XHCI_CMD_TIMEOUT_MS;
    while (!hc->cmd_done) {
        xhci_process_events(hc);
        if (hc->cmd_done) {
            break;
        }
        if (hc->failed) {
            return -1;
        }
        if (now_ms() > deadline) {
            klog("xhci%d: cmd type=%u timeout\n", hc->id, TRB_GET_TYPE(d3));
            cmd_abort(hc);
            return -1;
        }
        usleep(100);
    }
    if (slot_out != NULL) {
        *slot_out = hc->cmd_slot;
    }
    return hc->cmd_code;
}

/* ---------------- controller init ---------------- */

int xhci_init(xhci_hc_t* hc, int id, ewokos_addr_t cap_base) {
    memset(hc, 0, sizeof(*hc));
    hc->id = id;
    hc->base = cap_base;

    uint32_t cap = get32(cap_base);
    uint16_t version = (uint16_t)(cap >> 16);
    if (version < 0x0090 || version == 0xffff) {
        klog("xhci%d: no controller (HCIVERSION=%04x)\n", id, version);
        return -1;
    }
    hc->op = cap_base + (cap & 0xffu);
    hc->db = cap_base + (get32(cap_base + XHCI_DBOFF) & ~0x3u);
    hc->rt = cap_base + (get32(cap_base + XHCI_RTSOFF) & ~0x1fu);

    uint32_t hcs1 = get32(cap_base + XHCI_HCSPARAMS1);
    hc->num_ports = hcs1 >> 24;
    if (hc->num_ports > XHCI_MAX_PORTS) {
        hc->num_ports = XHCI_MAX_PORTS;
    }
    hc->max_slots = hcs1 & 0xffu;
    if (hc->max_slots > XHCI_MAX_SLOTS) {
        hc->max_slots = XHCI_MAX_SLOTS;
    }
    hc->csz = (get32(cap_base + XHCI_HCCPARAMS1) & (1u << 2)) ? 64 : 32;

    /* halt, then reset */
    uint64_t deadline = now_ms() + XHCI_RESET_TIMEOUT_MS;
    put32(hc->op + XHCI_USBCMD, get32(hc->op + XHCI_USBCMD) & ~USBCMD_RUN);
    while ((get32(hc->op + XHCI_USBSTS) & USBSTS_HCH) == 0) {
        if (now_ms() > deadline) {
            klog("xhci%d: halt timeout\n", id);
            return -1;
        }
        usleep(100);
    }
    put32(hc->op + XHCI_USBCMD, USBCMD_HCRST);
    deadline = now_ms() + XHCI_RESET_TIMEOUT_MS;
    while ((get32(hc->op + XHCI_USBCMD) & USBCMD_HCRST) != 0 ||
            (get32(hc->op + XHCI_USBSTS) & USBSTS_CNR) != 0) {
        if (now_ms() > deadline) {
            klog("xhci%d: HCRST timeout\n", id);
            return -1;
        }
        usleep(100);
    }

    /* DCBAA + scratchpad buffers */
    hc->dcbaa = xhci_dma_alloc((hc->max_slots + 1) * 8, 64, &hc->dcbaa_phys);
    if (hc->dcbaa == NULL) {
        return -1;
    }
    uint32_t hcs2 = get32(cap_base + XHCI_HCSPARAMS2);
    uint32_t n_scratch = (((hcs2 >> 21) & 0x1fu) << 5) | ((hcs2 >> 27) & 0x1fu);
    uint32_t scratch_page_size = xhci_pick_page_size(hc->op);
    if (n_scratch > 0) {
        uint64_t arr_bus;
        uint64_t* arr = xhci_dma_alloc(n_scratch * 8, 64, &arr_bus);
        if (arr == NULL) {
            return -1;
        }
        for (uint32_t i = 0; i < n_scratch; ++i) {
            uint64_t page_bus;
            if (xhci_dma_alloc(scratch_page_size, scratch_page_size, &page_bus) == NULL) {
                return -1;
            }
            arr[i] = page_bus;
        }
        hc->dcbaa[0] = arr_bus;
    }

    /* command ring: aligned to its own size so it stays in one 64KB window */
    hc->cmd = xhci_dma_alloc(RING_BYTES, RING_BYTES, &hc->cmd_phys);
    /* event ring + single-entry ERST, same 64KB rule */
    hc->evt = xhci_dma_alloc(EVT_BYTES, EVT_BYTES, &hc->evt_phys);
    uint64_t* erst = xhci_dma_alloc(16, 64, &hc->erst_phys);
    if (hc->cmd == NULL || hc->evt == NULL || erst == NULL) {
        return -1;
    }
    hc->cmd_enq = 0;
    hc->cmd_cycle = 1;
    hc->evt_deq = 0;
    hc->evt_cycle = 1;
    erst[0] = hc->evt_phys;
    erst[1] = EVT_TRBS; /* size in low 16 bits, rest reserved */

    put32(hc->op + XHCI_CONFIG, hc->max_slots);
    w64(hc->op + XHCI_DCBAAP, hc->dcbaa_phys);
    w64(hc->op + XHCI_CRCR, hc->cmd_phys | 1u); /* RCS=1 */

    put32(hc->rt + XHCI_IR0_ERSTSZ, 1);
    w64(hc->rt + XHCI_IR0_ERDP, hc->evt_phys | ERDP_EHB);
    w64(hc->rt + XHCI_IR0_ERSTBA, hc->erst_phys);
    /* IMAN.IE stays 0: fully polled */

    put32(hc->op + XHCI_USBCMD, USBCMD_RUN);
    deadline = now_ms() + XHCI_RESET_TIMEOUT_MS;
    while ((get32(hc->op + XHCI_USBSTS) & USBSTS_HCH) != 0) {
        if (now_ms() > deadline) {
            klog("xhci%d: run timeout\n", id);
            return -1;
        }
        usleep(100);
    }

    /* power up all root ports */
    for (uint32_t p = 1; p <= hc->num_ports; ++p) {
        uint32_t sc = get32(hc->op + XHCI_PORTSC(p));
        if ((sc & PORTSC_PP) == 0) {
            put32(hc->op + XHCI_PORTSC(p), (sc & PORTSC_PRESERVE) | PORTSC_PP);
        }
    }

    hc->present = true;
    klog("xhci%d: v%x.%02x ports=%u slots=%u csz=%u scratch=%u page=%u\n",
            id, version >> 8, version & 0xff,
            hc->num_ports, hc->max_slots, hc->csz, n_scratch, scratch_page_size);
    return 0;
}

/* ---------------- root ports ---------------- */

uint32_t xhci_port_status(xhci_hc_t* hc, int port) {
    return get32(hc->op + XHCI_PORTSC(port));
}

bool xhci_port_connected(xhci_hc_t* hc, int port) {
    return (get32(hc->op + XHCI_PORTSC(port)) & PORTSC_CCS) != 0;
}

uint32_t xhci_port_take_changes(xhci_hc_t* hc) {
    uint32_t changes = hc->port_change;
    hc->port_change = 0;

    for (uint32_t p = 1; p <= hc->num_ports; ++p) {
        uint32_t sc = get32(hc->op + XHCI_PORTSC(p));
        if (sc & PORTSC_CSC) {
            changes |= 1u << (p - 1);
            put32(hc->op + XHCI_PORTSC(p),
                    (sc & PORTSC_PRESERVE) | PORTSC_CSC);
        }
    }
    return changes;
}

int xhci_port_reset(xhci_hc_t* hc, int port) {
    uint32_t sc = get32(hc->op + XHCI_PORTSC(port));
    if ((sc & PORTSC_CCS) == 0) {
        return -1;
    }
    /* SS ports train by themselves; a hot reset would just bounce them */
    if ((sc & PORTSC_PED) != 0 && PORTSC_SPEED(sc) >= XHCI_SPEED_SUPER) {
        return (int)PORTSC_SPEED(sc);
    }

    put32(hc->op + XHCI_PORTSC(port), (sc & PORTSC_PRESERVE) | PORTSC_PR);
    uint64_t deadline = now_ms() + XHCI_RESET_TIMEOUT_MS;
    while (1) {
        xhci_process_events(hc);
        sc = get32(hc->op + XHCI_PORTSC(port));
        if (sc & PORTSC_PRC) {
            put32(hc->op + XHCI_PORTSC(port),
                    (sc & PORTSC_PRESERVE) | PORTSC_PRC | PORTSC_PEC);
            break;
        }
        if (now_ms() > deadline) {
            return -1;
        }
        usleep(1000);
    }
    sc = get32(hc->op + XHCI_PORTSC(port));
    if ((sc & PORTSC_PED) == 0 || (sc & PORTSC_CCS) == 0) {
        return -1;
    }
    usleep(50000);
    return (int)PORTSC_SPEED(sc);
}

/* ---------------- contexts ---------------- */

/* output (device) context entry: 0=slot, dci>=1 endpoints */
static inline uint32_t* octx(xhci_dev_t* dev, uint32_t idx) {
    return (uint32_t*)(dev->dev_ctx + dev->hc->csz * idx);
}

/* input context entry: 0=input control, 1=slot, 1+dci endpoints */
static inline uint32_t* ictx(xhci_dev_t* dev, uint32_t idx) {
    return (uint32_t*)(dev->in_ctx + dev->hc->csz * idx);
}

static void ictx_clear(xhci_dev_t* dev) {
    memset(dev->in_ctx, 0, dev->hc->csz * 33);
}

/* copy the live output slot context into the input context */
static void ictx_copy_slot(xhci_dev_t* dev) {
    memcpy(ictx(dev, 1), octx(dev, 0), 32);
    ictx(dev, 1)[7] = 0; /* clear xHC-owned state fields */
}

static uint32_t slot_ctx_entries(xhci_dev_t* dev) {
    return (octx(dev, 0)[0] >> 27) & 0x1fu;
}

/* ---------------- device lifecycle ---------------- */

static uint16_t default_mps0(int speed) {
    switch (speed) {
    case XHCI_SPEED_LOW:
    case XHCI_SPEED_FULL:
        return 8;
    case XHCI_SPEED_HIGH:
        return 64;
    default:
        return 512;
    }
}

int xhci_device_attach(xhci_hc_t* hc, int root_port, int speed,
        const xhci_dev_t* parent_hub, int hub_port, xhci_dev_t* dev) {
    memset(dev, 0, sizeof(*dev));

    uint32_t route = 0;
    uint8_t depth = 0;
    uint8_t tt_slot = 0;
    uint8_t tt_port = 0;
    if (parent_hub != NULL) {
        depth = parent_hub->depth + 1;
        if (depth > 5) {
            return -1;
        }
        uint32_t nib = hub_port > 15 ? 15u : (uint32_t)hub_port;
        route = parent_hub->route | (nib << (4u * parent_hub->depth));
        root_port = parent_hub->root_port;
        if (speed == XHCI_SPEED_LOW || speed == XHCI_SPEED_FULL) {
            if (parent_hub->speed == XHCI_SPEED_HIGH) {
                tt_slot = parent_hub->slot_id;
                tt_port = (uint8_t)hub_port;
            }
            else {
                tt_slot = parent_hub->tt_slot;
                tt_port = parent_hub->tt_port;
            }
        }
    }

    xhci_arena_t* arena = arena_take();
    if (arena == NULL) {
        klog("xhci%d: out of device arenas\n", hc->id);
        return -1;
    }

    uint8_t slot_id = 0;
    int code = xhci_cmd(hc, 0, 0, 0, TRB_TYPE(TRB_ENABLE_SLOT), &slot_id);
    if (code != CC_SUCCESS || slot_id == 0 || slot_id > hc->max_slots) {
        klog("xhci%d: enable slot failed code=%d slot=%u\n",
                hc->id, code, slot_id);
        arena_free(arena);
        return -1;
    }

    dev->used = true;
    dev->hc = hc;
    dev->slot_id = slot_id;
    dev->speed = (uint8_t)speed;
    dev->root_port = (uint8_t)root_port;
    dev->route = route;
    dev->depth = depth;
    dev->tt_slot = tt_slot;
    dev->tt_port = tt_port;
    dev->mps0 = 0;
    dev->arena = arena;
    dev->dev_ctx = arena->virt + ARENA_OFF_DEVCTX;
    dev->dev_ctx_phys = arena->bus + ARENA_OFF_DEVCTX;
    dev->in_ctx = arena->virt + ARENA_OFF_INCTX;
    dev->in_ctx_phys = arena->bus + ARENA_OFF_INCTX;

    /* EP0 transfer ring at its fixed arena slot */
    xhci_ep_t* ep0 = &dev->eps[1];
    ep0->open = true;
    ep0->ring = (uint32_t*)(arena->virt + ARENA_OFF_EP0_RING);
    ep0->ring_phys = arena->bus + ARENA_OFF_EP0_RING;
    ep0->enq = 0;
    ep0->cycle = 1;
    ep0->mps = default_mps0(speed);

    /* input context: address device with slot + EP0 */
    ictx_clear(dev);
    ictx(dev, 0)[1] = 0x3; /* add slot + EP0 */
    uint32_t* slot = ictx(dev, 1);
    slot[0] = route | ((uint32_t)speed << 20) | (1u << 27);
    slot[1] = (uint32_t)root_port << 16;
    if (tt_slot != 0) {
        slot[2] = tt_slot | ((uint32_t)tt_port << 8);
    }
    uint32_t* ep = ictx(dev, 2);
    ep[1] = (3u << 1) | (4u << 3) | ((uint32_t)ep0->mps << 16);
    ep[2] = (uint32_t)(ep0->ring_phys | 1u);
    ep[3] = (uint32_t)(ep0->ring_phys >> 32);
    ep[4] = 8;

    hc->dcbaa[slot_id] = dev->dev_ctx_phys;
    hc->slot_dev[slot_id] = dev;

    code = xhci_cmd(hc, (uint32_t)dev->in_ctx_phys,
            (uint32_t)(dev->in_ctx_phys >> 32), 0,
            TRB_TYPE(TRB_ADDR_DEV) | ((uint32_t)slot_id << 24), NULL);
    if (code != CC_SUCCESS) {
        klog("xhci%d: address device slot=%u failed code=%d\n",
                hc->id, slot_id, code);
        xhci_device_detach(dev);
        return -1;
    }
    dev->mps0 = ep0->mps;
    return 0;
}

void xhci_device_detach(xhci_dev_t* dev) {
    if (!dev->used) {
        return;
    }
    xhci_hc_t* hc = dev->hc;
    xhci_cmd(hc, 0, 0, 0,
            TRB_TYPE(TRB_DISABLE_SLOT) | ((uint32_t)dev->slot_id << 24), NULL);
    hc->dcbaa[dev->slot_id] = 0;
    hc->slot_dev[dev->slot_id] = NULL;
    arena_free((xhci_arena_t*)dev->arena);
    memset(dev, 0, sizeof(*dev));
}

/*
 * USB 2.0 9.6.1 / USB 3.x 9.6.1: for full/high speed bMaxPacketSize0 is the
 * size in bytes (8/16/32/64), but for SuperSpeed it is an exponent and the
 * only legal value is 9, meaning 512. Taking it literally would program a
 * 9-byte EP0 and every control transfer on an SS device would fail.
 */
static uint16_t decode_mps0(int speed, uint8_t mps0) {
    if (speed >= XHCI_SPEED_SUPER) {
        return mps0 == 0 ? 512 : (uint16_t)(1u << (mps0 > 9 ? 9 : mps0));
    }
    return mps0 == 0 ? 8 : (uint16_t)mps0;
}

int xhci_update_mps0(xhci_dev_t* dev, uint8_t mps0) {
    xhci_ep_t* ep0 = &dev->eps[1];
    uint16_t mps = decode_mps0(dev->speed, mps0);
    if (ep0->mps == mps) {
        return 0;
    }
    ep0->mps = mps;

    ictx_clear(dev);
    ictx(dev, 0)[1] = 0x2; /* evaluate EP0 only */
    uint32_t* ep = ictx(dev, 2);
    ep[1] = (3u << 1) | (4u << 3) | ((uint32_t)mps << 16);
    ep[2] = (uint32_t)((ep0->ring_phys + (uint64_t)ep0->enq * 16u) | ep0->cycle);
    ep[3] = (uint32_t)(ep0->ring_phys >> 32);
    ep[4] = 8;

    int code = xhci_cmd(dev->hc, (uint32_t)dev->in_ctx_phys,
            (uint32_t)(dev->in_ctx_phys >> 32), 0,
            TRB_TYPE(TRB_EVAL_CTX) | ((uint32_t)dev->slot_id << 24), NULL);
    if (code != CC_SUCCESS) {
        return -1;
    }
    dev->mps0 = mps;
    return 0;
}

int xhci_configure_hub(xhci_dev_t* dev, int num_ports) {
    ictx_clear(dev);
    ictx(dev, 0)[1] = 0x1; /* update slot context only */
    ictx_copy_slot(dev);
    uint32_t* slot = ictx(dev, 1);
    slot[0] |= 1u << 26; /* hub */
    slot[1] = (slot[1] & 0x00ffffffu) | ((uint32_t)num_ports << 24);
    /* TTT stays 0 (8 FS bit times) and MTT 0 for a single-TT HS hub */

    int code = xhci_cmd(dev->hc, (uint32_t)dev->in_ctx_phys,
            (uint32_t)(dev->in_ctx_phys >> 32), 0,
            TRB_TYPE(TRB_CONFIG_EP) | ((uint32_t)dev->slot_id << 24), NULL);
    return code == CC_SUCCESS ? 0 : -1;
}

/* ---------------- transfers ---------------- */

/* wait for a specific endpoint's TD to complete, driving the event loop */
static int ep_wait(xhci_dev_t* dev, xhci_ep_t* ep, uint32_t timeout_ms) {
    uint64_t deadline = now_ms() + timeout_ms;
    while (!ep->done) {
        xhci_process_events(dev->hc);
        if (ep->done) {
            break;
        }
        if (now_ms() > deadline) {
            return -1;
        }
        usleep(100);
    }
    return 0;
}

/*
 * After a STALL or transaction error the endpoint is halted: recover with
 * Reset Endpoint + Set TR Dequeue so the ring can be reused. For non-EP0 the
 * device still has its own halt latched; xhci_int_in_poll() reports STALL as
 * -2 so usbhostd can follow up with CLEAR_FEATURE(ENDPOINT_HALT).
 */
static void ep_recover(xhci_dev_t* dev, uint32_t dci) {
    xhci_ep_t* ep = &dev->eps[dci];
    xhci_hc_t* hc = dev->hc;

    ep->in_flight = false;
    ep->done = false;
    ep->short_left = 0;
    /*
     * The usual reason a transfer fails is that the device was unplugged.
     * Both commands below would then sit out their full timeout, so the
     * poll loop would freeze for seconds on every yank. Check the root port
     * first and let the periodic scan tear the slot down instead.
     */
    if (hc->failed || !xhci_port_connected(hc, dev->root_port)) {
        return;
    }
    xhci_cmd(hc, 0, 0, 0, TRB_TYPE(TRB_RESET_EP) |
            (dci << 16) | ((uint32_t)dev->slot_id << 24), NULL);
    uint64_t deq = (ep->ring_phys + (uint64_t)ep->enq * 16u) | ep->cycle;
    xhci_cmd(hc, (uint32_t)deq, (uint32_t)(deq >> 32), 0,
            TRB_TYPE(TRB_SET_TR_DEQ) |
            (dci << 16) | ((uint32_t)dev->slot_id << 24), NULL);
}

int xhci_control_xfer(xhci_dev_t* dev, const usb_setup_pkt_t* setup,
        void* data, bool dir_in) {
    xhci_ep_t* ep0 = &dev->eps[1];
    xhci_arena_t* arena = (xhci_arena_t*)dev->arena;
    uint8_t* bounce = arena->virt + ARENA_OFF_CTRL_BUF;
    uint64_t bounce_bus = arena->bus + ARENA_OFF_CTRL_BUF;
    uint16_t len = setup->wLength;

    if (len > XHCI_CTRL_BUF_SIZE) {
        return -1;
    }
    if (ep0->in_flight) {
        return -1; /* single outstanding control transfer */
    }
    if (len > 0 && !dir_in) {
        memcpy(bounce, data, len);
    }

    uint32_t setup_lo, setup_hi;
    memcpy(&setup_lo, setup, 4);
    memcpy(&setup_hi, (const uint8_t*)setup + 4, 4);

    ep0->done = false;
    ep0->short_left = 0;
    ep0->comp_code = 0;
    ep0->buf_len = len;

    /* setup stage (IDT: the packet rides in the TRB itself) */
    ring_push(ep0->ring, ep0->ring_phys, &ep0->enq, &ep0->cycle, false,
            setup_lo, setup_hi, 8,
            TRB_TYPE(TRB_SETUP) | TRB_IDT |
            (len > 0 ? (dir_in ? TRB_TRT_IN : TRB_TRT_OUT) : 0));
    /* data stage */
    if (len > 0) {
        ring_push(ep0->ring, ep0->ring_phys, &ep0->enq, &ep0->cycle, false,
                (uint32_t)bounce_bus, (uint32_t)(bounce_bus >> 32), len,
                TRB_TYPE(TRB_DATA) | TRB_ISP | (dir_in ? TRB_DIR_IN : 0));
    }
    /* status stage: opposite direction, IOC */
    ep0->td_last_phys = ring_push(ep0->ring, ep0->ring_phys,
            &ep0->enq, &ep0->cycle, false,
            0, 0, 0,
            TRB_TYPE(TRB_STATUS) | TRB_IOC |
            ((len == 0 || !dir_in) ? TRB_DIR_IN : 0));

    ep0->in_flight = true;
    ring_doorbell(dev->hc, dev->slot_id, 1);

    if (ep_wait(dev, ep0, XHCI_CTRL_TIMEOUT_MS) != 0) {
        klog("xhci%d: ctrl timeout slot=%u req=%02x/%02x\n", dev->hc->id,
                dev->slot_id, setup->bmRequestType, setup->bRequest);
        ep_recover(dev, 1);
        return -1;
    }
    if (ep0->comp_code != CC_SUCCESS && ep0->comp_code != CC_SHORT_PKT) {
        /* EP0 halts on STALL; Reset Endpoint alone un-stalls it */
        ep_recover(dev, 1);
        return -1;
    }
    uint32_t got = len; /* status-stage event: data stage went full length */
    if (dir_in && len > 0) {
        /* short data stage latched its residue via short_left/xfer_len */
        got = ep0->xfer_len > len ? len : ep0->xfer_len;
        memcpy(data, bounce, got);
    }
    return (int)got;
}

/* interrupt interval -> xHCI EP context interval exponent (125us * 2^n) */
static uint32_t int_interval(int speed, uint8_t bInterval) {
    uint32_t exp;
    if (speed == XHCI_SPEED_HIGH || speed >= XHCI_SPEED_SUPER) {
        /* bInterval is 1-16, period = 2^(bInterval-1) microframes */
        uint32_t bi = bInterval == 0 ? 1u : (bInterval > 16u ? 16u : bInterval);
        exp = bi - 1u;
    }
    else {
        /*
         * LS/FS: bInterval is in ms and xHCI 1.2 table 6-12 wants it
         * rounded *down* to the nearest base-2 multiple, i.e. the largest
         * 2^exp * 125us that still fits in bInterval ms. Rounding up
         * would poll slower than the device asked for (a 10ms mouse
         * would land on 16ms) and, worse, can exceed the 255ms cap the
         * spec puts on the FS/LS interval field.
         */
        uint32_t ms = bInterval == 0 ? 1 : bInterval;
        exp = 3; /* 1ms */
        while (exp < 10 && (1u << (exp + 1u)) <= ms * 8u) {
            exp++;
        }
    }
    return exp > 15 ? 15 : exp;
}

int xhci_int_in_open(xhci_dev_t* dev, uint8_t ep_addr, uint16_t mps,
        uint8_t bInterval) {
    uint32_t ep_num = ep_addr & 0x0fu;
    uint32_t dci = ep_num * 2u + 1u; /* IN endpoint */
    if (ep_num == 0 || dci >= XHCI_MAX_EPS || dev->eps[dci].open) {
        return -1;
    }
    xhci_arena_t* arena = (xhci_arena_t*)dev->arena;
    xhci_ep_t* ep = &dev->eps[dci];

    memset(ep, 0, sizeof(*ep));
    ep->ring = arena_alloc(arena, RING_BYTES, RING_BYTES, &ep->ring_phys);
    uint32_t buf_size = (mps + 63u) & ~63u;
    ep->data = arena_alloc(arena, buf_size, 64, &ep->data_phys);
    if (ep->ring == NULL || ep->data == NULL) {
        klog("xhci%d: arena full for slot=%u ep=%02x\n",
                dev->hc->id, dev->slot_id, ep_addr);
        return -1;
    }
    memset(ep->ring, 0, RING_BYTES);
    ep->enq = 0;
    ep->cycle = 1;
    ep->mps = mps;

    /* configure endpoint: keep already-configured EPs, add this one */
    ictx_clear(dev);
    ictx(dev, 0)[1] = 0x1u | (1u << dci);
    ictx_copy_slot(dev);
    uint32_t* slot = ictx(dev, 1);
    uint32_t entries = slot_ctx_entries(dev);
    if (dci > entries) {
        entries = dci;
    }
    slot[0] = (slot[0] & ~(0x1fu << 27)) | (entries << 27);

    uint32_t* epc = ictx(dev, 1 + dci);
    /* EP type 7 = interrupt IN; CErr=3 */
    epc[0] = int_interval(dev->speed, bInterval) << 16;
    epc[1] = (3u << 1) | (7u << 3) | ((uint32_t)mps << 16);
    epc[2] = (uint32_t)(ep->ring_phys | 1u);
    epc[3] = (uint32_t)(ep->ring_phys >> 32);
    /*
     * Low 16 bits: Average TRB Length. High 16 bits: Max ESIT Payload Lo
     * (xHCI 1.2 6.2.3.6) = mps * (MaxBurst+1) * (Mult+1); both are 0 here,
     * so it is just mps. Leaving it at zero makes the controller reserve no
     * periodic bandwidth for the endpoint, and Configure Endpoint is
     * rejected with a bandwidth error on the stricter DWC3 cores.
     */
    epc[4] = (uint32_t)mps | ((uint32_t)mps << 16);

    int code = xhci_cmd(dev->hc, (uint32_t)dev->in_ctx_phys,
            (uint32_t)(dev->in_ctx_phys >> 32), 0,
            TRB_TYPE(TRB_CONFIG_EP) | ((uint32_t)dev->slot_id << 24), NULL);
    if (code != CC_SUCCESS) {
        klog("xhci%d: config ep slot=%u dci=%u failed code=%d\n",
                dev->hc->id, dev->slot_id, dci, code);
        return -1;
    }
    ep->open = true;
    return 0;
}

int xhci_int_in_poll(xhci_dev_t* dev, uint8_t ep_addr, void* buf, int size) {
    uint32_t ep_num = ep_addr & 0x0fu;
    uint32_t dci = ep_num * 2u + 1u;
    if (dci >= XHCI_MAX_EPS || !dev->eps[dci].open) {
        return -1;
    }
    xhci_ep_t* ep = &dev->eps[dci];

    xhci_process_events(dev->hc);

    if (ep->done) {
        ep->done = false;
        if (ep->comp_code == CC_SUCCESS || ep->comp_code == CC_SHORT_PKT) {
            int n = (int)ep->xfer_len;
            if (n > size) {
                n = size;
            }
            if (n > 0) {
                memcpy(buf, ep->data, n);
            }
            return n;
        }
        /* STALL/error: recover the ring; caller decides on re-poll/drop */
        ep_recover(dev, dci);
        return ep->comp_code == CC_STALL ? -2 : -1;
    }

    if (!ep->in_flight) {
        /* arm the next TD; the controller polls the device in hardware */
        ep->buf_len = ep->mps;
        ep->short_left = 0;
        ep->td_last_phys = ring_push(ep->ring, ep->ring_phys,
                &ep->enq, &ep->cycle, false,
                (uint32_t)ep->data_phys, (uint32_t)(ep->data_phys >> 32),
                ep->mps, TRB_TYPE(TRB_NORMAL) | TRB_ISP | TRB_IOC);
        ep->in_flight = true;
        ring_doorbell(dev->hc, dev->slot_id, dci);
    }
    return 0;
}

/* ---------------- bulk endpoints (mass storage) ---------------- */

int xhci_bulk_open(xhci_dev_t* dev, uint8_t ep_addr, uint16_t mps) {
    uint32_t ep_num = ep_addr & 0x0fu;
    bool dir_in = (ep_addr & 0x80u) != 0;
    uint32_t dci = ep_num * 2u + (dir_in ? 1u : 0u);
    if (ep_num == 0 || dci >= XHCI_MAX_EPS || dev->eps[dci].open) {
        return -1;
    }
    xhci_arena_t* arena = (xhci_arena_t*)dev->arena;
    xhci_ep_t* ep = &dev->eps[dci];

    memset(ep, 0, sizeof(*ep));
    ep->ring = arena_alloc(arena, RING_BYTES, RING_BYTES, &ep->ring_phys);
    if (ep->ring == NULL) {
        klog("xhci%d: arena full for slot=%u ep=%02x\n",
                dev->hc->id, dev->slot_id, ep_addr);
        return -1;
    }
    memset(ep->ring, 0, RING_BYTES);
    ep->enq = 0;
    ep->cycle = 1;
    ep->mps = mps;

    /* configure endpoint: keep already-configured EPs, add this one */
    ictx_clear(dev);
    ictx(dev, 0)[1] = 0x1u | (1u << dci);
    ictx_copy_slot(dev);
    uint32_t* slot = ictx(dev, 1);
    uint32_t entries = slot_ctx_entries(dev);
    if (dci > entries) {
        entries = dci;
    }
    slot[0] = (slot[0] & ~(0x1fu << 27)) | (entries << 27);

    uint32_t* epc = ictx(dev, 1 + dci);
    /* EP type 2 = bulk OUT, 6 = bulk IN; CErr=3 */
    uint32_t type = dir_in ? 6u : 2u;
    epc[0] = 0;
    epc[1] = (3u << 1) | (type << 3) | ((uint32_t)mps << 16);
    epc[2] = (uint32_t)(ep->ring_phys | 1u);
    epc[3] = (uint32_t)(ep->ring_phys >> 32);
    epc[4] = 1024u; /* Average TRB Length; ESIT payload unused for bulk */

    int code = xhci_cmd(dev->hc, (uint32_t)dev->in_ctx_phys,
            (uint32_t)(dev->in_ctx_phys >> 32), 0,
            TRB_TYPE(TRB_CONFIG_EP) | ((uint32_t)dev->slot_id << 24), NULL);
    if (code != CC_SUCCESS) {
        klog("xhci%d: config bulk slot=%u dci=%u failed code=%d\n",
                dev->hc->id, dev->slot_id, dci, code);
        return -1;
    }
    ep->open = true;
    return 0;
}

int xhci_bulk_xfer(xhci_dev_t* dev, uint8_t ep_addr, void* data,
        int len, bool dir_in, uint32_t timeout_ms) {
    uint32_t ep_num = ep_addr & 0x0fu;
    uint32_t dci = ep_num * 2u + ((ep_addr & 0x80u) ? 1u : 0u);
    if (dci >= XHCI_MAX_EPS || !dev->eps[dci].open) {
        return -1;
    }
    xhci_ep_t* ep = &dev->eps[dci];
    if (ep->in_flight || len < 0 || len > (int)XHCI_BULK_BUF_SIZE ||
            _bulk_buf == NULL) {
        return -1;
    }
    if (len > 0 && !dir_in) {
        memcpy(_bulk_buf, data, len);
    }

    ep->done = false;
    ep->short_left = 0;
    ep->comp_code = 0;
    ep->buf_len = (uint32_t)len;
    ep->td_last_phys = ring_push(ep->ring, ep->ring_phys,
            &ep->enq, &ep->cycle, false,
            (uint32_t)_bulk_buf_phys, (uint32_t)(_bulk_buf_phys >> 32),
            (uint32_t)len, TRB_TYPE(TRB_NORMAL) | TRB_ISP | TRB_IOC);
    ep->in_flight = true;
    ring_doorbell(dev->hc, dev->slot_id, dci);

    if (ep_wait(dev, ep, timeout_ms) != 0) {
        klog("xhci%d: bulk timeout slot=%u ep=%02x\n",
                dev->hc->id, dev->slot_id, ep_addr);
        ep_recover(dev, dci);
        return -1;
    }
    if (ep->comp_code == CC_STALL) {
        ep_recover(dev, dci);
        return -2;
    }
    if (ep->comp_code != CC_SUCCESS && ep->comp_code != CC_SHORT_PKT) {
        ep_recover(dev, dci);
        return -1;
    }
    int n = (int)ep->xfer_len;
    if (n > len) {
        n = len;
    }
    if (n > 0 && dir_in) {
        memcpy(data, _bulk_buf, n);
    }
    return n;
}
