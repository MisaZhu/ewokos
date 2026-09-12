/*
 * hid_srv.c: transport-independent HID subscriber fan-out.
 *
 * The /dev/hid0 (usbhostd) and /dev/bt0 (btd) char devices deliver input
 * reports to their consumers through exactly the same contract: a client
 * opens the node, subscribes to one report id with fcntl(cmd 0) and then
 * reads fixed-size events. So the per-fd subscriber list, ring queues,
 * report-ID dispatch and vdevice callbacks live here, and both daemons
 * wire them straight into their vdevice_t; each daemon only supplies its
 * own transport loop and calls hid_dispatch().
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <hid/hid_defs.h>
#include <hid/hid_srv.h>

static fd_info_t* _fds = NULL;
/* node id of the daemon's own char device, cached after mount. Every
   subscriber parks with proc_block_by(node), so a direct
   proc_wakeup_by(pid, node) reaches exactly its own consumer -- see
   hid_dispatch_evt */
static ewokos_addr_t _node = 0;

void hid_set_node(ewokos_addr_t node) {
    _node = node;
}

const char* hid_input_type_name(hid_input_type_t type) {
    switch (type) {
    case HID_INPUT_KEYBOARD:
        return "keyboard";
    case HID_INPUT_MOUSE:
        return "mouse";
    case HID_INPUT_TOUCH:
        return "touch";
    case HID_INPUT_COMPOSITE:
        return "composite";
    default:
        return "unknown";
    }
}

static void queue_init(hid_queue_t* queue) {
    memset(queue, 0, sizeof(*queue));
}

static void queue_clear(hid_queue_t* queue) {
    queue->rd = 0;
    queue->wr = 0;
}

static bool queue_has_data(const hid_queue_t* queue) {
    return queue->rd != queue->wr;
}

static void queue_push(hid_queue_t* queue, const uint8_t* data, uint8_t len) {
    if (len > HID_MAX_EVENT_SIZE) {
        len = HID_MAX_EVENT_SIZE;
    }
    memcpy(queue->data[queue->wr], data, len);
    if (len < HID_MAX_EVENT_SIZE) {
        memset(queue->data[queue->wr] + len, 0, HID_MAX_EVENT_SIZE - len);
    }
    queue->len[queue->wr] = len;
    queue->wr = (uint8_t)((queue->wr + 1u) % HID_QUEUE_DEPTH);
    if (queue->wr == queue->rd) {
        queue->rd = (uint8_t)((queue->rd + 1u) % HID_QUEUE_DEPTH);
    }
}

/*
 * Pop as many WHOLE queued events as fit into the caller's buffer and
 * return their combined byte count. A consumer draining with a buffer
 * sized for the full queue (HID_QUEUE_DEPTH * event size) empties a
 * whole backlog in one read round-trip instead of one IPC per event.
 * Callers whose buffer fits a single event (the 8-byte keyboard read)
 * keep their one-event-per-read behaviour. An event larger than the
 * buffer is truncated and consumed anyway so the queue can never wedge.
 */
static int queue_pop(hid_queue_t* queue, void* buf, int size) {
    uint8_t* dst = (uint8_t*)buf;
    int total = 0;

    if (!queue_has_data(queue)) {
        return VFS_ERR_RETRY;
    }
    while (queue_has_data(queue)) {
        int len = queue->len[queue->rd];
        if (len > HID_MAX_EVENT_SIZE) {
            len = HID_MAX_EVENT_SIZE;
        }
        if (total + len > size) {
            break;
        }
        memcpy(dst + total, queue->data[queue->rd], len);
        total += len;
        queue->rd = (uint8_t)((queue->rd + 1u) % HID_QUEUE_DEPTH);
    }
    if (total == 0) {
        /* first event alone does not fit: legacy truncated copy */
        int len = queue->len[queue->rd];
        if (len > HID_MAX_EVENT_SIZE) {
            len = HID_MAX_EVENT_SIZE;
        }
        if (len > size) {
            len = size;
        }
        memcpy(dst, queue->data[queue->rd], len);
        queue->rd = (uint8_t)((queue->rd + 1u) % HID_QUEUE_DEPTH);
        total = len;
    }
    return total;
}

static fd_info_t* fd_find(int fd, int from_pid) {
    fd_info_t* cur = _fds;
    while (cur != NULL) {
        if (cur->fd == fd && cur->from_pid == from_pid) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

static void fd_add(fd_info_t* item) {
    fd_info_t** tail = &_fds;
    while (*tail != NULL) {
        tail = &((*tail)->next);
    }
    *tail = item;
    item->next = NULL;
}

static void fd_del(int fd, int from_pid) {
    fd_info_t** cur = &_fds;
    while (*cur != NULL) {
        if ((*cur)->fd == fd && (*cur)->from_pid == from_pid) {
            fd_info_t* old = *cur;
            *cur = old->next;
            free(old);
            return;
        }
        cur = &((*cur)->next);
    }
}

uint8_t hid_srv_report_id(int fd, int from_pid) {
    fd_info_t* info = fd_find(fd, from_pid);
    return info == NULL ? 0 : info->report_id;
}

/*
 * Fan one event out to every subscriber of report_id and wake each
 * subscriber whose queue went EMPTY -> non-empty directly with
 * proc_wakeup_by(pid, node). Waking per report while the consumer still
 * has unread events queued only piles load onto an already-fed reader, so
 * only the edge wakes. The wake is directed at the subscriber's own proc:
 * the old path went through vfs_wakeup(), which forced a synchronous IPC
 * into vfsd per edge and then broadcast-waked EVERY waiter parked on the
 * node -- with a mouse and a keyboard consumer both subscribed, every
 * mouse report cross-woke the keyboard consumer (~100 spurious empty-read
 * round trips per second) and vice versa. Subscribers still register a
 * VFS_BLOCK waiter and block on proc_block_by(node), so the CLOSE/exit
 * recovery path (node-token broadcast from vfsd) keeps working unchanged.
 * Returns true when at least one edge wake fired.
 */
bool hid_dispatch_evt(uint8_t report_id, const uint8_t* data, uint8_t len) {
    fd_info_t* cur = _fds;
    bool woke = false;
    while (cur != NULL) {
        if (cur->report_id == report_id) {
            bool was_empty = !queue_has_data(&cur->queue);
            queue_push(&cur->queue, data, len);
            if (was_empty && _node != 0) {
                proc_wakeup_by(cur->from_pid, _node);
                woke = true;
            }
        }
        cur = cur->next;
    }
    return woke;
}

void hid_dispatch(uint8_t report_id, const uint8_t* data, uint8_t len) {
    (void)hid_dispatch_evt(report_id, data, len);
}

/*
 * True when at least one subscriber still holds undrained events. The
 * daemon re-asserts its wakes at a bounded rate while this holds: the
 * empty->non-empty edge wake can be swallowed by a consumer sitting in a
 * generic token-0 IPC-return wait (the kernel drops a node-token wake
 * against a mismatched block without latching it), and once the queue is
 * non-empty no further edge fires -- without the re-assert the queued
 * reports would sit there forever.
 */
bool hid_backlog(void) {
    fd_info_t* cur = _fds;
    while (cur != NULL) {
        if (queue_has_data(&cur->queue)) {
            return true;
        }
        cur = cur->next;
    }
    return false;
}

/*
 * Directed re-assert of the edge wakes: re-fire proc_wakeup_by() for every
 * subscriber whose queue is still undrained. Same rationale as
 * hid_backlog's comment; unlike the old vfs_wakeup() re-assert this
 * touches only the subscribers that actually still have data, never the
 * whole node wait queue. Returns true when at least one wake fired.
 */
bool hid_rewake_backlog(void) {
    fd_info_t* cur = _fds;
    bool woke = false;
    if (_node == 0) {
        return false;
    }
    while (cur != NULL) {
        if (queue_has_data(&cur->queue)) {
            proc_wakeup_by(cur->from_pid, _node);
            woke = true;
        }
        cur = cur->next;
    }
    return woke;
}

int hid_vdev_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        int oflag, void* p) {
    fd_info_t* info;
    (void)dev;
    (void)node;
    (void)oflag;
    (void)p;
    if (fd < 0) {
        return -1;
    }
    info = (fd_info_t*)calloc(1, sizeof(fd_info_t));
    if (info == NULL) {
        return -1;
    }
    info->fd = fd;
    info->from_pid = from_pid;
    queue_init(&info->queue);
    fd_add(info);
    return 0;
}

int hid_vdev_close(vdevice_t* dev, int fd, int from_pid, ewokos_addr_t node,
        fsinfo_t* fsinfo, void* p) {
    (void)dev;
    (void)node;
    (void)fsinfo;
    (void)p;
    fd_del(fd, from_pid);
    return 0;
}

int hid_vdev_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, off_t offset, void* p) {
    fd_info_t* info;
    (void)dev;
    (void)node;
    (void)offset;
    (void)p;
    info = fd_find(fd, from_pid);
    if (info == NULL) {
        return VFS_ERR_RETRY;
    }
    return queue_pop(&info->queue, buf, size);
}

int hid_vdev_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        int cmd, proto_t* in, proto_t* out, void* p) {
    fd_info_t* item;
    (void)dev;
    (void)info;
    (void)out;
    (void)p;
    item = fd_find(fd, from_pid);
    if (item == NULL) {
        return -1;
    }
    if (cmd == 0) {
        item->report_id = (uint8_t)proto_read_int(in);
        queue_clear(&item->queue);
        return 0;
    }
    return -1;
}

uint32_t hid_vdev_check_poll_events(vdevice_t* dev, int fd, int from_pid,
        fsinfo_t* node, void* p) {
    fd_info_t* info;
    (void)dev;
    (void)node;
    (void)p;

    info = fd_find(fd, from_pid);
    if (info != NULL && queue_has_data(&info->queue)) {
        return VFS_EVT_RD;
    }
    return 0;
}
