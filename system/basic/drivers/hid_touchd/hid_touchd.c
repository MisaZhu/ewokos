#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/ipc.h>

#define HID_TOUCH_REPORT_ID 3
#define HID_IDLE_SLEEP_MIN_US 1000u
#define HID_IDLE_SLEEP_MAX_US 4000u
#define TOUCH_CACHE_SIZE 128u

/* retry cadence while /dev/hid0 is not there yet */
#define HID_CONNECT_SLEEP_US 200000u

static int hid = -1;
static const char* _dev_point = "/dev/hid0";
static uint16_t touch_data[TOUCH_CACHE_SIZE][3];
static uint32_t touch_data_read = 0;
static uint32_t touch_data_write = 0;
static uint32_t idle_sleep_us = HID_IDLE_SLEEP_MIN_US;

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
    PF->init(&in)->addi(&in, id);
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
 * registration before init.rd moves on, and blocking here would stall the
 * whole boot on a machine with no touch panel attached.
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
    hid = fd;
    return true;
}

static int touch_loop(vdevice_t* dev, void* p) {
    uint8_t buf[8] = {0};
        bool got = false;
    (void)p;

    if (!hid_connect()) {
        proc_usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    ipc_disable();
    while (true) {
        int ret = read(hid, buf, 7);
        if (ret != 7) {
            break;
        }
                touch_push((uint16_t)buf[0],
                                (uint16_t)buf[1] | ((uint16_t)buf[2] << 8),
                                (uint16_t)buf[3] | ((uint16_t)buf[4] << 8));
                got = true;
    }
    ipc_enable();

        if (touch_has_data()) {
        idle_sleep_us = HID_IDLE_SLEEP_MIN_US;
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    }
        else if (got) {
                idle_sleep_us = HID_IDLE_SLEEP_MIN_US;
        }
    else if (idle_sleep_us < HID_IDLE_SLEEP_MAX_US) {
        idle_sleep_us <<= 1;
        if (idle_sleep_us > HID_IDLE_SLEEP_MAX_US) {
            idle_sleep_us = HID_IDLE_SLEEP_MAX_US;
        }
    }
    /*
     * device_run() does not pace loop_step, so this has to sleep on every
     * pass. It used to skip the sleep whenever the ring still held events,
     * which pinned a core at 100% for as long as a reader was slow to drain
     * it. The floor is well under the endpoint's own bInterval, so nothing
     * is lost by always waiting.
     */
    usleep(idle_sleep_us);
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
    return device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
}
