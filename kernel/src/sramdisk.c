#include <sramdisk.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <base16.h>
#include <initfs.h>

void ramdiskClose(RamDiskT* rd, void (*fr)(void*)) {
	RamFileT* rf = rd->head;
	while(rf != NULL) {
		rd->head = rf->next;
		fr(rf);
		rf = rd->head;
	}
	rd->head = NULL;
}

void ramdiskOpen(const char*ram, RamDiskT* rd, void*(*alloc)(uint32_t)) {
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
		RamFileT* rf = (RamFileT*)alloc(sizeof(RamFileT));
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

char* decodeInitFS() {
	char* ret;
	char* p;
	ret = (char*)kmalloc(_initfsSize);
	int32_t i;
	int32_t sz;
	const char* s;

	p = ret;
	i = 0;
	while(1) {
		s = _initfs[i];
		if(s[0] == 0)
			break;

		base16Decode(s, strlen(s), p, &sz);
		p += sz;	
		i++;
	}
	return ret;
}
