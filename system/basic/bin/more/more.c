#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
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

static char*   _line = NULL;
static uint32_t _line_buf_size = 0;
static uint32_t  _lines_rd  = 16;
static uint32_t  _line_count  = 0;
static uint32_t  _line_index  = 0;
static bool _line_no  = false;
static bool _pause  = true;
	
static void line(char* str, int32_t sz, const char* key) {
	int32_t i = 0;
	static int32_t lines_rd = 0;
	if(lines_rd == 0)
		lines_rd = _lines_rd;
	while(i < sz) {
		char c = str[i++];
		if(c == 0 || c == '\r' || c == '\n') {
			if(_line != NULL) {
				if(_line_no)
					printf("%06d: %s\n", _line_index, _line);
				else
					printf("%s\n", _line);

				_line[0] = 0;
				_line_count++;
				_line_index++;

				if(_line_count >= lines_rd) {
					if(_pause) {
						while(read(VFS_BACKUP_FD0, &c, 1) == 1) {
							if(c == '\n' || c == '\r') {
								lines_rd = 1;
								break;
							}
							else if(c == ' ')  {
								lines_rd = _lines_rd;
								break;
							}
							else if(c == 'q') {
								_pause = false;
								break;
							}
						}
						_line_count = 0;
					}
				}
			}
		}
		else
			_line = get_line(_line, &_line_buf_size, c);
	}
}


static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "ln:");
		if(c == -1)
			break;

		switch (c) {
		case 'l':
			_line_no = true;
			break;
		case 'n':
			_lines_rd = atoi(optarg);
			break;
		case '?':
			return -1;
		default:
			c = -1;
			break;
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {
	_line = NULL;
	_line_buf_size = 0;
	_lines_rd  = 16;
	_line_count  = 0;
	_line_index  = 0;
	_line_no  = false;
	_pause  = true;

	if(doargs(argc, argv) != 0)
		return -1;

	if(optind < argc) {
		int fd = open(argv[optind], O_RDONLY);
		if(fd < 0)
			return -1;
		dup2(0, VFS_BACKUP_FD0);
		dup2(fd, 0);
		close(fd);
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
