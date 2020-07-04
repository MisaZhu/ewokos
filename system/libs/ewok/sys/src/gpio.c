#include <sys/gpio.h>
#include <sys/vfs.h>

int32_t gpio_config(int32_t fd, int32_t gpio_num, int32_t gpio_sel) {
	proto_t in;
	PF->init(&in, NULL, 0)->addi(&in, gpio_num)->addi(&in, gpio_sel);
	int32_t res = vfs_fcntl(fd, 0, &in, NULL); //0 for config
	PF->clear(&in);
	return res;
}

int32_t gpio_pull(int32_t fd, int32_t gpio_num, int32_t pull_dir) {
	proto_t in;
	PF->init(&in, NULL, 0)->addi(&in, gpio_num)->addi(&in, pull_dir);
	int32_t res = vfs_fcntl(fd, 1, &in, NULL); //1 for pull
	PF->clear(&in);
	return res;
}

int32_t gpio_write(int32_t fd, int32_t gpio_num, int32_t value) {
	proto_t in;
	PF->init(&in, NULL, 0)->addi(&in, gpio_num)->addi(&in, value);
	int32_t res = vfs_fcntl(fd, 2, &in, NULL); //2 for write
	PF->clear(&in);
	return res;
}

uint32_t gpio_read(int32_t fd, int32_t gpio_num) {
	proto_t in, out;
	PF->init(&out, NULL, 0);
	PF->init(&in, NULL, 0)->addi(&in, gpio_num);

	uint32_t res = 0;
	if(vfs_fcntl(fd, 3, &in, &out) == 0) { //3 for read
		res = proto_read_int(&out);
	}
	PF->clear(&in);
	PF->clear(&out);
	return res;
}

