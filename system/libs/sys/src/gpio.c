#include <sys/gpio.h>
#include <fcntl.h>

int32_t gpio_config(int32_t fd, int32_t gpio_num, int32_t gpio_sel) {
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_int(&in, gpio_num);
	proto_add_int(&in, gpio_sel);
	int32_t res = fcntl_raw(fd, 0, &in, NULL); //0 for config
	proto_clear(&in);
	return res;
}

int32_t gpio_pull(int32_t fd, int32_t gpio_num, int32_t pull_dir) {
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_int(&in, gpio_num);
	proto_add_int(&in, pull_dir);
	int32_t res = fcntl_raw(fd, 1, &in, NULL); //1 for pull
	proto_clear(&in);
	return res;
}

int32_t gpio_write(int32_t fd, int32_t gpio_num, int32_t value) {
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_int(&in, gpio_num);
	proto_add_int(&in, value);
	int32_t res = fcntl_raw(fd, 2, &in, NULL); //2 for write
	proto_clear(&in);
	return res;
}

uint32_t gpio_read(int32_t fd, int32_t gpio_num) {
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);
	proto_add_int(&in, gpio_num);

	uint32_t res = 0;
	if(fcntl_raw(fd, 3, &in, &out) == 0) { //3 for read
		res = proto_read_int(&out);
	}
	proto_clear(&in);
	proto_clear(&out);
	return res;
}

