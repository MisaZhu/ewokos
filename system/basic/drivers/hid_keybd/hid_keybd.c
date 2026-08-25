#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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

/* exponential idle backoff: poll the hid device fast while reports flow,
   back off to a slow cadence when it stays silent to stop burning CPU */
#define HID_IDLE_SLEEP_MIN_US 3000u
#define HID_IDLE_SLEEP_MAX_US 50000u
/* retry cadence while /dev/hid0 is not there yet */
#define HID_CONNECT_SLEEP_US 200000u

static int hid = -1;
static const char* _dev_point = "/dev/hid0";
static uint32_t _idle_sleep_us = HID_IDLE_SLEEP_MIN_US;

/* current held-key state, refreshed by each HID report snapshot */
static uint8_t _mod = 0;
static uint8_t _keys[MAX_KEY];
static int _key_count = 0;

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
    hid = fd;
    return true;
}

static int loop(vdevice_t* dev, void* p) {
    (void)p;

    if (!hid_connect()) {
        proc_usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    ipc_disable();

    bool got = false;
    while(true) {
        uint8_t buf[HID_KEYBOARD_REPORT_SIZE] = {0};
        int res = read(hid, buf, HID_KEYBOARD_REPORT_SIZE);
        if(res == HID_KEYBOARD_REPORT_SIZE) {
            got = true;
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
        }
        else {
            break;
        }
    }

    ipc_enable();
    /*
     * Level-triggered wakeup: re-assert VFS_EVT_RD whenever keys are
     * still held, not only on the edge when a new report arrives.
     * The libgloss _read() loop clears the sticky RD bit
     * (vfs_clear_poll_events) before vfs_block(), and vfs_block() only
     * prechecks sticky bits — so if the edge was consumed while keys
     * remain held, a blocked reader would sleep forever until the next
     * physical key event.
     */
    if(_key_count > 0) {
        _idle_sleep_us = HID_IDLE_SLEEP_MIN_US;
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    }
    else if(got) {
        _idle_sleep_us = HID_IDLE_SLEEP_MIN_US;
    }
    else if (_idle_sleep_us < HID_IDLE_SLEEP_MAX_US) {
        _idle_sleep_us <<= 1;
        if (_idle_sleep_us > HID_IDLE_SLEEP_MAX_US) {
            _idle_sleep_us = HID_IDLE_SLEEP_MAX_US;
        }
    }
    /*
     * device_run() does not pace loop_step, so this has to sleep on every
     * pass. It used to skip the sleep whenever keys were held, which meant
     * simply holding a key down pinned a core at 100%: _key_count stays
     * non-zero until the release report arrives. The floor is well under
     * the endpoint's own bInterval, so nothing is lost by always waiting.
     */
    proc_usleep(_idle_sleep_us);
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

    device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
    return 0;
}
