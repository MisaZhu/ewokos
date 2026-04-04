#ifndef _VIRTIO_NET_H_
#define _VIRTIO_NET_H_

#include <stdint.h>

#include <arch/virt/virtio.h>

virtio_dev_t virtio_net_get(void);

int virtio_net_init(virtio_dev_t dev);
int virtio_net_read_mac(virtio_dev_t dev, uint8_t mac[6]);
int virtio_net_pending_rx(virtio_dev_t dev);
int virtio_net_read(virtio_dev_t dev, void *buf, uint32_t size);
int virtio_net_write(virtio_dev_t dev, const void *buf, uint32_t size);

#endif
