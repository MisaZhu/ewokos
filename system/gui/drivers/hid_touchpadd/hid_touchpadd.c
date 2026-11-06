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
#include <hid/hid_touchpad.h>

/*
 * hid_touchpadd: Precision Touchpad (PTP) subscriber daemon.
 *
 * PTP digitizer reports arrive on the HID subscriber queue (usbhostd on
 * /dev/usb0, btd on /dev/bt0) under HID_REPORT_ID_TOUCHPAD as fixed-size
 * touchpad_evt_t frames. This daemon drains that queue and publishes the
 * frames verbatim on /dev/touchpad0, so the gesture client (xtouch in -m
 * mode) gets every contact transition - unlike the single-contact
 * /dev/touch0 path, there is NO movement coalescing here, because the
 * gesture engine must see every frame to track fingers by contact id.
 *
 * The fixed HID_TOUCHPAD_EVENT_SIZE stride lets one read drain a whole
 * backlog of frames in a single vfsd/usbhostd round-trip, exactly like the
 * 7-byte mouse/touch events.
 *
 * The daemon follows the same subscriber contract as hid_moused / hid_keybd:
 * one-shot per-fd VFS_BLOCK registration, event-driven proc_block_by() wait,
 * whole-queue batch drain, hard-failure reconnect, level-triggered wakeups,
 * and the optional argv[3] == "bt" upstream (btd's /dev/bt0) with its
 * bounded wait + attach poller. BT mode is wired but inert today: btd does
 * not broadcast touchpad reports yet, so a BT-only machine simply never
 * gets frames - the mode keeps the driver drop-in ready for that change.
 */

#define TP_EVT_SIZE HID_TOUCHPAD_EVENT_SIZE
#define TP_QUEUE_DEPTH 32
#define TP_DRAIN_SIZE (TP_QUEUE_DEPTH * TP_EVT_SIZE)
#define TP_CACHE_SIZE 64u
/* retry cadence while the HID host node is not there yet */
#define HID_CONNECT_SLEEP_US 200000u
/* BT mode: bounded idle wait + attach-poll cadence, same as hid_moused */
#define BT_WAIT_REPORT_US 500000u
#define BT_ATTACH_POLL_MS 2000u

static int hid = -1;
static const char* _dev_point = "/dev/usb0";
static bool _bt_mode = false;
/* cached fsinfo of the open source fd: node id and mount pid stay stable
   for the whole mount, so the wait path does not pay a VFS_GET_BY_FD IPC */
static fsinfo_t _hid_info;

/* fixed-size frame ring served to /dev/touchpad0 readers */
static touchpad_evt_t tp_data[TP_CACHE_SIZE];
static uint32_t tp_data_read = 0;
static uint32_t tp_data_write = 0;

static bool tp_has_data(void) {
        return (tp_data_write - tp_data_read) > 0;
}

static void tp_push(const touchpad_evt_t* evt) {
        if (tp_data_write - tp_data_read >= TP_CACHE_SIZE) {
                tp_data_read++;
        }
        tp_data[tp_data_write % TP_CACHE_SIZE] = *evt;
        tp_data_write++;
}

static int tp_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, off_t offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)node;
    (void)offset;
    (void)p;

        if (!tp_has_data()) {
        return VFS_ERR_RETRY;
    }

    /*
     * Hand out as many whole frames as fit, so a reader sized for the whole
     * ring (or the drain window) gets its backlog in one round-trip.
     */
    int total = 0;
    while (tp_has_data() && total + TP_EVT_SIZE <= size) {
        memcpy((uint8_t*)buf + total,
                &tp_data[tp_data_read % TP_CACHE_SIZE], TP_EVT_SIZE);
        memset(&tp_data[tp_data_read % TP_CACHE_SIZE], 0, TP_EVT_SIZE);
        tp_data_read++;
        total += TP_EVT_SIZE;
    }
    return total;
}

static int set_report_id(int fd, int id) {
    proto_t in;
    PF->init(&in);
    PF->addi(&in, id);
    int ret = vfs_fcntl(fd, 0, &in, NULL);
    PF->clear(&in);
    return ret;
}

static uint32_t tp_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)node;
    (void)p;

        return tp_has_data() ? VFS_EVT_RD : 0;
}

/*
 * usbhostd/btd only create their node once the transport is up, and a
 * touchpad may be enumerated well after boot. This is retried from the loop
 * instead of before device_run(), so /dev/touchpad0 gets registered
 * immediately: ipcserv waits for that registration before init.rd moves on.
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
    if (set_report_id(fd, HID_REPORT_ID_TOUCHPAD) != 0) {
        close(fd);
        return false;
    }
    if (vfs_get_by_fd(fd, &_hid_info) != 0 || _hid_info.node == 0) {
        close(fd);
        return false;
    }
    /*
     * Register on the node's read wait queue once and keep it permanently:
     * tp_wait_report() then needs only proc_block_by(). wait_queue_push
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
 * Wait until the per-fd subscriber queue holds a frame. Same contract as
 * hid_touchd's touch_wait_report: a single proc_block_by() after the one-time
 * waiter registration, with the kernel latching any wake that lands while we
 * are still RUNNING.
 *
 * BT mode (like hid_moused): the wait is bounded - an idle BT peripheral
 * produces no wakeup traffic, so the daemon must wake on its own or the
 * attach poller in tp_loop() would never run.
 */
static void tp_wait_report(void) {
    if (_bt_mode) {
        proc_block_timeout(_hid_info.node, BT_WAIT_REPORT_US);
        return;
    }
    proc_block_by(_hid_info.node);
}

/*
 * BT mode only: walk btd's device list and open a HID session on the first
 * connected device that can carry reports. The class-of-device test is the
 * whole Peripheral major class (0x05 with a non-zero minor), which covers
 * mice, keyboards AND touchpads, exactly like hid_moused/hid_keybd.
 */
static void bt_try_attach_hid(void) {
    char* list = dev_cmd(_dev_point, "devices");
    if (list == NULL)
        return;

    char* save = NULL;
    for (char* line = strtok_r(list, "\n", &save); line != NULL;
            line = strtok_r(NULL, "\n", &save)) {
        char addr[24];
        unsigned int cod = 0;
        int connected = 0;
        int le = 0;

        if (strstr(line, "device ") == NULL)
            continue;
        if (sscanf(line, "%*d: device %23s class=0x%x rssi=%*d connected=%d paired=%*d le=%d",
                addr, &cod, &connected, &le) != 4)
            continue;
        if (!connected)
            continue;
        if (!((((cod >> 8) & 0x1f) == 0x05 && (cod & 0xc0) != 0) || le))
            continue;

        char cmd[40];
        snprintf(cmd, sizeof(cmd), "hid_open %s", addr);
        char* ret = dev_cmd(_dev_point, cmd);
        printf("hid_touchpadd: attach %s: %s\n", addr, ret != NULL ? ret : "no_reply");
        if (ret != NULL)
            free(ret);
        break;
    }
    free(list);
}

static int tp_loop(vdevice_t* dev, void* p) {
    (void)p;

    if (!hid_connect()) {
        usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    /* event-driven wait: the kernel parks us until the host wakes us */
    tp_wait_report();

    bool failed = false;
    uint8_t buf[TP_DRAIN_SIZE];
    ipc_disable();
    while (true) {
        int ret = read(hid, buf, sizeof(buf));
        if (ret >= TP_EVT_SIZE) {
            /* payload per frame: one packed touchpad_evt_t */
            for (int off = 0; off + TP_EVT_SIZE <= ret; off += TP_EVT_SIZE) {
                tp_push((const touchpad_evt_t*)(buf + off));
            }
            if (ret < (int)sizeof(buf)) {
                break; /* short batch: the queue ran dry */
            }
            continue;
        }
        if (ret < 0 && errno != EAGAIN) {
            /* hard failure (node gone / host restarted): reconnect */
            failed = true;
        }
        break;
    }
    ipc_enable();

    if (failed) {
        close(hid);
        hid = -1;
        memset(&_hid_info, 0, sizeof(fsinfo_t));
        usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    /* BT mode: keep the HID session attach attempt going while no session
       is up (a touchpad asleep at boot, or paired after the daemon started,
       connects through this poller; the bounded wait above is what lets it
       run with zero report traffic) */
    if (_bt_mode) {
        static uint64_t last_attach_ms = 0;
        uint64_t now = kernel_tic_ms(0);
        if (now - last_attach_ms >= BT_ATTACH_POLL_MS) {
            last_attach_ms = now;
            char* st = dev_cmd(_dev_point, "hid_state");
            bool session_up = st != NULL && strstr(st, "active=1") != NULL;
            if (st != NULL)
                free(st);
            if (!session_up)
                bt_try_attach_hid();
        }
    }

    /*
     * Level-triggered wakeup for /dev/touchpad0 readers: re-assert VFS_EVT_RD
     * while the ring still holds unread frames, so a blocked reader cannot
     * sleep on data that is already queued for it.
     */
    if (tp_has_data()) {
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    }
    return 0;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1] : "/dev/touchpad0";
    if (argc > 2) {
        _dev_point = argv[2];
    }
    /* argv[3] == "bt": the upstream is btd (e.g. /dev/bt0), run in BT mode */
    if (argc > 3 && strcmp(argv[3], "bt") == 0) {
        _bt_mode = true;
    }

    vdevice_t dev;
    memset(&dev, 0, sizeof(dev));
    strcpy(dev.desc, "touchpad");
    dev.loop_step = tp_loop;
    dev.read = tp_read;
    dev.check_poll_events = tp_check_poll_events;
    return device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444, false);
}
