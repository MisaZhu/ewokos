#include <unistd.h>
#include <pmessage.h>
#include <syscall.h>
#include <stdlib.h>
#include <kstring.h>
#include <kserv/devserv.h>
#include <stdio.h>

void _start() {
	DeviceT dev;
	devInit(&dev);
<<<<<<< HEAD
	devMount(&dev, 1, "/dev/tty1", true);
=======
	devMount(1, "/dev/tty1", true);
>>>>>>> 91c114703b5ce9d3ff7972f6c78437a9d82a3447

	if(!devServRun("kserv.tty", &dev))
		exit(0);
}
