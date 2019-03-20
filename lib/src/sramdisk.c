#include <sramdisk.h>
#include <kstring.h>

void ram_disk_close(ram_disk_t* rd, void (*fr)(void*)) {
	ram_file_t* rf = rd->head;
	while(rf != NULL) {
		rd->head = rf->next;
		fr(rf);
		rf = rd->head;
	}
	rd->head = NULL;
}

void ram_disk_open(const char*ram, ram_disk_t* rd, void*(*alloc)(uint32_t)) {
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
		ram_file_t* rf = (ram_file_t*)alloc(sizeof(ram_file_t));
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

bool ram_disk_has(ram_disk_t* rd, const char* fname) {
	int sz;
	const char* p = ram_disk_read(rd, fname, &sz);
	return p == NULL ? false : true;
}

/*
read file content of fname, return content address and size.
*/
const char* ram_disk_read(ram_disk_t* rd, const char* fname, int* size) {
	if(rd == NULL)
		return NULL;

	ram_file_t* rf = rd->head;
	while(rf != NULL) {
		if(strcmp(fname, rf->name) == 0) {
			*size = rf->size;
			return rf->content;
		}
		rf = rf->next;
	}
	return NULL;
}

