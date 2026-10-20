#include <fcntl.h>
#include <unistd.h>

#define COPY_BUFFER_SIZE (16U * 1024U)

static size_t text_len(const char* text) {
    size_t n = 0;
    while (text[n] != 0)
        n++;
    return n;
}

static void error(const char* prefix, const char* path) {
    const char* suffix = ": failed\n";
    write(2, prefix, text_len(prefix));
    write(2, path, text_len(path));
    write(2, suffix, text_len(suffix));
}

static int copy_file(int out_fd, const char* path) {
    char buffer[COPY_BUFFER_SIZE];
    int in_fd = open(path, O_RDONLY);
    if (in_fd < 0) {
        error("concat: ", path);
        return -1;
    }
    for (;;) {
        ssize_t n = read(in_fd, buffer, sizeof(buffer));
        if (n == 0)
            break;
        if (n < 0) {
            error("concat: ", path);
            close(in_fd);
            return -1;
        }
        ssize_t off = 0;
        while (off < n) {
            ssize_t written = write(out_fd, buffer + off, (size_t)(n - off));
            if (written <= 0) {
                error("concat: ", "output");
                close(in_fd);
                return -1;
            }
            off += written;
        }
    }
    close(in_fd);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        const char* usage = "usage: concat <output> <input> [input ...]\n";
        write(2, usage, text_len(usage));
        return 1;
    }

    int out_fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (out_fd < 0) {
        error("concat: ", argv[1]);
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        if (copy_file(out_fd, argv[i]) != 0) {
            close(out_fd);
            return 1;
        }
    }
    close(out_fd);
    return 0;
}
