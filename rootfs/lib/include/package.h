#ifndef PKG_H
#define PKG_H

#include <types.h>

static const uint32_t PKG_TYPE_ERR = 0x0FFFFFFF;

typedef struct {
	int32_t id;
	int32_t pid;
	uint32_t type;
	uint32_t size;
} PackageT;

static inline uint32_t getPackageSize(PackageT* pkg) {
	if(pkg == NULL)
		return 0;
	return sizeof(PackageT) + pkg->size;	
}

static inline void* getPackageData(PackageT* pkg) {
	if(pkg == NULL) 
		return NULL;
	return (void*)(((char*)pkg) + sizeof(PackageT));
}

PackageT* newPackage(int32_t id, uint32_t type, void* data, uint32_t size, int32_t pid);

void freePackage(PackageT* pkg);

#endif
