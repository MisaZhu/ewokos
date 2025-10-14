#include <stdio.h>
#include <stdint.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proc.h>
#include <arch/virt/dma.h>

#define BE16(x) __builtin_bswap16(x)
#define BE64(x) __builtin_bswap64(x)
#define BE32(x) __builtin_bswap32(x)

#define fw_cfg_base ((uint8_t*)(_mmio_base + 0x1020000))
#define selector_register ((uint16_t*)(fw_cfg_base + 8))
#define data_register ((uint64_t*)(fw_cfg_base + 0))
#define dma_address ((uint64_t*)(fw_cfg_base + 16))

struct fw_dma_access {
    uint32_t control;
    uint32_t length;
    uint64_t address;
}__attribute__((packed));

struct fw_file {
    uint32_t size;
    uint16_t select;
    uint16_t reserved;
    char path[56];
}__attribute__((packed));

struct fw_dir {
    uint32_t count;
    struct fw_file files[16];
}__attribute__((packed));

static struct fw_cfg_t{
	struct fw_dma_access dma;
	struct fw_dir dirs;
	uint8_t buffer[256];
}*fw_cfg_vaddr, *fw_cfg_paddr;

void fw_cfg_write_selector(uint16_t selector) {
    *selector_register = BE16(selector);
}

uint64_t fw_cfg_read_data() {
    return *data_register;
}

void fw_cfg_dma_transfer(void* address, uint32_t length, uint32_t control) {
    fw_cfg_vaddr->dma.control = BE32(control);
    fw_cfg_vaddr->dma.address = BE64((uint64_t)address);
    fw_cfg_vaddr->dma.length = BE32(length);

    *dma_address = BE64(&fw_cfg_paddr->dma);
    while (BE32(fw_cfg_vaddr->dma.control) & ~0x01){
		proc_usleep(0);
	}
}

void fw_cfg_dma_read(void* buf, int e, int length) {
    uint32_t control = (e << 16) | 0x08 | 0x02;
    fw_cfg_dma_transfer(buf, length, control);
}

void fw_cfg_dma_write(void* buf, int e, int length) {
    uint32_t control = (e << 16) | 0x08 | 0x10;
    fw_cfg_dma_transfer(buf, length, control);
}

struct fw_file *fw_file_get(const char* path) {
	struct fw_dir *dir = &fw_cfg_vaddr->dirs;
    int count = BE32(dir->count);

    for (int i = 0; i < count; i++) {
        struct fw_file* file = &dir->files[i];
        if (strcmp(path, file->path) == 0) {
            return file;
        }
    }
    return NULL;
}

int fw_init(){
	uint32_t count;
	fw_cfg_vaddr = dma_user_alloc(sizeof(struct fw_cfg_t));
	fw_cfg_paddr = dma_user_phy(fw_cfg_vaddr);
	fw_cfg_dma_read(&fw_cfg_paddr->dirs, 0x19, sizeof(struct fw_dir));
	count = BE32(fw_cfg_vaddr->dirs.count);
	uint64_t size = sizeof(struct fw_dir) + (sizeof(struct fw_file) * count);
    fw_cfg_dma_read(&fw_cfg_paddr->dirs, 0x19, size);
}

int fw_set_cfg(const char* path, void* cfg, int len){
	struct fw_file* f = fw_file_get("etc/ramfb");
	if(f && len < sizeof(fw_cfg_vaddr->buffer)){
		memcpy(fw_cfg_vaddr->buffer, cfg, len);
		fw_cfg_dma_write(&fw_cfg_paddr->buffer, BE16(f->select), len);
		return 0;
	}
	return -1;
}
