#include <dev/devserv.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev_run(&dev, "dev.null", 0, "/dev/null", true);
	return 0;
}
