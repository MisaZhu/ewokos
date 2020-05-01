#include <kernel/global.h>
#include <kernel/kevqueue.h>
#include <kevent.h>

int32_t global_set(const char* key, proto_t* val) {
	proto_t in;
	PF->init(&in, NULL, 0)->adds(&in, key);
	if(val != NULL)
		PF->add(&in, val->data, val->size);
	kev_push(KEV_GLOBAL_SET, &in);
	PF->clear(&in);
	return 0;
}

int32_t global_set_int(const char* key, int32_t val) {
	proto_t in;
	PF->init(&in, NULL, 0)->addi(&in, val);
	global_set(key, &in);
	PF->clear(&in);
	return 0;
}

int32_t global_set_str(const char* key, const char* val) {
	proto_t in;
	PF->init(&in, NULL, 0)->adds(&in, val);
	global_set(key, &in);
	PF->clear(&in);
	return 0;
}


