#include <pmessage.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <stdlib.h>

void _start() {
	int id = popen(1);
	printf("%d\n", id);

	int pid = fork();
	if(pid == 0) {
		const char* p = "hello test!";
		while(true) {
			int sz = psend(id, 1234, (void*)p, strlen(p)+1);
			printf("%d, %d\n", id, sz);
		}
	}
	else {	
		uint32_t i = 0;
		while(true) {
			uint32_t type, size;
			int32_t from;
			char* r =  (char*)precv(id, &type, &size);
			from = pgetPidW(id);
			printf("%d, %d, %d, %d, %d, %s\n", i++, id, from, type, size, r);

			if(r != NULL)
				free(r);
		}
		pclose(id);
		exit(0);
	}
	while(1);
}
