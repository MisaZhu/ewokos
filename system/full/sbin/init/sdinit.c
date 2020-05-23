#include "sdinit.h"
#include <sys/syscall.h>
#include <sys/sd.h>
#include <stdlib.h>
#include <partition.h>
#include <string.h>

#define EXT2_BLOCK_SIZE 1024
static partition_t _partition;

int32_t sdinit_read(int32_t block, void* buf) {
  int32_t n = EXT2_BLOCK_SIZE/512;
  int32_t sector = block * n + _partition.start_sector;
  char* p = (char*)buf;

	while(n > 0) {
		if(sd_read_sector_arch(sector, p) != 0) {
			return -1;
		}
		sector++;
		n--;
		p += 512;
	}
	return 0;
}

#define PARTITION_MAX 4
static partition_t _partitions[PARTITION_MAX];

static int32_t read_partition(void) {
	uint8_t sector[512];
	if(sd_read_sector_arch(0, sector) != 0)
		return -1;
	//check magic 
	if(sector[510] != 0x55 || sector[511] != 0xAA) 
		return -1;

	memcpy(_partitions, sector+0x1BE, sizeof(partition_t)*PARTITION_MAX);
	return 0;
}

static int32_t partition_get(uint32_t id, partition_t* p) {
	if(id >= PARTITION_MAX)
		return -1;
	memcpy(p, &_partitions[id], sizeof(partition_t));
	return 0;
}

int32_t sdinit_quit(void) {
	return 0;
}

int32_t sdinit_init(void) {
	if(read_partition() != 0 || partition_get(1, &_partition) != 0) {
		memset(&_partition, 0, sizeof(partition_t));
	}
	return 0;
}
