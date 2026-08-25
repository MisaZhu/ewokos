/*
 * usbhidsrv.c: /dev/hid0 subscriber fan-out shared by every usbhostd.
 *
 * Extracted from the usbhostd implementations: per-fd subscriber list,
 * ring queues, report-ID dispatch and the vdevice callbacks. All state
 * is private to this module; the daemon talks to it only through
 * usbhid_dispatch() and the usbhid_vdev_* callbacks.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>
#include <usb/usb_defs.h>
#include <usb/usbhidsrv.h>

static fd_info_t* _fds = NULL;

const char* usbhid_input_type_name(usb_input_type_t type) {
    switch (type) {
    case USB_INPUT_KEYBOARD:
        return "keyboard";
    case USB_INPUT_MOUSE:
        return "mouse";
    case USB_INPUT_TOUCH:
        return "touch";
    case USB_INPUT_COMPOSITE:
        return "composite";
    default:
        return "unknown";
    }
}

static void queue_init(usb_queue_t* queue) {
    memset(queue, 0, sizeof(*queue));
}

static void queue_clear(usb_queue_t* queue) {
    queue->rd = 0;
    queue->wr = 0;
}

static bool queue_has_data(const usb_queue_t* queue) {
    return queue->rd != queue->wr;
}

static void queue_push(usb_queue_t* queue, const uint8_t* data, uint8_t len) {
    if (len > USB_MAX_EVENT_SIZE) {
        len = USB_MAX_EVENT_SIZE;
    }
    memcpy(queue->data[queue->wr], data, len);
    if (len < USB_MAX_EVENT_SIZE) {
        memset(queue->data[queue->wr] + len, 0, USB_MAX_EVENT_SIZE - len);
    }
    queue->len[queue->wr] = len;
    queue->wr = (uint8_t)((queue->wr + 1u) % USB_QUEUE_DEPTH);
    if (queue->wr == queue->rd) {
        queue->rd = (uint8_t)((queue->rd + 1u) % USB_QUEUE_DEPTH);
    }
}

static int queue_pop(usb_queue_t* queue, void* buf, int size) {
    int len;
    if (!queue_has_data(queue)) {
        return VFS_ERR_RETRY;
    }
    len = queue->len[queue->rd];
    if (len > size) {
        len = size;
    }
    memcpy(buf, queue->data[queue->rd], len);
    queue->rd = (uint8_t)((queue->rd + 1u) % USB_QUEUE_DEPTH);
    return len;
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

void usbhid_dispatch(uint8_t report_id, const uint8_t* data, uint8_t len) {
    fd_info_t* cur = _fds;
    while (cur != NULL) {
        if (cur->report_id == report_id) {
            queue_push(&cur->queue, data, len);
        }
        cur = cur->next;
    }
}

int usbhid_vdev_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
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

int usbhid_vdev_close(vdevice_t* dev, int fd, int from_pid, uint32_t node,
        fsinfo_t* fsinfo, void* p) {
    (void)dev;
    (void)node;
    (void)fsinfo;
    (void)p;
    fd_del(fd, from_pid);
    return 0;
}

int usbhid_vdev_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, int offset, void* p) {
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

int usbhid_vdev_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
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

uint32_t usbhid_vdev_check_poll_events(vdevice_t* dev, int fd, int from_pid,
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
