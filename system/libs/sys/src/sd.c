#include <sys/sd.h>
#include <dev/device.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <partition.h>
#include <string.h>
#include <unistd.h>

#define EXT2_BLOCK_SIZE 1024
#define SD_DEV_PID      1

static partition_t _partition;

typedef struct {
	uint32_t index;
	uint32_t* data;
} sector_buf_t;

static sector_buf_t* _sector_buf = NULL;
static uint32_t _sector_buf_num = 0;

static sector_buf_t* sector_buf_new(uint32_t num) {
	sector_buf_t* ret = (sector_buf_t*)malloc(sizeof(sector_buf_t)*num);
	memset(ret, 0, sizeof(sector_buf_t)*num);
	return ret;
}

static void sector_buf_free(sector_buf_t* buffer, uint32_t num) {
	while(num > 0) {
		if(buffer[num-1].data != NULL)
			free(buffer[num-1].data);
		num--;
	}
	free(buffer);
}

static inline void sector_buf_set(uint32_t index, const void* data) {
	index -= _partition.start_sector;
	if(_sector_buf == NULL || index >= _sector_buf_num)
		return;
	if(_sector_buf[index].data != NULL)
		free(_sector_buf[index].data);
	_sector_buf[index].data = malloc(SECTOR_SIZE);
	memcpy(_sector_buf[index].data, data, SECTOR_SIZE);
}

static inline void* sector_buf_get(uint32_t index) {
	index -= _partition.start_sector;
	if(_sector_buf == NULL || index >= _sector_buf_num) {
		return NULL;
	}
	return _sector_buf[index].data;
}


static int32_t read_sector(int32_t sector, void* buf) {
	if(syscall2(SYS_DEV_BLOCK_READ, DEV_SD, sector) != 0)
		return -1;
	while(1) {
		if(syscall2(SYS_DEV_BLOCK_READ_DONE, DEV_SD, (int32_t)buf)  == 0)
			break;
	}
	return 0;
}

static int32_t write_sector(int32_t sector, const void* buf) {
	if(syscall3(SYS_DEV_BLOCK_WRITE, DEV_SD, sector, (int32_t)buf) != 0)
		return -1;
	while(1) {
		if(syscall1(SYS_DEV_BLOCK_WRITE_DONE, DEV_SD)  == 0)
			break;
	}
	return 0;
}


int32_t sd_read_sector(int32_t sector, void* buf) {
	void* b = sector_buf_get(sector);
	if(b != NULL) {
		memcpy(buf, b, SECTOR_SIZE);
		return SECTOR_SIZE;
	}	
	//if(read_block(SD_DEV_PID, buf, SECTOR_SIZE, sector) == SECTOR_SIZE) {
	if(read_sector(sector, buf) == 0) {
		sector_buf_set(sector, buf);
		return SECTOR_SIZE;
	}
	return 0;
}

int32_t sd_write_sector(int32_t sector, const void* buf) {
	if(write_sector(sector, buf) == 0) 
		return SECTOR_SIZE;
	return 0;
	//return write_block(SD_DEV_PID, buf, SECTOR_SIZE, sector);
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

int32_t sd_init(void) {
	_sector_buf = NULL;
	_sector_buf_num = 0;
	memset(&_partition, 0, sizeof(partition_t));

	if(read_partition() != 0 || partition_get(1, &_partition) != 0) {
		memset(&_partition, 0, sizeof(partition_t));
	}
	return 0;
}
