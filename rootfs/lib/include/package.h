#ifndef PKG_H
#define PKG_H

#include <types.h>

static const uint32_t PKG_TYPE_ERR   = 0x0FFFFFFe;
static const uint32_t PKG_TYPE_AGAIN = 0x0FFFFFFF;

typedef struct {
	int32_t id;
	int32_t pid;
	uint32_t type;
	uint32_t size;
} package_t;

static inline uint32_t get_pkg_size(package_t* pkg) {
	if(pkg == NULL)
		return 0;
	return sizeof(package_t) + pkg->size;	
}

static inline void* get_pkg_data(package_t* pkg) {
	if(pkg == NULL) 
		return NULL;
	return (void*)(((char*)pkg) + sizeof(package_t));
}

package_t* pkg_new(int32_t id, uint32_t type, void* data, uint32_t size, int32_t pid);

void pkg_free(package_t* pkg);

#endif
