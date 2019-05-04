#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>

static char rpl(char c) {
	if(c < 0x20 || c > 0x7e)
		return ' ';
	return c;
}

int main(int argc, char* argv[]) {
	char type = 'c';
	if(argc >= 2)
		type = argv[1][0];

	int res;
	printf("      0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15\n");
	printf("      --------------------------------------------------------------");
	int i = 0;
	int ln = 0;
	while(true) { //non-block
		char c;
		res = read(0, &c, 1);
		if(res > 0) {
			if(i == 0)
				printf("\n%4d| ", ln++);
			if(type == 'd')
				printf("%4d", (int)c);
			else if(type == 'x')
				printf("%4x", (int)c);
			else
				printf("%c   ", rpl(c));
			i++;
			if(i == 16)
				i = 0;
		}
		else {
			if(errno != EAGAIN)
				break;
		}
	}
	printf("\n");
	return 0;
}

