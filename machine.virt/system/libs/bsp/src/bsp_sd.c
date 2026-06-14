#include <bsp/bsp_sd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sysinfo.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <sd/sd.h>
#include <arch/virt/sd.h>

static void* cache_entry[4096] = {0};
static uint8_t* prefetch_buf = 0;
static uint32_t prefetch_buf_sectors = 0;

#define SD_CACHE_PAGE_SECTORS 8U
#define SD_CACHE_PAGE_SIZE (SD_CACHE_PAGE_SECTORS * 512U)

static void** bsp_sd_get_l3(uint32_t sector, int create) {
	uint32_t l1 = (sector >> 21) & 0x1FF;
	if(cache_entry[l1] == 0 && create)
		cache_entry[l1] = calloc(4096, 1);
	if(cache_entry[l1] == 0)
		return 0;

	void **l2_entry = cache_entry[l1];
	uint32_t l2 = (sector >> 12) & 0x1FF;
	if(l2_entry[l2] == 0 && create)
		l2_entry[l2] = calloc(4096, 1);
	if(l2_entry[l2] == 0)
		return 0;

	return l2_entry[l2];
}

static int bsp_sd_ensure_prefetch_buf(uint32_t sectors) {
	if(sectors <= prefetch_buf_sectors)
		return 0;
	uint8_t* new_buf = realloc(prefetch_buf, sectors * 512U);
	if(new_buf == 0)
		return -1;
	prefetch_buf = new_buf;
	prefetch_buf_sectors = sectors;
	return 0;
}

static int bsp_sd_fill_page_cache(uint32_t start_page, uint32_t page_count) {
	uint32_t start_sector = start_page * SD_CACHE_PAGE_SECTORS;
	uint32_t sectors = page_count * SD_CACHE_PAGE_SECTORS;

	if(page_count == 0)
		return 0;
	if(bsp_sd_ensure_prefetch_buf(sectors) != 0)
		return -1;
	if(virt_sd_read_blocks((int32_t)start_sector, prefetch_buf, sectors) != 0)
		return -1;

	for(uint32_t i = 0; i < page_count; i++) {
		uint32_t page = start_page + i;
		void **l3_entry = bsp_sd_get_l3(page * SD_CACHE_PAGE_SECTORS, 1);
		uint32_t l3 = page & 0x1FF;
		if(l3_entry == 0)
			return -1;
		if(l3_entry[l3] == 0) {
			l3_entry[l3] = malloc(SD_CACHE_PAGE_SIZE);
			if(l3_entry[l3] == 0)
				return -1;
		}
		memcpy(l3_entry[l3], prefetch_buf + (i * SD_CACHE_PAGE_SIZE), SD_CACHE_PAGE_SIZE);
	}
	return 0;
}

static int32_t bsp_sd_read_cache_sectors(int32_t sector, void* buf, uint32_t count) {
	uint8_t *out = (uint8_t*)buf;
	uint32_t current_sector = (uint32_t)sector;
	uint32_t remaining = count;

	while(remaining > 0) {
		uint32_t page = current_sector >> 3;
		uint32_t sector_offset = current_sector & (SD_CACHE_PAGE_SECTORS - 1);
		void **l3_entry = bsp_sd_get_l3(current_sector, 1);
		uint32_t l3 = page & 0x1FF;
		if(l3_entry == 0)
			return -1;

		if(l3_entry[l3] == 0) {
			uint32_t needed_pages = (sector_offset + remaining + SD_CACHE_PAGE_SECTORS - 1) / SD_CACHE_PAGE_SECTORS;
			uint32_t max_pages = 0x200 - l3;
			if(needed_pages > max_pages)
				needed_pages = max_pages;
			if(bsp_sd_fill_page_cache(page, needed_pages) != 0)
				return -1;
		}

		uint8_t *page_buf = l3_entry[l3];
		uint32_t sectors_to_copy = SD_CACHE_PAGE_SECTORS - sector_offset;
		if(sectors_to_copy > remaining)
			sectors_to_copy = remaining;
		memcpy(out, page_buf + (sector_offset * 512U), sectors_to_copy * 512U);
		out += sectors_to_copy * 512U;
		current_sector += sectors_to_copy;
		remaining -= sectors_to_copy;
	}
	return 0;
}

static int32_t bsp_sd_read_cache(int32_t sector, void* buf) {
	return bsp_sd_read_cache_sectors(sector, buf, 1);
}

static int32_t bsp_sd_write_cache(int32_t sector, const void* buf) {
	if(virt_sd_write_blocks(sector, buf, 1) != 0)
		return -1;

	void **l3_entry = bsp_sd_get_l3((uint32_t)sector, 0);
	if(l3_entry != 0) {
		uint8_t *page = l3_entry[((uint32_t)sector >> 3) & 0x1FF];
		if(page != 0)
			memcpy(page + (((uint32_t)sector & (SD_CACHE_PAGE_SECTORS - 1)) * 512U), buf, 512U);
	}
	return 0;
}

int bsp_sd_init(void) {
  sys_info_t sysinfo;
  syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
  int res = sd_init_ex(virt_sd_init, bsp_sd_read_cache, bsp_sd_read_cache_sectors, bsp_sd_write_cache);
  if(res == 0)
    sd_enable_sector_buffer(0);
	return res;
}
