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
#define ETHER_TAP_TX_WAIT_MS 50
struct ether_tap {
    char name[IFNAMSIZ];
    int fd;
    unsigned int irq;
    pthread_mutex_t lock;
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

static ssize_t
ether_tap_write(struct net_device *dev, const uint8_t *frame, size_t flen)
{
    struct ether_tap *tap = PRIV(dev);
    int ret;
    struct pollfd pfd;
    TRACE();
    mutex_lock(&tap->lock);
    for (;;) {
        ret = write(tap->fd, frame, flen);
        if (ret >= 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN) {
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
            break;
        }
        /*
         * Reopen only on hard fd failures (e.g. EBADF). Congestion already
         * went through the POLLOUT wait above; reopening does not create TX
         * room and only helps when the fd itself has gone bad.
         */
        if (ether_tap_reopen_locked(tap) == 0) {
            ret = write(tap->fd, frame, flen);
        }
        break;
    }
    mutex_unlock(&tap->lock);
    TRACE();
    return ret;
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
     * and let intr_loop back off to the 50ms deep sleep between windows,
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
