#include "dev/initrd.h"

static DeviceT _initrdDev = {
	"",
	NULL, //bool (*mount)(TreeNodeT* node);
	NULL, //bool (*open)(TreeNodeT* node, int32_t flag);
	NULL, //bool (*close)(TreeNodeT* node);
	NULL, //bool (*info)(TreeNodeT* node, FSInfoT* info);
	NULL, //bool (*setopt)(TreeNodeT* node, int32_t cmd, int32_t value);
	NULL, //bool (*add)(TreeNodeT* node);
	NULL  //bool (*del)(TreeNodeT* node);
};

DeviceT* devInitrd() {
	return &_initrdDev;
}
