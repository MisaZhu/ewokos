#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ewoksys/vfs.h>

#define CP_BUF_SIZE VFS_BUF_SIZE

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

static void normalize_target_path(char* path) {
        size_t len;

        if(path == NULL)
                return;

        len = strlen(path);
        while(len > 1 && path[len - 1] == '/') {
                path[len - 1] = 0;
                len--;
        }

        while(len > 2 && path[len - 2] == '/' && path[len - 1] == '.') {
                path[len - 1] = 0;
                len--;
                while(len > 1 && path[len - 1] == '/') {
                        path[len - 1] = 0;
                        len--;
                }
        }
}

static int path_is_dir_arg(const char* path) {
        size_t len;

        if(path == NULL || path[0] == 0)
                return 0;

        if(strcmp(path, ".") == 0 || strcmp(path, "..") == 0)
                return 1;

        len = strlen(path);
        if(path[len - 1] == '/')
                return 1;
        if(len >= 2 && path[len - 2] == '/' && path[len - 1] == '.')
                return 1;
        if(len >= 3 && path[len - 3] == '/' && path[len - 2] == '.' && path[len - 1] == '.')
                return 1;
        return 0;
}

static int build_target_path(const char* src, const char* dst, const char* dst_arg, char* out, size_t out_size) {
	struct stat st;
        char normalized[FS_FULL_NAME_MAX+1] = {0};
        int is_dir = 0;

        snprintf(normalized, sizeof(normalized), "%s", dst);
        normalize_target_path(normalized);

        if(path_is_dir_arg(dst_arg))
                is_dir = 1;
        else if(stat(normalized, &st) == 0 && S_ISDIR(st.st_mode))
                is_dir = 1;

        if(is_dir) {
		const char* name = path_basename(src);
                size_t dst_len = strlen(normalized);

                if(dst_len == 1 && normalized[0] == '/')
                        return snprintf(out, out_size, "%s%s", normalized, name) < (int)out_size ? 0 : -1;
                return snprintf(out, out_size, "%s/%s", normalized, name) < (int)out_size ? 0 : -1;
	}

        return snprintf(out, out_size, "%s", normalized) < (int)out_size ? 0 : -1;
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
        char* buf = NULL;
        int ret = -1;
        unsigned long long total = 0;
        struct timeval tv_begin;
        struct timeval tv_end;

        gettimeofday(&tv_begin, NULL);

	if(argc < 3) {
		printf("  Usage: cp <file_from> <file_to>\n");
		return -1;
	}

	vfs_fullname(argv[1], src, FS_FULL_NAME_MAX);
	vfs_fullname(argv[2], dst, FS_FULL_NAME_MAX);

	int fd_from = open(src, O_RDONLY);
	if(fd_from < 0) {
		printf("'%s' open failed!\n", src);
		return -1;
	}

	struct stat st;
	if(fstat(fd_from, &st) != 0) {
		printf("'%s' stat info failed!\n", src);
		close(fd_from);
		return -1;
	}
	
        if(build_target_path(src, dst, argv[2], target, sizeof(target)) != 0) {
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

        buf = (char*)malloc(CP_BUF_SIZE);
        if(buf == NULL) {
                printf("buffer alloc failed!\n");
                close(fd_to);
                close(fd_from);
                return -1;
        }

	while(1) {
                int sz = read(fd_from, buf, CP_BUF_SIZE);
                int off = 0;

                if(sz == 0) {
                        ret = 0;
			break;
                }
                if(sz < 0)
                        break;

                while(off < sz) {
                        int wr = write(fd_to, buf + off, sz - off);
                        if(wr <= 0)
                                goto done;
                        off += wr;
                        total += (unsigned long long)wr;
                }
	}

done:
        gettimeofday(&tv_end, NULL);
	fchmod(fd_to, st.st_mode);
        free(buf);
	close(fd_to);
	close(fd_from);
        if(ret == 0) {
                unsigned long long usec = 0;
                unsigned long long begin_usec =
                                (unsigned long long)tv_begin.tv_sec * 1000000ULL +
                                (unsigned long long)tv_begin.tv_usec;
                unsigned long long end_usec =
                                (unsigned long long)tv_end.tv_sec * 1000000ULL +
                                (unsigned long long)tv_end.tv_usec;
                unsigned long long speed_x100 = 0;

                if(end_usec > begin_usec)
                        usec = end_usec - begin_usec;
                if(usec == 0)
                        usec = 1;

                speed_x100 = (total * 1000000ULL * 100ULL) / (usec * 1024ULL);
                printf("\n%llu.%02llu KB/s\n",
                                speed_x100 / 100ULL,
                                speed_x100 % 100ULL);
        }
        return ret;
}
