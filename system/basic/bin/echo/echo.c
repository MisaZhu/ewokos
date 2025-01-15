#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

char* parse(const char* s, uint32_t* rlen) {
	*rlen = 0;
	uint32_t len = strlen(s);
	if(len == 0)
		return NULL;
	char* ret = (char*)malloc(len+1);	
	memset(ret, 0, len+1);

	uint8_t esc = 0;
	uint32_t j = 0;
	for(uint32_t i=0; i<len; i++) {
		if(s[i] == '\\' && esc == 0) {
			esc = 1;
			continue;
		}

		if(esc != 0 && s[i] == '0' && s[i+1] == '3' && s[i+2] == '3') {
			ret[j] = 033;
			i+=2;
		}
		else if(s[i] == 'e' && esc != 0) //
			ret[j] = 033;
		else if(s[i] == 'n' && esc != 0) //
			ret[j] = '\n';
		else if(s[i] == 'r' && esc != 0) //
			ret[j] = '\r';
		else 
			ret[j] = s[i];
		esc = 0;
		j++;
	}	
	*rlen = j;
	return ret;
}

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;
	
	setbuf(stdout, NULL);

	uint32_t rlen;
	char* p = parse(argv[1], &rlen);
	if(p != NULL) {
		write(1, p, rlen);
		free(p);
	}
	return 0;
}
