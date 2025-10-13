#include <stdio.h>
#include <stdlib.h>
#include <ewoksys/mmio.h>
#include <arch/virt/dma.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Virtio MMIO寄存器定义
#define VIRTIO_MMIO_MAGIC         0x00
#define VIRTIO_MMIO_VERSION       0x04
#define VIRTIO_MMIO_DEVICE_ID     0x08
#define VIRTIO_MMIO_VENDOR_ID     0x0C
#define VIRTIO_MMIO_DEVICE_FEAT   0x10
#define VIRTIO_MMIO_DEVICE_FEAT_SEL   0x14
#define VIRTIO_MMIO_DRIVER_FEAT   0x20
#define VIRTIO_MMIO_DRIVER_FEAT_SEL   0x24
#define VIRTIO_MMIO_GUEST_PG_SZ   0x28
#define VIRTIO_MMIO_QUEUE_SEL     0x30
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x34
#define VIRTIO_MMIO_QUEUE_NUM     0x38
#define VIRTIO_MMIO_QUEUE_ALIGN   0x3C
#define VIRTIO_MMIO_QUEUE_PFN     0x40
#define VIRTIO_MMIO_QUEUE_READY   0x44
#define VIRTIO_MMIO_QUEUE_NOTIFY  0x50
#define VIRTIO_MMIO_INTERRUPT_STATUS        0x60
#define VIRTIO_MMIO_INTERRUPT_ACK        0x64
#define VIRTIO_MMIO_STATUS        0x70
#define VIRTIO_MMIO_QUEUE_DESC_LOW  0x80
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x84
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW 0x90
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH 0x94
#define VIRTIO_MMIO_QUEUE_USED_LOW  0xA0
#define VIRTIO_MMIO_QUEUE_USED_HIGH 0xA4
#define VIRTIO_MMIO_CONFIG          0x100

// Virtio 设备状态
#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER      2
#define VIRTIO_STATUS_FEATURES_OK 8
#define VIRTIO_STATUS_DRIVER_OK   4
#define VIRTIO_STATUS_FAILED      128

// Virtio 队列相关定义
#define VIRTIO_QUEUE_SIZE 32
#define VIRTIO_PAGE_SIZE  4096

// 描述符标志
#define VIRTQ_DESC_F_NEXT   (1 << 0) // 链接下一个描述符
#define VIRTQ_DESC_F_WRITE  (1 << 1) // 设备写入的内存

// 块设备请求类型
#define VIRTIO_BLK_T_IN     0
#define VIRTIO_BLK_T_OUT    1
#define VIRTIO_BLK_T_FLUSH  4
#define VIRTIO_BLK_OK       0
#define VIRTIO_BLK_IOERR    1
#define VIRTIO_BLK_UNSUPP   2

#define VIRTIO_ID_INPUT 18

#define VIRTIO_INPUT_CFG_ID_NAME    0x01
#define VIRTIO_INPUT_CFG_ID_SERIAL  0x02
#define VIRTIO_INPUT_CFG_ID_DEVIDS  0x03
#define VIRTIO_INPUT_CFG_PROP_BITS  0x10
#define VIRTIO_INPUT_CFG_EV_BITS    0x11
#define VIRTIO_INPUT_CFG_ABS_INFO   0x12

// 描述符结构
struct virtq_desc {
    uint64_t addr;  // 物理地址
    uint32_t len;   // 长度
    uint16_t flags; // 标志
    uint16_t next;  // 下一个描述符索引
} __attribute__((packed));

// 可用环结构
struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTIO_QUEUE_SIZE];
} __attribute__((packed));

// 已用环结构
struct virtq_used_elem {
    uint32_t id;    // 描述符链起始索引
    uint32_t len;   // 写入的总长度
} __attribute__((packed));

struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[VIRTIO_QUEUE_SIZE];
} __attribute__((packed));

// 块设备请求头
struct virtio_blk_req {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
} __attribute__((packed));

struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
} __attribute__((packed));

struct virtio_input_absinfo {
    uint32_t min;
    uint32_t max;
    uint32_t fuzz;
    uint32_t flat;
    uint32_t res;
};

struct virtio_input_devids {
    uint16_t bustype;
    uint16_t vendor;
    uint16_t product;
    uint16_t version;
};

struct virtio_input_config {
    uint8_t    select;
    uint8_t    subsel;
    uint8_t    size;
    uint8_t    reserved[5];
    union {
        char string[128];
        uint8_t bitmap[128];
        struct virtio_input_absinfo abs;
        struct virtio_input_devids ids;
    } u;
};

struct virtq_t {
    struct virtq_desc desc[VIRTIO_QUEUE_SIZE];
    struct virtq_avail avail;
    struct virtio_blk_req req;
    unsigned char buffer[4096 - (sizeof(struct virtq_desc) * VIRTIO_QUEUE_SIZE +
                        sizeof(struct virtq_avail) +
                        sizeof(struct virtio_blk_req))];
    struct virtq_used used;
    uint8_t pad[4096 - sizeof(struct virtq_used)];
};

// 设备状态结构
typedef struct virtio_device {
    uintptr_t base;
	struct virtq_t* virtq;
	uintptr_t phy;
    int queue_ready;
	uint64_t sector;
}virtio_dev_t;

#define VIRTIO_DEV_MAX 32
#define VIRTIO_BASE (_mmio_base + 0x02000000)
#define VIRTIO_TIMEOUT_MS  100000

static uint64_t get_phy_addr(struct virtio_device *dev, void* ptr){
	return dev->phy + (ptr - (void*)dev->virtq);
}

virtio_dev_t *virtio_get(int dev_id){
    for(int i = 0; i < VIRTIO_DEV_MAX; i++){
        uint32_t magic = get32(VIRTIO_BASE + i * 0x200 + VIRTIO_MMIO_MAGIC);
        uint32_t id = get32(VIRTIO_BASE + i * 0x200 + VIRTIO_MMIO_DEVICE_ID);
        uint32_t vendor = get32(VIRTIO_BASE + i * 0x200 + VIRTIO_MMIO_VENDOR_ID);
        uint32_t features = get32(VIRTIO_BASE + i * 0x200 + VIRTIO_MMIO_DEVICE_FEAT);
        //klog("i: %d, magic: 0x%x id: %d vender: 0x%x features: 0x%x\n", i, magic, id, vendor, features);
        if(magic != 0x74726976){
            continue;
        }

        if(id == dev_id){
            virtio_dev_t *dev = malloc(sizeof(virtio_dev_t));
            memset(dev, 0, sizeof(virtio_dev_t));
            dev->base = VIRTIO_BASE + i * 0x200;
            dev->virtq = dma_user_alloc(sizeof(struct virtq_t));
            dev->phy = dma_user_phy(dev->virtq);
            //klog("DMA alloc base:%08x v:%08x p:%08x\n", dev->base, dev->virtq, dev->phy);
            return dev;
        }
    }
    return 0;
}

int virtio_init(struct virtio_device *dev, uint32_t feature) {
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
    return 0;
}

int virtio_get_config(uint32_t base, uint8_t select, uint8_t subsel, void *buffer, uint32_t size){
    put32(base + VIRTIO_MMIO_CONFIG, select);
    put32(base + VIRTIO_MMIO_CONFIG + 4, subsel);
    put32(base + VIRTIO_MMIO_CONFIG + 8, size);
    proc_usleep(0);
    struct virtio_input_config *cfg = (struct virtio_input_config *)(base + VIRTIO_MMIO_CONFIG);

    if(buffer && size > 0){
        for(int i = 0; i < MIN(cfg->size, size); i++){
            ((uint8_t*)buffer)[i] = cfg->u.string[i];
        }
    }
    return cfg->size;
}


virtio_dev_t *virtio_input_get(const char* name){
 for(int i = 0; i < VIRTIO_DEV_MAX; i++){
        uint32_t base = VIRTIO_BASE + i * 0x200;
        uint32_t magic = get32(base + VIRTIO_MMIO_MAGIC);
        uint32_t id = get32(base + VIRTIO_MMIO_DEVICE_ID);
        uint32_t vendor = get32(base + VIRTIO_MMIO_VENDOR_ID);
        uint32_t features = get32(base + VIRTIO_MMIO_DEVICE_FEAT);  
        if(magic != 0x74726976 || id != VIRTIO_ID_INPUT){
            continue;
        }
        char id_name[128] = {0};
        virtio_get_config(base, VIRTIO_INPUT_CFG_ID_NAME, 0, id_name, sizeof(id_name));
        if(strcmp(id_name, name) == 0){
            virtio_dev_t *dev = malloc(sizeof(virtio_dev_t));
            memset(dev, 0, sizeof(virtio_dev_t));
            dev->base = VIRTIO_BASE + i * 0x200;
            dev->virtq = dma_user_alloc(sizeof(struct virtq_t));
            dev->phy = dma_user_phy(dev->virtq);
            //klog("DMA alloc base:%08x v:%08x p:%08x\n", dev->base, dev->virtq, dev->phy);
            return dev;
        }
    }
    return 0;
}

int virtio_input_read(virtio_dev_t *dev, void *buffer, uint32_t size){
    struct virtq_t *virtq = dev->virtq;
    volatile int used_idx = dev->virtq->used.idx;
    volatile int avail_idx = dev->virtq->avail.idx;
    uint32_t ret = 0;
    if(get32(dev->base + VIRTIO_MMIO_INTERRUPT_STATUS) & 0x1){
        put32(dev->base + VIRTIO_MMIO_INTERRUPT_ACK, 0x1);
        for(int i = avail_idx - VIRTIO_QUEUE_SIZE; i < used_idx; i++){
            struct virtio_input_event *event = (struct virtio_input_event *)(virtq->buffer + (i%VIRTIO_QUEUE_SIZE) * sizeof(struct virtio_input_event));
            memcpy(buffer + ret, event, sizeof(struct virtio_input_event));
            ret += sizeof(struct virtio_input_event);
        }
    }

    while(avail_idx - used_idx < VIRTIO_QUEUE_SIZE){
        virtq->desc[avail_idx % VIRTIO_QUEUE_SIZE].addr = get_phy_addr(dev, virtq->buffer + (avail_idx % VIRTIO_QUEUE_SIZE) * sizeof(struct virtio_input_event));
        virtq->desc[avail_idx % VIRTIO_QUEUE_SIZE].len = sizeof(struct virtio_input_event);
        virtq->desc[avail_idx % VIRTIO_QUEUE_SIZE].flags = VIRTQ_DESC_F_WRITE;
        virtq->desc[avail_idx % VIRTIO_QUEUE_SIZE].next = 0;
        virtq->avail.ring[avail_idx] = avail_idx % VIRTIO_QUEUE_SIZE;
        avail_idx++;
        virtq->avail.idx++;
    }

    put32(dev->base + VIRTIO_MMIO_QUEUE_NOTIFY, 0);
  
    return ret;
}

int virtio_blk_transfer(struct virtio_device *dev, uint64_t sector, void *buffer, uint32_t count, int isWrite) {
    uintptr_t base = dev->base;
	struct virtq_t *virtq = dev->virtq;

    virtq->req.type =  isWrite ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    virtq->req.sector = sector;

    virtq->pad[0] = 0xFF;
    uint8_t *buf_ptr = (uint8_t *)virtq->buffer;
    uint32_t bytes = count*512;
    uint16_t head_idx = 0;

    virtq->desc[head_idx].addr = get_phy_addr(dev, &virtq->req);
    virtq->desc[head_idx].len = sizeof(virtq->req);
    virtq->desc[head_idx].flags = VIRTQ_DESC_F_NEXT;
    virtq->desc[head_idx].next = head_idx + 1;

    virtq->desc[head_idx + 1].addr = get_phy_addr(dev, buf_ptr);
    virtq->desc[head_idx + 1].len = bytes;
    virtq->desc[head_idx + 1].flags = VIRTQ_DESC_F_NEXT|(isWrite ? 0 : VIRTQ_DESC_F_WRITE);
    virtq->desc[head_idx + 1].next = head_idx + 2;

    virtq->desc[head_idx + 2].addr = get_phy_addr(dev, &virtq->pad[0]);
    virtq->desc[head_idx + 2].len = sizeof(uint8_t);
    virtq->desc[head_idx + 2].flags = VIRTQ_DESC_F_WRITE;
    virtq->desc[head_idx + 2].next = 0;

    uint16_t avail_idx = virtq->avail.idx % VIRTIO_QUEUE_SIZE;
    virtq->avail.ring[avail_idx] = head_idx;
    virtq->avail.idx++;

    put32(base + VIRTIO_MMIO_QUEUE_NOTIFY, 0);
	uint32_t timeout = VIRTIO_TIMEOUT_MS;
    while (timeout--){
		uint32_t irq = get32(base + VIRTIO_MMIO_INTERRUPT_STATUS);
        //klog("irq: 0x%x used idx: %08x %d\n", irq, &virtq->used.idx, virtq->used.idx);
		if(irq & 0x1){
			put32(base + VIRTIO_MMIO_INTERRUPT_ACK, 0x1);
	        memcpy(buffer, buf_ptr, bytes);
			break;
		}
        proc_usleep(0);
    }

    memset(&virtq->desc[head_idx], 0, sizeof(struct virtq_desc) * 3);

    return  virtq->pad[0] == VIRTIO_BLK_OK ? 0 : -1;
}
