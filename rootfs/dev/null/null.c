#include <dev/devserv.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev_run(&dev, argc, argv);
	return 0;
}
