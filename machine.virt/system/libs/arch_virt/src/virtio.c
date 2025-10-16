#include <stdio.h>
#include <stdlib.h>
#include <ewoksys/mmio.h>
#include <arch/virt/dma.h>
#include <ewoksys/interrupt.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Virtio MMIO寄存器定义
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
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x80
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x84
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW 0x90
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH 0x94
#define VIRTIO_MMIO_QUEUE_USED_LOW 0xA0
#define VIRTIO_MMIO_QUEUE_USED_HIGH 0xA4
#define VIRTIO_MMIO_CONFIG 0x100

// Virtio 设备状态
#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_FEATURES_OK 8
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FAILED 128

// Virtio 队列相关定义
#define VIRTIO_QUEUE_SIZE 32
#define VIRTIO_PAGE_SIZE 4096

// 描述符标志
#define VIRTQ_DESC_F_NEXT (1 << 0)  // 链接下一个描述符
#define VIRTQ_DESC_F_WRITE (1 << 1) // 设备写入的内存

// 中断相关定义
#define VIRTIO_INTERRUPT_BASE 0x30

// 块设备请求类型
#define VIRTIO_ID_BLOCK 2
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_FLUSH 4
#define VIRTIO_BLK_OK 0
#define VIRTIO_BLK_IOERR 1
#define VIRTIO_BLK_UNSUPP 2

// 输入设备ID
#define VIRTIO_ID_INPUT 18
#define VIRTIO_INPUT_CFG_ID_NAME 0x01
#define VIRTIO_INPUT_CFG_ID_SERIAL 0x02
#define VIRTIO_INPUT_CFG_ID_DEVIDS 0x03
#define VIRTIO_INPUT_CFG_PROP_BITS 0x10
#define VIRTIO_INPUT_CFG_EV_BITS 0x11
#define VIRTIO_INPUT_CFG_ABS_INFO 0x12

// 描述符结构
struct virtq_desc
{
    uint64_t addr;  // 物理地址
    uint32_t len;   // 长度
    uint16_t flags; // 标志
    uint16_t next;  // 下一个描述符索引
} __attribute__((packed));

// 可用环结构
struct virtq_avail
{
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTIO_QUEUE_SIZE];
} __attribute__((packed));

// 已用环结构
struct virtq_used_elem
{
    uint32_t id;  // 描述符链起始索引
    uint32_t len; // 写入的总长度
} __attribute__((packed));

struct virtq_used
{
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[VIRTIO_QUEUE_SIZE];
} __attribute__((packed));

// 块设备请求头
struct virtio_blk_req
{
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
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
    unsigned char buf0[4096 - (sizeof(struct virtq_desc) * VIRTIO_QUEUE_SIZE +
                               sizeof(struct virtq_avail))];
    struct virtq_used used;
    uint8_t buf1[4096 - sizeof(struct virtq_used)];
};

// 设备状态结构
typedef struct virtio_device
{
    uintptr_t base;
    struct virtq_t *virtq;
    uintptr_t phy;
    int queue_ready;
    int interrupt;
    interrupt_handler_t virtio_handler;
    void (*interrupt_handler)(struct virtio_device *dev, struct virtio_input_event *event);
} virtio_dev_t;

#define VIRTIO_DEV_MAX 32
#define VIRTIO_BASE (_mmio_base + 0x02000000)
#define VIRTIO_TIMEOUT_MS 100000

static uint64_t get_phy_addr(struct virtio_device *dev, void *ptr)
{
    return dev->phy + (ptr - (void *)dev->virtq);
}

virtio_dev_t *virtio_get(int dev_id)
{
    for (int i = 0; i < VIRTIO_DEV_MAX; i++)
    {
        uint32_t magic = get32(VIRTIO_BASE + i * 0x200 + VIRTIO_MMIO_MAGIC);
        uint32_t id = get32(VIRTIO_BASE + i * 0x200 + VIRTIO_MMIO_DEVICE_ID);
        uint32_t vendor = get32(VIRTIO_BASE + i * 0x200 + VIRTIO_MMIO_VENDOR_ID);
        uint32_t features = get32(VIRTIO_BASE + i * 0x200 + VIRTIO_MMIO_DEVICE_FEAT);
        // klog("i: %d, magic: 0x%x id: %d vender: 0x%x features: 0x%x\n", i, magic, id, vendor, features);
        if (magic != 0x74726976)
        {
            continue;
        }

        if (id == dev_id)
        {
            virtio_dev_t *dev = malloc(sizeof(virtio_dev_t));
            memset(dev, 0, sizeof(virtio_dev_t));
            dev->base = VIRTIO_BASE + i * 0x200;
            dev->virtq = dma_user_alloc(sizeof(struct virtq_t));
            dev->phy = dma_user_phy(dev->virtq);
            // klog("DMA alloc base:%08x v:%08x p:%08x\n", dev->base, dev->virtq, dev->phy);
            return dev;
        }
    }
    return 0;
}

int virtio_get_config(uint32_t base, uint8_t select, uint8_t subsel, void *buffer, uint32_t size)
{
    put32(base + VIRTIO_MMIO_CONFIG, select);
    put32(base + VIRTIO_MMIO_CONFIG + 4, subsel);
    put32(base + VIRTIO_MMIO_CONFIG + 8, size);
    proc_usleep(0);
    struct virtio_input_config *cfg = (struct virtio_input_config *)(base + VIRTIO_MMIO_CONFIG);

    if (buffer && size > 0)
    {
        for (int i = 0; i < MIN(cfg->size, size); i++)
        {
            ((uint8_t *)buffer)[i] = cfg->u.string[i];
        }
    }
    return cfg->size;
}

virtio_dev_t *virtio_input_get(const char *name)
{
    for (int i = 0; i < VIRTIO_DEV_MAX; i++)
    {
        uint32_t base = VIRTIO_BASE + i * 0x200;
        uint32_t magic = get32(base + VIRTIO_MMIO_MAGIC);
        uint32_t id = get32(base + VIRTIO_MMIO_DEVICE_ID);
        uint32_t vendor = get32(base + VIRTIO_MMIO_VENDOR_ID);
        uint32_t features = get32(base + VIRTIO_MMIO_DEVICE_FEAT);
        if (magic != 0x74726976 || id != VIRTIO_ID_INPUT)
        {
            continue;
        }
        char id_name[128] = {0};
        virtio_get_config(base, VIRTIO_INPUT_CFG_ID_NAME, 0, id_name, sizeof(id_name));
        if (strcmp(id_name, name) == 0)
        {
            virtio_dev_t *dev = malloc(sizeof(virtio_dev_t));
            memset(dev, 0, sizeof(virtio_dev_t));
            dev->base = VIRTIO_BASE + i * 0x200;
            dev->virtq = dma_user_alloc(sizeof(struct virtq_t));
            dev->phy = dma_user_phy(dev->virtq);
            dev->interrupt = VIRTIO_INTERRUPT_BASE + i;
            // klog("DMA alloc base:%08x v:%08x p:%08x\n", dev->base, dev->virtq, dev->phy);
            return dev;
        }
    }
    return 0;
}

uint32_t virtio_init(struct virtio_device *dev, uint32_t feature)
{
    uint32_t base = dev->base;

    put32(dev->base + VIRTIO_MMIO_STATUS, 0);

    put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    put32(dev->base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_DRIVER);

    put32(dev->base + VIRTIO_MMIO_DRIVER_FEAT_SEL, 0);
    uint32_t features = get32(dev->base + VIRTIO_MMIO_DEVICE_FEAT);
    put32(dev->base + VIRTIO_MMIO_DRIVER_FEAT, features & feature);

    uint32_t status = get32(dev->base + VIRTIO_MMIO_STATUS);
    put32(dev->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_FEATURES_OK);

    put32(dev->base + VIRTIO_MMIO_QUEUE_SEL, 0);
    uint32_t max_queue_size = get32(dev->base + VIRTIO_MMIO_QUEUE_NUM_MAX);

    uint32_t queue_size = MIN(VIRTIO_QUEUE_SIZE, max_queue_size);
    put32(dev->base + VIRTIO_MMIO_QUEUE_NUM, queue_size);

    put32(dev->base + VIRTIO_MMIO_GUEST_PG_SZ, VIRTIO_PAGE_SIZE);
    put32(dev->base + VIRTIO_MMIO_QUEUE_PFN, dev->phy >> 12);

    status = get32(dev->base + VIRTIO_MMIO_STATUS);
    put32(dev->base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_DRIVER_OK);

    put32(dev->base + VIRTIO_MMIO_QUEUE_READY, 1);
    return feature;
}

static void virtio_input_fill_queue(virtio_dev_t *dev)
{
    struct virtq_t *virtq = dev->virtq;
    volatile int used_idx = virtq->used.idx;
    volatile int avail_idx = virtq->avail.idx;
    if (avail_idx - used_idx < VIRTIO_QUEUE_SIZE)
    {
        while (avail_idx - used_idx < VIRTIO_QUEUE_SIZE)
        {
            virtq->desc[avail_idx % VIRTIO_QUEUE_SIZE].addr = get_phy_addr(dev, virtq->buf0 + (avail_idx % VIRTIO_QUEUE_SIZE) * sizeof(struct virtio_input_event));
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
    virtio_dev_t *dev = (virtio_dev_t *)p;
    struct virtq_t *virtq = dev->virtq;
    put32(dev->base + VIRTIO_MMIO_INTERRUPT_ACK, 3);

    volatile int used_idx = virtq->used.idx;
    volatile int avail_idx = virtq->avail.idx;
    for (int i = avail_idx - VIRTIO_QUEUE_SIZE; i < used_idx; i++)
    {
        struct virtio_input_event *event = (struct virtio_input_event *)(virtq->buf0 + (i % VIRTIO_QUEUE_SIZE) * sizeof(struct virtio_input_event));
        dev->interrupt_handler(dev, event);
    }
    virtio_input_fill_queue(dev);
}

void virtio_interrupt_enable(struct virtio_device *dev, void (*interrupt_handler)(struct virtio_device *dev, struct virtio_input_event *event))
{
    put32(dev->base + VIRTIO_MMIO_INTERRUPT_STATUS, 0x1);

    dev->interrupt_handler = interrupt_handler;
    dev->virtio_handler.data = (uint32_t)dev;
    dev->virtio_handler.handler = virtio_interrupt_handle;
    sys_interrupt_setup(dev->interrupt, &dev->virtio_handler);
    virtio_input_fill_queue(dev);
}

int virtio_input_read(virtio_dev_t *dev, void *buffer, uint32_t size)
{
    struct virtq_t *virtq = dev->virtq;
    uint32_t ret = 0;
    if (get32(dev->base + VIRTIO_MMIO_INTERRUPT_STATUS) & 0x1)
    {
        put32(dev->base + VIRTIO_MMIO_INTERRUPT_ACK, 0x1);
        volatile int used_idx = dev->virtq->used.idx;
        volatile int avail_idx = dev->virtq->avail.idx;
        for (int i = avail_idx - VIRTIO_QUEUE_SIZE; i < used_idx && ret < size; i++)
        {
            struct virtio_input_event *event = (struct virtio_input_event *)(virtq->buf0 + (i % VIRTIO_QUEUE_SIZE) * sizeof(struct virtio_input_event));
            memcpy(buffer + ret, event, sizeof(struct virtio_input_event));
            ret += sizeof(struct virtio_input_event);
        }
    }
    virtio_input_fill_queue(dev);
    return ret;
}


int virtio_send_request(struct virtio_device *dev, void *req, uint32_t req_len, void *resp, uint32_t resp_len) {
    struct virtq_t *virtq = dev->virtq;
    uint8_t *req_buf = (uint8_t *)virtq->buf0;
    uint8_t *resp_buf = (uint8_t *)virtq->buf1;

    memcpy(req_buf, req, req_len);
    // 设置描述符链
    uint16_t desc_idx = 0;
    virtq->desc[desc_idx].addr = get_phy_addr(dev, req_buf);
    virtq->desc[desc_idx].len = req_len;
    virtq->desc[desc_idx].flags = VIRTQ_DESC_F_NEXT;
    virtq->desc[desc_idx].next = desc_idx + 1;

    virtq->desc[desc_idx + 1].addr = get_phy_addr(dev, resp_buf);
    virtq->desc[desc_idx + 1].len = resp_len;
    virtq->desc[desc_idx + 1].flags = VIRTQ_DESC_F_WRITE;
    virtq->desc[desc_idx + 1].next = 0;
    
    // 添加到可用环
    uint16_t avail_idx = virtq->avail.idx % VIRTIO_QUEUE_SIZE;
    virtq->avail.ring[avail_idx] = 0; // 使用第一个描述符
    
    // 更新索引
    virtq->avail.idx++;
    
    // 通知设备
    put32(dev->base + VIRTIO_MMIO_QUEUE_NOTIFY, 0);
   
    uint32_t timeout = VIRTIO_TIMEOUT_MS;
    while (timeout--)
    {
        uint32_t irq = get32(dev->base + VIRTIO_MMIO_INTERRUPT_STATUS);
        if (irq & 0x1)
        {
            //klog("irq: 0x%x used idx: %08x %d\n", irq, &virtq->used, virtq->used.idx);
            put32(dev->base + VIRTIO_MMIO_INTERRUPT_ACK, 0x1);
            memcpy(resp, resp_buf, resp_len);
            return resp_len;
        }
        proc_usleep(0);
    }

    return 0;
}

int virtio_blk_transfer(struct virtio_device *dev, uint64_t sector, void *buffer, uint32_t count, int isWrite)
{
    uintptr_t base = dev->base;
    struct virtq_t *virtq = dev->virtq;

    struct virtio_blk_req *req = (struct virtio_blk_req *)virtq->buf0;
    req->type = isWrite ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    req->sector = sector;

    uint8_t *buf = (uint8_t *)virtq->buf0 + sizeof(struct virtio_blk_req);
    uint32_t bytes = count * 512;
    buf[bytes] = 0xFF;

    uint16_t head_idx = 0;

    virtq->desc[head_idx].addr = get_phy_addr(dev, req);
    virtq->desc[head_idx].len = sizeof(struct virtio_blk_req);
    virtq->desc[head_idx].flags = VIRTQ_DESC_F_NEXT;
    virtq->desc[head_idx].next = head_idx + 1;

    virtq->desc[head_idx + 1].addr = get_phy_addr(dev, buf);
    virtq->desc[head_idx + 1].len = bytes;
    virtq->desc[head_idx + 1].flags = VIRTQ_DESC_F_NEXT | (isWrite ? 0 : VIRTQ_DESC_F_WRITE);
    virtq->desc[head_idx + 1].next = head_idx + 2;

    virtq->desc[head_idx + 2].addr = get_phy_addr(dev, &buf[bytes]);
    virtq->desc[head_idx + 2].len = sizeof(uint8_t);
    virtq->desc[head_idx + 2].flags = VIRTQ_DESC_F_WRITE;
    virtq->desc[head_idx + 2].next = 0;

    uint16_t avail_idx = virtq->avail.idx % VIRTIO_QUEUE_SIZE;
    virtq->avail.ring[avail_idx] = head_idx;
    virtq->avail.idx++;

    put32(base + VIRTIO_MMIO_QUEUE_NOTIFY, 0);
    uint32_t timeout = VIRTIO_TIMEOUT_MS;
    while (timeout--)
    {
        uint32_t irq = get32(base + VIRTIO_MMIO_INTERRUPT_STATUS);
        // klog("irq: 0x%x used idx: %08x %d\n", irq, &virtq->used.idx, virtq->used.idx);
        if (irq & 0x1)
        {
            put32(base + VIRTIO_MMIO_INTERRUPT_ACK, 0x1);
            if (!isWrite)
                memcpy(buffer, buf, bytes);
            break;
        }
        proc_usleep(0);
    }

    memset(&virtq->desc[head_idx], 0, sizeof(struct virtq_desc) * 3);

    return buf[bytes] == VIRTIO_BLK_OK ? 0 : -1;
}
