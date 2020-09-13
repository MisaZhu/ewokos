#include <arch/arm/bcm283x/mailbox.h>
#include <sys/vdevice.h>
#include <string.h>

static void actled(bool on) {
	mail_message_t msg;
	/*message head + tag head + property*/
	uint32_t size = 12 + 12 + 8;
	uint32_t buf[8] __attribute__((aligned(16)));

	/*message head*/
	buf[0] = size;
	buf[1] = 0;	//RPI_FIRMWARE_STATUS_REQUEST;
	/*tag head*/
	buf[2] = 0x00038041;								/*tag*/
	buf[3] = 8;									/*buffer size*/
	buf[4] = 0;									/*respons size*/
	/*property package*/
	buf[5] =  130;				/*actled pin number*/
	buf[6] =  on ? 1: 0;								/*property value*/
	/*message end*/
	buf[7] = 0;
	
	msg.data = ((uint32_t)buf + 0x40000000) >> 4;	
	mailbox_send(PROPERTY_CHANNEL, &msg);
	mailbox_read(PROPERTY_CHANNEL, &msg);
}

static int actled_write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)info;
	(void)from_pid;
	(void)offset;
	(void)p;

	if(size == 0 || ((const char*)buf)[0] == 0) {
		actled(false);
	}
	else {
		actled(true);
	}
	return size;
}

static int actled_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	(void)cmd;
	(void)ret;
	(void)p;

	if(proto_read_int(in) == 0) {
		actled(false);
	}
	else {
		actled(true);
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/actled";
	_mmio_base = mmio_map();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "actled");
	dev.write = actled_write;
	dev.dev_cntl = actled_dev_cntl;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
