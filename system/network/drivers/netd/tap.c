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
#include <ewoksys/klog.h>
#include <ewoksys/vdevice.h>

#include "platform.h"

#include "stack/util.h"
#include "stack/net.h"
#include "stack/ether.h"

#include "stack/ether_tap.h"


#define ETHER_TAP_IRQ (2)
#define ETHER_TAP_DRAIN_BURST 256
#define ETHER_TAP_READ_RETRY 4
#define ETHER_TAP_READ_RETRY_US 500

struct ether_tap {
    char name[IFNAMSIZ];
    int fd;
    unsigned int irq;
    pthread_mutex_t lock;
};

#define PRIV(x) ((struct ether_tap *)x->priv)

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
    TRACE();
    mutex_lock(&tap->lock);
    int ret = write(tap->fd, frame, flen);
    if (ret < 0) {
        close(tap->fd);
        tap->fd = open(tap->name, O_RDWR | O_NONBLOCK);
        if (tap->fd >= 0) {
            ret = write(tap->fd, frame, flen);
        }
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
    ssize_t len;
    struct ether_tap *tap = PRIV(dev);
    int pending = 0;
    int attempt;
    TRACE();
    mutex_lock(&tap->lock);
    len = -1;
    for (attempt = 0; attempt < ETHER_TAP_READ_RETRY; attempt++) {
        len = read(tap->fd, buf, size);
        if (len > 0) {
            break;
        }
        pending = tap_select(dev);
        if (pending <= 0) {
            break;
        }
        usleep(ETHER_TAP_READ_RETRY_US);
    }
    if (len <= 0) {
        pending = tap_select(dev);
        if (pending > 0) {
            close(tap->fd);
            tap->fd = open(tap->name, O_RDWR | O_NONBLOCK);
            if (tap->fd >= 0) {
                len = read(tap->fd, buf, size);
            }
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
    int budget = pending > 0 ? pending : 32;

    if (budget > ETHER_TAP_DRAIN_BURST) {
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
    /*
     * Only report activity (which resets the intr_loop to the fast busy
     * polling cadence) when at least one frame was actually delivered to the
     * stack. Frames dropped at the L2/ethertype filter (foreign-unicast or
     * unsupported types, e.g. background broadcast/multicast chatter) are still
     * drained from the device queue but must NOT keep netd spinning at the
     * busy cadence, otherwise idle CPU fluctuates with ambient wire traffic.
     */
    return delivered > 0 ? 0 : -1;
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
