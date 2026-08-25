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

/* fan out one event to every subscriber of report_id */
void usbhid_dispatch(uint8_t report_id, const uint8_t* data, uint8_t len);

/* vdevice callbacks wired straight into the daemon's vdevice_t */
int usbhid_vdev_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        int oflag, void* p);
int usbhid_vdev_close(vdevice_t* dev, int fd, int from_pid, uint32_t node,
        fsinfo_t* fsinfo, void* p);
int usbhid_vdev_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, int offset, void* p);
int usbhid_vdev_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        int cmd, proto_t* in, proto_t* out, void* p);
uint32_t usbhid_vdev_check_poll_events(vdevice_t* dev, int fd, int from_pid,
        fsinfo_t* node, void* p);

#endif /* __USBHIDSRV_H__ */
