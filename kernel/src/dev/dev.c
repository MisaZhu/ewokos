#include "dev/initrd.h"
#include "kstring.h"

DeviceT* devByName(const char* name) {
	if(strcmp(name, KDEV_INITRD) == 0)
		return devInitrd();
	return NULL;
}
