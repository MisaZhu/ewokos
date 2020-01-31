#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vprintf.h>
#include <string.h>

static char rpl(char c) {
	if(c < 0x20 || c > 0x7e || c == ' ')
		return '.';
	return c;
}

#define BUF_SIZE 128
int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	printf("      | 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 |\n");
	printf("      | ----------------------------------------------- |\n");
	int i = 0;
	int ln = 0;
	char out[128];
	char* p = out;
	while(1) {
		char buf[BUF_SIZE];
		char str[17];
		int res;
		res = read(0, buf, BUF_SIZE);
		if(res > 0) {
			int j;
			for(j=0; j<res; ++j) {
				if(i == 0) {
					snprintf(p, 16, "%6d| ", ln);
					p += 8;
				}
				char c = (char)buf[j];
				str[i] = rpl(c);
				snprintf(p, 4, "%02X ", (int)c);
				p += 3;
				i++;
				if(i == 16) {
					str[i] = 0;
					snprintf(p, 32, "| %s\n", str);
					printf("%s", out);
					p = out;
					i = 0;
					ln++;
				}
			}
		}
		else {
			if(errno != EAGAIN) {
				if(i > 0) {
					printf("%s", out);
					str[i] = 0;
					for(; i< 16; ++i)
						printf("   ");
					printf("| %s", str);
				}
				break;
			}
			sleep(0);
		}
	}
	printf("\n");
	return 0;
}
