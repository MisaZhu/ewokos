#include <sd/sd.h>
#include <sd/partition.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


#define EXT2_BLOCK_SIZE 1024
#define SD_DEV_PID      1

static partition_t _partition;

#define BUF_BLOCK_SIZE 1024

typedef struct {
	ewokos_addr_t* data;
	uint32_t refs;
} sector_buf_block_t;

static sector_buf_block_t* _sector_buf = NULL;
static uint32_t _sector_buf_num = 0;

static sector_buf_block_t* sector_buf_new(uint32_t num) {
	uint32_t block_num = num / BUF_BLOCK_SIZE;
	sector_buf_block_t* ret = (sector_buf_block_t*)malloc(sizeof(sector_buf_block_t)*block_num);
	memset(ret, 0, sizeof(sector_buf_block_t)*block_num);
	return ret;
}

static void sector_buf_free(sector_buf_block_t* buffer, uint32_t num) {
	uint32_t block_num = num / BUF_BLOCK_SIZE;
	while(block_num > 0) {
		if(buffer[block_num-1].data != NULL) {
			for(uint32_t i=0; i<BUF_BLOCK_SIZE; i++) {
				if(buffer[block_num-1].data[i] != NULL)
					free((void*)buffer[block_num-1].data[i]);
			}
			free((void*)buffer[block_num-1].data);
		}
		block_num--;
	}
	free(buffer);
}

static inline void sector_buf_set(uint32_t index, const void* data) {
	index -= _partition.start_sector;
	if(_sector_buf == NULL || index >= _sector_buf_num)
		return;

	uint32_t block_index = index / BUF_BLOCK_SIZE;
	index = index % BUF_BLOCK_SIZE;
	if(_sector_buf[block_index].data == NULL) {
		uint32_t sz = sizeof(ewokos_addr_t) * BUF_BLOCK_SIZE;
		_sector_buf[block_index].data = (ewokos_addr_t*)malloc(sz);
		if(_sector_buf[block_index].data == NULL)
			return;
		memset(_sector_buf[block_index].data, 0, sz);
	}

	_sector_buf[block_index].data[index] = (ewokos_addr_t)malloc(SECTOR_SIZE);
	memcpy((void*)_sector_buf[block_index].data[index], data, SECTOR_SIZE);
	_sector_buf[block_index].refs++;
}

static inline void* sector_buf_get(uint32_t index) {
	index -= _partition.start_sector;
	if(_sector_buf == NULL || index >= _sector_buf_num) {
		return NULL;
	}

	uint32_t block_index = index / BUF_BLOCK_SIZE;
	index = index % BUF_BLOCK_SIZE;
	if(_sector_buf[block_index].data == NULL)
		return NULL;

	return (void*)_sector_buf[block_index].data[index];
}

//sd arch functions 
static int32_t (*sd_init_arch)(void);
static int32_t (*sd_read_sector_arch)(int32_t sector, void* buf);
static int32_t (*sd_write_sector_arch)(int32_t sector, const void* buf);

int32_t sd_read_sector(int32_t sector, void* buf, uint32_t cache) {
	void* b = sector_buf_get(sector);
	if(b != NULL) {
		memcpy(buf, b, SECTOR_SIZE);
		return SECTOR_SIZE;
	}	
	//if(read_block(SD_DEV_PID, buf, SECTOR_SIZE, sector) == SECTOR_SIZE) {
	if(sd_read_sector_arch(sector, buf) == 0) {
		if(cache != 0)
			sector_buf_set(sector, buf);
		return SECTOR_SIZE;
	}
	return 0;
}

int32_t sd_write_sector(int32_t sector, const void* buf) {
	if(sd_write_sector_arch(sector, buf) == 0)  {
		sector_buf_set(sector, buf);
		return SECTOR_SIZE;
	}
	return 0;
	//return write_block(SD_DEV_PID, buf, SECTOR_SIZE, sector);
}

int32_t sd_read(int32_t block, void* buf, uint32_t cache) {
	int32_t n = EXT2_BLOCK_SIZE/512;
	int32_t sector = block * n + _partition.start_sector;
	char* p = (char*)buf;

	while(n > 0) {
		if(sd_read_sector(sector, p, cache) != SECTOR_SIZE) {
			return -1;
		}
		sector++;
		n--;
		p += 512;
	}
	return 0;
}

int32_t sd_write(int32_t block, const void* buf) {
	int32_t n = EXT2_BLOCK_SIZE/512;
	int32_t sector = block * n + _partition.start_sector;
	const char* p = (char*)buf;

	while(n > 0) {
		if(sd_write_sector(sector, p) != SECTOR_SIZE)
			return -1;
		sector++;
		n--;
		p += 512;
	}
	return 0;
}

#define PARTITION_MAX 4
static partition_t _partitions[PARTITION_MAX];

int32_t read_partition(void) {
	uint8_t sector[512];
	if(sd_read_sector(0, sector, true) != SECTOR_SIZE)
		return -1;
	//check magic 
	if(sector[510] != 0x55 || sector[511] != 0xAA) 
		return -1;

	memcpy(_partitions, sector+0x1BE, sizeof(partition_t)*PARTITION_MAX);
	return 0;
}

int32_t partition_get(uint32_t id, partition_t* p) {
	if(id >= PARTITION_MAX)
		return -1;
	memcpy(p, &_partitions[id], sizeof(partition_t));
	return 0;
}

int32_t sd_set_buffer(uint32_t sector_num) {
	_sector_buf = sector_buf_new(sector_num);
	_sector_buf_num = sector_num;
	return 0;
}

int32_t sd_quit(void) {
	sector_buf_free(_sector_buf, _sector_buf_num);
	_sector_buf = NULL;
	_sector_buf_num = 0;
	return 0;
}

static int32_t sd_read_sector_cache(int32_t sector, void* buf) {
	return sd_read_sector(sector, buf, 1);
}

int32_t sd_init(sd_init_func init, sd_read_sector_func rd, sd_write_sector_func wr) {
	_sector_buf = NULL;
	_sector_buf_num = 0;
	memset(&_partition, 0, sizeof(partition_t));

	sd_init_arch = init;
	sd_read_sector_arch = rd;
	sd_write_sector_arch = wr;

	if(sd_init_arch() != 0)
		return -1;

	_partition.start_sector = (uint32_t)get_rootfs_entry(sd_read_sector_cache);
	return 0;
}

#ifdef __cplusplus
}
#endif
