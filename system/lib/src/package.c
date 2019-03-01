#include "package.h"
#include "kstring.h"
#include "stdlib.h"

PackageT* newPackage(int32_t id, uint32_t type, void* data, uint32_t size, int32_t pid) {
	PackageT* pkg = (PackageT*)malloc(sizeof(PackageT) + size);
	if(pkg == NULL)
		return NULL;

	pkg->id = id;
	pkg->pid = pid;
	pkg->size = 0;
	pkg->type = type;

	void* p = getPackageData(pkg);
	if(size > 0 && data != NULL)
		memcpy(p, data, size);

	pkg->size = size;
	return pkg;
}

void freePackage(PackageT* pkg) {
	if(pkg != NULL)
		free(pkg);
}

