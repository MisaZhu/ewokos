#include <sconf.h>
#include <vfs/fs.h>
#include <stdlib.h>

sconf_t* sconf_load(const char* fname) {
	int32_t size;
	char* str = fs_read_file(fname, &size);
	if(str == NULL || size == 0)
		return NULL;
	sconf_t* ret = sconf_parse(str, malloc);	
	free(str);
	return ret;
}
