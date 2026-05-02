#include <stdint.h>
#include <string.h>

#include <arch/virt/virtio_net.h>
#include <ewoksys/proto.h>
#include <ewoksys/vdevice.h>

static virtio_dev_t _net = NULL;

static int net_read(int fd, int from_pid, fsinfo_t *info,
					void *buf, int size, int offset, void *p)
{
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)p;

	int ret = virtio_net_read(_net, (uint8_t *)buf + offset, (uint32_t)size);
	if (ret == 0 && size > 0)
	{
		return VFS_ERR_RETRY;
	}
	return ret;
}

static int net_write(int fd, int from_pid, fsinfo_t *info,
					 const void *buf, int size, int offset, void *p)
{
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)p;

	int ret = virtio_net_write(_net, (const uint8_t *)buf + offset, (uint32_t)size);
	if (ret == 0 && size > 0)
	{
		return VFS_ERR_RETRY;
	}
	return ret;
}

static int net_dcntl(int from_pid, int cmd, proto_t *in, proto_t *ret, void *p)
{
	(void)from_pid;
	(void)in;
	(void)p;

	switch (cmd)
	{
	case 0: /* get mac */
	{
		uint8_t mac[6] = {0};
		if (virtio_net_read_mac(_net, mac) == 0)
		{
			PF->add(ret, mac, sizeof(mac));
		}
		break;
	}
	case 1: /* get pending rx count */
		PF->addi(ret, virtio_net_pending_rx(_net));
		break;
	default:
		break;
	}

	return 0;
}

int main(int argc, char **argv)
{
	const char *mnt_point = argc > 1 ? argv[1] : "/dev/eth0";

	_net = virtio_net_get();
	if (_net == NULL)
	{
		return -1;
	}

	if (virtio_net_init(_net) != 0)
	{
		return -1;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "virtio-net");
	dev.read = net_read;
	dev.write = net_write;
	dev.dev_cntl = net_dcntl;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
