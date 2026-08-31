#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ewoksys/vfs.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/mmio.h>
#include <ewoksys/kernel_tic.h>
#include <mouse/mouse.h>
#include <fcntl.h>

/*
 * Event queue cap: never more than 8 events sit between this driver and
 * its reader. xmouse consumes at ~330 events/s while this driver produces
 * at most ~200/s (100Hz flush x move+wheel), so the queue normally stays
 * nearly empty; the cap is a hard bound for a wedged/slow reader.
 */
#define CACHE_SIZE (8)

/* retry cadence while /dev/hid0 is not there yet */
#define HID_CONNECT_SLEEP_US 200000u

/*
 * Movement/wheel coalescing cadence: pending deltas are flushed at most
 * once per interval, capping the event rate on /dev/mouse0 at 100Hz even
 * for 1000Hz USB mice. Button DOWN/UP events are NEVER coalesced - they
 * bypass this path and are pushed immediately.
 */
#define MOUSE_FLUSH_MS 10u

static int hid = -1;
static const char* _dev_point = "/dev/hid0";
/* cached fsinfo of the open /dev/hid0 fd: node id and mount pid stay stable
   for the whole mount, so the wait path does not pay a VFS_GET_BY_FD IPC */
static fsinfo_t _hid_info;
/* contiguous event queue: queued events live in [0, mouse_data_count);
   read pops slot 0, push appends, eviction removes a slot in the middle */
static mouse_evt_t mouse_data[CACHE_SIZE];
static int mouse_data_count = 0;
static uint8_t last_btn = 0;
/* coalesced movement/wheel waiting for the next flush tick */
static int pend_dx = 0;
static int pend_dy = 0;
static int pend_wheel = 0;
static uint64_t last_flush_ms = 0;

/*
 * Queue-full eviction policy: button DOWN/UP edges are NEVER dropped.
 * Make room by removing, in priority order, the oldest coalesced movement
 * (button==NONE), then the oldest wheel event; only if the queue is made
 * up entirely of button edges (pathological: reader wedged through 8+
 * clicks) is the incoming event refused instead of evicting a queued one.
 * memmove keeps the surviving events in their original relative order.
 */
static void mouse_push_evt(uint8_t state, uint8_t button, int16_t x, int16_t y) {
    if (mouse_data_count >= CACHE_SIZE) {
        int victim = -1;
        for (int i = 0; i < mouse_data_count; i++) {
            if (mouse_data[i].state == MOUSE_STATE_MOVE &&
                    mouse_data[i].button == MOUSE_BUTTON_NONE) {
                victim = i; /* oldest coalesced movement */
                break;
            }
        }
        if (victim < 0) {
            for (int i = 0; i < mouse_data_count; i++) {
                if (mouse_data[i].state == MOUSE_STATE_MOVE) {
                    victim = i; /* oldest wheel event */
                    break;
                }
            }
        }
        if (victim < 0)
            return; /* nothing but button edges queued: keep them all */
        memmove(&mouse_data[victim], &mouse_data[victim + 1],
                (size_t)(mouse_data_count - victim - 1) * sizeof(mouse_evt_t));
        mouse_data_count--;
    }
    mouse_evt_t* evt = &mouse_data[mouse_data_count++];
    memset(evt, 0, sizeof(mouse_evt_t));
    evt->type = MOUSE_TYPE_REL;
    evt->state = state;
    evt->button = button;
    evt->x = x;
    evt->y = y;
}

static uint8_t hid_btn_to_mouse(uint8_t mask) {
    if (mask & 0x01)
        return MOUSE_BUTTON_LEFT;
    if (mask & 0x02)
        return MOUSE_BUTTON_RIGHT;
    if (mask & 0x04)
        return MOUSE_BUTTON_MID;
    return MOUSE_BUTTON_NONE;
}

static int16_t clamp_i16(int v) {
    if (v > 32767)
        return 32767;
    if (v < -32768)
        return -32768;
    return (int16_t)v;
}

/*
 * Emit the coalesced movement/wheel as at most two events (one move plus
 * one scroll direction) and restart the flush window. Wheel notches inside
 * one window collapse into a single directional event to stay under the
 * 100Hz cap; physical wheel rates are far below that, so no notch a user
 * can produce is actually lost.
 */
static void mouse_flush_pending(void) {
    if (pend_dx != 0 || pend_dy != 0) {
        mouse_push_evt(MOUSE_STATE_MOVE, MOUSE_BUTTON_NONE,
                clamp_i16(pend_dx), clamp_i16(pend_dy));
    }
    if (pend_wheel > 0)
        mouse_push_evt(MOUSE_STATE_MOVE, MOUSE_BUTTON_SCROLL_UP, 0, 0);
    else if (pend_wheel < 0)
        mouse_push_evt(MOUSE_STATE_MOVE, MOUSE_BUTTON_SCROLL_DOWN, 0, 0);
    pend_dx = 0;
    pend_dy = 0;
    pend_wheel = 0;
    last_flush_ms = kernel_tic_ms(0);
}

static void mouse_handle_report(uint8_t btn, int8_t dx, int8_t dy, int8_t wheel) {
    uint8_t pressed = btn & (uint8_t)~last_btn;
    uint8_t released = last_btn & (uint8_t)~btn;
    last_btn = btn;

    if (pressed || released) {
        /*
         * Button edges are never coalesced or delayed by the flush cadence:
         * drop any queued movement first (keeps move-then-click ordering),
         * then push the edge immediately.
         */
        mouse_flush_pending();
        if (pressed)
            mouse_push_evt(MOUSE_STATE_DOWN, hid_btn_to_mouse(pressed), dx, dy);
        else
            mouse_push_evt(MOUSE_STATE_UP, hid_btn_to_mouse(released), dx, dy);
    }
    else if (dx != 0 || dy != 0) {
        pend_dx += dx;
        pend_dy += dy;
    }

    if (wheel != 0)
        pend_wheel += wheel;
}

static int _read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, int offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)offset;
    (void)p;
    (void)node;

    if (size < (int)sizeof(mouse_evt_t))
        return -1;

    if (mouse_data_count > 0) {
        memcpy(buf, &mouse_data[0], sizeof(mouse_evt_t));
        memmove(&mouse_data[0], &mouse_data[1],
                (size_t)(mouse_data_count - 1) * sizeof(mouse_evt_t));
        mouse_data_count--;
        return sizeof(mouse_evt_t);
    }
    return VFS_ERR_RETRY;
}

static uint32_t mouse_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)node;
    (void)p;

    if (mouse_data_count > 0) {
        return VFS_EVT_RD;
    }
    return 0;
}

static int set_report_id(int fd, int id) {

    proto_t in;
    PF->init(&in)->addi(&in, id);
    int ret = vfs_fcntl(fd, 0, &in , NULL);
    PF->clear(&in);
    return ret;
}

/*
 * usbhostd only creates /dev/hid0 once the bsp_usb layer has brought up
 * the controller, and a USB mouse may be plugged in (or enumerated) well
 * after boot. This is retried from the loop instead of before device_run(), so
 * /dev/mouse0 gets registered immediately: ipcserv waits for that
 * registration before init.rd moves on, and blocking here would stall the
 * whole boot on a machine with no mouse attached.
 */
static bool hid_connect(void) {
    int fd;
    if (hid >= 0)
        return true;
    /*
     * O_NONBLOCK: this driver waits in hid_wait_report() instead of inside
     * read(). Empty-queue reads must return EAGAIN immediately so the drain
     * loop terminates and the wait path stays in control of when to sleep.
     */
    fd = open(_dev_point, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        return false;
    if (set_report_id(fd, 1) != 0) {
        close(fd);
        return false;
    }
    if (vfs_get_by_fd(fd, &_hid_info) != 0 || _hid_info.node == 0) {
        close(fd);
        return false;
    }
    /*
     * Register on the node's read wait queue once and keep it permanently:
     * hid_wait_report() then needs only proc_block_by(). wait_queue_push
     * dedupes a same-waiter same-node entry, so reconnects cannot pile up.
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
 * with zero IPCs until usbhostd's vfs_wakeup() fires
 * proc_wakeup_by(pid, node) through vfsd. If the wakeup lands while we
 * are still RUNNING (mid-drain, or a spurious wake), the kernel latches
 * the token and proc_block_by() consumes it instead of sleeping - no edge
 * can be lost, and a spurious wake (a keyboard report hitting the shared
 * hid0 node, or an IPC preempting the block) costs one empty read before
 * we sleep again instead of the old register/dev_poll/unblock round trip.
 *
 * Readiness is deliberately NOT checked through poll events: the node's
 * sticky RD bit is set by every vfs_wakeup() and never cleared, and
 * vfs_get_poll_events() falls back to exactly those sticky bits when its
 * own dev_poll IPC fails - trusting them turned the wait loop into a
 * sleepless busy-spin ("stutter, then the mouse dies"). A stale wake only
 * costs one empty read; a missed one cannot happen because the
 * registration never goes away.
 */
static void hid_wait_report(void) {
    proc_block_by(_hid_info.node);
}

static int _loop(vdevice_t* dev, void* p) {
    (void)p;

    if (!hid_connect()) {
        proc_usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    /*
     * Event-driven wait: the kernel parks us (zero IPCs) until usbhostd's
     * vfs_wakeup() fires, so an idle mouse costs nothing instead of the old
     * ~250 blind poll iterations per second. hid_wait_report() verifies
     * readiness through the live per-fd queue only, never the sticky node
     * bits, so a failed IPC can neither busy-spin nor wedge us forever.
     */
    hid_wait_report();

    /*
     * Drain every queued report in one pass: reads are O_NONBLOCK, so the
     * loop stops at the first EAGAIN. Draining keeps cursor latency flat
     * when wakeups coalesce or a burst arrives; a plain one-report-per-pass
     * scheme amplifies backlog into visible lag under load.
     */
    bool failed = false;
    ipc_disable();
    while (true) {
        uint8_t buf[8] = {0};
        int res = read(hid, buf, 7);
        if (res == 7) {
            /* payload: buttons, dx, dy, wheel (deltas are signed) */
            mouse_handle_report(buf[0], (int8_t)buf[1], (int8_t)buf[2], (int8_t)buf[3]);
            continue;
        }
        if (res < 0 && errno != EAGAIN) {
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
     * Flush coalesced movement/wheel at most once per MOUSE_FLUSH_MS so the
     * event rate on /dev/mouse0 never exceeds 100Hz. Button edges were
     * already pushed during the drain above; reports arriving while we
     * sleep here queue up on /dev/hid0 and are drained (buttons included,
     * none lost) on the next loop pass - a click during the window waits
     * at most one flush interval.
     */
    if (pend_dx != 0 || pend_dy != 0 || pend_wheel != 0) {
        uint64_t now = kernel_tic_ms(0);
        uint64_t next = last_flush_ms + MOUSE_FLUSH_MS;
        if (now < next)
            proc_usleep((uint32_t)((next - now) * 1000u));
        mouse_flush_pending();
    }

    /*
     * Level-triggered wakeup for /dev/mouse0 readers: re-assert VFS_EVT_RD
     * while the queue still has unread events, not only on the edge when a
     * new report arrives, so a blocked xmouse cannot sleep on data that is
     * already queued for it.
     */
    if(mouse_data_count > 0) {
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    }
    return 0;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1]: "/dev/mouse0";
    if (argc > 2) {
        _dev_point = argv[2];
    }

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "mouse");
    dev.loop_step = _loop;
    dev.read = _read;
    dev.check_poll_events = mouse_check_poll_events;
    device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444, false);
    return 0;
}
