#include "package.h"
#include "kstring.h"
#include "stdlib.h"

package_t* pkg_new(int32_t id, uint32_t type, void* data, uint32_t size, int32_t pid) {
	package_t* pkg = (package_t*)malloc(sizeof(package_t) + size);
	if(pkg == NULL)
		return NULL;

	pkg->id = id;
	pkg->pid = pid;
	pkg->size = 0;
	pkg->type = type;

	void* p = get_pkg_data(pkg);
	if(size > 0 && data != NULL)
		memcpy(p, data, size);

	pkg->size = size;
	return pkg;
}

void pkg_free(package_t* pkg) {
	if(pkg != NULL)
		free(pkg);
}

