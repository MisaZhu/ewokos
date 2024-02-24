#include <unistd.h>
#include <stdio.h>
#include <ewoksys/vdevice.h>

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL);

	char* ret;
	if(argc < 3) {
		printf("Usage: devcmd <dev> <cmd>\n");
		return -1;
	}

	ret = dev_cmd(argv[1], argv[2]);
	if(ret == NULL) {
		printf("devcmd failed!\n");
		return -1;
	}

	printf("%s\n", ret);
	free(ret);
	return 0;
}
