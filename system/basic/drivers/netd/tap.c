#define _GNU_SOURCE /* for F_SETSIG */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "platform.h"

#include "stack/util.h"
#include "stack/net.h"
#include "stack/ether.h"

#include "stack/ether_tap.h"


#define ETHER_TAP_IRQ (2)

struct ether_tap {
    char name[IFNAMSIZ];
    int fd;
    unsigned int irq;
};

#define PRIV(x) ((struct ether_tap *)x->priv)

static int
ether_tap_addr(struct net_device *dev) {
    return 0;
}

static int
ether_tap_open(struct net_device *dev)
{
    struct ether_tap *tap;

    tap = PRIV(dev);
    tap->fd = open(tap->name, 0);
    if (tap->fd < 0) {
        printf("open: %s, dev=%s", strerror(errno), dev->name);
        return -1;
    }
 
    if (memcmp(dev->addr, ETHER_ADDR_ANY, ETHER_ADDR_LEN) == 0) {
        if (ether_tap_addr(dev) == -1) {
            errorf("ether_tap_addr() failure, dev=%s", dev->name);
            close(tap->fd);
            return -1;
        }
    }
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
    return write(PRIV(dev)->fd, frame, flen);
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

    len = read(PRIV(dev)->fd, buf, size);
    return len>0?len: -1;
}

static int
ether_tap_isr(unsigned int irq, void *id)
{
    struct net_device *dev = (struct net_device *)id;
    ether_poll_helper(dev, ether_tap_read);
    return 0;
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
        printf("memory_alloc() failure");
        return NULL;
    }
    strncpy(tap->name, name, sizeof(tap->name)-1);
    tap->fd = -1;
    tap->irq = ETHER_TAP_IRQ;
    dev->priv = tap;
	dev->next = NULL;
    if (net_device_register(dev) == -1) {
        errorf("net_device_register() failure");
        memory_free(tap);
        return NULL;
    }
    intr_request_irq(tap->irq, ether_tap_isr, NET_IRQ_SHARED, dev->name, dev);
    printf("ethernet device initialized, dev=%s", dev->name);
    return dev;
}
