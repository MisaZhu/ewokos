#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ewoksys/vfs.h>

static const char* path_basename(const char* path) {
	const char* base = path;
	const char* p;

	if(path == NULL || path[0] == 0)
		return "";

	p = path;
	while(*p != 0) {
		if(*p == '/')
			base = p + 1;
		p++;
	}

	while(base > path && base[0] == 0) {
		base--;
		if(*base != '/')
			break;
	}

	if(*base == '/')
		return base + 1;
	return base;
}

static int build_target_path(const char* src, const char* dst, char* out, size_t out_size) {
	struct stat st;

	if(stat(dst, &st) == 0 && S_ISDIR(st.st_mode)) {
		const char* name = path_basename(src);
		size_t dst_len = strlen(dst);

		if(dst_len > 0 && dst[dst_len - 1] == '/')
			return snprintf(out, out_size, "%s%s", dst, name) < (int)out_size ? 0 : -1;
		return snprintf(out, out_size, "%s/%s", dst, name) < (int)out_size ? 0 : -1;
	}

	return snprintf(out, out_size, "%s", dst) < (int)out_size ? 0 : -1;
}

void out(void* data, int32_t size) {
	char* buf = (char*)data;
	int32_t wr = 0;
	while(1) {
		if(size <= 0)
			break;

		int sz = write(1, buf, size);
		if(sz <= 0 && errno != EAGAIN)
			break;

		if(sz > 0) {
			size -= sz;
			wr += sz;
			buf += sz;
		}
	}
}

int main(int argc, char** argv) {
	char src[FS_FULL_NAME_MAX+1] = {0};
	char dst[FS_FULL_NAME_MAX+1] = {0};
	char target[FS_FULL_NAME_MAX+1] = {0};

	if(argc < 3) {
		printf("  Usage: cp <file_from> <file_to>\n");
		return -1;
	}

	vfs_fullname(argv[1], src, FS_FULL_NAME_MAX);
	vfs_fullname(argv[2], dst, FS_FULL_NAME_MAX);

	struct stat st;
	if(stat(src, &st) != 0) {
		printf("'%s' stat info failed!\n", src);
		return -1;
	}

	int fd_from = open(src, O_RDONLY);
	if(fd_from < 0) {
		printf("'%s' open failed!\n", src);
		return -1;
	}
	
	if(build_target_path(src, dst, target, sizeof(target)) != 0) {
		printf("target path too long!\n");
		close(fd_from);
		return -1;
	}

	int fd_to = open(target, O_WRONLY | O_CREAT | O_TRUNC);
	if(fd_to < 0) {
		printf("'%s' open failed!\n", target);
		close(fd_from);
		return -1;
	}

	while(1) {
		char buf[1024*4];
		int sz = read(fd_from, buf, sizeof(buf));
		if(sz > 0)
			sz = write(fd_to, buf, sz);
		if(sz <= 0)
			break;
	}

	fchmod(fd_to, st.st_mode);
	close(fd_to);
	close(fd_from);
	return 0;
}
