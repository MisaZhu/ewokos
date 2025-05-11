#include <sd/sd.h>
#include <sd/partition.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXT2_BLOCK_SIZE 1024
#define BUF_BLOCK_SIZE 1024

static partition_t _partition;

typedef struct {
	ewokos_addr_t* data;
	uint32_t refs;
	uint32_t counter; //for mark free priority
} sector_buf_block_t;

typedef struct {
	sector_buf_block_t* sector_buf;
	uint32_t max_sector_index;
	uint32_t sector_buffered;
	uint32_t max_buffer_num;
	uint32_t last_counter;
	uint32_t last_counter_check;
} sd_buffer_t;

static sd_buffer_t _sd_buffer;

static sector_buf_block_t* sector_buf_new(uint32_t num) {
	uint32_t block_num = num / BUF_BLOCK_SIZE;
	sector_buf_block_t* ret = (sector_buf_block_t*)malloc(sizeof(sector_buf_block_t)*block_num);
	memset(ret, 0, sizeof(sector_buf_block_t)*block_num);
	return ret;
}

static void sector_buf_free(sector_buf_block_t* buffer) {
	uint32_t block_num = _sd_buffer.max_sector_index / BUF_BLOCK_SIZE;
	while(block_num > 0) {
		if(buffer[block_num-1].data != NULL) {
			for(uint32_t i=0; i<BUF_BLOCK_SIZE; i++) {
				if(buffer[block_num-1].data[i] != 0)
					free((void*)buffer[block_num-1].data[i]);
			}
			free((void*)buffer[block_num-1].data);
		}
		block_num--;
	}
	free(buffer);
}

static inline void free_buffer_block_by_priority() {
	if((_sd_buffer.last_counter_check + 2) > _sd_buffer.last_counter)
		return;
	uint32_t counter_check = (_sd_buffer.last_counter - _sd_buffer.last_counter_check) / 2; //free half of buffer	

	uint32_t block_num = _sd_buffer.max_sector_index / BUF_BLOCK_SIZE;
	while(block_num > 0) {
		sector_buf_block_t* buf_block = &_sd_buffer.sector_buf[block_num-1];
		if(buf_block->data != NULL && 
				buf_block->counter < counter_check) {
			for(uint32_t i=0; i<BUF_BLOCK_SIZE; i++) {
				if(buf_block->data[i] != 0) {
					free((void*)buf_block->data[i]);
					buf_block->data[i] = 0;
					if(buf_block->refs > 0)
						buf_block->refs--;
					if(_sd_buffer.sector_buffered > 0)
						_sd_buffer.sector_buffered--;
				}
			}
			if(buf_block->refs == 0) {
				free((void*)buf_block->data);
				buf_block->data = 0;
			}
		}
		block_num--;
	}
	_sd_buffer.last_counter_check = counter_check;
}

static inline void sector_buf_set(uint32_t index, const void* data) {
	index -= _partition.start_sector;
	if(_sd_buffer.sector_buf == NULL || _sd_buffer.max_sector_index == 0 || index >= _sd_buffer.max_sector_index)
		return;

	if(_sd_buffer.sector_buffered >= _sd_buffer.max_buffer_num) { //overflowed, clear all buffer 
		free_buffer_block_by_priority();
		if(_sd_buffer.sector_buf == NULL)
			return;
	}

	uint32_t block_index = index / BUF_BLOCK_SIZE;
	index = index % BUF_BLOCK_SIZE;
	if(_sd_buffer.sector_buf[block_index].data == NULL) {
		uint32_t sz = sizeof(ewokos_addr_t) * BUF_BLOCK_SIZE;
		_sd_buffer.sector_buf[block_index].data = (ewokos_addr_t*)malloc(sz);
		if(_sd_buffer.sector_buf[block_index].data == NULL)
			return;
		memset(_sd_buffer.sector_buf[block_index].data, 0, sz);
	}

	if(_sd_buffer.sector_buf[block_index].data[index] == 0)
		_sd_buffer.sector_buf[block_index].data[index] = (ewokos_addr_t)malloc(SECTOR_SIZE);

	memcpy((void*)_sd_buffer.sector_buf[block_index].data[index], data, SECTOR_SIZE);
	_sd_buffer.sector_buf[block_index].refs++;
	_sd_buffer.last_counter++;
	_sd_buffer.sector_buf[block_index].counter = _sd_buffer.last_counter;
	_sd_buffer.sector_buffered++;
}

static inline void* sector_buf_get(uint32_t index) {
	index -= _partition.start_sector;
	if(_sd_buffer.sector_buf == NULL || _sd_buffer.sector_buffered == 0 || index >= _sd_buffer.max_sector_index) {
		return NULL;
	}

	uint32_t block_index = index / BUF_BLOCK_SIZE;
	index = index % BUF_BLOCK_SIZE;
	if(_sd_buffer.sector_buf[block_index].data == NULL)
		return NULL;

	return (void*)_sd_buffer.sector_buf[block_index].data[index];
}

//sd arch functions 
static int32_t (*sd_init_arch)(void);
static int32_t (*sd_read_sector_arch)(int32_t sector, void* buf);
static int32_t (*sd_write_sector_arch)(int32_t sector, const void* buf);

int32_t sd_read_sector(int32_t sector, void* buf) {
	void* b = sector_buf_get(sector);
	if(b != NULL) {
		memcpy(buf, b, SECTOR_SIZE);
		return SECTOR_SIZE;
	}	
	if(sd_read_sector_arch(sector, buf) == 0) {
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
}

int32_t sd_read(int32_t block, void* buf) {
	int32_t n = EXT2_BLOCK_SIZE/512;
	int32_t sector = block * n + _partition.start_sector;
	char* p = (char*)buf;

	while(n > 0) {
		if(sd_read_sector(sector, p) != SECTOR_SIZE) {
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
	if(sd_read_sector(0, sector) != SECTOR_SIZE)
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

int32_t sd_set_max_sector_index(uint32_t max_index) {
	_sd_buffer.sector_buf = sector_buf_new(max_index);
	_sd_buffer.max_sector_index = max_index;
	return 0;
}

void sd_set_buffer_size(uint32_t size) {
	_sd_buffer.max_buffer_num = size / SECTOR_SIZE;
	if(_sd_buffer.max_buffer_num > _sd_buffer.max_sector_index)
		_sd_buffer.max_buffer_num = _sd_buffer.max_sector_index;
}

int32_t sd_quit(void) {
	sector_buf_free(_sd_buffer.sector_buf);
	_sd_buffer.sector_buf = NULL;
	_sd_buffer.max_sector_index = 0;
	return 0;
}

int32_t sd_init(sd_init_func init, sd_read_sector_func rd, sd_write_sector_func wr) {
	memset(&_sd_buffer, 0, sizeof(sd_buffer_t));
	memset(&_partition, 0, sizeof(partition_t));

	sd_init_arch = init;
	sd_read_sector_arch = rd;
	sd_write_sector_arch = wr;

	if(sd_init_arch() != 0)
		return -1;

	_partition.start_sector = (uint32_t)get_rootfs_entry(sd_read_sector);
	return 0;
}

#ifdef __cplusplus
}
#endif
