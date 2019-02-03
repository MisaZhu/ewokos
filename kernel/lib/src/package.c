#include <package.h>
#include <string.h>

/*PackageT* newPackage(uint32_t type, void* data, uint32_t size) {
	PackageT* pkg = (PackageT*)malloc(sizeof(PackageT) + size);
	if(pkg == NULL)
		return NULL;

	pkg->id = -1;
	pkg->pid = -1;
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
*/

uint32_t getPackageSize(PackageT* pkg) {
	if(pkg == NULL)
		return 0;
	return sizeof(PackageT) + pkg->size;	
}

void* getPackageData(PackageT* pkg) {
	if(pkg == NULL) 
		return NULL;
	return (void*)(((char*)pkg) + sizeof(PackageT));
}



