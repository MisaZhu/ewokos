#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arch/virt/dma.h>
#include <arch/virt/virtio.h>
#include <arch/virt/virtio_net.h>
#include <arch/virt/virtio_snd.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/klog.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proc.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ALIGN_UP(x, a) (((x) + (a)-1) & ~((a)-1))

#define VIRTIO_MMIO_MAGIC 0x00
#define VIRTIO_MMIO_VERSION 0x04
#define VIRTIO_MMIO_DEVICE_ID 0x08
#define VIRTIO_MMIO_VENDOR_ID 0x0C
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
#define VIRTIO_STATUS_FEATURES_OK 8
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FAILED 128

#define VIRTIO_QUEUE_SIZE 32
#define VIRTIO_PAGE_SIZE 4096

#define VIRTQ_DESC_F_NEXT (1u << 0)
#define VIRTQ_DESC_F_WRITE (1u << 1)

#define VIRTIO_INTERRUPT_BASE 0x30

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_OK 0

#define VIRTIO_INPUT_CFG_ID_NAME 0x01

#define VIRTIO_NET_F_MAC 5
#define VIRTIO_NET_RX_QUEUE 0
#define VIRTIO_NET_TX_QUEUE 1
#define VIRTIO_NET_TX_DESC_ID 0
#define VIRTIO_NET_HDR_SIZE 10
#define VIRTIO_NET_FRAME_SIZE_MAX 1514
#define VIRTIO_NET_RX_BUF_SIZE 2048
#define VIRTIO_NET_RX_DESC_COUNT 8

#define VIRTIO_DEV_MAX 32
#define VIRTIO_BASE (_mmio_base + 0x02000000)
#define VIRTIO_TIMEOUT_LOOPS 100000

#define VIRTIO_SND_DMA_ARENA_SIZE (128 * 1024)
#define VIRTIO_SND_CTL_TIMEOUT_MS 2000

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

struct virtio_blk_req
{
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
} __attribute__((packed));

struct virtio_net_hdr
{
	uint8_t flags;
	uint8_t gso_type;
	uint16_t hdr_len;
	uint16_t gso_size;
	uint16_t csum_start;
	uint16_t csum_offset;
} __attribute__((packed));

struct virtio_input_event
{
	uint16_t type;
	uint16_t code;
	uint32_t value;
} __attribute__((packed));

struct virtio_input_absinfo
{
	uint32_t min;
	uint32_t max;
	uint32_t fuzz;
	uint32_t flat;
	uint32_t res;
};

struct virtio_input_devids
{
	uint16_t bustype;
	uint16_t vendor;
	uint16_t product;
	uint16_t version;
};

struct virtio_input_config
{
	uint8_t select;
	uint8_t subsel;
	uint8_t size;
	uint8_t reserved[5];
	union
	{
		char string[128];
		uint8_t bitmap[128];
		struct virtio_input_absinfo abs;
		struct virtio_input_devids ids;
	} u;
};

struct virtq_t
{
	struct virtq_desc desc[VIRTIO_QUEUE_SIZE];
	struct virtq_avail avail;
	uint8_t buf0[4096 - (sizeof(struct virtq_desc) * VIRTIO_QUEUE_SIZE +
						 sizeof(struct virtq_avail))];
	struct virtq_used used;
	uint8_t buf1[4096 - sizeof(struct virtq_used)];
};

struct virtio_snd_tx_slot
{
	uint16_t head_desc;
	bool inflight;
	uint8_t *data;
	uintptr_t data_phy;
	struct virtio_snd_pcm_xfer *xfer;
	uintptr_t xfer_phy;
	struct virtio_snd_pcm_status *status;
	uintptr_t status_phy;
};

struct virtio_net_state
{
	struct virtq_t *rxq;
	uintptr_t rxq_phy;
	struct virtq_t *txq;
	uintptr_t txq_phy;
	uint8_t *rxbuf;
	uintptr_t rxbuf_phy;
	uint16_t rx_used_idx;
	uint16_t tx_used_idx;
	bool tx_inflight;
	uint8_t mac[6];
};

struct virtio_snd_state
{
	struct virtq_t *queues[4];
	uintptr_t queue_phys[4];
	uint16_t ctrl_used_idx;
	uint16_t event_used_idx;
	uint16_t tx_used_idx;
	uint8_t *dma_base;
	uintptr_t dma_phy;
	size_t dma_used;
	uint32_t tx_slot_bytes;
	uint32_t tx_slot_count;
	int last_error;
	struct virtio_snd_config config;
	struct virtio_snd_tx_slot tx_slots[VIRTIO_SND_TX_SLOT_MAX];
};

struct virtio_device
{
	uintptr_t base;
	struct virtq_t *virtq;
	uintptr_t phy;
	int queue_ready;
	int interrupt;
	int device_id;
	interrupt_handler_t virtio_handler;
	void (*interrupt_handler)(virtio_dev_t dev, struct virtio_input_event *event);
	struct virtio_net_state *net;
	struct virtio_snd_state *snd;
};

static virtio_dev_t _virtio_irq_devs[VIRTIO_DEV_MAX] = {0};

static int virtio_ensure_mmio(void)
{
	if (_mmio_base == 0 && mmio_map() == 0)
	{
		return -1;
	}
	return 0;
}

static uint64_t get_phy_addr(virtio_dev_t dev, void *ptr)
{
	return dev->phy + ((uintptr_t)ptr - (uintptr_t)dev->virtq);
}

static uint64_t virtio_queue_phy_addr(struct virtq_t *virtq, uintptr_t phy, const void *ptr)
{
	return phy + ((uintptr_t)ptr - (uintptr_t)virtq);
}

static uintptr_t virtio_find_device(int dev_id, const char *input_name, int *interrupt)
{
	if (virtio_ensure_mmio() != 0)
	{
		return 0;
	}

	for (int i = 0; i < VIRTIO_DEV_MAX; i++)
	{
		uintptr_t base = VIRTIO_BASE + i * 0x200;
		if (get32(base + VIRTIO_MMIO_MAGIC) != 0x74726976 ||
			get32(base + VIRTIO_MMIO_DEVICE_ID) != (uint32_t)dev_id)
		{
			continue;
		}

		if (input_name != NULL)
		{
			char id_name[128] = {0};
			struct virtio_input_config *cfg = (struct virtio_input_config *)(base + VIRTIO_MMIO_CONFIG);
			put32(base + VIRTIO_MMIO_CONFIG, VIRTIO_INPUT_CFG_ID_NAME);
			put32(base + VIRTIO_MMIO_CONFIG + 4, 0);
			put32(base + VIRTIO_MMIO_CONFIG + 8, sizeof(id_name));
			proc_usleep(0);
			for (int j = 0; j < MIN(cfg->size, (uint8_t)sizeof(id_name)); j++)
			{
				id_name[j] = cfg->u.string[j];
			}
			if (strcmp(id_name, input_name) != 0)
			{
				continue;
			}
		}

		if (interrupt != NULL)
		{
			*interrupt = VIRTIO_INTERRUPT_BASE + i;
		}
		return base;
	}

	return 0;
}

static virtio_dev_t virtio_alloc_device(uintptr_t base, int dev_id, bool alloc_queue, int interrupt)
{
	virtio_dev_t dev = malloc(sizeof(*dev));
	if (dev == NULL)
	{
		return NULL;
	}

	memset(dev, 0, sizeof(*dev));
	dev->base = base;
	dev->device_id = dev_id;
	dev->interrupt = interrupt;

	if (alloc_queue)
	{
		dev->virtq = dma_user_alloc(sizeof(struct virtq_t));
		if (dev->virtq == NULL || (intptr_t)dev->virtq == -1)
		{
			free(dev);
			return NULL;
		}
		dev->phy = (uintptr_t)dma_user_phy(dev->virtq);
	}

	return dev;
}

static int virtio_setup_queue(virtio_dev_t dev, uint16_t queue_idx, struct virtq_t *q, uintptr_t q_phy)
{
	put32(dev->base + VIRTIO_MMIO_QUEUE_SEL, queue_idx);
	uint32_t max_queue_size = get32(dev->base + VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (max_queue_size < VIRTIO_QUEUE_SIZE)
	{
		klog("virtio: queue %u too small: %u\n", queue_idx, max_queue_size);
		return -1;
	}

	memset(q, 0, sizeof(*q));
	put32(dev->base + VIRTIO_MMIO_QUEUE_NUM, VIRTIO_QUEUE_SIZE);
	put32(dev->base + VIRTIO_MMIO_QUEUE_ALIGN, VIRTIO_PAGE_SIZE);
	put32(dev->base + VIRTIO_MMIO_QUEUE_PFN, q_phy >> 12);
	put32(dev->base + VIRTIO_MMIO_QUEUE_READY, 1);
	return 0;
}

static uint32_t virtio_ack_interrupt(uintptr_t base, uint32_t mask)
{
	uint32_t irq = get32(base + VIRTIO_MMIO_INTERRUPT_STATUS);
	if (irq & mask)
	{
		put32(base + VIRTIO_MMIO_INTERRUPT_ACK, irq & mask);
	}
	return irq;
}

static int virtio_send_request_queue(uintptr_t base, struct virtq_t *virtq, uintptr_t phy,
									 uint16_t queue_idx, const void *req, uint32_t req_len,
									 void *resp, uint32_t resp_len, uint32_t loops, uint32_t sleep_us)
{
	if (virtq == NULL || req == NULL || req_len > sizeof(virtq->buf0) || resp_len > sizeof(virtq->buf1))
	{
		return 0;
	}

	memset(virtq->buf0, 0, sizeof(virtq->buf0));
	memset(virtq->buf1, 0, sizeof(virtq->buf1));
	memcpy(virtq->buf0, req, req_len);

	virtq->desc[0].addr = phy + ((uintptr_t)virtq->buf0 - (uintptr_t)virtq);
	virtq->desc[0].len = req_len;
	virtq->desc[0].flags = VIRTQ_DESC_F_NEXT;
	virtq->desc[0].next = 1;

	virtq->desc[1].addr = phy + ((uintptr_t)virtq->buf1 - (uintptr_t)virtq);
	virtq->desc[1].len = resp_len;
	virtq->desc[1].flags = VIRTQ_DESC_F_WRITE;
	virtq->desc[1].next = 0;

	uint16_t used_before = virtq->used.idx;
	virtq->avail.ring[virtq->avail.idx % VIRTIO_QUEUE_SIZE] = 0;
	__sync_synchronize();
	virtq->avail.idx++;
	put32(base + VIRTIO_MMIO_QUEUE_NOTIFY, queue_idx);

	for (uint32_t i = 0; i < loops; i++)
	{
		virtio_ack_interrupt(base, 0x1);
		if (virtq->used.idx != used_before)
		{
			if (resp != NULL && resp_len > 0)
			{
				memcpy(resp, virtq->buf1, resp_len);
			}
			return (int)resp_len;
		}
		proc_usleep(sleep_us);
	}

	return 0;
}

static struct virtio_net_state *virtio_net_state(virtio_dev_t dev)
{
	return dev != NULL ? dev->net : NULL;
}

static struct virtio_snd_state *virtio_snd_state(virtio_dev_t dev)
{
	return dev != NULL ? dev->snd : NULL;
}

static uint64_t virtio_net_rxbuf_phy_addr(struct virtio_net_state *net, uint16_t desc_id)
{
	return net->rxbuf_phy + ((uint64_t)desc_id * VIRTIO_NET_RX_BUF_SIZE);
}

static uint8_t *virtio_net_rxbuf_addr(struct virtio_net_state *net, uint16_t desc_id)
{
	return net->rxbuf + ((size_t)desc_id * VIRTIO_NET_RX_BUF_SIZE);
}

static void virtio_net_submit_rx_desc(virtio_dev_t dev, struct virtio_net_state *net, uint16_t desc_id)
{
	net->rxq->desc[desc_id].addr = virtio_net_rxbuf_phy_addr(net, desc_id);
	net->rxq->desc[desc_id].len = VIRTIO_NET_RX_BUF_SIZE;
	net->rxq->desc[desc_id].flags = VIRTQ_DESC_F_WRITE;
	net->rxq->desc[desc_id].next = 0;
	net->rxq->avail.ring[net->rxq->avail.idx % VIRTIO_QUEUE_SIZE] = desc_id;
	__sync_synchronize();
	net->rxq->avail.idx++;
	(void)dev;
}

static void virtio_net_fill_rx_queue(virtio_dev_t dev, struct virtio_net_state *net)
{
	for (uint16_t i = 0; i < VIRTIO_NET_RX_DESC_COUNT; i++)
	{
		virtio_net_submit_rx_desc(dev, net, i);
	}
	put32(dev->base + VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_RX_QUEUE);
}

static void virtio_net_reap_tx(struct virtio_net_state *net)
{
	if (net->tx_inflight && net->txq->used.idx != net->tx_used_idx)
	{
		net->tx_used_idx = net->txq->used.idx;
		net->tx_inflight = false;
	}
}

static void *virtio_snd_dma_alloc(struct virtio_snd_state *snd, size_t size, size_t align)
{
	if (snd->dma_base == NULL)
	{
		snd->dma_base = dma_user_alloc(VIRTIO_SND_DMA_ARENA_SIZE);
		if (snd->dma_base == NULL || (intptr_t)snd->dma_base == -1)
		{
			return NULL;
		}
		snd->dma_phy = (uintptr_t)dma_user_phy(snd->dma_base);
		snd->dma_used = 0;
		memset(snd->dma_base, 0, VIRTIO_SND_DMA_ARENA_SIZE);
	}

	size_t start = ALIGN_UP(snd->dma_used, align);
	size_t end = ALIGN_UP(start + size, 16);
	if (end > VIRTIO_SND_DMA_ARENA_SIZE)
	{
		return NULL;
	}

	void *ptr = snd->dma_base + start;
	memset(ptr, 0, size);
	snd->dma_used = end;
	return ptr;
}

static uintptr_t virtio_snd_dma_phy_addr(struct virtio_snd_state *snd, const void *ptr)
{
	return snd->dma_phy + ((uintptr_t)ptr - (uintptr_t)snd->dma_base);
}

static void virtio_snd_submit_event_desc(virtio_dev_t dev, struct virtio_snd_state *snd, uint16_t desc_id)
{
	struct virtio_snd_event *event =
		(struct virtio_snd_event *)(snd->queues[VIRTIO_SND_VQ_EVENT]->buf0 +
									desc_id * sizeof(struct virtio_snd_event));

	memset(event, 0, sizeof(*event));
	snd->queues[VIRTIO_SND_VQ_EVENT]->desc[desc_id].addr =
		snd->queue_phys[VIRTIO_SND_VQ_EVENT] +
		((uintptr_t)event - (uintptr_t)snd->queues[VIRTIO_SND_VQ_EVENT]);
	snd->queues[VIRTIO_SND_VQ_EVENT]->desc[desc_id].len = sizeof(*event);
	snd->queues[VIRTIO_SND_VQ_EVENT]->desc[desc_id].flags = VIRTQ_DESC_F_WRITE;
	snd->queues[VIRTIO_SND_VQ_EVENT]->desc[desc_id].next = 0;
	snd->queues[VIRTIO_SND_VQ_EVENT]->avail.ring[
		snd->queues[VIRTIO_SND_VQ_EVENT]->avail.idx % VIRTIO_QUEUE_SIZE] = desc_id;
	__sync_synchronize();
	snd->queues[VIRTIO_SND_VQ_EVENT]->avail.idx++;
}

static void virtio_snd_fill_event_queue(virtio_dev_t dev, struct virtio_snd_state *snd)
{
	for (uint16_t i = 0; i < VIRTIO_QUEUE_SIZE; i++)
	{
		virtio_snd_submit_event_desc(dev, snd, i);
	}
	put32(dev->base + VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_SND_VQ_EVENT);
}

static void virtio_snd_reap_events(virtio_dev_t dev, struct virtio_snd_state *snd)
{
	while (snd->event_used_idx != snd->queues[VIRTIO_SND_VQ_EVENT]->used.idx)
	{
		struct virtq_used_elem *used =
			&snd->queues[VIRTIO_SND_VQ_EVENT]->used.ring[snd->event_used_idx % VIRTIO_QUEUE_SIZE];
		uint16_t desc_id = used->id % VIRTIO_QUEUE_SIZE;
		struct virtio_snd_event *event =
			(struct virtio_snd_event *)(snd->queues[VIRTIO_SND_VQ_EVENT]->buf0 +
										desc_id * sizeof(struct virtio_snd_event));

		if (event->hdr.code == VIRTIO_SND_EVT_PCM_XRUN)
		{
			snd->last_error = -32;
		}

		snd->event_used_idx++;
		virtio_snd_submit_event_desc(dev, snd, desc_id);
	}
}

static void virtio_snd_reap_tx(struct virtio_snd_state *snd)
{
	while (snd->tx_used_idx != snd->queues[VIRTIO_SND_VQ_TX]->used.idx)
	{
		struct virtq_used_elem *used =
			&snd->queues[VIRTIO_SND_VQ_TX]->used.ring[snd->tx_used_idx % VIRTIO_QUEUE_SIZE];
		uint16_t head_desc = used->id % VIRTIO_QUEUE_SIZE;
		uint32_t slot_id = head_desc / 3;

		if (slot_id < VIRTIO_SND_TX_SLOT_MAX &&
			snd->tx_slots[slot_id].head_desc == head_desc)
		{
			struct virtio_snd_tx_slot *slot = &snd->tx_slots[slot_id];
			if (slot->status != NULL && slot->status->status != VIRTIO_SND_S_OK)
			{
				snd->last_error = -32;
			}
			slot->inflight = false;
		}

		snd->tx_used_idx++;
	}
}

static int virtio_snd_wait_slot(virtio_dev_t dev, struct virtio_snd_state *snd, uint32_t timeout_ms)
{
	for (uint32_t i = 0; i < timeout_ms; i++)
	{
		virtio_snd_poll(dev);
		if (snd->last_error < 0)
		{
			return snd->last_error;
		}

		for (uint32_t slot = 0; slot < snd->tx_slot_count; slot++)
		{
			if (!snd->tx_slots[slot].inflight)
			{
				return 0;
			}
		}
		proc_usleep(1000);
	}
	return -1;
}

virtio_dev_t virtio_get(int dev_id)
{
	uintptr_t base = virtio_find_device(dev_id, NULL, NULL);
	if (base == 0)
	{
		return NULL;
	}
	return virtio_alloc_device(base, dev_id, true, 0);
}

virtio_dev_t virtio_input_get(const char *name)
{
	int interrupt = 0;
	uintptr_t base = virtio_find_device(VIRTIO_ID_INPUT, name, &interrupt);
	if (base == 0)
	{
		return NULL;
	}
	return virtio_alloc_device(base, VIRTIO_ID_INPUT, true, interrupt);
}

virtio_dev_t virtio_net_get(void)
{
	uintptr_t base = virtio_find_device(VIRTIO_ID_NET, NULL, NULL);
	if (base == 0)
	{
		return NULL;
	}
	return virtio_alloc_device(base, VIRTIO_ID_NET, true, 0);
}

virtio_dev_t virtio_snd_get(void)
{
	uintptr_t base = virtio_find_device(VIRTIO_ID_SOUND, NULL, NULL);
	if (base == 0)
	{
		return NULL;
	}
	return virtio_alloc_device(base, VIRTIO_ID_SOUND, false, 0);
}

void virtio_free(virtio_dev_t dev)
{
	if (dev == NULL)
	{
		return;
	}
	if (dev->net != NULL)
	{
		free(dev->net);
	}
	if (dev->snd != NULL)
	{
		free(dev->snd);
	}
	free(dev);
}

int virtio_init(virtio_dev_t dev, uint32_t feature)
{
	if (dev == NULL || dev->virtq == NULL)
	{
		return -1;
	}

	put32(dev->base + VIRTIO_MMIO_STATUS, 0);
	put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
	put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_DRIVER);

	put32(dev->base + VIRTIO_MMIO_DRIVER_FEAT_SEL, 0);
	uint32_t features = get32(dev->base + VIRTIO_MMIO_DEVICE_FEAT);
	put32(dev->base + VIRTIO_MMIO_DRIVER_FEAT, features & feature);

	uint32_t status = get32(dev->base + VIRTIO_MMIO_STATUS);
	put32(dev->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_FEATURES_OK);

	put32(dev->base + VIRTIO_MMIO_GUEST_PG_SZ, VIRTIO_PAGE_SIZE);
	if (virtio_setup_queue(dev, 0, dev->virtq, dev->phy) != 0)
	{
		put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
		return -1;
	}

	status = get32(dev->base + VIRTIO_MMIO_STATUS);
	put32(dev->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_DRIVER_OK);
	dev->queue_ready = 1;
	return 0;
}

static void virtio_input_fill_queue(virtio_dev_t dev)
{
	struct virtq_t *virtq = dev->virtq;
	volatile int used_idx = virtq->used.idx;
	volatile int avail_idx = virtq->avail.idx;

	if (avail_idx - used_idx < VIRTIO_QUEUE_SIZE)
	{
		while (avail_idx - used_idx < VIRTIO_QUEUE_SIZE)
		{
			virtq->desc[avail_idx % VIRTIO_QUEUE_SIZE].addr =
				get_phy_addr(dev, virtq->buf0 + (avail_idx % VIRTIO_QUEUE_SIZE) * sizeof(struct virtio_input_event));
			virtq->desc[avail_idx % VIRTIO_QUEUE_SIZE].len = sizeof(struct virtio_input_event);
			virtq->desc[avail_idx % VIRTIO_QUEUE_SIZE].flags = VIRTQ_DESC_F_WRITE;
			virtq->desc[avail_idx % VIRTIO_QUEUE_SIZE].next = 0;
			virtq->avail.ring[avail_idx % VIRTIO_QUEUE_SIZE] = avail_idx % VIRTIO_QUEUE_SIZE;
			avail_idx++;
			virtq->avail.idx++;
		}

		put32(dev->base + VIRTIO_MMIO_QUEUE_NOTIFY, 0);
	}
}

static void virtio_interrupt_handle(uint32_t interrupt, uint32_t p)
{
	if (p >= VIRTIO_DEV_MAX)
	{
		return;
	}

	virtio_dev_t dev = _virtio_irq_devs[p];
	if (dev == NULL)
	{
		return;
	}
	struct virtq_t *virtq = dev->virtq;
	virtio_ack_interrupt(dev->base, 0x3);

	volatile int used_idx = virtq->used.idx;
	volatile int avail_idx = virtq->avail.idx;
	for (int i = avail_idx - VIRTIO_QUEUE_SIZE; i < used_idx; i++)
	{
		struct virtio_input_event *event =
			(struct virtio_input_event *)(virtq->buf0 + (i % VIRTIO_QUEUE_SIZE) * sizeof(struct virtio_input_event));
		dev->interrupt_handler(dev, event);
	}
	virtio_input_fill_queue(dev);
}

void virtio_interrupt_enable(virtio_dev_t dev, void (*interrupt_handler)(virtio_dev_t dev, struct virtio_input_event *event))
{
	put32(dev->base + VIRTIO_MMIO_INTERRUPT_STATUS, 0x1);
	dev->interrupt_handler = interrupt_handler;
	dev->virtio_handler.data = (uint32_t)(dev->interrupt - VIRTIO_INTERRUPT_BASE);
	_virtio_irq_devs[dev->virtio_handler.data] = dev;
	dev->virtio_handler.handler = virtio_interrupt_handle;
	sys_interrupt_setup(dev->interrupt, &dev->virtio_handler);
	virtio_input_fill_queue(dev);
}

int virtio_input_read(virtio_dev_t dev, void *buffer, uint32_t size)
{
	struct virtq_t *virtq = dev->virtq;
	uint32_t ret = 0;
	if (get32(dev->base + VIRTIO_MMIO_INTERRUPT_STATUS) & 0x1)
	{
		put32(dev->base + VIRTIO_MMIO_INTERRUPT_ACK, 0x1);
		volatile int used_idx = virtq->used.idx;
		volatile int avail_idx = virtq->avail.idx;
		for (int i = avail_idx - VIRTIO_QUEUE_SIZE; i < used_idx && ret < size; i++)
		{
			struct virtio_input_event *event =
				(struct virtio_input_event *)(virtq->buf0 + (i % VIRTIO_QUEUE_SIZE) * sizeof(struct virtio_input_event));
			memcpy((uint8_t *)buffer + ret, event, sizeof(struct virtio_input_event));
			ret += sizeof(struct virtio_input_event);
		}
	}
	virtio_input_fill_queue(dev);
	return ret;
}

int virtio_send_request(virtio_dev_t dev, const void *req, uint32_t req_len, void *resp, uint32_t resp_len)
{
	if (dev == NULL || dev->virtq == NULL)
	{
		return 0;
	}
	return virtio_send_request_queue(dev->base, dev->virtq, dev->phy, 0,
									 req, req_len, resp, resp_len,
									 VIRTIO_TIMEOUT_LOOPS, 0);
}

int virtio_net_init(virtio_dev_t dev)
{
	if (dev == NULL || dev->device_id != VIRTIO_ID_NET || dev->virtq == NULL)
	{
		return -1;
	}
	if (dev->net != NULL)
	{
		return 0;
	}

	struct virtio_net_state *net = calloc(1, sizeof(*net));
	if (net == NULL)
	{
		return -1;
	}

	net->rxq = dev->virtq;
	net->rxq_phy = dev->phy;

	net->txq = dma_user_alloc(sizeof(struct virtq_t));
	if (net->txq == NULL || (intptr_t)net->txq == -1)
	{
		free(net);
		klog("virtio-net: alloc tx queue failed\n");
		return -1;
	}
	net->txq_phy = (uintptr_t)dma_user_phy(net->txq);

	net->rxbuf = dma_user_alloc(VIRTIO_NET_RX_DESC_COUNT * VIRTIO_NET_RX_BUF_SIZE);
	if (net->rxbuf == NULL || (intptr_t)net->rxbuf == -1)
	{
		free(net);
		klog("virtio-net: alloc rx buffers failed\n");
		return -1;
	}
	net->rxbuf_phy = (uintptr_t)dma_user_phy(net->rxbuf);
	memset(net->rxbuf, 0, VIRTIO_NET_RX_DESC_COUNT * VIRTIO_NET_RX_BUF_SIZE);

	put32(dev->base + VIRTIO_MMIO_STATUS, 0);
	put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
	put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

	put32(dev->base + VIRTIO_MMIO_DEVICE_FEAT_SEL, 0);
	uint32_t device_features = get32(dev->base + VIRTIO_MMIO_DEVICE_FEAT);
	uint32_t driver_features = 0;
	if (device_features & (1u << VIRTIO_NET_F_MAC))
	{
		driver_features |= (1u << VIRTIO_NET_F_MAC);
	}

	put32(dev->base + VIRTIO_MMIO_DRIVER_FEAT_SEL, 0);
	put32(dev->base + VIRTIO_MMIO_DRIVER_FEAT, driver_features);
	put32(dev->base + VIRTIO_MMIO_GUEST_PG_SZ, VIRTIO_PAGE_SIZE);

	uint32_t status = get32(dev->base + VIRTIO_MMIO_STATUS);
	put32(dev->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_FEATURES_OK);
	status = get32(dev->base + VIRTIO_MMIO_STATUS);
	if ((status & VIRTIO_STATUS_FEATURES_OK) == 0)
	{
		put32(dev->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_FAILED);
		free(net);
		klog("virtio-net: feature negotiation failed\n");
		return -1;
	}

	if (virtio_setup_queue(dev, VIRTIO_NET_RX_QUEUE, net->rxq, net->rxq_phy) != 0 ||
		virtio_setup_queue(dev, VIRTIO_NET_TX_QUEUE, net->txq, net->txq_phy) != 0)
	{
		put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
		free(net);
		return -1;
	}

	virtio_net_fill_rx_queue(dev, net);

	if (driver_features & (1u << VIRTIO_NET_F_MAC))
	{
		for (int i = 0; i < 6; i++)
		{
			net->mac[i] = get8(dev->base + VIRTIO_MMIO_CONFIG + i);
		}
	}

	status = get32(dev->base + VIRTIO_MMIO_STATUS);
	put32(dev->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_DRIVER_OK);

	net->rx_used_idx = net->rxq->used.idx;
	net->tx_used_idx = net->txq->used.idx;
	net->tx_inflight = false;
	dev->net = net;
	dev->queue_ready = 1;
	return 0;
}

int virtio_net_read_mac(virtio_dev_t dev, uint8_t mac[6])
{
	struct virtio_net_state *net = virtio_net_state(dev);
	if (net == NULL || mac == NULL)
	{
		return -1;
	}
	memcpy(mac, net->mac, 6);
	return 0;
}

int virtio_net_pending_rx(virtio_dev_t dev)
{
	struct virtio_net_state *net = virtio_net_state(dev);
	if (net == NULL || net->rxq == NULL)
	{
		return 0;
	}

	virtio_ack_interrupt(dev->base, 0x3);
	__sync_synchronize();
	return (uint16_t)(net->rxq->used.idx - net->rx_used_idx);
}

int virtio_net_read(virtio_dev_t dev, void *buf, uint32_t size)
{
	struct virtio_net_state *net = virtio_net_state(dev);
	if (net == NULL || buf == NULL)
	{
		return -1;
	}
	if (virtio_net_pending_rx(dev) <= 0)
	{
		return 0;
	}

	struct virtq_used_elem *used = &net->rxq->used.ring[net->rx_used_idx % VIRTIO_QUEUE_SIZE];
	uint16_t desc_id = used->id % VIRTIO_QUEUE_SIZE;
	if (desc_id >= VIRTIO_NET_RX_DESC_COUNT)
	{
		klog("virtio-net: invalid rx desc id=%u used_idx=%u\n", desc_id, net->rx_used_idx);
		net->rx_used_idx++;
		return 0;
	}

	uint8_t *frame = virtio_net_rxbuf_addr(net, desc_id);
	int payload_len = (int)used->len - VIRTIO_NET_HDR_SIZE;
	if (payload_len < 0)
	{
		payload_len = 0;
	}

	int read_len = payload_len;
	if ((uint32_t)read_len > size)
	{
		read_len = (int)size;
	}
	if (read_len > 0)
	{
		memcpy(buf, frame + VIRTIO_NET_HDR_SIZE, (size_t)read_len);
	}

	net->rx_used_idx++;
	virtio_net_submit_rx_desc(dev, net, desc_id);
	put32(dev->base + VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_RX_QUEUE);
	return read_len;
}

int virtio_net_write(virtio_dev_t dev, const void *buf, uint32_t size)
{
	struct virtio_net_state *net = virtio_net_state(dev);
	if (net == NULL || buf == NULL)
	{
		return -1;
	}

	virtio_ack_interrupt(dev->base, 0x3);
	virtio_net_reap_tx(net);
	if (net->tx_inflight)
	{
		return 0;
	}

	uint32_t write_len = size;
	if (write_len > VIRTIO_NET_FRAME_SIZE_MAX)
	{
		write_len = VIRTIO_NET_FRAME_SIZE_MAX;
	}
	if (write_len == 0)
	{
		return 0;
	}

	struct virtio_net_hdr *hdr = (struct virtio_net_hdr *)net->txq->buf0;
	memset(hdr, 0, sizeof(*hdr));
	memcpy(net->txq->buf0 + sizeof(*hdr), buf, write_len);

	net->txq->desc[VIRTIO_NET_TX_DESC_ID].addr =
		virtio_queue_phy_addr(net->txq, net->txq_phy, net->txq->buf0);
	net->txq->desc[VIRTIO_NET_TX_DESC_ID].len = sizeof(*hdr) + write_len;
	net->txq->desc[VIRTIO_NET_TX_DESC_ID].flags = 0;
	net->txq->desc[VIRTIO_NET_TX_DESC_ID].next = 0;

	net->txq->avail.ring[net->txq->avail.idx % VIRTIO_QUEUE_SIZE] = VIRTIO_NET_TX_DESC_ID;
	__sync_synchronize();
	net->txq->avail.idx++;
	put32(dev->base + VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_NET_TX_QUEUE);

	net->tx_inflight = true;
	return (int)write_len;
}

int virtio_blk_transfer(virtio_dev_t dev, uint64_t sector, void *buffer, uint32_t count, int isWrite)
{
	uintptr_t base = dev->base;
	struct virtq_t *virtq = dev->virtq;

	struct virtio_blk_req *req = (struct virtio_blk_req *)virtq->buf0;
	req->type = isWrite ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
	req->sector = sector;

	uint8_t *buf = (uint8_t *)virtq->buf0 + sizeof(struct virtio_blk_req);
	uint32_t bytes = count * 512;
	buf[bytes] = 0xFF;

	if (isWrite)
	{
		memcpy(buf, buffer, bytes);
	}

	virtq->desc[0].addr = get_phy_addr(dev, req);
	virtq->desc[0].len = sizeof(struct virtio_blk_req);
	virtq->desc[0].flags = VIRTQ_DESC_F_NEXT;
	virtq->desc[0].next = 1;

	virtq->desc[1].addr = get_phy_addr(dev, buf);
	virtq->desc[1].len = bytes;
	virtq->desc[1].flags = VIRTQ_DESC_F_NEXT | (isWrite ? 0 : VIRTQ_DESC_F_WRITE);
	virtq->desc[1].next = 2;

	virtq->desc[2].addr = get_phy_addr(dev, &buf[bytes]);
	virtq->desc[2].len = sizeof(uint8_t);
	virtq->desc[2].flags = VIRTQ_DESC_F_WRITE;
	virtq->desc[2].next = 0;

	virtq->avail.ring[virtq->avail.idx % VIRTIO_QUEUE_SIZE] = 0;
	virtq->avail.idx++;
	put32(base + VIRTIO_MMIO_QUEUE_NOTIFY, 0);

	for (uint32_t i = 0; i < VIRTIO_TIMEOUT_LOOPS; i++)
	{
		if (virtio_ack_interrupt(base, 0x1) & 0x1)
		{
			if (!isWrite)
			{
				memcpy(buffer, buf, bytes);
			}
			break;
		}
		proc_usleep(0);
	}

	memset(&virtq->desc[0], 0, sizeof(struct virtq_desc) * 3);
	return buf[bytes] == VIRTIO_BLK_OK ? 0 : -1;
}

int virtio_snd_init(virtio_dev_t dev)
{
	if (dev == NULL || dev->device_id != VIRTIO_ID_SOUND)
	{
		return -1;
	}
	if (dev->snd != NULL)
	{
		return 0;
	}

	struct virtio_snd_state *snd = calloc(1, sizeof(*snd));
	if (snd == NULL)
	{
		return -1;
	}

	dev->snd = snd;

	for (uint32_t i = 0; i < 4; i++)
	{
		snd->queues[i] = virtio_snd_dma_alloc(snd, sizeof(struct virtq_t), VIRTIO_PAGE_SIZE);
		if (snd->queues[i] == NULL)
		{
			return -1;
		}
		snd->queue_phys[i] = virtio_snd_dma_phy_addr(snd, snd->queues[i]);
	}

	put32(dev->base + VIRTIO_MMIO_STATUS, 0);
	put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
	put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

	put32(dev->base + VIRTIO_MMIO_DEVICE_FEAT_SEL, 0);
	(void)get32(dev->base + VIRTIO_MMIO_DEVICE_FEAT);
	put32(dev->base + VIRTIO_MMIO_DRIVER_FEAT_SEL, 0);
	put32(dev->base + VIRTIO_MMIO_DRIVER_FEAT, 0);
	put32(dev->base + VIRTIO_MMIO_GUEST_PG_SZ, VIRTIO_PAGE_SIZE);

	uint32_t status = get32(dev->base + VIRTIO_MMIO_STATUS);
	put32(dev->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_FEATURES_OK);
	status = get32(dev->base + VIRTIO_MMIO_STATUS);
	if ((status & VIRTIO_STATUS_FEATURES_OK) == 0)
	{
		put32(dev->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_FAILED);
		return -1;
	}

	for (uint32_t i = 0; i < 4; i++)
	{
		if (virtio_setup_queue(dev, i, snd->queues[i], snd->queue_phys[i]) != 0)
		{
			put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
			return -1;
		}
	}

	virtio_snd_fill_event_queue(dev, snd);

	status = get32(dev->base + VIRTIO_MMIO_STATUS);
	put32(dev->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_DRIVER_OK);

	snd->ctrl_used_idx = snd->queues[VIRTIO_SND_VQ_CONTROL]->used.idx;
	snd->event_used_idx = snd->queues[VIRTIO_SND_VQ_EVENT]->used.idx;
	snd->tx_used_idx = snd->queues[VIRTIO_SND_VQ_TX]->used.idx;
	snd->last_error = 0;

	return virtio_snd_read_config(dev, &snd->config);
}

int virtio_snd_read_config(virtio_dev_t dev, struct virtio_snd_config *cfg)
{
	struct virtio_snd_state *snd = virtio_snd_state(dev);
	if (snd == NULL)
	{
		return -1;
	}

	snd->config.jacks = get32(dev->base + VIRTIO_MMIO_CONFIG + 0);
	snd->config.streams = get32(dev->base + VIRTIO_MMIO_CONFIG + 4);
	snd->config.chmaps = get32(dev->base + VIRTIO_MMIO_CONFIG + 8);
	snd->config.controls = get32(dev->base + VIRTIO_MMIO_CONFIG + 12);

	if (cfg != NULL)
	{
		memcpy(cfg, &snd->config, sizeof(*cfg));
	}
	return 0;
}

int virtio_snd_ctl(virtio_dev_t dev, const void *req, uint32_t req_len, void *resp, uint32_t resp_len)
{
	struct virtio_snd_state *snd = virtio_snd_state(dev);
	if (snd == NULL)
	{
		return -1;
	}

	int ret = virtio_send_request_queue(dev->base, snd->queues[VIRTIO_SND_VQ_CONTROL],
										snd->queue_phys[VIRTIO_SND_VQ_CONTROL],
										VIRTIO_SND_VQ_CONTROL, req, req_len, resp, resp_len,
										VIRTIO_SND_CTL_TIMEOUT_MS, 1000);
	if (ret != (int)resp_len)
	{
		return -1;
	}

	snd->ctrl_used_idx = snd->queues[VIRTIO_SND_VQ_CONTROL]->used.idx;
	return 0;
}

int virtio_snd_query_pcm_info(virtio_dev_t dev, uint32_t stream_id, struct virtio_snd_pcm_info *info)
{
	struct virtio_snd_query_info req;
	struct
	{
		struct virtio_snd_hdr hdr;
		struct virtio_snd_pcm_info info;
	} resp;

	memset(&req, 0, sizeof(req));
	memset(&resp, 0, sizeof(resp));
	req.hdr.code = VIRTIO_SND_R_PCM_INFO;
	req.start_id = stream_id;
	req.count = 1;
	req.size = sizeof(struct virtio_snd_pcm_info);

	if (virtio_snd_ctl(dev, &req, sizeof(req), &resp, sizeof(resp)) != 0 ||
		resp.hdr.code != VIRTIO_SND_S_OK)
	{
		return -1;
	}

	memcpy(info, &resp.info, sizeof(*info));
	return 0;
}

int virtio_snd_pcm_set_params(virtio_dev_t dev, uint32_t stream_id, uint32_t buffer_bytes,
							  uint32_t period_bytes, uint32_t features, uint8_t channels,
							  uint8_t format, uint8_t rate)
{
	struct virtio_snd_pcm_set_params req;
	struct virtio_snd_hdr resp;

	memset(&req, 0, sizeof(req));
	memset(&resp, 0, sizeof(resp));
	req.hdr.hdr.code = VIRTIO_SND_R_PCM_SET_PARAMS;
	req.hdr.stream_id = stream_id;
	req.buffer_bytes = buffer_bytes;
	req.period_bytes = period_bytes;
	req.features = features;
	req.channels = channels;
	req.format = format;
	req.rate = rate;

	if (virtio_snd_ctl(dev, &req, sizeof(req), &resp, sizeof(resp)) != 0)
	{
		return -1;
	}
	return resp.code == VIRTIO_SND_S_OK ? 0 : -1;
}

int virtio_snd_pcm_ctl(virtio_dev_t dev, uint32_t code, uint32_t stream_id)
{
	struct virtio_snd_pcm_hdr req;
	struct virtio_snd_hdr resp;

	memset(&req, 0, sizeof(req));
	memset(&resp, 0, sizeof(resp));
	req.hdr.code = code;
	req.stream_id = stream_id;

	if (virtio_snd_ctl(dev, &req, sizeof(req), &resp, sizeof(resp)) != 0)
	{
		return -1;
	}
	return resp.code == VIRTIO_SND_S_OK ? 0 : -1;
}

int virtio_snd_poll(virtio_dev_t dev)
{
	struct virtio_snd_state *snd = virtio_snd_state(dev);
	if (snd == NULL)
	{
		return -1;
	}

	virtio_ack_interrupt(dev->base, 0x3);
	virtio_snd_reap_tx(snd);
	virtio_snd_reap_events(dev, snd);
	return snd->last_error;
}

int virtio_snd_last_error(virtio_dev_t dev)
{
	struct virtio_snd_state *snd = virtio_snd_state(dev);
	return snd != NULL ? snd->last_error : -1;
}

void virtio_snd_clear_error(virtio_dev_t dev)
{
	struct virtio_snd_state *snd = virtio_snd_state(dev);
	if (snd != NULL)
	{
		snd->last_error = 0;
	}
}

int virtio_snd_tx_init(virtio_dev_t dev, uint32_t slot_count, uint32_t period_bytes)
{
	struct virtio_snd_state *snd = virtio_snd_state(dev);
	if (snd == NULL || slot_count == 0 || slot_count > VIRTIO_SND_TX_SLOT_MAX || period_bytes == 0)
	{
		return -1;
	}

	if (snd->tx_slots[0].data == NULL || snd->tx_slot_bytes < period_bytes)
	{
		struct virtio_snd_pcm_xfer *xfers =
			virtio_snd_dma_alloc(snd, sizeof(struct virtio_snd_pcm_xfer) * VIRTIO_SND_TX_SLOT_MAX, 16);
		struct virtio_snd_pcm_status *statuses =
			virtio_snd_dma_alloc(snd, sizeof(struct virtio_snd_pcm_status) * VIRTIO_SND_TX_SLOT_MAX, 16);
		uint8_t *buffers =
			virtio_snd_dma_alloc(snd, (size_t)period_bytes * VIRTIO_SND_TX_SLOT_MAX, 16);

		if (xfers == NULL || statuses == NULL || buffers == NULL)
		{
			return -1;
		}

		for (uint32_t i = 0; i < VIRTIO_SND_TX_SLOT_MAX; i++)
		{
			snd->tx_slots[i].head_desc = i * 3;
			snd->tx_slots[i].xfer = &xfers[i];
			snd->tx_slots[i].xfer_phy = virtio_snd_dma_phy_addr(snd, snd->tx_slots[i].xfer);
			snd->tx_slots[i].status = &statuses[i];
			snd->tx_slots[i].status_phy = virtio_snd_dma_phy_addr(snd, snd->tx_slots[i].status);
			snd->tx_slots[i].data = buffers + (size_t)i * period_bytes;
			snd->tx_slots[i].data_phy = virtio_snd_dma_phy_addr(snd, snd->tx_slots[i].data);
			snd->tx_slots[i].inflight = false;
		}

		snd->tx_slot_bytes = period_bytes;
	}

	snd->tx_slot_count = slot_count;
	virtio_snd_tx_reset(dev);
	return 0;
}

void virtio_snd_tx_reset(virtio_dev_t dev)
{
	struct virtio_snd_state *snd = virtio_snd_state(dev);
	if (snd == NULL)
	{
		return;
	}

	for (uint32_t i = 0; i < VIRTIO_SND_TX_SLOT_MAX; i++)
	{
		snd->tx_slots[i].inflight = false;
		if (snd->tx_slots[i].status != NULL)
		{
			memset(snd->tx_slots[i].status, 0, sizeof(struct virtio_snd_pcm_status));
		}
	}
	snd->tx_used_idx = snd->queues[VIRTIO_SND_VQ_TX]->used.idx;
	snd->last_error = 0;
}

int virtio_snd_tx_avail_bytes(virtio_dev_t dev)
{
	struct virtio_snd_state *snd = virtio_snd_state(dev);
	if (snd == NULL || snd->tx_slot_bytes == 0)
	{
		return -1;
	}

	virtio_snd_poll(dev);
	if (snd->last_error < 0)
	{
		return snd->last_error;
	}

	int free_slots = 0;
	for (uint32_t i = 0; i < snd->tx_slot_count; i++)
	{
		if (!snd->tx_slots[i].inflight)
		{
			free_slots++;
		}
	}
	return free_slots * (int)snd->tx_slot_bytes;
}

int virtio_snd_tx_write(virtio_dev_t dev, uint32_t stream_id, const void *data, uint32_t size,
						uint32_t timeout_ms)
{
	struct virtio_snd_state *snd = virtio_snd_state(dev);
	if (snd == NULL || snd->tx_slot_bytes == 0 || snd->tx_slot_count == 0 || data == NULL)
	{
		return -1;
	}

	const uint8_t *src = data;
	uint32_t written = 0;

	while (written < size)
	{
		virtio_snd_poll(dev);
		if (snd->last_error < 0)
		{
			return written > 0 ? (int)written : snd->last_error;
		}

		struct virtio_snd_tx_slot *slot = NULL;
		for (uint32_t i = 0; i < snd->tx_slot_count; i++)
		{
			if (!snd->tx_slots[i].inflight)
			{
				slot = &snd->tx_slots[i];
				break;
			}
		}

		if (slot == NULL)
		{
			int wait_ret = virtio_snd_wait_slot(dev, snd, timeout_ms);
			if (wait_ret != 0)
			{
				return written > 0 ? (int)written : wait_ret;
			}
			continue;
		}

		uint32_t chunk = MIN(size - written, snd->tx_slot_bytes);
		memset(slot->status, 0, sizeof(*slot->status));
		slot->xfer->stream_id = stream_id;
		memcpy(slot->data, src + written, chunk);

		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc].addr = slot->xfer_phy;
		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc].len = sizeof(struct virtio_snd_pcm_xfer);
		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc].flags = VIRTQ_DESC_F_NEXT;
		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc].next = slot->head_desc + 1;

		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc + 1].addr = slot->data_phy;
		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc + 1].len = chunk;
		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc + 1].flags = VIRTQ_DESC_F_NEXT;
		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc + 1].next = slot->head_desc + 2;

		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc + 2].addr = slot->status_phy;
		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc + 2].len = sizeof(struct virtio_snd_pcm_status);
		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc + 2].flags = VIRTQ_DESC_F_WRITE;
		snd->queues[VIRTIO_SND_VQ_TX]->desc[slot->head_desc + 2].next = 0;

		snd->queues[VIRTIO_SND_VQ_TX]->avail.ring[
			snd->queues[VIRTIO_SND_VQ_TX]->avail.idx % VIRTIO_QUEUE_SIZE] = slot->head_desc;
		__sync_synchronize();
		snd->queues[VIRTIO_SND_VQ_TX]->avail.idx++;
		slot->inflight = true;
		put32(dev->base + VIRTIO_MMIO_QUEUE_NOTIFY, VIRTIO_SND_VQ_TX);

		written += chunk;
	}

	return (int)written;
}
