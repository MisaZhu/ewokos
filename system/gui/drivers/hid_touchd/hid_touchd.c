#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/kernel_tic.h>
#include <hid/hid_defs.h>
#include <hid/hid_touch.h>
#include <hid/hid_mt.h>
#include <hid/hid_touch_map.h>

#define HID_TOUCH_REPORT_ID 3

/*
 * Movement coalescing cadence: while fingers are in contact, positions are
 * collapsed onto the latest ones and flushed at most once per interval,
 * capping the event rate on /dev/touch0 at 100Hz even for high-rate touch
 * panels. Contact PRESS/RELEASE edges are NEVER coalesced - they bypass
 * this path and are queued immediately.
 */
#define TOUCH_FLUSH_MS 10u

/* retry cadence while /dev/usb0 is not there yet */
#define HID_CONNECT_SLEEP_US 200000u

/*
 * /dev/usb0 subscriber-queue protocol (see libs/hid): fixed-size
 * multi-contact touch frames (hid/hid_touch.h), queue depth 32. One read
 * sized for the whole queue drains an entire backlog in a single
 * vfsd/usbhostd round-trip. Every touch source - MT digitizer, legacy
 * single-contact panel, absolute mouse - arrives in this one format.
 *
 * /dev/touch0 serves the multi-touch edge record stream (hid/hid_mt.h)
 * that xtouch reads from the goodixd/himaxd panels; single-finger mouse
 * compatibility and gestures live in xtouch/xserverd.
 */
#define HID_EVT_SIZE HID_TOUCH_EVENT_SIZE
#define TOUCH_DRAIN_SIZE (HID_QUEUE_DEPTH * HID_EVT_SIZE)

static int hid = -1;
static const char* _dev_point = "/dev/usb0";
/* cached fsinfo of the open /dev/usb0 fd: node id and mount pid stay stable
   for the whole mount, so the wait path does not pay a VFS_GET_BY_FD IPC */
static fsinfo_t _hid_info;

/* wire-slot edge queue the /dev/touch0 readers drain */
static hid_mt_t _mt;
/* device contact id -> wire slot */
static hid_touch_map_t _map;
/* the last report's contact set: press/release edges are spotted against it */
static hid_mt_contact_t _last[HID_MT_MAX_CONTACTS];
/* coalesced contact set waiting for the next flush tick */
static hid_mt_contact_t _pend[HID_MT_MAX_CONTACTS];
static bool _pend_valid = false;
static uint64_t last_flush_ms = 0;
/* MTDBG: drain telemetry exposed via `devcmd /dev/touch0 stat` */
static int _dbg_connected = 0;
static int _dbg_last_ret = -999;
static int _dbg_last_err = 0;
static uint8_t _dbg_b0 = 0;
static int _dbg_reads_total = 0;
static int _dbg_frames_total = 0;
static int _dbg_cc = -1;
/* MTDBG: last frame's first contact + edge/tip telemetry */
static int _dbg_edges_total = 0;
static int _dbg_tip_frames = 0;
static int _dbg_active_frames = 0;
static uint8_t _dbg_cid = 0;
static uint8_t _dbg_tip = 0;
static uint16_t _dbg_x = 0;
static uint16_t _dbg_y = 0;
static uint16_t _dbg_xmax = 0;
static uint16_t _dbg_ymax = 0;

static char* touch_dev_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
    (void)dev; (void)from_pid; (void)p;
    if (argc <= 0 || strcmp(argv[0], "stat") != 0)
        return NULL;
    char* ret = (char*)malloc(256);
    if (ret == NULL)
        return NULL;
    snprintf(ret, 256,
            "conn=%d ret=%d err=%d reads=%d frames=%d cc=%d mt=%u edges=%d tipf=%d actf=%d cid=%u tip=%u x=%u y=%u xmax=%u ymax=%u\n",
            _dbg_connected, _dbg_last_ret, _dbg_last_err,
            _dbg_reads_total, _dbg_frames_total, _dbg_cc, _mt.count,
            _dbg_edges_total, _dbg_tip_frames, _dbg_active_frames,
            _dbg_cid, _dbg_tip, _dbg_x, _dbg_y, _dbg_xmax, _dbg_ymax);
    return ret;
}

static bool touch_has_data(void) {
    return _mt.count != 0;
}

/*
 * Emit the coalesced contact set and restart the flush window. All reports
 * inside one window collapse onto the latest positions, which keeps
 * /dev/touch0 under the 100Hz cap.
 */
static void touch_flush_pending(void) {
    if (_pend_valid) {
        hid_mt_update(&_mt, _pend, NULL);
        _pend_valid = false;
    }
    last_flush_ms = kernel_tic_ms(0);
}

/*
 * One normalized report frame. Contact edges (a slot pressed or released,
 * or handed to another finger) are never coalesced or delayed: any pending
 * move is flushed first to keep ordering, then the edge frame is queued
 * right away. Move-only frames only update the pending set; a frame equal
 * to what is already pending (or last emitted) adds nothing and is dropped.
 */
static void touch_handle_frame(const touch_mt_evt_t* frame) {
    hid_mt_contact_t cur[HID_MT_MAX_CONTACTS];
    bool edge = false;

    hid_touch_map_frame(&_map, frame, cur);
    /* MTDBG: capture first contact + frame-level telemetry */
    _dbg_cid = frame->contacts[0].id;
    _dbg_tip = frame->contacts[0].tip;
    _dbg_x = frame->contacts[0].x;
    _dbg_y = frame->contacts[0].y;
    _dbg_xmax = frame->x_max;
    _dbg_ymax = frame->y_max;
    if (frame->contacts[0].tip) _dbg_tip_frames++;
    if (cur[0].active) _dbg_active_frames++;
    for (unsigned s = 0; s < HID_MT_MAX_CONTACTS; s++) {
        if (cur[s].active != _last[s].active ||
                (cur[s].active && cur[s].fresh))
            edge = true;
    }
    if (edge) _dbg_edges_total++; /* MTDBG */
    if (edge) {
        touch_flush_pending();
        hid_mt_update(&_mt, cur, NULL);
    }
    else {
        /* next positions to be emitted: the pending set wins over the last */
        const hid_mt_contact_t* ref = _pend_valid ? _pend : _last;
        bool moved = false;
        for (unsigned s = 0; s < HID_MT_MAX_CONTACTS; s++) {
            if (cur[s].active &&
                    (cur[s].x != ref[s].x || cur[s].y != ref[s].y))
                moved = true;
        }
        if (moved) {
            memcpy(_pend, cur, sizeof(_pend));
            _pend_valid = true;
        }
    }
    memcpy(_last, cur, sizeof(_last));
}

static int touch_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, off_t offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)node;
    (void)offset;
    (void)p;

    hid_mt_record_t rec;
    if (!touch_has_data()) {
        return VFS_ERR_RETRY;
    }
    if (size < (int)sizeof(rec)) {
        return -1;
    }
    if (!hid_mt_pop(&_mt, &rec)) {
        return VFS_ERR_RETRY;
    }
    memcpy(buf, &rec, sizeof(rec));
    return (int)sizeof(rec);
}

static int set_report_id(int fd, int id) {
    proto_t in;
    PF->init(&in);
    PF->addi(&in, id);
    int ret = vfs_fcntl(fd, 0, &in, NULL);
    PF->clear(&in);
    return ret;
}

static uint32_t touch_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)node;
    (void)p;

    return touch_has_data() ? VFS_EVT_RD : 0;
}

/*
 * usbhostd only creates /dev/usb0 once the bsp_usb layer has brought up
 * the controller, and the touch interface may be enumerated well after
 * boot.
 * This is retried from the loop instead of before device_run(), so
 * /dev/touch0 gets registered immediately: ipcserv waits for that
 * registration before init.rd moves on, and blocking here would stall
 * the whole boot on a machine with no touch panel attached.
 */
static bool hid_connect(void) {
    int fd;
    if (hid >= 0) {
        return true;
    }
    fd = open(_dev_point, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        return false;
    }
    if (set_report_id(fd, HID_TOUCH_REPORT_ID) != 0) {
        close(fd);
        return false;
    }
    if (vfs_get_by_fd(fd, &_hid_info) != 0 || _hid_info.node == 0) {
        close(fd);
        return false;
    }
    /*
     * Register on the node's read wait queue once and keep it permanently:
     * touch_wait_report() then needs only proc_block_by().
     * wait_queue_push dedupes a same-waiter same-node entry, so reconnects
     * cannot pile up.
     */
    proto_t in;
    PF->init(&in)->
        addi(&in, _hid_info.node)->
        addi(&in, VFS_EVT_RD);
    ipc_call(get_vfsd_pid(), VFS_BLOCK, &in, NULL);
    PF->clear(&in);
    hid = fd;
    _dbg_connected = 1; /* MTDBG */
    return true;
}

/*
 * Wait until the per-fd subscriber queue on /dev/usb0 holds a report.
 *
 * The waiter registration is set up once in hid_connect() and never
 * removed, so waiting is a single proc_block_by(): the kernel parks us
 * with zero IPCs until usbhostd's dispatch fires proc_wakeup_by(pid, node)
 * on the empty -> non-empty edge of OUR queue. If the wakeup lands while
 * we are still RUNNING (mid-drain, or a spurious wake), the kernel latches
 * the token and proc_block_by() consumes it instead of sleeping - no edge
 * can be lost, and a spurious wake costs one empty read before we sleep
 * again. Waiting is deliberately not verified through the node's sticky
 * poll bits: a stale wake only costs one empty read, while trusting the
 * sticky bits could busy-spin.
 */
static void touch_wait_report(void) {
    proc_block_by(_hid_info.node);
}

static int touch_loop(vdevice_t* dev, void* p) {
    (void)p;

    if (!hid_connect()) {
        usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    /*
     * Event-driven wait: the kernel parks us (zero IPCs) until usbhostd's
     * dispatch wakes us, so an idle touch panel costs nothing instead of
     * the old blind poll loop that kept reading /dev/usb0 every 1-4ms
     * forever. Touch reports only ever arrive through the subscriber
     * queue, so there is nothing to re-check on a timer.
     */
    touch_wait_report();

    bool failed = false;
    uint8_t buf[TOUCH_DRAIN_SIZE];
    int dbg_reads = 0, dbg_first = 0, dbg_err = 0, dbg_frames = 0, dbg_cc = -1; /* MTDBG */
    uint8_t dbg_b0 = 0; /* MTDBG */
    /*
     * Drain every queued report in one pass: reads are O_NONBLOCK, so the
     * loop stops at the first EAGAIN. Each read fetches a whole batch (up
     * to the full queue depth) from hid_srv's batched queue_pop, so a
     * backlog costs one vfsd/usbhostd round-trip, not one per report.
     * IPC is held off for the drain: touch_read() is a handler that can
     * cut into the loop anywhere, and the edge queue must not be updated
     * and popped inside one another.
     */
    ipc_disable();
    while (true) {
        int ret = read(hid, buf, sizeof(buf));
        if (dbg_reads == 0) { dbg_first = ret; dbg_err = errno; dbg_b0 = (ret > 0) ? buf[0] : 0; } /* MTDBG */
        dbg_reads++; /* MTDBG */
        if (ret >= HID_EVT_SIZE) {
            for (int off = 0; off + HID_EVT_SIZE <= ret; off += HID_EVT_SIZE) {
                touch_mt_evt_t frame;
                memcpy(&frame, buf + off, sizeof(frame));
                dbg_frames++; if (dbg_cc < 0) dbg_cc = frame.contact_count; /* MTDBG */
                touch_handle_frame(&frame);
            }
            if (ret < (int)sizeof(buf)) {
                break; /* short batch: the queue ran dry */
            }
            continue;
        }
        if (ret < 0 && errno != EAGAIN) {
            /* hard failure (node gone / usbhostd restarted): reconnect */
            failed = true;
        }
        break;
    }
    ipc_enable();
    _dbg_last_ret = dbg_first; _dbg_last_err = dbg_err; _dbg_b0 = dbg_b0; /* MTDBG */
    _dbg_reads_total += dbg_reads; _dbg_frames_total += dbg_frames; /* MTDBG */
    if (dbg_cc >= 0) _dbg_cc = dbg_cc; /* MTDBG */

    if (failed) {
        close(hid);
        hid = -1;
        memset(&_hid_info, 0, sizeof(fsinfo_t));
        /* the report source died: lift whatever the readers still hold,
           then forget the device's slot layout along with the connection */
        ipc_disable();
        hid_mt_update(&_mt, NULL, NULL);
        hid_touch_map_init(&_map);
        memset(_last, 0, sizeof(_last));
        _pend_valid = false;
        ipc_enable();
        usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    /*
     * Flush the coalesced positions at most once per TOUCH_FLUSH_MS so the
     * event rate on /dev/touch0 never exceeds 100Hz. Contact edges were
     * already queued during the drain above; reports arriving while we
     * sleep here queue up on /dev/usb0 and are drained (edges included,
     * none lost) on the next loop pass - a tap during the window waits at
     * most one flush interval. The edge wake is latched while we are
     * RUNNING, so touch_wait_report() returns at once afterwards.
     */
    if (_pend_valid) {
        uint64_t now = kernel_tic_ms(0);
        uint64_t next = last_flush_ms + TOUCH_FLUSH_MS;
        if (now < next)
            usleep((uint32_t)((next - now) * 1000u));
        ipc_disable();
        touch_flush_pending();
        ipc_enable();
    }

    /*
     * Level-triggered wakeup for /dev/touch0 readers: re-assert VFS_EVT_RD
     * while the queue still has unread events, not only on the edge when a
     * new report arrives, so a blocked reader cannot sleep on data that is
     * already queued for it.
     */
    if (touch_has_data()) {
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    }
    return 0;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1] : "/dev/touch0";
    if (argc > 2) {
        _dev_point = argv[2];
    }

    hid_mt_init(&_mt);
    hid_touch_map_init(&_map);

    vdevice_t dev;
    memset(&dev, 0, sizeof(dev));
    strcpy(dev.desc, "touch");
    dev.loop_step = touch_loop;
    dev.read = touch_read;
    dev.check_poll_events = touch_check_poll_events;
    dev.cmd = touch_dev_cmd; /* MTDBG */
    return device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444, false);
}