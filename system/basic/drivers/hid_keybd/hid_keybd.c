#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ewoksys/vfs.h>
#include <ewoksys/klog.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/mmio.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/timer.h>
#include <ewoksys/kernel_tic.h>
#include <fcntl.h>
#include <ewoksys/keydef.h>

#define KEY_MOD_LCTRL  0x01
#define KEY_MOD_LSHIFT 0x02
#define KEY_MOD_LALT   0x04
#define KEY_MOD_LMETA  0x08
#define KEY_MOD_RCTRL  0x10
#define KEY_MOD_RSHIFT 0x20
#define KEY_MOD_RALT   0x40
#define KEY_MOD_RMETA  0x80

#define HID_KEYBOARD_REPORT_SIZE 8
#define HID_KEYBOARD_FIRST_KEY_IDX 2
#define MAX_KEY 6 /* standard boot keyboard snapshot: mod, reserved, key[6] */

/* re-check cadence used while keys are held: some keyboards go silent
   between the press and release reports, so an unbounded kernel block
   could stall the level-triggered wakeups of /dev/keyb0 readers */
#define HID_WAIT_FALLBACK_US 20000u
/* retry cadence while /dev/hid0 is not there yet */
#define HID_CONNECT_SLEEP_US 200000u

/*
 * Cap the drain/wakeup pass rate at 100Hz: gaming keyboards report at up
 * to 1000Hz, and without pacing every report would wake the loop for a
 * full drain pass. Pacing is lossless here - every report is a complete
 * state snapshot and the drain keeps only the newest one anyway, so
 * coalescing passes changes nothing the reader could observe. The
 * keys-held repeat stream runs at the slower HID_WAIT_FALLBACK_US cadence
 * and is unaffected.
 */
#define KEYB_PASS_MS 10u

static int hid = -1;
static const char* _dev_point = "/dev/hid0";
/* cached fsinfo of the open /dev/hid0 fd: node id and mount pid stay stable
   for the whole mount, so the wait path does not pay a VFS_GET_BY_FD IPC */
static fsinfo_t _hid_info;

/* current held-key state, refreshed by each HID report snapshot */
static uint8_t _mod = 0;
static uint8_t _keys[MAX_KEY];
static int _key_count = 0;
/* timestamp of the last drain pass, for the KEYB_PASS_MS rate cap */
static uint64_t _last_pass_ms = 0;

const char downMap[] = {  
        ' ',' ',' ',' ','a','b','c','d',    'e','f','g','h','i','j','k','l',
        'm','n','o','p','q','r','s','t',    'u','v','w','x','y','z','1','2',
        '3','4','5','6','7','8','9','0',    '\r','\x1b','\b','\t','\x20', '-', '=', '[', 
        ']', '\\', '$', ';', '\'', '`',',','.',     '/',
    };

const char upMap[] = {
        ' ',' ',' ',' ','A','B','C','D',    'E','F','G','H','I','J','K','L',
        'M','N','O','P','Q','R','S','T',    'U','V','W','X','Y','Z','!','@',
        '#','$','%','^','&','*','(',')',    '\r','\x1b','\b','\t','\x20', '_', '+', '{', 
        '}', '|', '$', ':', '\"', '~','<','>',      '?',
};

static uint8_t do_ctrl(char c) {
    /* Standard ASCII control-code mapping for letters:
       Ctrl+a..Ctrl+z -> 0x01..0x1A (so Ctrl+c -> 0x03 = SIGINT,
       Ctrl+d -> 0x04 = EOF, etc.). Non-letters pass through unchanged. */
    if (c >= 'a' && c <= 'z')
        return (uint8_t)(c - 'a' + 1);
    if (c >= 'A' && c <= 'Z')
        return (uint8_t)(c - 'A' + 1);
    return c;
}

uint8_t getKeyChar(uint8_t alt, uint8_t keycode){
    if(keycode > 0 && keycode < sizeof(upMap)){
        if((alt & KEY_MOD_LCTRL) ||(alt & KEY_MOD_RCTRL)){
            return do_ctrl(downMap[keycode]);
        }
        if((alt & KEY_MOD_LSHIFT) ||(alt & KEY_MOD_RSHIFT)){
            return upMap[keycode];
        }else{
            return downMap[keycode];
        }
    }else if(keycode == 0x4f){
        return KEY_RIGHT;
    }else if(keycode == 0x50){
        return KEY_LEFT;
    }else if(keycode == 0x51){
        return KEY_DOWN;
    }else if(keycode == 0x52){
        return KEY_UP;
    }
    return 0;
}

/* Produce the byte stream for the currently-held snapshot.
 *
 * IMPORTANT: this must return >0 bytes whenever keyb_check_poll_events()
 * reports VFS_EVT_RD, otherwise a blocking reader of /dev/keyb0 enters a
 * vfsd sticky-event busy-spin (vfs_block returns immediately because the
 * RD bit is sticky, but read returns VFS_ERR_RETRY -> EAGAIN -> loop).
 *
 * Aligned with machine.virt keybd: when getKeyChar() cannot map a held
 * HID keycode (e.g. CapsLock, F-keys, Del, Home/End/PgUp/PgDn, media
 * keys, or any keycode >= sizeof(downMap)), fall back to emitting the
 * raw HID keycode so the consumer sees *something* and the read/poll
 * contract stays consistent.
 */
static int get_key_code(char *buf, int size) {
    int num = 0;
    for (int i = 0; i < _key_count && num < size; i++) {
        uint8_t c = getKeyChar(_mod, _keys[i]);
        if (c != 0) {
            buf[num++] = (char)c;
        } else {
            /* unmapped keycode: pass through raw so poll/read stay
               consistent (matches machine.virt keybd behavior) */
            buf[num++] = (char)_keys[i];
        }
    }
    return num;
}

static int keyb_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node, 
        void* buf, int size, int offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)offset;
    (void)p;
    (void)node;

    int num = get_key_code(buf, size);
    return num ? num : VFS_ERR_RETRY;
}

static uint32_t keyb_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)node;
    (void)p;

    return _key_count > 0 ? VFS_EVT_RD : 0;
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
 * the controller, and a USB keyboard may be plugged in (or enumerated) well
 * after boot. This is attempted from the loop instead of before
 * device_run(), so /dev/keyb0 gets registered immediately: ipcserv waits
 * for that registration before init.rd moves on, and blocking here would
 * stall the whole boot on a machine with no keyboard attached.
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
    if (set_report_id(fd, 2) != 0) {
        close(fd);
        return false;
    }
    if (vfs_get_by_fd(fd, &_hid_info) != 0 || _hid_info.node == 0) {
        close(fd);
        return false;
    }
    /*
     * Register on the node's read wait queue once and keep it permanently:
     * hid_wait_report() then needs only proc_block_by()/usleep.
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
 * removed, so when idle the wait is a single proc_block_by(): the kernel
 * parks us with zero IPCs until usbhostd's vfs_wakeup() fires
 * proc_wakeup_by(pid, node) through vfsd. Wakeups landing while we are
 * RUNNING are latched by the kernel and consumed by proc_block_by()
 * instead of sleeping, so no edge can be lost, and a spurious wake (xim's
 * 20ms non-blocking polls of /dev/keyb0 preempt the block through IPC, or
 * a mouse report hits the shared hid0 node) costs one empty read before
 * we sleep again - the old per-pass register/dev_poll/unblock round trip
 * burned ~4 IPCs on every such wake, which is exactly why this driver
 * still showed CPU while the idle mouse did not.
 *
 * Difference from the mouse: while keys are HELD we never block unbounded.
 * Some keyboards send no repeat reports between the press and the release
 * snapshot, and /dev/keyb0 readers rely on this driver's level-triggered
 * wakeups to keep seeing the held-key stream, so the hold case uses a
 * bounded timed block instead: it still keeps the cadence when the keyboard
 * goes silent, but a report edge (release/new press) wakes us early.
 */
static void hid_wait_report(void) {
    if (_key_count > 0) {
        /*
         * Timed block on the hid0 node token instead of proc_usleep():
         * usbhostd's wakeups carry the node token, so a report arriving
         * while keys are held releases the wait early; the deadline keeps
         * the level-triggered cadence when the keyboard sends no repeats.
         * proc_block_timeout() drops the latched generic token-0 wakes
         * xim's /dev/keyb0 poll IPCs leave behind, so they cannot break
         * the pacing.
         */
        proc_block_timeout(_hid_info.node, HID_WAIT_FALLBACK_US);
    }
    else {
        proc_block_by(_hid_info.node);
    }
}

static int loop(vdevice_t* dev, void* p) {
    (void)p;

    if (!hid_connect()) {
        proc_usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    /*
     * Event-driven wait: the kernel parks us (zero IPCs) until usbhostd's
     * vfs_wakeup() fires, so an idle keyboard costs nothing instead of the
     * old endless poll loop with its exponential backoff sleep.
     * hid_wait_report() verifies readiness through the live per-fd queue
     * only, never the sticky node bits, so a failed IPC can neither
     * busy-spin nor wedge us forever; while keys are held it keeps a
     * bounded cadence for the level-triggered wakeups below.
     */
    hid_wait_report();

    /*
     * Rate cap: never run a drain pass sooner than KEYB_PASS_MS after the
     * previous one, so a 1000Hz keyboard cannot drive the loop above 100Hz.
     * Reports arriving during this paced sleep queue up on /dev/hid0 (the
     * edge wakeup is latched, so the next hid_wait_report() returns at
     * once), and since the drain below keeps only the newest full snapshot
     * nothing is lost - the reader always sees the current key state. The
     * keys-held path already sleeps 20ms in hid_wait_report(), so this is
     * a no-op there and only bites on the idle/edge-driven path.
     */
    uint64_t now = kernel_tic_ms(0);
    uint64_t next = _last_pass_ms + KEYB_PASS_MS;
    if (now < next)
        proc_usleep((uint32_t)((next - now) * 1000u));

    /*
     * Drain every queued snapshot in one pass: reads are O_NONBLOCK, so the
     * loop stops at the first EAGAIN. Only the LAST snapshot matters - each
     * report is a full state image, earlier ones in the same burst are stale.
     */
    ipc_disable();
    bool failed = false;
    while(true) {
        uint8_t buf[HID_KEYBOARD_REPORT_SIZE] = {0};
        int res = read(hid, buf, HID_KEYBOARD_REPORT_SIZE);
        if(res == HID_KEYBOARD_REPORT_SIZE) {
            /* each report is a full snapshot: mod, reserved, keycodes */
            uint8_t keys[MAX_KEY];
            int count = 0;
            for (int i = HID_KEYBOARD_FIRST_KEY_IDX; i < HID_KEYBOARD_REPORT_SIZE; i++) {
                if (buf[i] != 0) {
                    keys[count++] = buf[i];
                }
            }
            _mod = buf[0];
            _key_count = count;
            memcpy(_keys, keys, sizeof(keys));
            continue;
        }
        if (res < 0 && errno != EAGAIN) {
            /* hard failure (node gone / usbhostd restarted): reconnect */
            failed = true;
        }
        break;
    }
    ipc_enable();
    _last_pass_ms = kernel_tic_ms(0);

    if (failed) {
        close(hid);
        hid = -1;
        memset(&_hid_info, 0, sizeof(fsinfo_t));
        proc_usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    /*
     * Level-triggered wakeup: re-assert VFS_EVT_RD whenever keys are still
     * held, not only on the edge when a new report arrives. /dev/keyb0
     * readers see held keys as a continuously readable stream
     * (keyb_check_poll_events reports RD while _key_count > 0); if the only
     * edge they were woken by got consumed while keys remain held, they
     * could sleep with data still visible. Re-asserting on every pass where
     * keys are held keeps them fed - at the bounded hid_wait_report()
     * cadence, not a busy loop.
     */
    if(_key_count > 0) {
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    }
    return 0;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1]: "/dev/keyb0";
    if (argc > 2) {
        _dev_point = argv[2];
    }

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "keyb");
    dev.read = keyb_read;
    dev.loop_step = loop;
    dev.check_poll_events = keyb_check_poll_events;

    device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444, false);
    return 0;
}
