/*
 * usbhidsrv.h: /dev/hid0 subscriber fan-out shared by every usbhostd.
 *
 * Report-ID based delivery to hid_keybd/hid_moused/hid_touchd: clients
 * open /dev/hid0, subscribe to one report id via fcntl(cmd 0) and read
 * fixed-size events. The queue/fd bookkeeping and the vdevice callbacks
 * are identical on every platform, so they live here; the daemon only
 * supplies its polling/transfer loop.
 */
#ifndef __USBHIDSRV_H__
#define __USBHIDSRV_H__

#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/vdevice.h>
#include <usb/usb_defs.h>

typedef enum {
    USB_INPUT_NONE = 0,
    USB_INPUT_KEYBOARD,
    USB_INPUT_MOUSE,
    USB_INPUT_TOUCH,
    USB_INPUT_COMPOSITE, /* one interrupt EP carrying kbd+mouse via report IDs */
} usb_input_type_t;

typedef struct {
    uint8_t data[USB_QUEUE_DEPTH][USB_MAX_EVENT_SIZE];
    uint8_t len[USB_QUEUE_DEPTH];
    uint8_t rd;
    uint8_t wr;
} usb_queue_t;

typedef struct fd_info {
    int fd;
    int from_pid;
    uint8_t report_id;
    usb_queue_t queue;
    struct fd_info* next;
} fd_info_t;

const char* usbhid_input_type_name(usb_input_type_t type);

/* cache the /dev/hid0 node id for the directed subscriber wakes; the
   daemon calls this once its vdevice is mounted (node id is stable) */
void usbhid_set_node(ewokos_addr_t node);

/* fan out one event to every subscriber of report_id; each subscriber
   whose queue went empty -> non-empty is woken directly with
   proc_wakeup_by(pid, node). The _evt variant returns true when at least
   one such edge wake fired */
bool usbhid_dispatch_evt(uint8_t report_id, const uint8_t* data, uint8_t len);
void usbhid_dispatch(uint8_t report_id, const uint8_t* data, uint8_t len);

/* true while any subscriber queue still holds undrained events; the daemon
   re-asserts its wakes at a bounded rate while this holds (see
   usbhid_backlog in usbhidsrv.c) */
bool usbhid_backlog(void);

/* re-fire the directed wakes for every subscriber with an undrained queue
   (the bounded-rate re-assert); returns true when a wake fired */
bool usbhid_rewake_backlog(void);

/* vdevice callbacks wired straight into the daemon's vdevice_t */
int usbhid_vdev_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        int oflag, void* p);
int usbhid_vdev_close(vdevice_t* dev, int fd, int from_pid, ewokos_addr_t node,
        fsinfo_t* fsinfo, void* p);
int usbhid_vdev_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, int offset, void* p);
int usbhid_vdev_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        int cmd, proto_t* in, proto_t* out, void* p);
uint32_t usbhid_vdev_check_poll_events(vdevice_t* dev, int fd, int from_pid,
        fsinfo_t* node, void* p);

#endif /* __USBHIDSRV_H__ */
