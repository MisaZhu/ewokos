#ifndef SIMPLE_RAMDISK_H
#define SIMPLE_RAMDISK_H

/*
very simple read only RAM file system for kernel.
	disk: [file0][file1][file...]
	file: [name_len: 4][name: name_len][content_len][content: content_len]
*/

#include <types.h>

typedef struct RamFile {
	struct RamFile* next;
	char name[NAME_MAX];
	const char* content;
	int32_t size; 
} ram_file_t;

typedef struct {
	ram_file_t* head;
	const char* ram;
} ram_disk_t;

bool ram_disk_has(ram_disk_t* rd, const char* fname);
void ram_disk_open(const char*ram, ram_disk_t* rd, void*(*alloc)(uint32_t));
void ram_disk_close(ram_disk_t* rd, void (*fr)(void*));
/*
read file content of fname, return content address and size.
*/
const char* ram_disk_read(ram_disk_t* rd, const char* fname, int* size);

#endif
