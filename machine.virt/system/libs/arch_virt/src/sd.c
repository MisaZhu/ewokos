#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>
#include <ewoksys/mmio.h>
#include <arch/virt/virtio.h>

virtio_dev_t dev;

int32_t virt_sd_init(void) {
	_mmio_base = mmio_map();
    dev = virtio_get(VIRTIO_ID_BLOCK);
	if (!dev || virtio_init(dev, 0) != 0) {
        klog("Virtio-blk init failed\n");
        return -1;
    }
	return 0;
}

int32_t virt_sd_read_sector(int32_t sector, void* buf) {
	return virtio_blk_transfer(dev, sector, buf, 1, 0);
}

int32_t virt_sd_write_sector(int32_t sector, const void* buf) {
	return 0;
}
