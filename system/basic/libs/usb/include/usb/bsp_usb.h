/*
 * bsp_usb.h: platform USB host abstraction.
 *
 * The usbhostd policy layer (enumeration, HID registration, hub class
 * requests, /dev/hid0 fan-out) talks to the hardware only through this
 * interface. The per-machine implementation lives in
 * machines/<hw>/system/libs/bsp/src/bsp_usb.c and rides on the
 * controller driver from libs/arch_<HW> (xHCI on raspi5, DWC2 on
 * raspix). The contract itself is shared: one header for every
 * platform, shipped with libusb.
 *
 * Model:
 *  - root ports are numbered flat and 1-based across all controllers;
 *  - devices are opaque handles owned by the bsp layer;
 *  - everything is polled: bsp_usb_poll() drains controller events,
 *    bsp_usb_int_in_poll() is non-blocking.
 */
#ifndef __BSP_USB_H__
#define __BSP_USB_H__

#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/vdevice.h>
#include <usb/usb_defs.h>

/* device speeds */
#define BSP_USB_SPEED_LOW  0
#define BSP_USB_SPEED_FULL 1
#define BSP_USB_SPEED_HIGH 2

typedef struct bsp_usb_dev bsp_usb_dev_t;

/* bring up the controller(s). Always returns 0: on failure the daemon
   degrades to "no usb" instead of dying (ipcserv would hang otherwise);
   every call below is then a safe no-op */
int  bsp_usb_init(void);
/* full controller re-init after repeated enumeration failures: some
   controllers latch a wedged port state that only a bring-up from
   scratch clears. Drops all device state. Returns 0 on success,
   -1 when the platform does not support it (the policy layer then
   keeps its paced retries). */
int  bsp_usb_reinit(void);
/* drain controller event rings; call from the daemon's main loop */
void bsp_usb_poll(void);

/* root ports, flat 1-based numbering across all controllers */
int  bsp_usb_root_port_count(void);
bool bsp_usb_root_port_connected(int port);
/* reset+enable; returns BSP_USB_SPEED_* or < 0 */
int  bsp_usb_root_port_reset(int port);
/* fetch-and-clear the connect-change latch, bit(port-1) */
uint32_t bsp_usb_root_port_changes(void);

/* device lifecycle. For a root device pass parent_hub == NULL and its
   root port; for a device behind a hub pass the hub handle and the
   1-based hub port. Returns NULL when no device slot is left. */
bsp_usb_dev_t* bsp_usb_device_attach(int root_port, int speed,
        bsp_usb_dev_t* parent_hub, int hub_port);
void bsp_usb_device_detach(bsp_usb_dev_t* dev);
/* update EP0 max packet size after the first 8 descriptor bytes */
int  bsp_usb_device_update_mps0(bsp_usb_dev_t* dev, uint8_t mps0);
/* mark the device as a hub before addressing children behind it */
int  bsp_usb_device_configure_hub(bsp_usb_dev_t* dev, int num_ports);

/* control transfer: returns bytes moved or < 0 */
int  bsp_usb_control_xfer(bsp_usb_dev_t* dev, const usb_setup_pkt_t* setup,
        void* data, bool dir_in);

/* interrupt-IN endpoint: open once, then poll.
   poll returns >0 data bytes, 0 no data yet, -1 transient error,
   -2 endpoint stalled */
int  bsp_usb_int_in_open(bsp_usb_dev_t* dev, uint8_t ep_addr, uint16_t mps,
        uint8_t interval);
int  bsp_usb_int_in_poll(bsp_usb_dev_t* dev, uint8_t ep_addr, void* buf,
        int size);

/* bulk endpoints (mass storage): optional, may return -1 when the
   platform controller driver does not implement them */
int  bsp_usb_bulk_open(bsp_usb_dev_t* dev, uint8_t ep_addr, uint16_t mps);
int  bsp_usb_bulk_xfer(bsp_usb_dev_t* dev, uint8_t ep_addr, void* data,
        int len, bool dir_in);

/* clear the device-side endpoint halt and resync controller state;
   needed after a STALL or the endpoint stays dead until replug */
int  bsp_usb_ep_clear_halt(bsp_usb_dev_t* dev, uint8_t ep_addr);

/* mass storage: platforms whose controller driver implements bulk
   transfers may claim the first bulk-only MSC interface during
   enumeration and serve sector I/O over the device node's dev_cntl.
   Platforms without MSC support stub all four out. */
/* scan a freshly configured device's descriptor set for a bulk-only MSC
   interface and claim it; 0 claimed, -1 nothing for this platform */
int  bsp_usb_msc_probe(bsp_usb_dev_t* dev, const uint8_t* cfg, int cfg_len);
/* device (or its tree) is being removed: tear the claim down */
void bsp_usb_msc_detach(bsp_usb_dev_t* dev);
bool bsp_usb_msc_attached(bsp_usb_dev_t* dev);
/* USBMSC_CMD_* served on the vdevice node for the claimed disk */
int  bsp_usb_msc_cntl(vdevice_t* vdev, int from_pid, int cmd,
        proto_t* in, proto_t* out);

#endif /* __BSP_USB_H__ */
