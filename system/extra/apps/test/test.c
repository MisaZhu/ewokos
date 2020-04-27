#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/global.h>

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	int i=0;
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_str(&in, "hello!");
	while(1) {
		set_global("test.test", &in);	
		usleep(1000);
		proto_t* r = get_global("test.test");	
		if(r != NULL) {
			printf("ret:'%s'\n", proto_read_str(r));
		}
		i++;
	}
	proto_clear(&in);
  return 0;
}

