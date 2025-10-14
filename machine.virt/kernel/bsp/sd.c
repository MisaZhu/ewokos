#include <kernel/system.h>
#include <mm/mmu.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <kernel/kernel.h>
#include <kernel/proc.h>
#include <kernel/hw_info.h>
#include <kernel/system.h>
#include <dev/sd.h>
#include <kprintf.h>

#include <stdint.h>

#include <stdint.h>
#include <stddef.h>

// Virtio MMIO寄存器定义
#define VIRTIO_MMIO_MAGIC         0x00
#define VIRTIO_MMIO_VERSION       0x04
#define VIRTIO_MMIO_DEVICE_ID     0x08
#define VIRTIO_MMIO_VENDOR_ID     0x0C
#define VIRTIO_MMIO_DEVICE_FEAT   0x10
#define VIRTIO_MMIO_DRIVER_FEAT   0x20
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

// Virtio 设备状态
#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER      2
#define VIRTIO_STATUS_FEATURES_OK 8
#define VIRTIO_STATUS_DRIVER_OK   4
#define VIRTIO_STATUS_FAILED      128

// Virtio 队列相关定义
#define VIRTIO_QUEUE_SIZE 8
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

struct virtq_t {
    struct virtq_desc desc[VIRTIO_QUEUE_SIZE];
    struct virtq_avail avail;
    struct virtq_used used;
    struct virtio_blk_req req;
    unsigned char buffer[512];
    uint8_t pad[8192 - (sizeof(struct virtq_desc) * VIRTIO_QUEUE_SIZE + 
                        sizeof(struct virtq_avail) + 
                        sizeof(struct virtq_used) +
                        sizeof(struct virtio_blk_req) + 512)];
};

// 设备状态结构
struct virtio_device {
    uintptr_t base;
	struct virtq_t* virtq;
	uintptr_t phy;
    int queue_ready;
	uint64_t sector;
};

static inline void mmio_write(uintptr_t addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
}

static inline uint32_t mmio_read(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

// 全局设备实例
static struct virtio_device virtio_dev;

uint64_t get_phy_addr(struct virtio_device *dev, void* ptr){
	return dev->phy + (ptr - (void*)dev->virtq);
}

// 初始化virtio-blk设备
int virtio_blk_init(struct virtio_device *dev) {
    uint32_t base = dev->base;
	uint32_t paddr = dev->phy;

    //printf("MAGIC:   %08x\n", mmio_read(base + VIRTIO_MMIO_MAGIC));
    //printf("VERSION: %08x\n", mmio_read(base + VIRTIO_MMIO_VERSION));
    //printf("ID:      %08x\n", mmio_read(base + VIRTIO_MMIO_DEVICE_ID));
    
    mmio_write(base + VIRTIO_MMIO_STATUS, 0);
    dev->queue_ready = 0;

    mmio_write(base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    mmio_write(base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_DRIVER);

    uint32_t features = mmio_read(base + VIRTIO_MMIO_DEVICE_FEAT);
    mmio_write(base + VIRTIO_MMIO_DRIVER_FEAT, features & 0);

    uint32_t status = mmio_read(base + VIRTIO_MMIO_STATUS);
    mmio_write(base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_FEATURES_OK);
  
    mmio_write(base + VIRTIO_MMIO_QUEUE_SEL, 0);
    uint32_t max_queue_size = mmio_read(base + VIRTIO_MMIO_QUEUE_NUM_MAX);
    
    mmio_write(base + VIRTIO_MMIO_QUEUE_NUM, 8);

    mmio_write(base + VIRTIO_MMIO_GUEST_PG_SZ, VIRTIO_PAGE_SIZE);
    mmio_write(base + VIRTIO_MMIO_QUEUE_PFN, paddr >> 12);

    status = mmio_read(base + VIRTIO_MMIO_STATUS);
    mmio_write(base + VIRTIO_MMIO_STATUS, status | VIRTIO_STATUS_DRIVER_OK);
    virtio_dev.queue_ready = 1;

    return 0;
}

int virtio_blk_rw(struct virtio_device *dev, void *buffer, uint32_t count, int write) {

    uintptr_t base = dev->base;
	struct virtq_t *virtq = dev->virtq;
    
    virtq->req.type =  write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    virtq->req.sector = dev->sector;

    virtq->pad[0] = 0xFF;
    uint8_t *buf_ptr = (uint8_t *)virtq->buffer;
    uint32_t bytes = count * 512;

    uint16_t head_idx = 0;
    
    virtq->desc[head_idx].addr = get_phy_addr(dev, &virtq->req);
    virtq->desc[head_idx].len = sizeof(virtq->req);
    virtq->desc[head_idx].flags = VIRTQ_DESC_F_NEXT;
    virtq->desc[head_idx].next = head_idx + 1;
    
    virtq->desc[head_idx + 1].addr = get_phy_addr(dev, buf_ptr);
    virtq->desc[head_idx + 1].len = bytes;
    virtq->desc[head_idx + 1].flags = VIRTQ_DESC_F_NEXT|(write ? 0 : VIRTQ_DESC_F_WRITE);
    virtq->desc[head_idx + 1].next = head_idx + 2;
    
    virtq->desc[head_idx + 2].addr = get_phy_addr(dev, &virtq->pad[0]);
    virtq->desc[head_idx + 2].len = sizeof(uint8_t);
    virtq->desc[head_idx + 2].flags = VIRTQ_DESC_F_WRITE;
    virtq->desc[head_idx + 2].next = 0;
    
    uint16_t avail_idx = virtq->avail.idx % VIRTIO_QUEUE_SIZE;
    virtq->avail.ring[avail_idx] = head_idx;
    virtq->avail.idx++;
   
    mmio_write(base + VIRTIO_MMIO_QUEUE_NOTIFY, 0);
	uint32_t timeout = 10000;
    while (timeout--){
		uint32_t irq = mmio_read(base + VIRTIO_MMIO_INTERRUPT_STATUS);
		if(irq & 0x1){
			mmio_write(base + VIRTIO_MMIO_INTERRUPT_ACK, 0x1);
	        memcpy(buffer, buf_ptr, bytes);
			break;
		}
        for (volatile int i = 0; i < 1000; i++);
    }
    
    memset(&virtq->desc[head_idx], 0, sizeof(struct virtq_desc) * 3);
   
    return  virtq->pad[0] == VIRTIO_BLK_OK ? 0 : -1;
}

/****************************************************************************/

static struct virtio_device device;

static int plat_dma_alloc(void** vaddr, void** paddr, int size){
	static __attribute__((__aligned__(PAGE_DIR_SIZE))) struct virtq_t virtq;
	memset(&virtq, 0, sizeof(virtq));
	ewokos_addr_t phy = (ewokos_addr_t)V2P(&virtq);
    map_pages_size(_kernel_vm, &virtq, phy, sizeof(virtq), AP_RW_D, PTE_ATTR_DEV);
    flush_tlb();
	*vaddr = &virtq;
	*paddr = phy;
    //printf("DMA alloc p:%08x v:%08x size:%d\n", *paddr, *vaddr, size);
}

int32_t sd_init(void){
    // QEMU默认virtio-blk地址
    uintptr_t base = MMIO_BASE + 0x02003e00;

   	memset(&device, 0, sizeof(device));
	device.base = base;
	plat_dma_alloc(&device.virtq, &device.phy, 8192);

    if (virtio_blk_init(&device) != 0) {
        printf("Virtio-blk init failed\n");
        return -1;
    }
    return 0;
}

int32_t sd_dev_read(int32_t sector) {
	device.sector = sector;
	return 0;
}

int32_t sd_dev_read_done(void* buffer) {
    return virtio_blk_rw(&device, buffer, 1, 0);
}

int32_t sd_dev_write(int32_t sector, const void* buf) {
	return 0;
}

int32_t sd_dev_write_done(void) {
	return 0;
}
