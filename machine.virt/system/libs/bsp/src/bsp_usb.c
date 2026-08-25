/*
 * bsp_usb.c: machine.virt USB host abstraction on top of the polled
 * xHCI driver.
 *
 * Owns the single PCI xHCI controller found through the ECAM window,
 * exposes its root ports as a 1-based port space and hands out opaque
 * device handles backed by xhci_dev_t slots. The usbhostd policy layer
 * never touches xhci_* directly.
 *
 * Also owns the mass-storage policy: the bulk-only transport rides on
 * xhci's bulk endpoints and is served through the bsp_usb_msc_* hooks
 * of the shared usbhostd, including the fat32fsd auto-mount.
 */
#include <usb/bsp_usb.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sysinfo.h>
#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>
#include <ewoksys/usbmsc.h>
#include <arch/virt/pci.h>
#include <arch/virt/xhci.h>

#define BSP_USB_MAX_DEVS 16

/* generic bulk timeout for the contract entry points */
#define BSP_USB_BULK_TIMEOUT_MS 2000u

typedef struct {
    bool ready;
    bool claimed;
    bsp_usb_dev_t* dev;
    uint8_t iface_num;
    uint8_t ep_in;   /* endpoint address (with USB_ENDPOINT_IN) */
    uint8_t ep_out;
    uint16_t mps_in;
    uint16_t mps_out;
    uint32_t tag;
    uint32_t sector_count;
    uint32_t sector_size;
    int child_pid;
} usb_msc_t;

struct bsp_usb_dev {
    bool used;
    xhci_dev_t xdev;
};

static xhci_hc_t _hc;
static bsp_usb_dev_t _devs[BSP_USB_MAX_DEVS];
static uint32_t _bar0 = 0;      /* xHCI BAR0 physical address */
static bool _inited = false;
static usb_msc_t _msc;

static inline uint32_t be32(const void* p) {
    const uint8_t* b = (const uint8_t*)p;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
            ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

static inline void put_be32(void* p, uint32_t v) {
    uint8_t* b = (uint8_t*)p;
    b[0] = (uint8_t)(v >> 24);
    b[1] = (uint8_t)(v >> 16);
    b[2] = (uint8_t)(v >> 8);
    b[3] = (uint8_t)v;
}

static int speed_to_xhci(int speed) {
    switch (speed) {
    case BSP_USB_SPEED_LOW:
        return XHCI_SPEED_LOW;
    case BSP_USB_SPEED_FULL:
        return XHCI_SPEED_FULL;
    case BSP_USB_SPEED_HIGH:
        return XHCI_SPEED_HIGH;
    default:
        return XHCI_SPEED_FULL;
    }
}

static int speed_from_xhci(int speed) {
    switch (speed) {
    case XHCI_SPEED_LOW:
        return BSP_USB_SPEED_LOW;
    case XHCI_SPEED_HIGH:
    case XHCI_SPEED_SUPER:
        return BSP_USB_SPEED_HIGH;
    default:
        return BSP_USB_SPEED_FULL;
    }
}

/* locate the xHCI controller and bring it up; 0 = present */
static int hc_probe(void) {
    if (virt_pci_init() != 0) {
        klog("bsp_usb: pci map failed, running without usb\n");
        return -1;
    }
    if (xhci_dma_init() != 0) {
        klog("bsp_usb: dma_init_failed, running without usb\n");
        return -1;
    }
    _bar0 = virt_pci_find_xhci();
    if (_bar0 == 0) {
        klog("bsp_usb: no xhci on pci bus, running without usb\n");
        return -1;
    }
    if (xhci_init(&_hc, 0,
            virt_pci_win_base() + (_bar0 - VIRT_PCI_WIN_PHY)) != 0) {
        klog("bsp_usb: xhci init failed, running without usb\n");
        _bar0 = 0;
        return -1;
    }
    return 0;
}

/* ---- init / poll ---- */

int bsp_usb_init(void) {
    if (_inited) {
        return 0;
    }
    _inited = true;
    memset(_devs, 0, sizeof(_devs));
    memset(&_msc, 0, sizeof(_msc));
    /*
     * Never fail hard: ipcserv blocks in ipc_wait_ready() until the daemon
     * registers its mount point, so degrading to "no usb" is the only safe
     * path. Every entry point below is gated on _hc.present.
     */
    (void)hc_probe();
    return 0;
}

int bsp_usb_reinit(void) {
    if (!_inited || _bar0 == 0) {
        return -1;
    }
    /* HCRST drops every slot context: all attached devices become stale,
       so forget the local handles and MSC state too. The policy layer
       re-enumerates the tree from scratch afterwards. */
    memset(_devs, 0, sizeof(_devs));
    memset(&_msc, 0, sizeof(_msc));
    if (xhci_init(&_hc, 0,
            virt_pci_win_base() + (_bar0 - VIRT_PCI_WIN_PHY)) != 0) {
        klog("bsp_usb: reinit xhci failed\n");
        return -1;
    }
    return 0;
}

void bsp_usb_poll(void) {
    if (_hc.present) {
        xhci_process_events(&_hc);
    }
}

/* ---- root ports ---- */

int bsp_usb_root_port_count(void) {
    return _hc.present ? (int)_hc.num_ports : 0;
}

bool bsp_usb_root_port_connected(int port) {
    if (!_hc.present) {
        return false;
    }
    return xhci_port_connected(&_hc, port);
}

int bsp_usb_root_port_reset(int port) {
    int speed;
    if (!_hc.present) {
        return -1;
    }
    speed = xhci_port_reset(&_hc, port);
    if (speed < 0) {
        return -1;
    }
    return speed_from_xhci(speed);
}

uint32_t bsp_usb_root_port_changes(void) {
    if (!_hc.present) {
        return 0;
    }
    return xhci_port_take_changes(&_hc);
}

/* ---- device lifecycle ---- */

bsp_usb_dev_t* bsp_usb_device_attach(int root_port, int speed,
        bsp_usb_dev_t* parent_hub, int hub_port) {
    bsp_usb_dev_t* dev = NULL;
    xhci_hc_t* hc;
    int hc_port;

    for (int i = 0; i < BSP_USB_MAX_DEVS; ++i) {
        if (!_devs[i].used) {
            dev = &_devs[i];
            break;
        }
    }
    if (dev == NULL) {
        return NULL;
    }

    if (parent_hub != NULL) {
        hc = parent_hub->xdev.hc;
        hc_port = parent_hub->xdev.root_port;
    }
    else {
        if (!_hc.present || root_port < 1 ||
                root_port > (int)_hc.num_ports) {
            return NULL;
        }
        hc = &_hc;
        hc_port = root_port;
    }

    memset(dev, 0, sizeof(*dev));
    if (xhci_device_attach(hc, hc_port, speed_to_xhci(speed),
            parent_hub != NULL ? &parent_hub->xdev : NULL, hub_port,
            &dev->xdev) != 0) {
        return NULL;
    }
    dev->used = true;
    return dev;
}

void bsp_usb_device_detach(bsp_usb_dev_t* dev) {
    if (dev == NULL || !dev->used) {
        return;
    }
    xhci_device_detach(&dev->xdev);
    memset(dev, 0, sizeof(*dev));
}

int bsp_usb_device_update_mps0(bsp_usb_dev_t* dev, uint8_t mps0) {
    if (dev == NULL) {
        return -1;
    }
    return xhci_update_mps0(&dev->xdev, mps0);
}

int bsp_usb_device_configure_hub(bsp_usb_dev_t* dev, int num_ports) {
    if (dev == NULL) {
        return -1;
    }
    return xhci_configure_hub(&dev->xdev, num_ports);
}

/* ---- transfers ---- */

int bsp_usb_control_xfer(bsp_usb_dev_t* dev, const usb_setup_pkt_t* setup,
        void* data, bool dir_in) {
    if (dev == NULL) {
        return -1;
    }
    return xhci_control_xfer(&dev->xdev, setup, data, dir_in);
}

int bsp_usb_int_in_open(bsp_usb_dev_t* dev, uint8_t ep_addr, uint16_t mps,
        uint8_t interval) {
    if (dev == NULL) {
        return -1;
    }
    return xhci_int_in_open(&dev->xdev, ep_addr, mps, interval);
}

int bsp_usb_int_in_poll(bsp_usb_dev_t* dev, uint8_t ep_addr, void* buf,
        int size) {
    if (dev == NULL) {
        return -1;
    }
    return xhci_int_in_poll(&dev->xdev, ep_addr, buf, size);
}

int bsp_usb_bulk_open(bsp_usb_dev_t* dev, uint8_t ep_addr, uint16_t mps) {
    if (dev == NULL) {
        return -1;
    }
    return xhci_bulk_open(&dev->xdev, ep_addr, mps);
}

int bsp_usb_bulk_xfer(bsp_usb_dev_t* dev, uint8_t ep_addr, void* data,
        int len, bool dir_in) {
    if (dev == NULL) {
        return -1;
    }
    return xhci_bulk_xfer(&dev->xdev, ep_addr, data, len, dir_in,
            BSP_USB_BULK_TIMEOUT_MS);
}

/*
 * A STALL leaves the halt latched on both sides: the xhci driver already
 * did Reset Endpoint + Set TR Dequeue on the controller side, but the
 * device keeps returning STALL until its own halt feature is cleared
 * (USB 2.0 9.4.5).
 */
int bsp_usb_ep_clear_halt(bsp_usb_dev_t* dev, uint8_t ep_addr) {
    usb_setup_pkt_t setup;
    if (dev == NULL) {
        return -1;
    }
    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = USB_REQTYPE_STD_EP_OUT;
    setup.bRequest = USB_REQ_CLEAR_FEATURE;
    setup.wValue = USB_FEAT_ENDPOINT_HALT;
    setup.wIndex = ep_addr;
    return bsp_usb_control_xfer(dev, &setup, NULL, false);
}

/* ---------------- mass storage (bulk-only transport) ---------------- */

#define MSC_BULK_TIMEOUT_MS 2000u
#define MSC_CBW_TIMEOUT_MS 500u

/* one bulk transaction; xhci tracks data toggles in hardware */
static int msc_bulk_xfer(bool dir_in, uint8_t ep_addr, void* data,
        uint32_t len, uint32_t timeout_ms) {
    return xhci_bulk_xfer(&_msc.dev->xdev, ep_addr, data, (int)len,
            dir_in, timeout_ms);
}

/*
 * bulk-only mass storage reset + endpoint unhalt. Controller-side ring
 * recovery (Reset Endpoint + Set TR Dequeue) already happened inside
 * xhci_bulk_xfer() on the failed transaction.
 */
static void msc_recover(void) {
    usb_setup_pkt_t setup;

    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = USB_REQTYPE_CLASS_IFACE_OUT;
    setup.bRequest = 0xFF; /* bulk-only mass storage reset */
    setup.wIndex = _msc.iface_num;
    (void)bsp_usb_control_xfer(_msc.dev, &setup, NULL, false);

    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = USB_REQTYPE_STD_EP_OUT;
    setup.bRequest = USB_REQ_CLEAR_FEATURE;
    setup.wValue = USB_FEAT_ENDPOINT_HALT;
    setup.wIndex = _msc.ep_in;
    (void)bsp_usb_control_xfer(_msc.dev, &setup, NULL, false);
    setup.wIndex = _msc.ep_out;
    (void)bsp_usb_control_xfer(_msc.dev, &setup, NULL, false);
}

/* CBW -> optional data phase -> CSW. dir_in: data phase is IN.
   Returns 0 on CSW status "passed", -1 otherwise. */
static int usb_msc_command(const uint8_t* cdb, uint8_t cdb_len, bool dir_in,
        void* data, uint32_t data_len) {
    usb_cbw_t cbw;
    usb_csw_t csw;
    uint8_t* payload = NULL;
    uint32_t tag;
    int ret = -1;

    if (!_msc.claimed || _msc.dev == NULL || cdb_len > 16) {
        return -1;
    }
    if (data_len > 0) {
        payload = (uint8_t*)malloc(data_len);
        if (payload == NULL) {
            return -1;
        }
        if (!dir_in) {
            memcpy(payload, data, data_len);
        }
    }

    for (int attempt = 0; attempt < 2; ++attempt) {
        tag = ++_msc.tag;
        if (tag == 0) {
            tag = ++_msc.tag;
        }
        memset(&cbw, 0, sizeof(cbw));
        cbw.dCBWSignature = USB_MSC_CBW_SIG;
        cbw.dCBWTag = tag;
        cbw.dCBWDataTransferLength = data_len;
        cbw.bmCBWFlags = dir_in ? 0x80u : 0x00u;
        cbw.bCBWLUN = 0;
        cbw.bCBWCBLength = cdb_len;
        memcpy(cbw.CBWCB, cdb, cdb_len);

        if (msc_bulk_xfer(false, _msc.ep_out, &cbw, sizeof(cbw),
                MSC_CBW_TIMEOUT_MS) != (int)sizeof(cbw)) {
            msc_recover();
            continue;
        }

        if (data_len > 0) {
            int xret = msc_bulk_xfer(dir_in,
                    dir_in ? _msc.ep_in : _msc.ep_out,
                    payload, data_len, MSC_BULK_TIMEOUT_MS);
            if (xret != (int)data_len) {
                msc_recover();
                continue;
            }
        }

        if (msc_bulk_xfer(true, _msc.ep_in, &csw, sizeof(csw),
                MSC_BULK_TIMEOUT_MS) != (int)sizeof(csw)) {
            msc_recover();
            continue;
        }

        if (csw.dCSWSignature != USB_MSC_CSW_SIG || csw.dCSWTag != tag) {
            msc_recover();
            continue;
        }
        if (csw.bCSWStatus != 0) {
            ret = -1;
            goto out;
        }
        if (dir_in && data != NULL && data_len > 0) {
            memcpy(data, payload, data_len);
        }
        ret = 0;
        goto out;
    }

out:
    free(payload);
    return ret;
}

static int usb_msc_test_unit_ready(void) {
    uint8_t cdb[6];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_OPCODE_TEST_UNIT_READY;
    return usb_msc_command(cdb, 6, false, NULL, 0);
}

static int usb_msc_sync_cache(void) {
    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_OPCODE_SYNC_CACHE10;
    return usb_msc_command(cdb, 10, false, NULL, 0);
}

static int msc_attach(bsp_usb_dev_t* dev, uint8_t iface_num,
        uint8_t ep_in, uint8_t ep_out, uint16_t mps_in, uint16_t mps_out) {
    uint8_t inquiry[36];
    uint8_t capacity[8];
    uint8_t cdb[10];

    memset(&_msc, 0, sizeof(_msc));
    _msc.claimed = true;
    _msc.dev = dev;
    _msc.iface_num = iface_num;
    _msc.ep_in = ep_in;
    _msc.ep_out = ep_out;
    _msc.mps_in = mps_in == 0 ? 64 : mps_in;
    _msc.mps_out = mps_out == 0 ? 64 : mps_out;
    _msc.tag = 1;

    /* xhci bulk endpoints must be configured before the first transfer */
    if (xhci_bulk_open(&dev->xdev, ep_in, _msc.mps_in) != 0 ||
            xhci_bulk_open(&dev->xdev, ep_out, _msc.mps_out) != 0) {
        klog("bsp_usb: msc bulk_open_failed slot=%u\n", dev->xdev.slot_id);
        memset(&_msc, 0, sizeof(_msc));
        return -1;
    }

    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_OPCODE_INQUIRY;
    cdb[4] = sizeof(inquiry);
    memset(inquiry, 0, sizeof(inquiry));
    if (usb_msc_command(cdb, 6, true, inquiry, sizeof(inquiry)) != 0) {
        klog("bsp_usb: msc inquiry_failed slot=%u\n", dev->xdev.slot_id);
        memset(&_msc, 0, sizeof(_msc));
        return -1;
    }
    klog("bsp_usb: msc inquiry slot=%u type=%02x vendor=%.8s product=%.16s\n",
            dev->xdev.slot_id, inquiry[0], (const char*)(inquiry + 8),
            (const char*)(inquiry + 16));

    /* media may need a spin-up/debounce window after plug-in */
    {
        int ready = -1;
        for (int i = 0; i < 20; ++i) {
            ready = usb_msc_test_unit_ready();
            if (ready == 0) {
                break;
            }
            proc_usleep(100000);
        }
        if (ready != 0) {
            klog("bsp_usb: msc not_ready slot=%u\n", dev->xdev.slot_id);
            memset(&_msc, 0, sizeof(_msc));
            return -1;
        }
    }

    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_OPCODE_READ_CAPACITY10;
    memset(capacity, 0, sizeof(capacity));
    if (usb_msc_command(cdb, 10, true, capacity, sizeof(capacity)) != 0) {
        klog("bsp_usb: msc read_capacity_failed slot=%u\n", dev->xdev.slot_id);
        memset(&_msc, 0, sizeof(_msc));
        return -1;
    }
    /* READ CAPACITY returns the last LBA; sector_count is last_lba + 1 */
    _msc.sector_count = be32(capacity) + 1u;
    _msc.sector_size = be32(capacity + 4);
    if (_msc.sector_size != USB_MSC_SECTOR_SIZE) {
        klog("bsp_usb: msc unsupported_sector_size=%u slot=%u\n",
                _msc.sector_size, dev->xdev.slot_id);
        memset(&_msc, 0, sizeof(_msc));
        return -1;
    }
    _msc.ready = true;
    klog("bsp_usb: msc attached slot=%u sectors=%u size=%u\n",
            dev->xdev.slot_id, _msc.sector_count, _msc.sector_size);

    /* auto-mount the FAT32 volume: spawn a fat32fsd bound to this device.
       A stale mount left by a daemon that did not exit blocks the new
       mount, so skip the spawn in that case instead of double-mounting. */
    {
        fsinfo_t mnt_info;
        if (vfs_get_by_name("/mnt/udisk0", &mnt_info) == 0) {
            klog("bsp_usb: msc mount busy, /mnt/udisk0 still mounted\n");
        }
        else {
            int pid = fork();
            if (pid == 0) {
                proc_detach();
                if (proc_exec("/drivers/fat32fsd -u /dev/hid0 /mnt/udisk0") != 0) {
                    exit(-1);
                }
            }
            else if (pid > 0) {
                _msc.child_pid = pid;
                klog("bsp_usb: msc mounting /mnt/udisk0 pid=%d\n", pid);
            }
            else {
                klog("bsp_usb: msc mount_fork_failed\n");
            }
        }
    }
    return 0;
}

/* scan the config descriptor for a bulk-only mass-storage interface and
   claim it; only the first MSC interface is used */
int bsp_usb_msc_probe(bsp_usb_dev_t* dev, const uint8_t* cfg, int cfg_len) {
    const usb_iface_desc_t* msc_iface = NULL;
    uint8_t ep_in = 0, ep_out = 0;
    uint16_t mps_in = 0, mps_out = 0;

    if (dev == NULL || cfg == NULL || _msc.claimed || _msc.ready) {
        return -1;
    }

    for (int off = 0; off + 2 <= cfg_len; ) {
        uint8_t len = cfg[off];
        uint8_t type = cfg[off + 1];

        if (len < 2 || off + len > cfg_len) {
            break;
        }
        if (type == USB_DESC_INTERFACE && len >= sizeof(usb_iface_desc_t)) {
            const usb_iface_desc_t* iface = (const usb_iface_desc_t*)(cfg + off);
            if (iface->bInterfaceClass == USB_CLASS_MSC &&
                    iface->bInterfaceProtocol == USB_MSC_PROTO_BBB &&
                    (iface->bInterfaceSubClass == USB_MSC_SUBCLASS_SCSI ||
                     iface->bInterfaceSubClass == USB_MSC_SUBCLASS_UFI)) {
                if (msc_iface == NULL) {
                    msc_iface = iface;
                    ep_in = 0;
                    ep_out = 0;
                }
            }
            else {
                /* endpoints belong to the preceding interface only */
                msc_iface = NULL;
            }
        }
        else if (type == USB_DESC_ENDPOINT && msc_iface != NULL &&
                len >= sizeof(usb_ep_desc_t)) {
            const usb_ep_desc_t* ep = (const usb_ep_desc_t*)(cfg + off);
            if ((ep->bmAttributes & 0x3u) == USB_ENDPOINT_XFER_BULK) {
                if ((ep->bEndpointAddress & USB_ENDPOINT_IN) != 0) {
                    ep_in = ep->bEndpointAddress;
                    mps_in = (uint16_t)(ep->wMaxPacketSize & 0x07FFu);
                }
                else {
                    ep_out = ep->bEndpointAddress;
                    mps_out = (uint16_t)(ep->wMaxPacketSize & 0x07FFu);
                }
            }
        }
        off += len;
    }

    if (msc_iface == NULL || ep_in == 0 || ep_out == 0) {
        return -1;
    }
    klog("bsp_usb: msc found slot=%u iface=%u subclass=%u ep_in=%02x ep_out=%02x\n",
            dev->xdev.slot_id, msc_iface->bInterfaceNumber,
            msc_iface->bInterfaceSubClass, ep_in, ep_out);
    return msc_attach(dev, msc_iface->bInterfaceNumber,
            ep_in, ep_out, mps_in, mps_out);
}

/* device (or the tree it sits on) is gone: tell the fs daemon to exit
   and drop all MSC state. Safe to call when nothing is attached. */
void bsp_usb_msc_detach(bsp_usb_dev_t* dev) {
    if (!_msc.claimed || _msc.dev != dev) {
        return;
    }
    klog("bsp_usb: msc detached slot=%u\n", _msc.dev->xdev.slot_id);
    if (_msc.child_pid > 0) {
        /* fire-and-forget: the device is already gone, the daemon only
           needs the hint to unmount and exit */
        dev_cntl_by_pid(_msc.child_pid, USBFS_CMD_QUIT, NULL, NULL);
    }
    memset(&_msc, 0, sizeof(_msc));
}

bool bsp_usb_msc_attached(bsp_usb_dev_t* dev) {
    return _msc.claimed && _msc.dev == dev;
}

/* sector transport served to fat32fsd over FS_CMD_DEV_CNTL */
static int usb_msc_read_sectors(uint32_t sector, uint32_t count, proto_t* out) {
    uint8_t cdb[10];
    uint32_t len = count * USB_MSC_SECTOR_SIZE;
    uint8_t buf[USBMSC_MAX_SECTORS * USB_MSC_SECTOR_SIZE];

    if (len > sizeof(buf)) {
        return -1;
    }
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_OPCODE_READ10;
    put_be32(cdb + 2, sector);
    cdb[7] = (uint8_t)(count >> 8);
    cdb[8] = (uint8_t)(count & 0xFFu);
    if (usb_msc_command(cdb, 10, true, buf, len) != 0) {
        return -1;
    }
    PF->add(out, buf, len);
    return 0;
}

static int usb_msc_write_sectors(uint32_t sector, uint32_t count,
        const void* data, int32_t data_len) {
    uint8_t cdb[10];
    uint32_t len = count * USB_MSC_SECTOR_SIZE;

    if ((uint32_t)data_len < len) {
        return -1;
    }
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_OPCODE_WRITE10;
    put_be32(cdb + 2, sector);
    cdb[7] = (uint8_t)(count >> 8);
    cdb[8] = (uint8_t)(count & 0xFFu);
    return usb_msc_command(cdb, 10, false, (void*)data, len);
}

int bsp_usb_msc_cntl(vdevice_t* vdev, int from_pid, int cmd,
        proto_t* in, proto_t* out) {
    (void)vdev;
    (void)from_pid;

    switch (cmd) {
    case USBMSC_CMD_INFO:
        PF->addi(out, _msc.ready ? 1 : 0);
        PF->addi(out, _msc.sector_count);
        PF->addi(out, _msc.sector_size);
        return 0;
    case USBMSC_CMD_READ: {
        uint32_t sector = (uint32_t)proto_read_int(in);
        uint32_t count = (uint32_t)proto_read_int(in);
        if (!_msc.ready || count == 0 || count > USBMSC_MAX_SECTORS ||
                sector + count > _msc.sector_count) {
            return -1;
        }
        return usb_msc_read_sectors(sector, count, out);
    }
    case USBMSC_CMD_WRITE: {
        uint32_t sector = (uint32_t)proto_read_int(in);
        uint32_t count = (uint32_t)proto_read_int(in);
        int32_t sz = 0;
        void* data = proto_read(in, &sz);
        if (!_msc.ready || count == 0 || count > USBMSC_MAX_SECTORS ||
                sector + count > _msc.sector_count || data == NULL) {
            return -1;
        }
        return usb_msc_write_sectors(sector, count, data, sz);
    }
    case USBMSC_CMD_FLUSH:
        if (!_msc.ready) {
            return 0;
        }
        return (usb_msc_sync_cache() == 0) ? 0 : -1;
    default:
        return -1;
    }
}
