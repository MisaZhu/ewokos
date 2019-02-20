#include <unistd.h>
#include <pmessage.h>
#include <syscall.h>
#include <stdlib.h>
#include <kstring.h>
#include <kserv/devserv.h>
#include <stdio.h>

void _start() {
	DeviceT dev;
	devInit(&dev, "tty");
	devMount(&dev, 1, "/dev");

	if(!devServRun(&dev))
		exit(0);
}
