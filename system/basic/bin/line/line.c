#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <ewoksys/vfs.h>

#define BUF_SIZE 128

static char* get_line(char* ln, int32_t* sz, char c) {
	int len;
	char* ret;
	if(ln == NULL) {
		ln = (char*)malloc(BUF_SIZE);
		*sz = BUF_SIZE;
		ln[0] = c;
		ln[1] = 0;
		return ln;
	}

	len = strlen(ln);
	if(len < *sz-1) {
		ln[len] = c;
		ln[len+1] = 0;
		return ln;
	}

	*sz = len + BUF_SIZE;
	ret = (char*)malloc(*sz);
	strcpy(ret, ln);
	ret[len] = c;
	ret[len+1] = 0;
	free(ln);

	return ret;
}

static char* _line = NULL;
static int32_t _line_buf_size = 0;
static int _lines_rd  = 1;
static int _line_no  = 0;
	
static void line(char* str, int32_t sz, const char* key) {
	int32_t i = 0;
	while(i < sz) {
		char c = str[i++];
		if(c == 0 || c == '\r' || c == '\n') {
			if(_line != NULL) {
				printf("%s\n", _line);
				_line[0] = 0;
				_line_no++;

				if(_line_no >= _lines_rd) {
					while(read(VFS_BACKUP_FD0, &c, 1) == 1) {
						if(c == '\n' || c == '\r' || c == ' ')
							break;
					}
					_line_no = 0;
				}
			}
		}
		else
			_line = get_line(_line, &_line_buf_size, c);
	}
}

int main(int argc, char* argv[]) {
	_line = NULL;
	_line_buf_size = 0;
	_lines_rd  = 1;
	_line_no  = 0;

	if(argc > 1) {
		_lines_rd = atoi(argv[1]);
	}

	while(1) {
		char buf[128];
		int sz;
		while(1) { 
			sz = read(0, buf, 128);
			if(sz < 0 && errno == EAGAIN) {
				continue;
			}
			break;
		}
		if(sz <= 0)
			break;
		line(buf, sz, argv[1]);
	}
	if(_line[0] != 0)
		printf("%s\n", _line);
	free(_line);
	return 0;
}
