#define _GNU_SOURCE /* for F_SETSIG */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <pthread.h>
#include <ewoksys/proto.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/vdevice.h>

#include "platform.h"

#include "stack/util.h"
#include "stack/net.h"
#include "stack/ether.h"

#include "stack/ether_tap.h"


#define ETHER_TAP_IRQ (2)
#define ETHER_TAP_DRAIN_BURST 256

/*
 * TX batching: frames are coalesced as [u16 len][frame] entries and pushed to
 * the device with a single write() IPC (parsed by the wlan driver's net_write).
 * A 32KB socket write bursts ~22 TCP segments; without coalescing each segment
 * costs one vfsd+driver IPC round trip, capping throughput near ~700 frames/s.
 * Batch cap stays under the driver's TX watermark band (64 slots on wl0) so an
 * admitted batch is guaranteed to enqueue atomically (single writer + the DPC
 * only drains, so queue depth cannot grow between admission and push).
 */
#define ETHER_TAP_BATCH_MAX_FRAMES 16
#define ETHER_TAP_FRAME_MAX 1514
#define ETHER_TAP_BATCH_BUF_SIZE (ETHER_TAP_BATCH_MAX_FRAMES * (2 + ETHER_TAP_FRAME_MAX))
/*
 * How long to wait for POLLOUT before giving up on a write() and letting TCP
 * retransmit. This must exceed the underlying device's transient TX-backpressure
 * window, not just its typical latency.
 *
 * On raspix/raspi5 the wl0 SDIO driver returns VFS_ERR_RETRY (EAGAIN) while its
 * firmware TX credit window is momentarily exhausted (see brcm.c txctl_ok /
 * "wtx: ... cstall/brk"). Those stalls are backpressure, not loss: the frame is
 * NOT enqueued, so nothing is dropped, and the driver raises VFS_EVT_WR the
 * instant it drains a frame — so this poll returns early whenever real progress
 * happens. The full timeout is only consumed during a genuine no-grant stall.
 *
 * The driver bounds worst-case starvation to 500ms (its credit "breakthrough"),
 * and on-device wtx logs show these stalls almost always resolve well under that
 * (brk=0). A 50ms ceiling was shorter than the stall, so every transient credit
 * stall during an upload turned into a "device transmit failure" + retransmit,
 * and the retransmit re-competed for the same scarce credits — amplifying the
 * stall. 200ms rides the observed stalls while still bounding head-of-line
 * delay for other flows sharing tap->lock, and POLLERR/POLLHUP below still
 * fast-fails a truly dead link.
 */
#define ETHER_TAP_TX_WAIT_MS 200
struct ether_tap {
    char name[IFNAMSIZ];
    int fd;
    unsigned int irq;
    pthread_mutex_t lock;
    uint8_t *batch;      /* coalesced TX frames: [u16 len][frame]... */
    uint8_t batch_single[2 + ETHER_TAP_FRAME_MAX]; /* no-batch fallback */
    size_t batch_len;
    int batch_frames;
};

#define PRIV(x) ((struct ether_tap *)x->priv)

static int
ether_tap_reopen_locked(struct ether_tap *tap)
{
    if (tap->fd >= 0) {
        close(tap->fd);
    }
    tap->fd = open(tap->name, O_RDWR | O_NONBLOCK);
    return tap->fd >= 0 ? 0 : -1;
}

static int
ether_tap_addr(struct net_device *dev) {
    int ret = -1;
    struct ether_tap *tap;
    tap = PRIV(dev);
    proto_t  out;

    while (ret) {
        PF->init(&out);
        ret = dev_cntl (tap->name, 0, NULL, &out);
        if(ret == 0){
            proto_read_to(&out, dev->addr, 6);
            PF->clear(&out);
            return 0;
        }
        PF->clear(&out);
        usleep(1000);
    }
    return -1;
}

static int
ether_tap_open(struct net_device *dev)
{
    struct ether_tap *tap;

    tap = PRIV(dev);
    tap->fd = open(tap->name, O_RDWR | O_NONBLOCK);
    if (tap->fd < 0) {
        slog("open: %s, dev=%s", strerror(errno), dev->name);
        return -1;
    }
 
    if (memcmp(dev->addr, ETHER_ADDR_ANY, ETHER_ADDR_LEN) == 0) {
        if (ether_tap_addr(dev) == -1) {
            errorf("ether_tap_addr() failure, dev=%s", dev->name);
            close(tap->fd);
            return -1;
        }
    }

    pthread_mutex_init(&tap->lock, NULL);
    return 0;
};

static int
ether_tap_close(struct net_device *dev)
{
    close(PRIV(dev)->fd);
    return 0;
}

/*
 * One write() attempt of the current batch content. Returns the number of
 * bytes accepted (0 or more), -1 on a hard fd error (reopen already tried),
 * or NET_DEVICE_TX_AGAIN when the device stayed congested for the whole
 * ETHER_TAP_TX_WAIT_MS window. Caller must hold tap->lock.
 *
 * nowait: make a single write() attempt and report congestion immediately
 * instead of parking on POLLOUT. The receive path needs this -- it flushes
 * the window-update ACK from an IPC worker, and holding tap->lock across a
 * 200ms POLLOUT park would block the main thread's ether_tap_isr() from
 * draining inbound frames, i.e. a download could freeze RX. A batch left
 * behind by a nowait flush simply rides the next flush.
 */
static ssize_t
ether_tap_write_batch_once(struct ether_tap *tap, int nowait)
{
    int ret;
    struct pollfd pfd;

    for (;;) {
        ret = write(tap->fd, tap->batch, tap->batch_len);
        if (ret >= 0) {
            return ret;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN) {
            if (nowait) {
                return NET_DEVICE_TX_AGAIN;
            }
            /*
             * /dev/wl0 uses VFS_ERR_RETRY to signal software-TX congestion.
             * Waiting for POLLOUT preserves backpressure and keeps TCP from
             * treating a temporary queue-full condition as a hard link error.
             */
            pfd.fd = tap->fd;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            if (poll(&pfd, 1, ETHER_TAP_TX_WAIT_MS) > 0 &&
                (pfd.revents & (POLLOUT | POLLERR | POLLHUP | POLLNVAL)) == POLLOUT) {
                continue;
            }
            /*
             * POLLOUT never arrived within the window. A POLLERR/HUP/NVAL means
             * the fd/link went bad -> hard failure (-1). A plain timeout is
             * transient TX-credit backpressure or a not-yet-associated link
             * (typical for early ARP/DHCP/broadcast frames): report TX_AGAIN so
             * net_device_output() logs it silently and the upper layer retries.
             */
            return (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) ?
                  -1 : NET_DEVICE_TX_AGAIN;
        }
        /*
         * Reopen only on hard fd failures (e.g. EBADF). Congestion already
         * went through the POLLOUT wait above; reopening does not create TX
         * room and only helps when the fd itself has gone bad.
         */
        if (ether_tap_reopen_locked(tap) == 0) {
            ret = write(tap->fd, tap->batch, tap->batch_len);
        }
        return ret >= 0 ? ret : -1;
    }
}

/*
 * Flush coalesced frames to the device. Batched content survives transient
 * congestion (TX_AGAIN) and is retried by the next flush; frames are never
 * dropped here. Caller must hold tap->lock.
 */
static int
ether_tap_flush_locked(struct ether_tap *tap, int nowait)
{
    while (tap->batch_len > 0) {
        ssize_t n = ether_tap_write_batch_once(tap, nowait);
        if (n <= 0) {
            return (int)n; /* -1 or NET_DEVICE_TX_AGAIN: batch retained */
        }
        if ((size_t)n < tap->batch_len) {
            memmove(tap->batch, tap->batch + n, tap->batch_len - n);
        }
        tap->batch_len -= n;
    }
    tap->batch_frames = 0;
    return 0;
}

static int
ether_tap_flush(struct net_device *dev)
{
    struct ether_tap *tap = PRIV(dev);
    int ret;

    mutex_lock(&tap->lock);
    ret = (tap->batch_len == 0) ? 0 : ether_tap_flush_locked(tap, 0);
    mutex_unlock(&tap->lock);
    return ret;
}

/*
 * Non-blocking variant for the receive path: push a pending window-update
 * ACK out now, but never park on POLLOUT and never wait for the lock.
 *
 * trylock because the whole point is to avoid queueing behind a blocking
 * writer. If another thread already owns the tap (a bulk writer mid-batch,
 * or the main thread draining RX) the ACK rides that owner's flush or the
 * next intr_step() round -- both are within one cadence period.
 */
static int
ether_tap_tryflush(struct net_device *dev)
{
    struct ether_tap *tap = PRIV(dev);
    int ret;

    if (pthread_mutex_trylock(&tap->lock) != 0)
        return 0;
    ret = (tap->batch_len == 0) ? 0 : ether_tap_flush_locked(tap, 1);
    pthread_mutex_unlock(&tap->lock);
    return ret;
}

static ssize_t
ether_tap_write(struct net_device *dev, const uint8_t *frame, size_t flen)
{
    struct ether_tap *tap = PRIV(dev);
    int ret;
    TRACE();

    if (flen == 0 || flen > ETHER_TAP_FRAME_MAX) {
        return -1;
    }
    if (tap->batch == NULL) {
        /* batching unavailable (alloc failed): frame it on the stack and
           write directly (the device still expects [u16 len][frame]) */
        uint8_t single[2 + ETHER_TAP_FRAME_MAX];
        single[0] = (uint8_t)(flen & 0xff);
        single[1] = (uint8_t)((flen >> 8) & 0xff);
        memcpy(single + 2, frame, flen);
        mutex_lock(&tap->lock);
        memcpy(tap->batch_single, single, 2 + flen);
        tap->batch_len = 2 + flen;
        ret = (int)ether_tap_write_batch_once(tap, 0);
        tap->batch_len = 0;
        mutex_unlock(&tap->lock);
        TRACE();
        return (ret == (ssize_t)(2 + flen)) ? (ssize_t)flen :
               (ret > 0 ? NET_DEVICE_TX_AGAIN : ret);
    }

    mutex_lock(&tap->lock);
    /* Append [u16 len][frame] to the batch */
    tap->batch[tap->batch_len] = (uint8_t)(flen & 0xff);
    tap->batch[tap->batch_len + 1] = (uint8_t)((flen >> 8) & 0xff);
    memcpy(tap->batch + tap->batch_len + 2, frame, flen);
    tap->batch_len += 2 + flen;
    tap->batch_frames++;

    if (tap->batch_frames >= ETHER_TAP_BATCH_MAX_FRAMES) {
        ret = ether_tap_flush_locked(tap, 0);
        mutex_unlock(&tap->lock);
        TRACE();
        /*
         * Report TX_AGAIN on congestion so upper layers back off; the batch
         * content is retained and goes out on the next flush.
         */
        return (ret == 0) ? (ssize_t)flen : ret;
    }
    mutex_unlock(&tap->lock);
    TRACE();
    return (ssize_t)flen;
}

int
ether_tap_transmit(struct net_device *dev, uint16_t type, const uint8_t *buf, size_t len, const void *dst)
{
    return ether_transmit_helper(dev, type, buf, len, dst, ether_tap_write);
}

static ssize_t
ether_tap_read(struct net_device *dev, uint8_t *buf, size_t size)
{
    struct ether_tap *tap = PRIV(dev);
    ssize_t len;
    TRACE();
    mutex_lock(&tap->lock);
    len = read(tap->fd, buf, size);
    if (len < 0 && errno != EAGAIN && errno != EINTR) {
        /*
         * tap_select() is only a hint from the device. Re-checking it here and
         * looping with usleep()/reopen amplified false-positive "pending"
         * reports into a hot fake-ready path. Retry only after hard fd errors.
         */
        if (ether_tap_reopen_locked(tap) == 0) {
            len = read(tap->fd, buf, size);
        }
    }
    mutex_unlock(&tap->lock);
    TRACE();
    return len > 0 ? len : -1;
}

int tap_select(struct net_device *dev){
    struct ether_tap *tap = PRIV(dev);
    proto_t out;
    int ret = -1;
    int pending = 0;

    if (tap->fd < 0) {
        return 0;
    }

    PF->init(&out);
    ret = dev_cntl(tap->name, 1, NULL, &out);
    if (ret == 0) {
        pending = proto_read_int(&out);
    }
    PF->clear(&out);

    if (ret != 0 || pending <= 0) {
        return 0;
    }
    return pending;
}

static int
ether_tap_isr(unsigned int irq, void *id)
{
    struct net_device *dev = (struct net_device *)id;
    int drained = 0;
    int delivered = 0;
    int pending = tap_select(dev);
    int more_work = 0;

    /*
     * Nothing queued: skip the drain attempt entirely. Previously this ran a
     * speculative 32-frame budget even when the driver reported an empty
     * queue, which cost one read plus up to two extra tap_select() IPCs per
     * poll iteration for zero work — significant on raspix where every op is
     * a synchronous round-trip to the wlan driver. Queued frames are never
     * lost by waiting: they stay in the driver queue and are drained on the
     * next poll once tap_select() reports them.
     */
    if (pending <= 0) {
        return -1;
    }

    /*
     * wl0 reports its current software RX queue depth here, so when backlog
     * has already accumulated we should drain that backlog in the same wakeup
     * instead of stopping after a small fixed burst and letting the driver
     * queue stay near saturation. Keep the 256-frame floor for devices that
     * under-report or only return a boolean "ready" hint.
     */
    int budget = pending;
    if (budget < ETHER_TAP_DRAIN_BURST) {
        budget = ETHER_TAP_DRAIN_BURST;
    }

    while (drained < budget) {
        int ret = ether_poll_helper(dev, ether_tap_read);
        if (ret < 0) {
            break;
        }
        drained++;
        if (ret > 0) {
            delivered++;
        }
    }
    more_work = (drained == budget && delivered > 0);
    /*
     * Any delivered frame keeps the fast cadence. A steady scp/ssh stream
     * delivers ~23 frames per 32KB TCP window -- well under the 256-frame
     * burst budget -- so gating the fast path on a FULL burst (the previous
     * more_work-only condition) classified every bulk-transfer round as idle
     * and let intr_step back off to the 50ms deep sleep between windows,
     * capping throughput near 32KB/50ms (~600KB/s). Idle rounds still return
     * -1 (delivered == 0), so the anti-spin backoff is preserved.
     */
    (void)more_work;
    return (delivered > 0) ? 0 : -1;
}

static struct net_device_ops ether_tap_ops = {
    .open = ether_tap_open,
    .close = ether_tap_close,
    .transmit = ether_tap_transmit,
    .flush = ether_tap_flush,
    .tryflush = ether_tap_tryflush,
};

struct net_device *
ether_tap_init(const char *name, const char *addr)
{
    struct net_device *dev;
    struct ether_tap *tap;

    dev = net_device_alloc(ether_setup_helper);
    if (!dev) {
        errorf("net_device_alloc() failure");
        return NULL;
    }
    if (addr) {
        if (ether_addr_pton(addr, dev->addr) == -1) {
            errorf("invalid address, addr=%s", addr);
            return NULL;
        }
    }
    dev->ops = &ether_tap_ops;
    tap = memory_alloc(sizeof(*tap));
    if (!tap) {
        slog("memory_alloc() failure");
        return NULL;
    }
    strncpy(tap->name, name, sizeof(tap->name)-1);
    tap->fd = -1;
    tap->irq = SIGIRQ;
    tap->batch = memory_alloc(ETHER_TAP_BATCH_BUF_SIZE);
    tap->batch_len = 0;
    tap->batch_frames = 0;
    dev->priv = tap;
    dev->next = NULL;
    if (net_device_register(dev) == -1) {
        errorf("net_device_register() failure");
        memory_free(tap);
        return NULL;
    }
    intr_request_irq(tap->irq, ether_tap_isr, NET_IRQ_SHARED, dev->name, dev);
    slog("ethernet device initialized, dev=%s", dev->name);
    return dev;
}
