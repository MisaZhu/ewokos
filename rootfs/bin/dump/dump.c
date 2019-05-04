#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>

static void help() {
	printf(
			"usage: dump [OPTION]\n"
			"   -c : dump ascii\n"
			"   -d : dump decimal\n"
			"   -x : dump hex\n");
}

static char rpl(char c) {
	if(c < 0x20 || c > 0x7e)
		return ' ';
	return c;
}

int main(int argc, char* argv[]) {
	char arg = 0;
	if(argc >= 2) 
		arg = argv[1][1];
	if(arg == 0)
		arg = 'x';
	if(arg == 'h') {
		help();
		return 0;
	}
				
	int res;
	printf("      0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15\n");
	printf("      --------------------------------------------------------------");
	int i = 0;
	int ln = 0;
	while(true) { //non-block
		char buf[32];
		res = read(0, buf, 32);
		if(res > 0) {
			int j;
			for(j=0; j<res; ++j) {
				if(i == 0)
					printf("\n%4d| ", ln++);

				if(arg == 'd')
					printf("%4d", (int)buf[j]);
				else if(arg == 'x')
					printf("%4x", (int)buf[j]);
				else
					printf("%c   ", rpl(buf[j]));
				i++;
				if(i == 16)
					i = 0;
			}
		}
		else {
			if(errno != EAGAIN)
				break;
		}
	}
	printf("\n");
	return 0;
}

