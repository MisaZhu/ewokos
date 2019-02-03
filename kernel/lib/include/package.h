#ifndef PKG_H
#define PKG_H

#include <types.h>

static const uint32_t PKG_TYPE_ERR = 0x0FFFFFFF;

typedef struct {
	int id;
	int pid;
	uint32_t type;
	uint32_t size;
} PackageT;

//PackageT* newPackage(uint32_t type, void* data, uint32_t size);
//void freePackage(PackageT* pkg);
uint32_t getPackageSize(PackageT* pkg);
void* getPackageData(PackageT* pkg);

#endif
