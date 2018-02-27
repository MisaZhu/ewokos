#include <kramdisk.h>
#include <kmalloc.h>
#include <string.h>

void ramdiskClose(RamDiskT* rd) {
	RamFileT* rf = rd->head;
	while(rf != NULL) {
		rd->head = rf->next;
		kmfree(rf);
		rf = rd->head;
	}
	rd->head = NULL;
}

void ramdiskOpen(const char*ram, RamDiskT* rd) {
	rd->ram = ram;
	rd->head = NULL;

	while(1) {
		//read name len
		int32_t nameLen;
		memcpy(&nameLen, ram, 4);
		if(nameLen == 0) //end of disk
			break;
		ram += 4;
	
		//read name
		RamFileT* rf = (RamFileT*)kmalloc(sizeof(RamFileT));
		memcpy(rf->name, ram, nameLen);
		rf->name[nameLen] = 0;
		ram += nameLen;

		//read content len
		memcpy(&rf->size, ram, 4);
		ram += 4;
	
		//set content base
		rf->content = ram;
		ram += rf->size;

		rf->next = rd->head;
		rd->head = rf;
	}
}

/*
read file content of fname, return content address and size.
*/
const char* ramdiskRead(RamDiskT* rd, const char* fname, int* size) {
	if(rd == NULL)
		return NULL;

	RamFileT* rf = rd->head;
	while(rf != NULL) {
		if(strcmp(fname, rf->name) == 0) {
			*size = rf->size;
			return rf->content;
		}
		rf = rf->next;
	}
	return NULL;
}

