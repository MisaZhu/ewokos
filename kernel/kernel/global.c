#include <kernel/global.h>
#include <kernel/kevqueue.h>
#include <kevent.h>

int32_t global_set(const char* key, proto_t* val) {
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_str(&in, key);
	if(val != NULL)
		proto_add(&in, val->data, val->size);
	kev_push(KEV_GLOBAL_SET, &in);
	proto_clear(&in);
	return 0;
}

int32_t global_set_int(const char* key, int32_t val) {
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_int(&in, val);
	global_set(key, &in);
	proto_clear(&in);
	return 0;
}

int32_t global_set_str(const char* key, const char* val) {
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_str(&in, val);
	global_set(key, &in);
	proto_clear(&in);
	return 0;
}


