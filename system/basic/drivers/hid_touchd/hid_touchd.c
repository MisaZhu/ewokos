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

#define HID_TOUCH_REPORT_ID 3
#define TOUCH_CACHE_SIZE 128u

/*
 * Movement coalescing cadence: while a finger is in contact, positions are
 * collapsed onto the latest one and flushed at most once per interval,
 * capping the event rate on /dev/touch0 at 100Hz even for high-rate touch
 * panels. Contact DOWN/UP edges are NEVER coalesced - they bypass this
 * path and are pushed immediately.
 */
#define TOUCH_FLUSH_MS 10u

/* retry cadence while /dev/hid0 is not there yet */
#define HID_CONNECT_SLEEP_US 200000u

/*
 * /dev/hid0 subscriber-queue protocol (see libs/usb/usb_defs.h): fixed
 * 7-byte touch events, queue depth 32. One read sized for the whole
 * queue drains an entire backlog in a single vfsd/usbhostd round-trip;
 * the old one-event-per-read drain paid that round-trip per report.
 */
#define HID_EVT_SIZE 7
#define HID_QUEUE_DEPTH 32
#define TOUCH_DRAIN_SIZE (HID_QUEUE_DEPTH * HID_EVT_SIZE)

static int hid = -1;
static const char* _dev_point = "/dev/hid0";
/* cached fsinfo of the open /dev/hid0 fd: node id and mount pid stay stable
   for the whole mount, so the wait path does not pay a VFS_GET_BY_FD IPC */
static fsinfo_t _hid_info;
static uint16_t touch_data[TOUCH_CACHE_SIZE][3];
static uint32_t touch_data_read = 0;
static uint32_t touch_data_write = 0;
/* last contact state and position emitted to the reader, used to spot
   empty (repeated release / stationary) reports */
static uint16_t last_state = 0;
static uint16_t last_x = 0;
static uint16_t last_y = 0;
/* coalesced position waiting for the next flush tick */
static uint16_t pend_x = 0;
static uint16_t pend_y = 0;
static bool pend_valid = false;
static uint64_t last_flush_ms = 0;

static bool touch_has_data(void) {
        return (touch_data_write - touch_data_read) > 0;
}

static void touch_push(uint16_t state, uint16_t x, uint16_t y) {
        if (touch_data_write - touch_data_read >= TOUCH_CACHE_SIZE) {
                touch_data_read++;
        }

        uint16_t* evt = touch_data[touch_data_write % TOUCH_CACHE_SIZE];
        evt[0] = state;
        evt[1] = x;
        evt[2] = y;
        touch_data_write++;
        last_x = x;
        last_y = y;
}

/*
 * Emit the coalesced position as a single move event and restart the flush
 * window. All reports inside one window collapse onto the latest position,
 * which keeps /dev/touch0 under the 100Hz cap.
 */
static void touch_flush_pending(void) {
    if (pend_valid) {
        touch_push(1, pend_x, pend_y);
        pend_valid = false;
    }
    last_flush_ms = kernel_tic_ms(0);
}

/*
 * Contact edges (state 0<->1) are never coalesced or delayed: any pending
 * move is flushed first to keep ordering, then the edge is pushed right
 * away. Moves during contact only update the pending position.
 * Empty reports are dropped here: a release while already released carries
 * no information, and a stationary finger re-reporting the same position
 * adds nothing the reader does not already know.
 */
static void touch_handle_report(uint16_t state, uint16_t x, uint16_t y) {
    if (state == 0) {
        if (last_state == 0)
            return; /* empty: repeated release with no contact in between */
        touch_flush_pending();
        touch_push(0, x, y);
    }
    else if (last_state == 0) {
        touch_flush_pending();
        touch_push(1, x, y);
    }
    else {
        /* next position to be emitted, pending move wins over last push */
        uint16_t cx = pend_valid ? pend_x : last_x;
        uint16_t cy = pend_valid ? pend_y : last_y;
        if (cx == x && cy == y)
            return; /* empty: stationary finger */
        pend_x = x;
        pend_y = y;
        pend_valid = true;
    }
    last_state = state;
}

static int touch_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, int offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)node;
    (void)offset;
    (void)p;

        if (!touch_has_data()) {
        return VFS_ERR_RETRY;
    }
    if (size < 6) {
        return -1;
    }

        memcpy(buf, touch_data[touch_data_read % TOUCH_CACHE_SIZE], 6);
        memset(touch_data[touch_data_read % TOUCH_CACHE_SIZE], 0, sizeof(touch_data[0]));
        touch_data_read++;
    return 6;
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
 * usbhostd only creates /dev/hid0 once the bsp_usb layer has brought up
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
    return true;
}

/*
 * Wait until the per-fd subscriber queue on /dev/hid0 holds a report.
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
        proc_usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    /*
     * Event-driven wait: the kernel parks us (zero IPCs) until usbhostd's
     * dispatch wakes us, so an idle touch panel costs nothing instead of
     * the old blind poll loop that kept reading /dev/hid0 every 1-4ms
     * forever. Touch reports only ever arrive through the subscriber
     * queue, so there is nothing to re-check on a timer.
     */
    touch_wait_report();

    bool failed = false;
    uint8_t buf[TOUCH_DRAIN_SIZE];
    /*
     * Drain every queued report in one pass: reads are O_NONBLOCK, so the
     * loop stops at the first EAGAIN. Each read fetches a whole batch (up
     * to the full queue depth) from usbhidsrv's batched queue_pop, so a
     * backlog costs one vfsd/usbhostd round-trip, not one per report.
     */
    ipc_disable();
    while (true) {
        int ret = read(hid, buf, sizeof(buf));
        if (ret >= HID_EVT_SIZE) {
            /* payload per event: contact state, x, y (little-endian u16) */
            for (int off = 0; off + HID_EVT_SIZE <= ret; off += HID_EVT_SIZE) {
                touch_handle_report((uint16_t)buf[off],
                        (uint16_t)buf[off + 1] | ((uint16_t)buf[off + 2] << 8),
                        (uint16_t)buf[off + 3] | ((uint16_t)buf[off + 4] << 8));
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

    if (failed) {
        close(hid);
        hid = -1;
        memset(&_hid_info, 0, sizeof(fsinfo_t));
        proc_usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    /*
     * Flush the coalesced position at most once per TOUCH_FLUSH_MS so the
     * event rate on /dev/touch0 never exceeds 100Hz. Contact edges were
     * already pushed during the drain above; reports arriving while we
     * sleep here queue up on /dev/hid0 and are drained (edges included,
     * none lost) on the next loop pass - a tap during the window waits at
     * most one flush interval. The edge wake is latched while we are
     * RUNNING, so touch_wait_report() returns at once afterwards.
     */
    if (pend_valid) {
        uint64_t now = kernel_tic_ms(0);
        uint64_t next = last_flush_ms + TOUCH_FLUSH_MS;
        if (now < next)
            proc_usleep((uint32_t)((next - now) * 1000u));
        touch_flush_pending();
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

    vdevice_t dev;
    memset(&dev, 0, sizeof(dev));
    strcpy(dev.desc, "touch");
    dev.loop_step = touch_loop;
    dev.read = touch_read;
    dev.check_poll_events = touch_check_poll_events;
    return device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444, false);
}
