#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 128

static char* line(char* ln, int32_t* sz, char c) {
	if(ln == NULL) {
		ln = (char*)malloc(BUF_SIZE);
		*sz = BUF_SIZE;
		ln[0] = c;
		ln[1] = 0;
		return ln;
	}

	int len = strlen(ln);
	if(len < *sz-1) {
		ln[len] = c;
		ln[len+1] = 0;
		return ln;
	}

	*sz = len + BUF_SIZE;
	char* ret = (char*)malloc(*sz);
	strcpy(ret, ln);
	ret[len] = c;
	ret[len+1] = 0;
	free(ln);

	return ret;
}

static char* _line = NULL;
static int32_t _line_buf_size = 0;

static void grep_line(char* str, const char* key) {
	if(str == NULL || str[0] == 0 || key == NULL )
		return;
	if(strstr(str, key) != NULL) 
		printf("%s\n", str);
}
	
static void grep(char* str, int32_t sz, const char* key) {
	int32_t i = 0;
	while(i < sz) {
		char c = str[i++];
		if(c == 0 || c == '\r' || c == '\n') {
			if(_line != NULL) {
				grep_line(_line, key);
				_line[0] = 0;
			}
		}
		else
			_line = line(_line, &_line_buf_size, c);
	}
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("usage: grep <keyword>\n");
		return -1;
	}

	_line = NULL;
	_line_buf_size = 0;
	while(1) {
		char buf[128];
		int sz;
		while(1) { 
			sz = read(0, buf, 128);
			if(sz < 0 && errno == EAGAIN) {
				sleep(0);
				continue;
			}
			break;
		}
		if(sz <= 0)
			break;
		grep(buf, sz, argv[1]);
	}
	if(_line[0] != 0)
		grep_line(_line, argv[1]);
	free(_line);
	return 0;
}
