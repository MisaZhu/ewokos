#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <string.h>
#include <fcntl.h>

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

static int _streaming = 0;

static int doargs(int argc, char* argv[]) {
    int c = 0;
    while (c != -1) {
        c = getopt (argc, argv, "s");
        if(c == -1)
            break;

        switch (c) {
        case 's':
            _streaming = 1;
            break;
        default:
            c = -1;
            break;
        }
    }
    return optind;
}

int main(int argc, char** argv) {
    int argind = doargs(argc, argv);

    /* no args (or "-"): copy stdin to stdout so cat works in pipelines */
    int nf = argc - argind;
    if(nf == 0) {
        /* stdin: read until EOF (0); EAGAIN means retry, not stop */
        while(1) {
            char buf[1024*4];
            errno = 0;
            int sz = read(0, buf, sizeof(buf));
            if(sz > 0)
                out(buf, sz);
            else if(sz == 0)
                break;
            else if(errno != EAGAIN && errno != EINTR)
                break;
        }
        return 0;
    }

    for(int f = argind; f < argc; f++) {
        const char* fname = argv[f];
        int fd = (fname[0] == '-' && fname[1] == 0) ? 0 : open(fname, 0);
        if(fd < 0) {
            printf("Can't open [%s]!\n", fname);
            return -1;
        }

        while(1) {
            char buf[1024*4];
            errno = 0;
            int sz = read(fd, buf, sizeof(buf));
            if(sz > 0) {
                out(buf, sz);
            }
            else {
                if(!_streaming && (sz == 0 || errno != EAGAIN))
                    break;
            }
        }

        if(fd > 0)
            close(fd);
    }

    return 0;
}
