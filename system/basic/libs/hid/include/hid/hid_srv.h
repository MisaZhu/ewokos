/*
 * hid_srv.h: transport-independent HID subscriber fan-out.
 *
 * Report-ID based delivery to hid_keybd/hid_moused/hid_touchd (and to
 * bt_moused over Bluetooth): clients open the daemon's char device,
 * subscribe to one report id via fcntl(cmd 0) and read fixed-size events.
 * The queue/fd bookkeeping and the vdevice callbacks are identical for
 * every HID host, so they live here; the daemon only supplies its own
 * transport loop and calls hid_dispatch().
 */
#ifndef __HID_SRV_H__
#define __HID_SRV_H__

#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/vdevice.h>
#include <hid/hid_defs.h>

typedef enum {
    HID_INPUT_NONE = 0,
    HID_INPUT_KEYBOARD,
    HID_INPUT_MOUSE,
    HID_INPUT_TOUCH,
    HID_INPUT_COMPOSITE, /* one interrupt source carrying kbd+mouse via report IDs */
} hid_input_type_t;

typedef struct {
    uint8_t data[HID_QUEUE_DEPTH][HID_MAX_EVENT_SIZE];
    uint8_t len[HID_QUEUE_DEPTH];
    uint8_t rd;
    uint8_t wr;
} hid_queue_t;

typedef struct fd_info {
    int fd;
    int from_pid;
    uint8_t report_id;
    hid_queue_t queue;
    struct fd_info* next;
} fd_info_t;

const char* hid_input_type_name(hid_input_type_t type);

/* the report id fd subscribed to, or 0 when it has not subscribed (or is
   unknown). Lets a daemon multiplex its own text/event stream with the
   fixed-size HID events on one node - btd does exactly that on /dev/bt0,
   where report id 0 selects the Bluetooth command/event text stream. */
uint8_t hid_srv_report_id(int fd, int from_pid);

/* cache the daemon's char-device node id for the directed subscriber
   wakes; the daemon calls this once its vdevice is mounted (node id is
   stable for the whole mount) */
void hid_set_node(ewokos_addr_t node);

/* fan out one event to every subscriber of report_id; each subscriber
   whose queue went empty -> non-empty is woken directly with
   proc_wakeup_by(pid, node). The _evt variant returns true when at least
   one such edge wake fired */
bool hid_dispatch_evt(uint8_t report_id, const uint8_t* data, uint8_t len);
void hid_dispatch(uint8_t report_id, const uint8_t* data, uint8_t len);

/* true while any subscriber queue still holds undrained events; the daemon
   re-asserts its wakes at a bounded rate while this holds (see
   hid_backlog in hid_srv.c) */
bool hid_backlog(void);

/* re-fire the directed wakes for every subscriber with an undrained queue
   (the bounded-rate re-assert); returns true when a wake fired */
bool hid_rewake_backlog(void);

/* vdevice callbacks wired straight into the daemon's vdevice_t */
int hid_vdev_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        int oflag, void* p);
int hid_vdev_close(vdevice_t* dev, int fd, int from_pid, ewokos_addr_t node,
        fsinfo_t* fsinfo, void* p);
int hid_vdev_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, off_t offset, void* p);
int hid_vdev_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        int cmd, proto_t* in, proto_t* out, void* p);
uint32_t hid_vdev_check_poll_events(vdevice_t* dev, int fd, int from_pid,
        fsinfo_t* node, void* p);

#endif /* __HID_SRV_H__ */
