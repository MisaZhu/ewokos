#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <string.h>

static char rpl(char c) {
	if(c < 0x20 || c > 0x7e || c == ' ')
		return '.';
	return c;
}

static int write_all_retry(int fd, const void* buf, size_t len) {
	const char* p = (const char*)buf;
	size_t off = 0;

	while(off < len) {
		ssize_t wr = write(fd, p + off, len - off);
		if(wr > 0) {
			off += (size_t)wr;
			continue;
		}
		if(wr < 0 && (errno == EAGAIN || errno == EINTR)) {
			usleep(1000);
			continue;
		}
		if(wr == 0 && errno == 0)
			errno = EPIPE;
		return -1;
	}
	return 0;
}

#define BUF_SIZE 128
int main(int argc, char* argv[]) {
	int i = 0;
	int ln = 0;
	char out[128];
	char* p = out;
	(void)argc;
	(void)argv;

	(void)write_all_retry(1, "      | 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 |\n",
			strlen("      | 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 |\n"));
	(void)write_all_retry(1, "      | ----------------------------------------------- |\n",
			strlen("      | ----------------------------------------------- |\n"));
	while(1) {
		char buf[BUF_SIZE];
		char str[17], c;
		int res;
		int saved_errno;
		errno = 0;
		res = read(0, buf, BUF_SIZE);
		saved_errno = errno;
		if(res > 0) {
			int j;
			for(j=0; j<res; ++j) {
				if(i == 0) {
					snprintf(p, 16, "%6d| ", ln);
					p += 8;
				}
				c = (char)buf[j];
				str[i] = rpl(c);
				snprintf(p, 4, "%02X ", (int)c);
				p += 3;
				i++;
				if(i == 16) {
					str[i] = 0;
					snprintf(p, 32, "| %s\n", str);
					(void)write_all_retry(1, out, strlen(out));
					p = out;
					i = 0;
					ln++;
				}
			}
		}
		else if(res == 0) {
			if(i > 0) {
				(void)write_all_retry(1, out, strlen(out));
				str[i] = 0;
				for(; i< 16; ++i)
					(void)write_all_retry(1, "   ", 3);
				(void)write_all_retry(1, "| ", 2);
				(void)write_all_retry(1, str, strlen(str));
			}
			break;
		}
		else {
			if(saved_errno == EAGAIN || saved_errno == EINTR)
				continue;
			if(i > 0) {
				(void)write_all_retry(1, out, strlen(out));
				str[i] = 0;
				for(; i< 16; ++i)
					(void)write_all_retry(1, "   ", 3);
				(void)write_all_retry(1, "| ", 2);
				(void)write_all_retry(1, str, strlen(str));
			}
			break;
		}
	}
	(void)write_all_retry(1, "\n", 1);
	return 0;
}
