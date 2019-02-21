#include "dev/initrd.h"
#include "kstring.h"

#define MAX_DEV 16

static DeviceT* _devices[MAX_DEV];

void devInit() {
	int32_t i;
	for(i=0; i<MAX_DEV; i++) {
		_devices[i] = NULL;
	}

	regDev(devInitrd());
}

DeviceT* devByName(const char* name) {
	int32_t i;
	for(i=0; i<MAX_DEV; i++) {
		DeviceT* dev = _devices[i];
		if(dev != NULL && strcmp(name, dev->name) == 0)
			return dev;
	}
	return NULL;
}

DeviceT* devByID(int32_t id) {
	if(id<0 || id>=MAX_DEV)
		return NULL;
	return _devices[id];
}

int32_t regDev(DeviceT* dev) {
	int32_t i;
	for(i=0; i<MAX_DEV; i++) {
		DeviceT* d = _devices[i];
		if(d != NULL) {
			if(strcmp(dev->name, d->name) == 0)
				return i;
		}
		else {
			_devices[i] = dev;
			return i;
		}
	}
	return -1;
}

