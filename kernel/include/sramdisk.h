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
	char name[FNAME_MAX];
	const char* content;
	int32_t size; 
} RamFileT;

typedef struct {
	RamFileT* head;
	const char* ram;
} RamDiskT;

void ramdiskOpen(const char*ram, RamDiskT* rd, void*(*alloc)(size_t));
void ramdiskClose(RamDiskT* rd, void (*fr)(void*));
/*
read file content of fname, return content address and size.
*/
const char* ramdiskRead(RamDiskT* rd, const char* fname, int* size);

#endif
