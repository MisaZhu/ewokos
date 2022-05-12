#include <sys/timer.h>
#include <sys/vdevice.h>
#include <sys/proto.h>

uint32_t timer_set(uint32_t usec, timer_handle_t handle) {
	uint32_t id = 0;
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->addi(&in, 1000000)->addi(&in, (uint32_t)handle);
	if(dev_cntl("/dev/timer", 0, &in, &out) == 0)
		id = (uint32_t)proto_read_int(&out);
	PF->clear(&in);
	PF->clear(&out);
	return id;
}

void timer_remove(uint32_t id) {
	proto_t in;
	PF->init(&in)->addi(&in, id);
	dev_cntl("/dev/timer", 1, &in, NULL);
	PF->clear(&in);
}