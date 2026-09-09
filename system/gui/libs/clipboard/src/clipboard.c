#include <clipboard/clipboard.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CLIPBOARD_PATH "/tmp/.clipboard"
/* World readable/writable: any user may access the global clipboard. */
#define CLIPBOARD_MODE 0666

static int clipboard_open_write(void) {
	/* O_TRUNC is what makes a set a whole-content overwrite: vfsd drops
	 * the node size and the ramfsd open hook discards the old backing
	 * store, so no stale bytes survive a shorter rewrite. */
	int fd = open(CLIPBOARD_PATH, O_WRONLY | O_CREAT | O_TRUNC, CLIPBOARD_MODE);
	if(fd < 0)
		return -1;
	/* A restrictive process umask must not lock other users out of the
	 * clipboard: force the final mode no matter who created the file. */
	fchmod(fd, CLIPBOARD_MODE);
	return fd;
}

int clipboard_set(const void* data, uint32_t size) {
	if(data == NULL && size > 0)
		return -1;

	int fd = clipboard_open_write();
	if(fd < 0)
		return -1;

	const char* p = (const char*)data;
	uint32_t left = size;
	while(left > 0) {
		ssize_t wr = write(fd, p, left);
		if(wr <= 0) {
			close(fd);
			return -1;
		}
		p += wr;
		left -= (uint32_t)wr;
	}
	close(fd);
	return 0;
}

int clipboard_set_text(const char* text) {
	if(text == NULL)
		return -1;
	return clipboard_set(text, (uint32_t)strlen(text));
}

int clipboard_get(void* buf, uint32_t size) {
	if(buf == NULL)
		return -1;

	int fd = open(CLIPBOARD_PATH, O_RDONLY);
	if(fd < 0)
		return -1;

	char* p = (char*)buf;
	uint32_t got = 0;
	while(got < size) {
		ssize_t rd = read(fd, p + got, size - got);
		if(rd < 0) {
			close(fd);
			return -1;
		}
		if(rd == 0)
			break;
		got += (uint32_t)rd;
	}
	close(fd);
	return (int)got;
}

uint32_t clipboard_size(void) {
	struct stat st;
	if(stat(CLIPBOARD_PATH, &st) != 0)
		return 0;
	if(st.st_size <= 0)
		return 0;
	return (uint32_t)st.st_size;
}

char* clipboard_get_text(void) {
	uint32_t size = clipboard_size();
	char* buf = (char*)malloc(size + 1);
	if(buf == NULL)
		return NULL;
	if(size > 0 && clipboard_get(buf, size) != (int)size) {
		free(buf);
		return NULL;
	}
	buf[size] = 0;
	return buf;
}

int clipboard_clear(void) {
	return clipboard_set(NULL, 0);
}
