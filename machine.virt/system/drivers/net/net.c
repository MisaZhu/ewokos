#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arch/virt/dma.h>
#include <arch/virt/virtio.h>
#include <arpa/inet.h>
#include <ewoksys/klog.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proto.h>
#include <ewoksys/vdevice.h>

#define VIRTIO_MMIO_DEVICE_FEAT 0x10
#define VIRTIO_MMIO_DEVICE_FEAT_SEL 0x14
#define VIRTIO_MMIO_DRIVER_FEAT 0x20
#define VIRTIO_MMIO_DRIVER_FEAT_SEL 0x24
#define VIRTIO_MMIO_GUEST_PG_SZ 0x28
#define VIRTIO_MMIO_QUEUE_SEL 0x30
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x34
#define VIRTIO_MMIO_QUEUE_NUM 0x38
#define VIRTIO_MMIO_QUEUE_ALIGN 0x3C
#define VIRTIO_MMIO_QUEUE_PFN 0x40
#define VIRTIO_MMIO_QUEUE_READY 0x44
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x50
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x60
#define VIRTIO_MMIO_INTERRUPT_ACK 0x64
#define VIRTIO_MMIO_STATUS 0x70
#define VIRTIO_MMIO_CONFIG 0x100

#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8
#define VIRTIO_STATUS_FAILED 128

#define VIRTIO_NET_F_MAC 5

#define VIRTIO_PAGE_SIZE 4096
#define VIRTIO_QUEUE_SIZE 32
#define VIRTNET_RX_QUEUE 0
#define VIRTNET_TX_QUEUE 1

#define VIRTQ_DESC_F_NEXT (1u << 0)
#define VIRTQ_DESC_F_WRITE (1u << 1)

#define TX_DESC_ID 0
#define VIRTNET_HDR_SIZE 10
#define VIRTNET_FRAME_SIZE_MAX 1514
#define VIRTNET_RX_BUF_SIZE 2048
#define VIRTNET_RX_DESC_COUNT 8

struct virtq_desc
{
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
} __attribute__((packed));

struct virtq_avail
{
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[VIRTIO_QUEUE_SIZE];
} __attribute__((packed));

struct virtq_used_elem
{
	uint32_t id;
	uint32_t len;
} __attribute__((packed));

struct virtq_used
{
	uint16_t flags;
	uint16_t idx;
	struct virtq_used_elem ring[VIRTIO_QUEUE_SIZE];
} __attribute__((packed));

struct virtq_t
{
	struct virtq_desc desc[VIRTIO_QUEUE_SIZE];
	struct virtq_avail avail;
	uint8_t buf0[4096 - (sizeof(struct virtq_desc) * VIRTIO_QUEUE_SIZE +
						 sizeof(struct virtq_avail))];
	struct virtq_used used;
	uint8_t buf1[4096 - sizeof(struct virtq_used)];
};

/* This mirrors the leading fields allocated by arch_virt/src/virtio.c */
typedef struct
{
	uintptr_t base;
	struct virtq_t *virtq;
	uintptr_t phy;
} virtio_common_dev_t;

struct virtio_net_hdr
{
	uint8_t flags;
	uint8_t gso_type;
	uint16_t hdr_len;
	uint16_t gso_size;
	uint16_t csum_start;
	uint16_t csum_offset;
} __attribute__((packed));

static virtio_common_dev_t *_net = NULL;
static struct virtq_t *_rxq = NULL;
static uintptr_t _rxq_phy = 0;

static struct virtq_t *_txq = NULL;
static uintptr_t _txq_phy = 0;
static uint8_t *_rxbuf = NULL;
static uintptr_t _rxbuf_phy = 0;

static uint16_t _rx_used_idx = 0;
static uint16_t _tx_used_idx = 0;
static bool _tx_inflight = false;
static uint8_t _mac[6] = {0};

static uint64_t virtq_phy_addr(struct virtq_t *q, uintptr_t q_phy, const void *ptr)
{
	return q_phy + ((uintptr_t)ptr - (uintptr_t)q);
}

static uint64_t virtio_rxbuf_phy_addr(uint16_t desc_id)
{
	return _rxbuf_phy + ((uint64_t)desc_id * VIRTNET_RX_BUF_SIZE);
}

static uint8_t *virtio_rxbuf_addr(uint16_t desc_id)
{
	return _rxbuf + ((size_t)desc_id * VIRTNET_RX_BUF_SIZE);
}

static void virtio_ack_interrupt(void)
{
	uint32_t status = get32(_net->base + VIRTIO_MMIO_INTERRUPT_STATUS);
	if (status != 0)
	{
		put32(_net->base + VIRTIO_MMIO_INTERRUPT_ACK, status & 0x3);
	}
}

static int virtio_setup_queue(uint16_t queue_idx, struct virtq_t *q, uintptr_t q_phy)
{
	put32(_net->base + VIRTIO_MMIO_QUEUE_SEL, queue_idx);
	uint32_t max_queue_size = get32(_net->base + VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (max_queue_size < VIRTIO_QUEUE_SIZE)
	{
		klog("virtio-net: queue %d too small: %d\n", queue_idx, max_queue_size);
		return -1;
	}

	memset(q, 0, sizeof(struct virtq_t));
	put32(_net->base + VIRTIO_MMIO_QUEUE_NUM, VIRTIO_QUEUE_SIZE);
	put32(_net->base + VIRTIO_MMIO_QUEUE_ALIGN, VIRTIO_PAGE_SIZE);
	put32(_net->base + VIRTIO_MMIO_QUEUE_PFN, q_phy >> 12);
	put32(_net->base + VIRTIO_MMIO_QUEUE_READY, 1);
	return 0;
}

static void virtio_net_submit_rx_desc(uint16_t desc_id)
{
	_rxq->desc[desc_id].addr = virtio_rxbuf_phy_addr(desc_id);
	_rxq->desc[desc_id].len = VIRTNET_RX_BUF_SIZE;
	_rxq->desc[desc_id].flags = VIRTQ_DESC_F_WRITE;
	_rxq->desc[desc_id].next = 0;
	_rxq->avail.ring[_rxq->avail.idx % VIRTIO_QUEUE_SIZE] = desc_id;
	__sync_synchronize();
	_rxq->avail.idx++;
}

static void virtio_net_fill_rx_queue(void)
{
	for (uint16_t i = 0; i < VIRTNET_RX_DESC_COUNT; i++)
	{
		virtio_net_submit_rx_desc(i);
	}
	put32(_net->base + VIRTIO_MMIO_QUEUE_NOTIFY, VIRTNET_RX_QUEUE);
}

static int virtio_net_init(void)
{
	uint32_t device_features;
	uint32_t driver_features = 0;
	uint32_t status;

	_mmio_base = mmio_map();

	_net = (virtio_common_dev_t *)virtio_get(VIRTIO_ID_NET);
	if (_net == NULL)
	{
		klog("virtio-net: no virtio-net device found\n");
		return -1;
	}

	_rxq = _net->virtq;
	_rxq_phy = _net->phy;

	_txq = dma_user_alloc(sizeof(struct virtq_t));
	if (_txq == NULL || (intptr_t)_txq == -1)
	{
		klog("virtio-net: alloc tx queue failed\n");
		return -1;
	}
	_txq_phy = (uintptr_t)dma_user_phy(_txq);

	_rxbuf = dma_user_alloc(VIRTNET_RX_DESC_COUNT * VIRTNET_RX_BUF_SIZE);
	if (_rxbuf == NULL || (intptr_t)_rxbuf == -1)
	{
		klog("virtio-net: alloc rx buffers failed\n");
		return -1;
	}
	_rxbuf_phy = (uintptr_t)dma_user_phy(_rxbuf);
	memset(_rxbuf, 0, VIRTNET_RX_DESC_COUNT * VIRTNET_RX_BUF_SIZE);

	put32(_net->base + VIRTIO_MMIO_STATUS, 0);
	put32(_net->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
	put32(_net->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

	put32(_net->base + VIRTIO_MMIO_DEVICE_FEAT_SEL, 0);
	device_features = get32(_net->base + VIRTIO_MMIO_DEVICE_FEAT);
	if (device_features & (1u << VIRTIO_NET_F_MAC))
	{
		driver_features |= (1u << VIRTIO_NET_F_MAC);
	}

	put32(_net->base + VIRTIO_MMIO_DRIVER_FEAT_SEL, 0);
	put32(_net->base + VIRTIO_MMIO_DRIVER_FEAT, driver_features);
	put32(_net->base + VIRTIO_MMIO_GUEST_PG_SZ, VIRTIO_PAGE_SIZE);

	status = get32(_net->base + VIRTIO_MMIO_STATUS);
	put32(_net->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_FEATURES_OK);
	status = get32(_net->base + VIRTIO_MMIO_STATUS);
	if ((status & VIRTIO_STATUS_FEATURES_OK) == 0)
	{
		klog("virtio-net: feature negotiation failed\n");
		put32(_net->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_FAILED);
		return -1;
	}

	if (virtio_setup_queue(VIRTNET_RX_QUEUE, _rxq, _rxq_phy) != 0)
	{
		put32(_net->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
		return -1;
	}
	if (virtio_setup_queue(VIRTNET_TX_QUEUE, _txq, _txq_phy) != 0)
	{
		put32(_net->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
		return -1;
	}

	virtio_net_fill_rx_queue();

	if (driver_features & (1u << VIRTIO_NET_F_MAC))
	{
		for (int i = 0; i < 6; i++)
		{
			_mac[i] = get8(_net->base + VIRTIO_MMIO_CONFIG + i);
		}
	}

	status = get32(_net->base + VIRTIO_MMIO_STATUS);
	put32(_net->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_DRIVER_OK);

	_rx_used_idx = _rxq->used.idx;
	_tx_used_idx = _txq->used.idx;
	_tx_inflight = false;
	klog("virtio-net: mac=%02x:%02x:%02x:%02x:%02x:%02x rx_desc=%d\n",
		 _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5], VIRTNET_RX_DESC_COUNT);

	return 0;
}

static void virtio_net_reap_tx(void)
{
	if (_tx_inflight && _txq->used.idx != _tx_used_idx)
	{
		_tx_used_idx = _txq->used.idx;
		_tx_inflight = false;
	}
}

static int virtio_net_pending_rx(void)
{
	if (_rxq == NULL)
	{
		return 0;
	}
	__sync_synchronize();
	return (uint16_t)(_rxq->used.idx - _rx_used_idx);
}

static int net_read(int fd, int from_pid, fsinfo_t *node,
					void *buf, int size, int offset, void *p)
{
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)p;

	virtio_ack_interrupt();

	if (virtio_net_pending_rx() <= 0)
	{
		return VFS_ERR_RETRY;
	}

	struct virtq_used_elem *used = &_rxq->used.ring[_rx_used_idx % VIRTIO_QUEUE_SIZE];
	uint16_t desc_id = used->id % VIRTIO_QUEUE_SIZE;
	if (desc_id >= VIRTNET_RX_DESC_COUNT)
	{
		klog("virtio-net: invalid rx desc id=%u used_idx=%u\n", desc_id, _rx_used_idx);
		_rx_used_idx++;
		return VFS_ERR_RETRY;
	}

	uint8_t *frame = virtio_rxbuf_addr(desc_id);
	int payload_len = (int)used->len - VIRTNET_HDR_SIZE;
	int read_len = payload_len;
	if (read_len < 0)
	{
		read_len = 0;
	}
	if (read_len > size)
	{
		read_len = size;
	}
	if (read_len > 0)
	{
		memcpy((uint8_t *)buf + offset, frame + VIRTNET_HDR_SIZE, read_len);
	}

	_rx_used_idx++;

	virtio_net_submit_rx_desc(desc_id);
	put32(_net->base + VIRTIO_MMIO_QUEUE_NOTIFY, VIRTNET_RX_QUEUE);

	return read_len > 0 ? read_len : VFS_ERR_RETRY;
}

static int net_write(int fd, int from_pid, fsinfo_t *node,
					 const void *buf, int size, int offset, void *p)
{
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)p;

	virtio_ack_interrupt();
	virtio_net_reap_tx();
	if (_tx_inflight)
	{
		return VFS_ERR_RETRY;
	}

	int write_len = size;
	if (write_len > VIRTNET_FRAME_SIZE_MAX)
	{
		write_len = VIRTNET_FRAME_SIZE_MAX;
	}
	if (write_len <= 0)
	{
		return 0;
	}

	struct virtio_net_hdr *hdr = (struct virtio_net_hdr *)_txq->buf0;
	memset(hdr, 0, sizeof(struct virtio_net_hdr));
	memcpy(_txq->buf0 + sizeof(struct virtio_net_hdr), (const uint8_t *)buf + offset, write_len);

	_txq->desc[TX_DESC_ID].addr = virtq_phy_addr(_txq, _txq_phy, _txq->buf0);
	_txq->desc[TX_DESC_ID].len = sizeof(struct virtio_net_hdr) + write_len;
	_txq->desc[TX_DESC_ID].flags = 0;
	_txq->desc[TX_DESC_ID].next = 0;

	_txq->avail.ring[_txq->avail.idx % VIRTIO_QUEUE_SIZE] = TX_DESC_ID;
	__sync_synchronize();
	_txq->avail.idx++;
	put32(_net->base + VIRTIO_MMIO_QUEUE_NOTIFY, VIRTNET_TX_QUEUE);

	_tx_inflight = true;
	return write_len;
}

static int net_dcntl(int from_pid, int cmd, proto_t *in, proto_t *ret, void *p)
{
	(void)from_pid;
	(void)in;
	(void)p;

	switch (cmd)
	{
	case 0: /* get mac */
		PF->add(ret, _mac, sizeof(_mac));
		break;
	case 1: /* get pending rx count */
		virtio_ack_interrupt();
		PF->addi(ret, virtio_net_pending_rx());
		break;
	default:
		break;
	}

	return 0;
}

int main(int argc, char **argv)
{
	const char *mnt_point = argc > 1 ? argv[1] : "/dev/eth0";

	if (virtio_net_init() != 0)
	{
		return -1;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "virtio-net");
	dev.read = net_read;
	dev.write = net_write;
	dev.dev_cntl = net_dcntl;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
