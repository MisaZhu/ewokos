#include <sd/sd.h>
#include <sd/partition.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sysinfo.h>
#include <arch/bcm283x/sd.h>

#ifdef __cplusplus
extern "C" {
#endif


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
	//if(read_block(SD_DEV_PID, buf, SECTOR_SIZE, sector) == SECTOR_SIZE) {
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

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);

	if(strncmp(sysinfo.machine, "raspi", 5) == 0) {
		sd_init_arch = bcm283x_sd_init;
		sd_read_sector_arch = bcm283x_sd_read_sector;
		sd_write_sector_arch = bcm283x_sd_write_sector;
	}

	if(sd_init_arch() != 0)
		return -1;

	if(read_partition() != 0 || partition_get(1, &_partition) != 0) {
		memset(&_partition, 0, sizeof(partition_t));
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
