#include <sconf.h>
#include <sdread.h>
#include <mm/kmalloc.h>

sconf_t* sconf_load(const char* fname) {
	int32_t size;
	char* str = from_sd(fname, &size);
	if(str == NULL || size == 0)
		return NULL;
	sconf_t* ret = sconf_parse(str, KMFS);	
	km_free(str);
	return ret;
}
