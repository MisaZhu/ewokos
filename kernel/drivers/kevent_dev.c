#include <dev/basic_dev.h>
#include <system.h>

bool kevent_init() {
	return true;
}

#define KEVENT_BUF_SIZE 32

static char _buffer_data[KEVENT_BUF_SIZE];
static dev_buffer_t _kevent_buffer = { _buffer_data, KEVENT_BUF_SIZE, 0, 0 };
static int32_t _kevent_lock = 0;

void dev_kevent_push(uint32_t ev) {
	const char* p = (char*)&ev;
	CRIT_IN(_kevent_lock)
	dev_buffer_push(&_kevent_buffer, p[0], true);
	dev_buffer_push(&_kevent_buffer, p[1], true);
	dev_buffer_push(&_kevent_buffer, p[2], true);
	dev_buffer_push(&_kevent_buffer, p[3], true);
	CRIT_OUT(_kevent_lock)
}

int32_t dev_kevent_read(int16_t id, void* buf, uint32_t size) {
	(void)id;

	CRIT_IN(_kevent_lock)
	int32_t sz = size < _kevent_buffer.size ? size:_kevent_buffer.size;
	char* p = (char*)buf;	
	int32_t i = 0;
	while(i < sz) {
		char c;
		if(dev_buffer_pop(&_kevent_buffer, &c) != 0) 
			break;
		p[i] = c;
		i++;
	}
	CRIT_OUT(_kevent_lock)
	return i;	
}
