#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/ipc_serv.h>
#include <ewoksys/klog.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/charbuf.h>

static int log_read(vdevice_t* dev,
        int fd,
        int from_pid,
        fsinfo_t* info,
        void* buf,
        int size,
        off_t offset,
        void* p) {

    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)offset;
    (void)p;

    uint8_t *data = (uint8_t *)buf;
    charbuf_t* buffer = (charbuf_t*)p;
    int i = 0;
    while(i<size) {
        if(charbuf_pop(buffer, &data[i]) != 0)
            break;
        i++;
    }

    /* Once the buffered log content is drained, report EOF (0) so the read
     * finishes normally like a regular file, instead of returning
     * VFS_ERR_RETRY which makes the reader block waiting for more logs. */
    return i;
}

static bool _log_kmsg = false;

/*
 * Append raw bytes to the log ring buffer and wake any reader blocked on the
 * node. Shared by the file write() path and the CTRL_WRITE dev_cntl path so
 * both feed exactly the same buffer. Returns the number of bytes accepted.
 */
static int log_push(vdevice_t* dev, charbuf_t* buffer, const uint8_t* data, int size) {
    int i = 0;
    while(i<size) {
        if(charbuf_push(buffer, data[i], true) != 0)
            break;
        i++;
    }
    if(i > 0)
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    return i;
}

static int log_write(vdevice_t* dev,
        int fd,
        int from_pid,
        fsinfo_t* info,
        const void* buf,
        int size,
        off_t offset,
        void* p) {

    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)buf;
    (void)offset;
    (void)p;

    charbuf_t* buffer = (charbuf_t*)p;
    uint8_t *data = (uint8_t *)buf;

    if(_log_kmsg) {
        data[size] = '\0';
        kout(data, size);
    }

    return log_push(dev, buffer, data, size);
}

/*
 * Direct-write control path: a client (e.g. core forwarding a proc core-dump
 * event) hands logd a ready-made string without opening /dev/log as a file.
 */
static int log_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
    (void)from_pid;
    (void)ret;

    if(cmd != CTRL_WRITE)
        return -1;

    charbuf_t* buffer = (charbuf_t*)p;
    const char* str = proto_read_str(in);
    if(str == NULL)
        return -1;

    int size = strlen(str);
    if(_log_kmsg && size > 0)
        kout(str, size);

    log_push(dev, buffer, (const uint8_t*)str, size);
    return 0;
}

#define LOG_SIZE_DEFAULT (1024*64)
static uint32_t _log_size = LOG_SIZE_DEFAULT;
static int doargs(int argc, char* argv[]) {
    int c = 0;
    while (c != -1) {
        c = getopt (argc, argv, "b:k");
        if(c == -1)
            break;

        switch (c) {
        case 'b':
            _log_size = atoi(optarg);
            break;
        case 'k':
            _log_kmsg = true;
        default:
            c = -1;
            break;
        }
    }
    return optind;
}

int main(int argc, char** argv) {
    const char* mnt_point = "/dev/log";
    int argind = doargs(argc, argv);
    if(argind < argc)
        mnt_point = argv[argind];

    if(_log_size < LOG_SIZE_DEFAULT)
        _log_size = LOG_SIZE_DEFAULT;

    charbuf_t* _buffer = charbuf_new(_log_size);
    if(_buffer == NULL)
        return -1;

    if(ipc_serv_reg(IPC_SERV_LOG) != 0) {
        klog("reg log ipc_serv error!\n");
        return -1;
    }

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "slog");
    dev.read = log_read;
    dev.write = log_write;
    dev.dev_cntl = log_dev_cntl;
    dev.extra_data = _buffer;

    device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666, false);

    charbuf_free(_buffer);
    return 0;
}
