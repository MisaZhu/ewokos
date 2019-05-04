#include <sconf.h>
#include <mm/kmalloc.h>

char* from_sd(const char *filename, int32_t* sz);

sconf_t* sconf_load(const char* fname) {
	int32_t size;
	char* str = from_sd(fname, &size);
	if(str == NULL || size == 0)
		return NULL;
	sconf_t* ret = sconf_parse(str, km_alloc);	
	km_free(str);
	return ret;
}
