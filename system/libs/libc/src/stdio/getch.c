#include <stdio.h>
#include <unistd.h>

int getch(void) {
	while(1) {
		char c;
		int i = read(0, &c, 1);
		if(i == 1) {
			return c;
		}
		if(i <= 0 && errno != EAGAIN)
			break;
	}
	return 0;
}

