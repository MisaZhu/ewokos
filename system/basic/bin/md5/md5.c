#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ewoksys/md5.h>

static char* hex_digest(const uint8_t* d) {
    static char out[33];
    static const char* H = "0123456789abcdef";
    for(int i = 0; i < 16; i++) {
        out[i*2]   = H[(d[i] >> 4) & 0xF];
        out[i*2+1] = H[d[i] & 0xF];
    }
    out[32] = 0;
    return out;
}

int main(int argc, char* argv[]) {
    if(argc < 2)
        return -1;

    setbuf(stdout, NULL);

    /* If the argument names a readable file, hash its content
     * (binary-safe); otherwise hash the argument string itself. */
    int fd = open(argv[1], O_RDONLY);
    if(fd >= 0) {
        struct stat st;
        uint8_t* buf = NULL;
        uint32_t sz = 0, off = 0;
        if(fstat(fd, &st) == 0 && st.st_size > 0) {
            sz = (uint32_t)st.st_size;
            buf = (uint8_t*)malloc(sz);
        }
        if(buf != NULL) {
            while(off < sz) {
                int n = read(fd, buf + off, sz - off);
                if(n <= 0)
                    break;
                off += n;
            }
        }
        close(fd);
        if(buf == NULL || off != sz) {
            free(buf);
            return -1;
        }
        uint8_t digest[16];
        md5_encode(buf, sz, digest);
        free(buf);
        printf("%s\n", hex_digest(digest));
        return 0;
    }

    const char* p = md5_encode_str((const uint8_t*)argv[1], strlen(argv[1]));
    printf("%s\n", p);
    return 0;
}
