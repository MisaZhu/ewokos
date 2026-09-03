/*
 * usbhostd: USB HID host daemon on /dev/hid0.
 *
 * Platform-neutral policy layer: device enumeration, HID report-descriptor
 * classification (libusb's usbhid), report-ID based fan-out to
 * hid_keybd/hid_moused/hid_touchd (libusb's usbhidsrv), and the vdevice
 * glue. All controller access goes through the machine's bsp_usb layer,
 * which rides on the concrete HCD in libs/arch_<HW> (xHCI on raspi5,
 * DWC2 on raspix, ...).
 *
 * Policy notes that hold on every platform:
 *  - hubs work through the standard hub class requests;
 *  - interrupt-IN endpoints are opened once and polled from the loop;
 *  - a STALL needs the device-side halt cleared or the endpoint stays
 *    dead until replug.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <usb/bsp_usb.h>
#include <usb/usb_defs.h>
#include <usb/usbhid.h>
#include <usb/usbhidsrv.h>

#define USB_MAX_INPUTS 8
#define USB_MAX_DEVS 8
#define USB_SCAN_INTERVAL_MS 1000u
/* active-loop pacing floor. Interrupt reports arrive at most every 8ms
   (INT_IN_MIN_EXP clamp in the xhci bsp), so a 2ms loop floor adds at
   most a quarter-report of latency while halving the loop's own share of
   the active CPU load versus a 1ms floor */
#define USB_IDLE_SLEEP_MIN_US 2000u
#define USB_IDLE_SLEEP_MAX_US 8000u
#define USB_NO_INPUT_SLEEP_US 20000u
#define USB_MAX_HUB_DEPTH 2
/* hub downstream-port bring-up: cap the bPwrOn2PwrGood wait, give
   slow-debouncing devices a grace window after power-good, and pace
   attach retries for ports whose first enumeration failed */
#define USB_HUB_PWR_WAIT_MAX_MS 500u
#define USB_HUB_CONNECT_GRACE_MS 300u
#define USB_HUB_RETRY_INTERVAL_MS 500u
/* root-port bring-up failures: after this many consecutive failures the
   whole controller is re-initialized (a wedged port state on some
   controllers only clears with a bring-up from scratch) */
#define USB_ENUM_FAIL_REINIT_AFTER 6u
/* backlog wake re-assert: an edge wake can be swallowed by a consumer
   parked in a generic token-0 IPC wait (see usbhid_backlog), so while a
   subscriber queue stays undrained its directed wake is re-fired at this
   cadence. Healthy consumers drain within a few ms, so this normally
   never fires */
#define USB_WAKE_REASSERT_MS 30u

/* enumeration / bring-up logging; per-report traffic stays silent */
#define USB_LOG_ENABLE 1
#if USB_LOG_ENABLE
#define slog(...) klog(__VA_ARGS__)
#else
static inline void usb_log_none(const char* fmt, ...) { (void)fmt; }
#define slog(...) usb_log_none(__VA_ARGS__)
#endif

/* an enumerated USB device (input device or hub) */
typedef struct {
    bool present;
    bool is_hub;
    bool unsupported;    /* enumerated, but no interface we can drive */
    uint8_t hub_ports;
    int8_t parent;       /* _devs index of parent hub, -1 = root port */
    uint8_t parent_port; /* hub port (1-based) when parent >= 0 */
    uint8_t depth;
    int root_port;       /* bsp flat root port this tree hangs off */
    uint64_t port_retry_ms[9]; /* per hub port: next allowed attach retry */
    bsp_usb_dev_t* hdev;
} usb_dev_t;

typedef struct {
    bool present;
    usb_input_type_t type;
    int8_t dev_idx; /* _devs index */
    uint8_t iface_num;
    uint8_t ep_addr;
    uint16_t max_packet;
    uint8_t report_len;
    uint8_t kbd_report_id;   /* report-ID keyboard / composite */
    uint8_t mouse_report_id; /* composite only */
    mouse_parser_t mouse;
    touch_parser_t touch;
    uint8_t last_report[USB_MAX_REPORT];
    uint8_t last_len;
    uint8_t last_mouse_btn;  /* buttons of the last dispatched mouse frame */
} usb_input_dev_t;

static usb_dev_t _devs[USB_MAX_DEVS];
static usb_input_dev_t _inputs[USB_MAX_INPUTS];
static uint64_t _next_scan_ms = 0;
static uint32_t _idle_sleep_us = USB_IDLE_SLEEP_MIN_US;
static uint32_t _enum_fail_streak = 0;
static uint64_t _next_reassert_ms = 0;

static inline uint16_t le16(const void* p) {
    const uint8_t* b = (const uint8_t*)p;
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

/* ---------------- control request helpers on top of bsp_usb ---------------- */

static int usb_get_descriptor(bsp_usb_dev_t* hdev, uint8_t req_type,
        uint8_t desc_type, uint8_t index, uint16_t lang_or_iface,
        void* buf, uint16_t size) {
    usb_setup_pkt_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = req_type;
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wValue = (uint16_t)((desc_type << 8) | index);
    setup.wIndex = lang_or_iface;
    setup.wLength = size;
    return bsp_usb_control_xfer(hdev, &setup, buf, true);
}

static int usb_set_configuration(bsp_usb_dev_t* hdev, uint8_t config) {
    usb_setup_pkt_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = USB_REQTYPE_STD_OUT;
    setup.bRequest = USB_REQ_SET_CONFIGURATION;
    setup.wValue = config;
    return bsp_usb_control_xfer(hdev, &setup, NULL, false);
}

static int usb_hid_set_protocol(bsp_usb_dev_t* hdev, uint8_t iface, uint8_t protocol) {
    usb_setup_pkt_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = USB_REQTYPE_CLASS_IFACE_OUT;
    setup.bRequest = USB_REQ_SET_PROTOCOL;
    setup.wValue = protocol; /* 0 = boot, 1 = report */
    setup.wIndex = iface;
    return bsp_usb_control_xfer(hdev, &setup, NULL, false);
}

static int usb_hid_set_idle(bsp_usb_dev_t* hdev, uint8_t iface) {
    usb_setup_pkt_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = USB_REQTYPE_CLASS_IFACE_OUT;
    setup.bRequest = USB_REQ_SET_IDLE;
    setup.wValue = 0; /* duration 0: report only on change */
    setup.wIndex = iface;
    return bsp_usb_control_xfer(hdev, &setup, NULL, false);
}

static int usb_hub_port_status(bsp_usb_dev_t* hdev, uint8_t port,
        uint16_t* status, uint16_t* change) {
    usb_setup_pkt_t setup;
    uint8_t buf[4];
    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = USB_REQTYPE_CLASS_PORT_IN;
    setup.bRequest = USB_REQ_GET_STATUS;
    setup.wIndex = port;
    setup.wLength = 4;
    if (bsp_usb_control_xfer(hdev, &setup, buf, true) < 4) {
        return -1;
    }
    if (status != NULL) {
        *status = le16(buf);
    }
    if (change != NULL) {
        *change = le16(buf + 2);
    }
    return 0;
}

static int usb_hub_port_feature(bsp_usb_dev_t* hdev, uint8_t port,
        uint16_t feature, bool set) {
    usb_setup_pkt_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = USB_REQTYPE_CLASS_PORT_OUT;
    setup.bRequest = set ? USB_REQ_SET_FEATURE : USB_REQ_CLEAR_FEATURE;
    setup.wValue = feature;
    setup.wIndex = port;
    return bsp_usb_control_xfer(hdev, &setup, NULL, false);
}

/* ---------------- device table ---------------- */

static void usb_dev_remove(int idx);

static int usb_dev_alloc(void) {
    for (int i = 0; i < USB_MAX_DEVS; ++i) {
        if (!_devs[i].present) {
            return i;
        }
    }
    /*
     * Parked unsupported devices (see usb_enumerate_device) hold their slot
     * on purpose, so reclaim one before giving up: a device we can actually
     * drive is always worth more than a stick we only keep to avoid
     * re-enumerating it. Never reclaim a device the bsp layer claimed
     * for mass storage though: its sector I/O would die mid-flight.
     */
    for (int i = 0; i < USB_MAX_DEVS; ++i) {
        if (_devs[i].unsupported && !bsp_usb_msc_attached(_devs[i].hdev)) {
            usb_dev_remove(i);
            return i;
        }
    }
    return -1;
}

static int usb_input_alloc(void) {
    for (int i = 0; i < USB_MAX_INPUTS; ++i) {
        if (!_inputs[i].present) {
            return i;
        }
    }
    return -1;
}

/* drop one device and every input riding on it */
static void usb_dev_remove(int idx) {
    if (idx < 0 || !_devs[idx].present) {
        return;
    }
    for (int i = 0; i < USB_MAX_INPUTS; ++i) {
        if (_inputs[i].present && _inputs[i].dev_idx == idx) {
            memset(&_inputs[i], 0, sizeof(_inputs[i]));
        }
    }
    bsp_usb_msc_detach(_devs[idx].hdev);
    bsp_usb_device_detach(_devs[idx].hdev);
    memset(&_devs[idx], 0, sizeof(_devs[idx]));
}

/* remove a whole subtree: the device plus everything behind it (hubs) */
static void usb_dev_remove_tree(int idx) {
    for (int i = 0; i < USB_MAX_DEVS; ++i) {
        if (_devs[i].present && _devs[i].parent == idx) {
            usb_dev_remove_tree(i);
        }
    }
    usb_dev_remove(idx);
}

static void usb_root_remove(int root_port) {
    for (int i = 0; i < USB_MAX_DEVS; ++i) {
        if (_devs[i].present && _devs[i].parent < 0 &&
                _devs[i].root_port == root_port) {
            usb_dev_remove_tree(i);
        }
    }
}

/* ---------------- HID candidate parsing/registration ---------------- */

static int usb_parse_candidates(const uint8_t* cfg, int cfg_len,
        hid_candidate_t* candidates, int max_candidates) {
    const usb_iface_desc_t* current_iface = NULL;
    uint16_t current_report_desc_len = 0;
    int count = 0;

    for (int off = 0; off + 2 <= cfg_len; ) {
        uint8_t len = cfg[off];
        uint8_t type = cfg[off + 1];

        if (len < 2 || off + len > cfg_len) {
            break;
        }

        if (type == USB_DESC_INTERFACE && len >= sizeof(usb_iface_desc_t)) {
            current_iface = (const usb_iface_desc_t*)(cfg + off);
            current_report_desc_len = 0;
        }
        else if (type == USB_DESC_HID && current_iface != NULL && len >= sizeof(usb_hid_desc_t)) {
            const usb_hid_desc_t* hid = (const usb_hid_desc_t*)(cfg + off);
            current_report_desc_len = hid->wReportDescriptorLength;
        }
        else if (type == USB_DESC_ENDPOINT &&
                current_iface != NULL &&
                len >= sizeof(usb_ep_desc_t) &&
                current_iface->bInterfaceClass == USB_CLASS_HID) {
            const usb_ep_desc_t* ep = (const usb_ep_desc_t*)(cfg + off);

            if ((ep->bEndpointAddress & USB_ENDPOINT_IN) != 0 &&
                    (ep->bmAttributes & 0x3u) == USB_ENDPOINT_XFER_INTERRUPT &&
                    count < max_candidates) {
                candidates[count].valid = true;
                candidates[count].iface_num = current_iface->bInterfaceNumber;
                candidates[count].subclass = current_iface->bInterfaceSubClass;
                candidates[count].protocol = current_iface->bInterfaceProtocol;
                candidates[count].ep_addr = ep->bEndpointAddress;
                /* bInterval is passed through verbatim: the bsp layer
                   decides how to honour it on its controller */
                candidates[count].interval = ep->bInterval == 0 ? 10 : ep->bInterval;
                candidates[count].max_packet = (uint16_t)(ep->wMaxPacketSize & 0x07FFu);
                candidates[count].report_desc_len = current_report_desc_len;
                count++;
            }
        }

        off += len;
    }
    return count;
}

/* open the interrupt endpoint and fill the common input slot fields */
static int usb_input_setup(int dev_idx, const hid_candidate_t* cand,
        usb_input_type_t type) {
    bsp_usb_dev_t* hdev = _devs[dev_idx].hdev;
    uint16_t mps = cand->max_packet == 0 ? 8 : cand->max_packet;
    int slot;

    if (mps > USB_MAX_REPORT) {
        mps = USB_MAX_REPORT;
    }
    slot = usb_input_alloc();
    if (slot < 0) {
        slog("usbhostd: register %s failed no_slot iface=%u\n",
                usbhid_input_type_name(type), cand->iface_num);
        return -1;
    }
    if (bsp_usb_int_in_open(hdev, cand->ep_addr, mps, cand->interval) != 0) {
        slog("usbhostd: register %s failed ep_open iface=%u ep=%02x\n",
                usbhid_input_type_name(type), cand->iface_num, cand->ep_addr);
        return -1;
    }
    memset(&_inputs[slot], 0, sizeof(_inputs[slot]));
    _inputs[slot].present = true;
    _inputs[slot].type = type;
    _inputs[slot].dev_idx = (int8_t)dev_idx;
    _inputs[slot].iface_num = cand->iface_num;
    _inputs[slot].ep_addr = cand->ep_addr;
    _inputs[slot].max_packet = mps;
    return slot;
}

static int usb_register_keyboard(int dev_idx, const hid_candidate_t* cand,
        uint8_t kbd_rid) {
    bsp_usb_dev_t* hdev = _devs[dev_idx].hdev;
    int slot;

    /* Set_Protocol is only defined for boot-subclass interfaces; non-boot
       interfaces default to Report protocol and must not receive it.
       Report IDs only exist in Report protocol, so a boot interface whose
       descriptor carries one must run in Report protocol or the IDs are
       silently dropped from every report. */
    if (cand->subclass == USB_SUBCLASS_BOOT) {
        (void)usb_hid_set_protocol(hdev, cand->iface_num, kbd_rid != 0 ? 1 : 0);
    }
    (void)usb_hid_set_idle(hdev, cand->iface_num);
    slot = usb_input_setup(dev_idx, cand, USB_INPUT_KEYBOARD);
    if (slot < 0) {
        return -1;
    }
    _inputs[slot].kbd_report_id = kbd_rid;
    if (kbd_rid != 0) {
        /* the endpoint may also carry other collections (consumer/media):
           request full packets and filter by report ID when polling */
        _inputs[slot].report_len = (uint8_t)_inputs[slot].max_packet;
    }
    else {
        _inputs[slot].report_len = 8;
    }
    slog("usbhostd: register keyboard slot=%d dev=%d iface=%u ep=%02x interval=%u maxpkt=%u rid=%u\n",
            slot, dev_idx, cand->iface_num, cand->ep_addr, cand->interval,
            cand->max_packet, kbd_rid);
    return 0;
}

static int usb_register_mouse(int dev_idx, const hid_candidate_t* cand, const mouse_parser_t* parser) {
    bsp_usb_dev_t* hdev = _devs[dev_idx].hdev;
    bool boot = cand->subclass == USB_SUBCLASS_BOOT;
    uint16_t mps = cand->max_packet == 0 ? 8 : cand->max_packet;
    bool use_parser;
    int slot;

    /* A wrong parser result (garbled descriptor read, unusual descriptor)
       makes normalize decode X/Y from the wrong bits -- typically X still
       looks fine while Y reads a padding/wheel field and stays 0 or barely
       moves. Only trust the parser when the layout is plausible. strict is
       used for boot interfaces where we can fall back to the guaranteed
       [btn,dx,dy,wheel] boot layout instead. */
    use_parser = parser != NULL && parser->valid &&
            mouse_parser_sane(parser, mps, boot);
    if (parser != NULL && parser->valid && !use_parser) {
        slog("usbhostd: mouse parser rejected dev=%d iface=%u rid=%u bytes=%u x=%d/%d y=%d/%d wheel=%d/%d maxpkt=%u fallback=%s\n",
                dev_idx, cand->iface_num, parser->report_id, parser->report_bytes,
                parser->x_bit, parser->x_size, parser->y_bit, parser->y_size,
                parser->wheel_bit, parser->wheel_size, cand->max_packet,
                boot ? "boot" : "raw");
    }
    /* Set_Protocol is only defined for boot-subclass interfaces; non-boot
       interfaces default to Report protocol and must not receive it.
       When the parser is not trusted a boot mouse is kept in boot protocol
       so the raw [btn,dx,dy,wheel] layout stays valid. */
    if (boot) {
        (void)usb_hid_set_protocol(hdev, cand->iface_num, use_parser ? 1 : 0);
    }
    (void)usb_hid_set_idle(hdev, cand->iface_num);
    slot = usb_input_setup(dev_idx, cand, USB_INPUT_MOUSE);
    if (slot < 0) {
        return -1;
    }
    if (use_parser) {
        _inputs[slot].mouse = *parser;
        _inputs[slot].report_len = parser->report_bytes;
        slog("usbhostd: register mouse slot=%d dev=%d iface=%u ep=%02x report_id=%u report_len=%u maxpkt=%u x=%d/%d y=%d/%d wheel=%d/%d\n",
                slot, dev_idx, cand->iface_num, cand->ep_addr,
                parser->report_id, parser->report_bytes, cand->max_packet,
                parser->x_bit, parser->x_size, parser->y_bit, parser->y_size,
                parser->wheel_bit, parser->wheel_size);
    }
    else {
        _inputs[slot].report_len = cand->max_packet > 0 ? (uint8_t)_inputs[slot].max_packet : 4;
        slog("usbhostd: register mouse slot=%d dev=%d iface=%u ep=%02x interval=%u maxpkt=%u raw\n",
                slot, dev_idx, cand->iface_num, cand->ep_addr, cand->interval, cand->max_packet);
    }
    return 0;
}

static int usb_register_composite(int dev_idx, const hid_candidate_t* cand,
        uint8_t kbd_id, uint8_t mouse_id, const mouse_parser_t* parser) {
    bsp_usb_dev_t* hdev = _devs[dev_idx].hdev;
    int slot;

    /* report IDs only exist in Report protocol; boot protocol would strip
       the mouse collection entirely */
    if (cand->subclass == USB_SUBCLASS_BOOT) {
        (void)usb_hid_set_protocol(hdev, cand->iface_num, 1);
    }
    (void)usb_hid_set_idle(hdev, cand->iface_num);
    slot = usb_input_setup(dev_idx, cand, USB_INPUT_COMPOSITE);
    if (slot < 0) {
        return -1;
    }
    /* variable-size reports: always request a full packet */
    _inputs[slot].report_len = (uint8_t)_inputs[slot].max_packet;
    _inputs[slot].kbd_report_id = kbd_id;
    _inputs[slot].mouse_report_id = mouse_id;
    if (parser != NULL && parser->valid) {
        if (mouse_parser_sane(parser, cand->max_packet, false)) {
            _inputs[slot].mouse = *parser;
        }
        else {
            slog("usbhostd: composite mouse parser rejected dev=%d iface=%u rid=%u bytes=%u x=%d/%d y=%d/%d maxpkt=%u\n",
                    dev_idx, cand->iface_num, parser->report_id, parser->report_bytes,
                    parser->x_bit, parser->x_size, parser->y_bit, parser->y_size,
                    cand->max_packet);
        }
    }
    slog("usbhostd: register composite slot=%d dev=%d iface=%u ep=%02x kbd_id=%u mouse_id=%u mouse_len=%u\n",
            slot, dev_idx, cand->iface_num, cand->ep_addr, kbd_id, mouse_id,
            _inputs[slot].mouse.valid ? _inputs[slot].mouse.report_bytes : 0);
    return 0;
}

static int usb_register_touch(int dev_idx, const hid_candidate_t* cand,
        const touch_parser_t* parser) {
    bsp_usb_dev_t* hdev = _devs[dev_idx].hdev;
    int slot;

    /*
     * Some USB touch panels advertise boot-mouse compatibility on the
     * interface descriptor, but their real touch data is only available
     * in Report protocol. Keep them out of Boot protocol here.
     */
    if (cand->subclass == USB_SUBCLASS_BOOT) {
        (void)usb_hid_set_protocol(hdev, cand->iface_num, 1);
    }
    (void)usb_hid_set_idle(hdev, cand->iface_num);
    slot = usb_input_setup(dev_idx, cand, USB_INPUT_TOUCH);
    if (slot < 0) {
        return -1;
    }
    _inputs[slot].report_len = parser->report_bytes;
    _inputs[slot].touch = *parser;
    slog("usbhostd: register touch slot=%d dev=%d iface=%u ep=%02x report_id=%u report_len=%u tip=%d x=%d y=%d\n",
            slot, dev_idx, cand->iface_num, cand->ep_addr, parser->report_id,
            parser->report_bytes, parser->tip_bit, parser->x_bit, parser->y_bit);
    return 0;
}

/* ---------------- enumeration ---------------- */

static int usb_enumerate_hub(int dev_idx);

/*
 * Enumerate one freshly reset device. parent < 0 means it sits on bsp root
 * port root_port; otherwise it hangs off _devs[parent] hub port hub_port.
 * speed is BSP_USB_SPEED_*. Returns the number of registered inputs (a hub
 * counts its children), or < 0 on failure.
 */
static int usb_enumerate_device(int root_port, int speed, int parent, int hub_port) {
    usb_device_desc_t dev_desc;
    uint8_t cfg_head[9];
    uint8_t* cfg_buf = NULL;
    uint16_t total_len;
    int cand_count;
    hid_candidate_t candidates[USB_MAX_CANDIDATES];
    int registered = 0;
    int dev_idx;
    uint8_t depth = parent >= 0 ? _devs[parent].depth + 1 : 0;

    dev_idx = usb_dev_alloc();
    if (dev_idx < 0) {
        slog("usbhostd: enumerate no_dev_slot\n");
        return -1;
    }
    usb_dev_t* dev = &_devs[dev_idx];
    memset(dev, 0, sizeof(*dev));

    dev->hdev = bsp_usb_device_attach(root_port, speed,
            parent >= 0 ? _devs[parent].hdev : NULL, hub_port);
    if (dev->hdev == NULL) {
        return -1;
    }
    dev->present = true;
    dev->parent = (int8_t)parent;
    dev->parent_port = (uint8_t)(parent >= 0 ? hub_port : 0);
    dev->depth = depth;
    dev->root_port = root_port;

    /* first 8 bytes reveal bMaxPacketSize0 before the full descriptor */
    memset(&dev_desc, 0, sizeof(dev_desc));
    if (usb_get_descriptor(dev->hdev, USB_REQTYPE_STD_IN,
            USB_DESC_DEVICE, 0, 0, &dev_desc, 8) < 8) {
        slog("usbhostd: enumerate get_desc8_failed\n");
        usb_dev_remove(dev_idx);
        return -1;
    }
    if (bsp_usb_device_update_mps0(dev->hdev, dev_desc.bMaxPacketSize0) != 0) {
        slog("usbhostd: enumerate update_mps0_failed mps0=%u\n",
                dev_desc.bMaxPacketSize0);
        usb_dev_remove(dev_idx);
        return -1;
    }
    if (usb_get_descriptor(dev->hdev, USB_REQTYPE_STD_IN,
            USB_DESC_DEVICE, 0, 0, &dev_desc, sizeof(dev_desc)) < (int)sizeof(dev_desc)) {
        slog("usbhostd: enumerate get_device_desc_failed\n");
        usb_dev_remove(dev_idx);
        return -1;
    }
    slog("usbhostd: dev=%d vid=%04x pid=%04x class=%02x mps0=%u speed=%d depth=%u\n",
            dev_idx, dev_desc.idVendor, dev_desc.idProduct,
            dev_desc.bDeviceClass, dev_desc.bMaxPacketSize0, speed, depth);

    if (usb_get_descriptor(dev->hdev, USB_REQTYPE_STD_IN,
            USB_DESC_CONFIG, 0, 0, cfg_head, sizeof(cfg_head)) < (int)sizeof(cfg_head)) {
        slog("usbhostd: enumerate get_config_head_failed\n");
        usb_dev_remove(dev_idx);
        return -1;
    }
    total_len = le16(cfg_head + 2);
    if (total_len < sizeof(usb_config_desc_t) || total_len > 1024) {
        slog("usbhostd: enumerate invalid_config_len total=%u\n", total_len);
        usb_dev_remove(dev_idx);
        return -1;
    }

    cfg_buf = (uint8_t*)malloc(total_len);
    if (cfg_buf == NULL) {
        usb_dev_remove(dev_idx);
        return -1;
    }
    if (usb_get_descriptor(dev->hdev, USB_REQTYPE_STD_IN,
            USB_DESC_CONFIG, 0, 0, cfg_buf, total_len) < total_len) {
        slog("usbhostd: enumerate get_config_failed total=%u\n", total_len);
        free(cfg_buf);
        usb_dev_remove(dev_idx);
        return -1;
    }

    if (dev_desc.bDeviceClass == USB_CLASS_HUB) {
        free(cfg_buf);
        if (depth >= USB_MAX_HUB_DEPTH) {
            slog("usbhostd: hub too deep depth=%u\n", depth);
            usb_dev_remove(dev_idx);
            return 0;
        }
        if (usb_set_configuration(dev->hdev, ((usb_config_desc_t*)cfg_head)->bConfigurationValue) < 0) {
            slog("usbhostd: hub set_config_failed\n");
            usb_dev_remove(dev_idx);
            return -1;
        }
        return usb_enumerate_hub(dev_idx);
    }

    if (usb_set_configuration(dev->hdev, ((usb_config_desc_t*)cfg_buf)->bConfigurationValue) < 0) {
        slog("usbhostd: enumerate set_config_failed\n");
        free(cfg_buf);
        usb_dev_remove(dev_idx);
        return -1;
    }

    /* give the bsp layer a chance to claim a bulk-only mass-storage
       interface before the HID walk (platforms without bulk support
       return -1 and the device falls through to HID/unsupported) */
    if (bsp_usb_msc_probe(dev->hdev, cfg_buf, total_len) == 0) {
        slog("usbhostd: msc claimed dev=%d\n", dev_idx);
        registered++;
    }

    memset(candidates, 0, sizeof(candidates));
    cand_count = usb_parse_candidates(cfg_buf, total_len, candidates, USB_MAX_CANDIDATES);
    free(cfg_buf);

    for (int i = 0; i < cand_count; ++i) {
        uint8_t* report_desc = NULL;
        bool desc_ok = false;
        bool mouse_desc_ok = false;
        bool touch_desc_ok = false;
        uint8_t kbd_id = 0, mouse_id = 0;
        hid_dev_type_t dev_type = HID_DEV_TYPE_UNKNOWN;
        mouse_parser_t mouse_probe;
        touch_parser_t touch_probe;

        if (!candidates[i].valid) {
            continue;
        }
        if (candidates[i].report_desc_len > 0 && candidates[i].report_desc_len <= 1024) {
            report_desc = (uint8_t*)malloc(candidates[i].report_desc_len);
            if (report_desc != NULL &&
                    usb_get_descriptor(dev->hdev, USB_REQTYPE_STD_IFACE_IN,
                        USB_DESC_HID_REPORT, 0, candidates[i].iface_num, report_desc,
                        candidates[i].report_desc_len) >= candidates[i].report_desc_len) {
                desc_ok = true;
            }
        }
        if (desc_ok) {
            mouse_desc_ok = hid_probe_mouse_report(report_desc,
                    candidates[i].report_desc_len, &mouse_probe);
            touch_desc_ok = hid_probe_touch_report(report_desc,
                    candidates[i].report_desc_len, &touch_probe);
            dev_type = hid_detect_device_type(report_desc, candidates[i].report_desc_len);
            if (!mouse_desc_ok && !touch_desc_ok && dev_type == HID_DEV_TYPE_UNKNOWN) {
                /* nothing recognized: the control read may have returned
                   garbled data of the right length -- refetch once */
                if (usb_get_descriptor(dev->hdev, USB_REQTYPE_STD_IFACE_IN,
                        USB_DESC_HID_REPORT, 0, candidates[i].iface_num, report_desc,
                        candidates[i].report_desc_len) >= candidates[i].report_desc_len) {
                    mouse_desc_ok = hid_probe_mouse_report(report_desc,
                            candidates[i].report_desc_len, &mouse_probe);
                    touch_desc_ok = hid_probe_touch_report(report_desc,
                            candidates[i].report_desc_len, &touch_probe);
                    dev_type = hid_detect_device_type(report_desc, candidates[i].report_desc_len);
                }
            }
            if (mouse_desc_ok && dev_type == HID_DEV_TYPE_UNKNOWN) {
                dev_type = HID_DEV_TYPE_MOUSE;
            }
            if (touch_desc_ok) {
                dev_type = HID_DEV_TYPE_TOUCH;
            }
        }

        /* composite: boot-keyboard interface whose report descriptor
           actually multiplexes kbd+mouse via report IDs */
        if (desc_ok && hid_parse_report_ids(report_desc,
                candidates[i].report_desc_len, &kbd_id, &mouse_id) == 0) {
            if (usb_register_composite(dev_idx, &candidates[i], kbd_id, mouse_id,
                    mouse_desc_ok ? &mouse_probe : NULL) == 0) {
                registered++;
            }
        }
        else if (dev_type == HID_DEV_TYPE_TOUCH && touch_desc_ok) {
            if (usb_register_touch(dev_idx, &candidates[i], &touch_probe) == 0) {
                registered++;
            }
        }
        else if (dev_type == HID_DEV_TYPE_KEYBOARD ||
                (candidates[i].subclass == USB_SUBCLASS_BOOT &&
                 candidates[i].protocol == USB_PROTOCOL_KEYBOARD)) {
            /* a keyboard collection may multiplex extra reports (e.g.
               consumer/media) on the same endpoint via report IDs even
               without a mouse collection; plain boot layouts have ID 0 */
            uint8_t kbd_rid = desc_ok ?
                    hid_find_kbd_report_id(report_desc,
                            candidates[i].report_desc_len) : 0;
            if (usb_register_keyboard(dev_idx, &candidates[i], kbd_rid) == 0) {
                registered++;
            }
        }
        else if (dev_type == HID_DEV_TYPE_MOUSE ||
                (candidates[i].subclass == USB_SUBCLASS_BOOT &&
                 candidates[i].protocol == USB_PROTOCOL_MOUSE)) {
            if (usb_register_mouse(dev_idx, &candidates[i],
                    mouse_desc_ok ? &mouse_probe : NULL) == 0) {
                registered++;
            }
        }
        else {
            /* unknown HID type, skip */
        }

        if (report_desc != NULL) {
            free(report_desc);
        }
    }

    if (registered == 0) {
        /*
         * Nothing we can drive on it. Keep the slot: it stays addressed
         * and configured, so it is quiet on the bus and — more
         * importantly — the scan paths see have_dev and stop re-running
         * the whole descriptor walk once a second for every mass-storage
         * stick or printer plugged in. The slot is released for real when
         * the port reports a disconnect.
         */
        dev->unsupported = true;
        return 0;
    }
    return registered;
}

/* reset one hub port and enumerate whatever shows up behind it */
static int usb_hub_attach_port(int dev_idx, uint8_t port) {
    usb_dev_t* dev = &_devs[dev_idx];
    uint16_t status = 0, change = 0;
    int speed;
    bool enabled = false;

    for (int attempt = 1; attempt <= 3; ++attempt) {
        if (usb_hub_port_feature(dev->hdev, port, USB_HUB_FEAT_PORT_RESET, true) < 0) {
            slog("usbhostd: hub dev=%d port=%u reset_req_failed\n", dev_idx, port);
            return -1;
        }
        for (int waited = 0; waited < 20; ++waited) {
            proc_usleep(10000);
            if (usb_hub_port_status(dev->hdev, port, &status, &change) != 0) {
                return -1;
            }
            if ((status & USB_HUB_PS_RESET) == 0 && (status & USB_HUB_PS_ENABLE) != 0) {
                enabled = true;
                break;
            }
        }
        usb_hub_port_feature(dev->hdev, port, USB_HUB_FEAT_C_PORT_RESET, false);
        if (enabled) {
            break;
        }
        proc_usleep(50000);
    }
    if (!enabled) {
        slog("usbhostd: hub dev=%d port=%u reset_failed status=%04x\n",
                dev_idx, port, status);
        return -1;
    }
    proc_usleep(50000);

    if (status & USB_HUB_PS_LOW_SPEED) {
        speed = BSP_USB_SPEED_LOW;
    }
    else if (status & USB_HUB_PS_HIGH_SPEED) {
        speed = BSP_USB_SPEED_HIGH;
    }
    else {
        speed = BSP_USB_SPEED_FULL;
    }
    slog("usbhostd: hub dev=%d port=%u connected speed=%d\n", dev_idx, port, speed);
    return usb_enumerate_device(dev->root_port, speed, dev_idx, port);
}

static int usb_enumerate_hub(int dev_idx) {
    usb_dev_t* dev = &_devs[dev_idx];
    usb_setup_pkt_t setup;
    uint8_t hub_desc[9];
    uint8_t num_ports;
    int registered = 0;

    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = USB_REQTYPE_CLASS_DEV_IN;
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wValue = (uint16_t)(USB_DESC_HUB << 8);
    setup.wLength = sizeof(hub_desc);
    if (bsp_usb_control_xfer(dev->hdev, &setup, hub_desc, true) < (int)sizeof(hub_desc)) {
        slog("usbhostd: hub dev=%d get_hub_desc_failed\n", dev_idx);
        usb_dev_remove(dev_idx);
        return -1;
    }
    num_ports = hub_desc[2];
    if (num_ports > 8) {
        num_ports = 8;
    }
    dev->is_hub = true;
    dev->hub_ports = num_ports;
    slog("usbhostd: hub dev=%d ports=%u depth=%u\n", dev_idx, num_ports, dev->depth);

    /* the controller may need to know about the hub before children attach */
    if (bsp_usb_device_configure_hub(dev->hdev, num_ports) != 0) {
        slog("usbhostd: hub dev=%d configure_hub_failed\n", dev_idx);
        usb_dev_remove(dev_idx);
        return -1;
    }

    for (uint8_t port = 1; port <= num_ports; ++port) {
        usb_hub_port_feature(dev->hdev, port, USB_HUB_FEAT_PORT_POWER, true);
    }
    /* bPwrOn2PwrGood is in 2ms units; add margin for slow rails. The hub
       only reports stable connection status once downstream VBUS has
       ramped, so actually waiting here is what lets the first scan pass
       find the devices */
    uint32_t pwr_ms = (uint32_t)hub_desc[5] * 2u + 100u;
    if (pwr_ms > USB_HUB_PWR_WAIT_MAX_MS) {
        pwr_ms = USB_HUB_PWR_WAIT_MAX_MS;
    }
    proc_usleep(pwr_ms * 1000u);

    /* Scan ports until every port is handled, with a bounded grace window
       for devices that debounce slowly after power-good: a device missed
       here has its connection change bit consumed below, so without the
       grace window it would only resurface on a later scan tick */
    uint32_t handled = 0;
    uint64_t grace_deadline = kernel_tic_ms(0) + USB_HUB_CONNECT_GRACE_MS;
    for (;;) {
        bool all_done = true;

        for (uint8_t port = 1; port <= num_ports; ++port) {
            uint16_t status = 0, change = 0;
            int ret;

            if ((handled & (1u << port)) != 0) {
                continue;
            }
            if (usb_hub_port_status(dev->hdev, port, &status, &change) != 0) {
                handled |= (1u << port);
                continue;
            }
            if (change & USB_HUB_PC_CONNECTION) {
                usb_hub_port_feature(dev->hdev, port, USB_HUB_FEAT_C_PORT_CONNECTION, false);
            }
            if ((status & USB_HUB_PS_CONNECTION) == 0) {
                all_done = false;
                continue;
            }
            handled |= (1u << port);
            ret = usb_hub_attach_port(dev_idx, port);
            if (ret > 0) {
                registered += ret;
            }
            else {
                /* bring-up failed: pace the recovery retries in usb_hub_scan */
                dev->port_retry_ms[port] = kernel_tic_ms(0) + USB_HUB_RETRY_INTERVAL_MS;
            }
        }
        if (all_done || kernel_tic_ms(0) >= grace_deadline) {
            break;
        }
        proc_usleep(20000);
    }
    /* the hub device itself stays registered even with no children yet:
       the periodic scan keeps watching its ports */
    return registered;
}

static bool hub_port_has_child(int dev_idx, uint8_t port) {
    for (int i = 0; i < USB_MAX_DEVS; ++i) {
        if (_devs[i].present && _devs[i].parent == dev_idx &&
                _devs[i].parent_port == port) {
            return true;
        }
    }
    return false;
}

/* poll a live hub for connect/disconnect changes on its ports */
static void usb_hub_scan(int dev_idx) {
    usb_dev_t* dev = &_devs[dev_idx];
    uint64_t now = kernel_tic_ms(0);

    for (uint8_t port = 1; port <= dev->hub_ports; ++port) {
        uint16_t status = 0, change = 0;

        if (usb_hub_port_status(dev->hdev, port, &status, &change) != 0) {
            continue;
        }
        if ((change & USB_HUB_PC_CONNECTION) != 0) {
            usb_hub_port_feature(dev->hdev, port, USB_HUB_FEAT_C_PORT_CONNECTION, false);

            /* drop whatever was on this port */
            for (int i = 0; i < USB_MAX_DEVS; ++i) {
                if (_devs[i].present && _devs[i].parent == dev_idx &&
                        _devs[i].parent_port == port) {
                    slog("usbhostd: hub dev=%d port=%u disconnected\n", dev_idx, port);
                    usb_dev_remove_tree(i);
                }
            }
            if (status & USB_HUB_PS_CONNECTION) {
                if (usb_hub_attach_port(dev_idx, port) <= 0) {
                    dev->port_retry_ms[port] = now + USB_HUB_RETRY_INTERVAL_MS;
                }
                else {
                    dev->port_retry_ms[port] = 0;
                }
            }
            continue;
        }
        /* a connected port with no child failed its bring-up earlier (its
           connection change bit was already consumed during enumeration):
           keep retrying at a paced interval instead of leaving the device
           dead until a physical replug */
        if ((status & USB_HUB_PS_CONNECTION) != 0 &&
                !hub_port_has_child(dev_idx, port) &&
                now >= dev->port_retry_ms[port]) {
            dev->port_retry_ms[port] = now + USB_HUB_RETRY_INTERVAL_MS;
            if (usb_hub_attach_port(dev_idx, port) > 0) {
                dev->port_retry_ms[port] = 0;
            }
        }
    }
}

static void usb_enum_failed(void);

/* check the bsp root ports for connect/disconnect */
static void usb_scan_root_ports(void) {
    int port_count = bsp_usb_root_port_count();
    uint32_t changes = bsp_usb_root_port_changes();

    for (int p = 1; p <= port_count; ++p) {
        bool connected = bsp_usb_root_port_connected(p);
        bool have_dev = false;

        for (int i = 0; i < USB_MAX_DEVS; ++i) {
            if (_devs[i].present && _devs[i].parent < 0 &&
                    _devs[i].root_port == p) {
                have_dev = true;
                break;
            }
        }

        if ((changes & (1u << (p - 1))) != 0 && have_dev && !connected) {
            slog("usbhostd: root port=%d disconnected\n", p);
            usb_root_remove(p);
            have_dev = false;
        }
        if (connected && !have_dev) {
            int speed = bsp_usb_root_port_reset(p);
            if (speed < 0) {
                slog("usbhostd: root port=%d reset_failed\n", p);
                usb_enum_failed();
                continue;
            }
            slog("usbhostd: root port=%d connected speed=%d\n", p, speed);
            if (usb_enumerate_device(p, speed, -1, 0) < 0) {
                usb_enum_failed();
            }
            else {
                _enum_fail_streak = 0;
            }
        }
    }
}

/*
 * Root-port bring-up failed this pass. Retry tighter than the normal scan
 * tick -- devices whose firmware needs a moment after power-up come back
 * fast -- backing off stepwise towards the scan interval. After a streak
 * of failures re-initialize the whole controller: some controllers latch a
 * wedged port state (reset succeeds, then no SETUP ever gets answered)
 * that only a bring-up from scratch clears.
 */
static void usb_enum_failed(void) {
    uint64_t now = kernel_tic_ms(0);
    uint32_t retry_ms;

    _enum_fail_streak++;
    retry_ms = 200u + (_enum_fail_streak - 1u) * 100u;
    if (retry_ms > USB_SCAN_INTERVAL_MS) {
        retry_ms = USB_SCAN_INTERVAL_MS;
    }
    if (_enum_fail_streak >= USB_ENUM_FAIL_REINIT_AFTER) {
        _enum_fail_streak = 0;
        if (bsp_usb_reinit() == 0) {
            slog("usbhostd: controller re-init after repeated enumeration failures\n");
            /* the controller forgot every device (all addresses and
               endpoints are gone): drop the policy tables too, stale bsp
               handles must never be polled -- the next scan re-enumerates
               the tree from scratch */
            memset(_devs, 0, sizeof(_devs));
            memset(_inputs, 0, sizeof(_inputs));
            retry_ms = 500u; /* let the controller settle */
        }
    }
    if (now + retry_ms < _next_scan_ms) {
        _next_scan_ms = now + retry_ms;
    }
}

/* ---------------- input polling / dispatch ---------------- */

/*
 * Mouse reports are incremental: unlike keyboard snapshots, a frame byte-
 * identical to the previous one can still carry fresh movement, so the
 * memcmp dedupe used for keyboards is wrong here. The only frame that
 * carries no information at all is "buttons unchanged AND zero movement
 * AND zero wheel". Devices that ignore Set_Idle resend exactly that frame
 * at up to 1000Hz (raspi5 honours the real 1ms bInterval), and without
 * this drop the whole dispatch/wake/drain chain runs flat-out forever on
 * an idle mouse.
 */
static bool mouse_payload_idle(usb_input_dev_t* in, const uint8_t* payload) {
    if (payload[0] != in->last_mouse_btn ||
            payload[1] != 0 || payload[2] != 0 || payload[3] != 0) {
        return false;
    }
    return true;
}

static bool usb_poll_inputs(vdevice_t* dev) {
    uint8_t report[USB_MAX_REPORT];
    uint8_t payload[USB_MAX_EVENT_SIZE];
    bool got = false;
    /* at least one subscriber queue went empty -> non-empty this pass */
    bool any_edge = false;

    /* subscribers are woken directly inside usbhid_dispatch_evt() on the
       empty -> non-empty edge of their own queue; no node broadcast here */
    usbhid_set_node(dev->mnt_info.node);

    for (int i = 0; i < USB_MAX_INPUTS; ++i) {
        usb_input_dev_t* in = &_inputs[i];
        int ret;

        if (!in->present) {
            continue;
        }
        memset(report, 0, sizeof(report));
        ret = bsp_usb_int_in_poll(_devs[in->dev_idx].hdev, in->ep_addr,
                report, sizeof(report));
        if (ret == -2) {
            /* STALL: clear the device-side halt (and let the bsp layer
               resync the controller) or it stalls again forever */
            slog("usbhostd: input slot=%d ep=%02x stalled, clearing halt\n",
                    i, in->ep_addr);
            (void)bsp_usb_ep_clear_halt(_devs[in->dev_idx].hdev, in->ep_addr);
            continue;
        }
        if (ret <= 0) {
            /* 0: no data yet; <0: transient error, the next call retries */
            continue;
        }

        if (in->type == USB_INPUT_KEYBOARD) {
            /* the endpoint may carry reports from other collections
               (consumer/media): keep only the keyboard collection's
               report ID */
            if (in->kbd_report_id != 0 &&
                    (ret < 2 || report[0] != in->kbd_report_id)) {
                continue;
            }
            if ((uint8_t)ret == in->last_len && memcmp(in->last_report, report, ret) == 0) {
                continue;
            }
            memcpy(in->last_report, report, ret);
            in->last_len = (uint8_t)ret;
            memset(payload, 0, sizeof(payload));
            if (in->kbd_report_id != 0) {
                /* strip the report ID: consumers expect the plain body */
                memcpy(payload, report + 1,
                        (ret - 1) > USB_KEYBOARD_EVENT_SIZE ?
                                USB_KEYBOARD_EVENT_SIZE : (ret - 1));
            }
            else {
                memcpy(payload, report,
                        ret > USB_KEYBOARD_EVENT_SIZE ? USB_KEYBOARD_EVENT_SIZE : ret);
            }
            if (usbhid_dispatch_evt(USB_REPORT_ID_KEYBOARD, payload, USB_KEYBOARD_EVENT_SIZE)) {
                any_edge = true;
            }
            got = true;
        }
        else if (in->type == USB_INPUT_MOUSE) {
            memset(payload, 0, sizeof(payload));
            if (mouse_normalize_report(&in->mouse, report, ret, payload) == USB_POINTER_EVENT_SIZE) {
                if (mouse_payload_idle(in, payload)) {
                    continue;
                }
                in->last_mouse_btn = payload[0];
            }
            else {
                memcpy(payload, report, ret > USB_POINTER_EVENT_SIZE ? USB_POINTER_EVENT_SIZE : ret);
            }
            if (usbhid_dispatch_evt(USB_REPORT_ID_MOUSE, payload, USB_POINTER_EVENT_SIZE)) {
                any_edge = true;
            }
            got = true;
        }
        else if (in->type == USB_INPUT_TOUCH) {
            if ((uint8_t)ret == in->last_len && memcmp(in->last_report, report, ret) == 0) {
                continue;
            }
            memcpy(in->last_report, report, ret);
            in->last_len = (uint8_t)ret;
            if (touch_normalize_report(&in->touch, in->report_len, report, ret, payload) ==
                    USB_POINTER_EVENT_SIZE) {
                if (usbhid_dispatch_evt(USB_REPORT_ID_TOUCH, payload, USB_POINTER_EVENT_SIZE)) {
                    any_edge = true;
                }
                got = true;
            }
        }
        else if (in->type == USB_INPUT_COMPOSITE) {
            /* first byte is the HID report ID; strip it and route */
            uint8_t rid = report[0];
            if (ret < 2) {
                continue;
            }
            if (rid == in->kbd_report_id) {
                if ((uint8_t)ret == in->last_len && memcmp(in->last_report, report, ret) == 0) {
                    continue;
                }
                memcpy(in->last_report, report, ret);
                in->last_len = (uint8_t)ret;
                memset(payload, 0, sizeof(payload));
                memcpy(payload, report + 1,
                        (ret - 1) > USB_KEYBOARD_EVENT_SIZE ? USB_KEYBOARD_EVENT_SIZE : (ret - 1));
                if (usbhid_dispatch_evt(USB_REPORT_ID_KEYBOARD, payload, USB_KEYBOARD_EVENT_SIZE)) {
                    any_edge = true;
                }
                got = true;
            }
            else if (rid == in->mouse_report_id) {
                memset(payload, 0, sizeof(payload));
                if (mouse_normalize_report(&in->mouse, report, ret, payload) == USB_POINTER_EVENT_SIZE) {
                    if (mouse_payload_idle(in, payload)) {
                        continue;
                    }
                    in->last_mouse_btn = payload[0];
                }
                else {
                    memcpy(payload, report + 1,
                            (ret - 1) > USB_POINTER_EVENT_SIZE ? USB_POINTER_EVENT_SIZE : (ret - 1));
                }
                if (usbhid_dispatch_evt(USB_REPORT_ID_MOUSE, payload, USB_POINTER_EVENT_SIZE)) {
                    any_edge = true;
                }
                got = true;
            }
            /* other report IDs (gamepad etc.): no consumer yet, drop */
        }
    }

    /*
     * The edge wakes already fired inside usbhid_dispatch_evt(), directed
     * at each subscriber's own proc. Arm the backlog re-assert in case an
     * edge was spent on a consumer's generic token-0 IPC wait before it
     * reached its node block (see usbhid_backlog): while a queue stays
     * undrained, usb_step re-fires the directed wakes at a bounded rate.
     */
    if (any_edge) {
        _next_reassert_ms = kernel_tic_ms(0) + USB_WAKE_REASSERT_MS;
    }
    return got;
}

/* ---------------- main loop ---------------- */

static int usb_dev_cntl(vdevice_t* dev, int from_pid, int cmd,
        proto_t* in, proto_t* out, void* p) {
    (void)p;
    return bsp_usb_msc_cntl(dev, from_pid, cmd, in, out);
}

static int usb_step(vdevice_t* dev, void* p) {
    uint64_t now = kernel_tic_ms(0);
    bool have_inputs = false;
    bool got;
    (void)p;

    if (now >= _next_scan_ms) {
        _next_scan_ms = now + USB_SCAN_INTERVAL_MS;
        usb_scan_root_ports();
        for (int i = 0; i < USB_MAX_DEVS; ++i) {
            if (_devs[i].present && _devs[i].is_hub) {
                usb_hub_scan(i);
            }
        }
    }

    bsp_usb_poll();
    got = usb_poll_inputs(dev);

    /*
     * Re-assert the directed wakes while a subscriber queue is still
     * undrained (see usbhid_backlog): the original edge wake may have been
     * spent on a consumer's generic IPC wait without ever reaching its node
     * block. Paced by USB_WAKE_REASSERT_MS so a wedged consumer costs at
     * most a few dozen syscalls per second instead of a per-report storm.
     */
    if (kernel_tic_ms(0) >= _next_reassert_ms && usbhid_backlog()) {
        usbhid_rewake_backlog();
        _next_reassert_ms = kernel_tic_ms(0) + USB_WAKE_REASSERT_MS;
    }

    for (int i = 0; i < USB_MAX_INPUTS; ++i) {
        if (_inputs[i].present) {
            have_inputs = true;
            break;
        }
    }
    /*
     * The controller schedules the interrupt endpoints (in hardware or in
     * the bsp poll path); this loop only drains completed transfers, so
     * sleeping is cheap. Back off while idle, snap back to the floor as
     * soon as a report arrives.
     */
    if (got) {
        _idle_sleep_us = USB_IDLE_SLEEP_MIN_US;
    }
    else if (_idle_sleep_us < USB_IDLE_SLEEP_MAX_US) {
        _idle_sleep_us <<= 1;
        if (_idle_sleep_us > USB_IDLE_SLEEP_MAX_US) {
            _idle_sleep_us = USB_IDLE_SLEEP_MAX_US;
        }
    }
    proc_usleep(have_inputs ? _idle_sleep_us : USB_NO_INPUT_SLEEP_US);
    return 0;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1] : "/dev/hid0";
    vdevice_t dev;

    /*
     * bsp_usb_init() never fails: on controller bring-up errors it logs
     * and degrades to "no usb", because exiting before device_run() would
     * leave ipcserv spinning in ipc_wait_ready() and hang init.rd forever.
     * With no controller every bsp call is a safe no-op and /dev/hid0
     * stays readable (empty).
     */
    bsp_usb_init();

    memset(_devs, 0, sizeof(_devs));
    memset(_inputs, 0, sizeof(_inputs));

    /* first scan right away so boot-attached devices come up fast */
    usb_scan_root_ports();
    _next_scan_ms = kernel_tic_ms(0) + USB_SCAN_INTERVAL_MS;

    memset(&dev, 0, sizeof(dev));
    strcpy(dev.desc, "usb-hid");
    dev.loop_step = usb_step;
    dev.open = usbhid_vdev_open;
    dev.close = usbhid_vdev_close;
    dev.read = usbhid_vdev_read;
    dev.fcntl = usbhid_vdev_fcntl;
    dev.check_poll_events = usbhid_vdev_check_poll_events;
    /* USBMSC_CMD_* sector I/O for a claimed mass-storage device;
       platforms without MSC always return -1 */
    dev.dev_cntl = usb_dev_cntl;
    return device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444, false);
}
