#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
 * Big-memory smoke test: allocate several GB in one malloc() call plus a
 * second large block, touch every page so demand paging really maps it, and
 * verify the pattern survived.
 */
int main(int argc, char* argv[]) {
    size_t big = 5ull * 1024ull * 1024ull * 1024ull;   /* 5GB single malloc */
    size_t second = 1ull * 1024ull * 1024ull * 1024ull; /* +1GB */

    setbuf(stdout, NULL);
    printf("malloc4g: asking %llu + %llu bytes\n",
            (unsigned long long)big, (unsigned long long)second);

    unsigned char* p = (unsigned char*)malloc(big);
    if(p == NULL) {
        printf("malloc4g: FAIL malloc(%llu) returned NULL\n",
                (unsigned long long)big);
        return 1;
    }
    printf("malloc4g: got p=0x%llx\n", (unsigned long long)(uintptr_t)p);

    for(size_t i = 0; i < big; i += 4096)
        p[i] = (unsigned char)(i >> 12);

    unsigned char* q = (unsigned char*)malloc(second);
    if(q == NULL) {
        printf("malloc4g: FAIL second malloc(%llu) returned NULL\n",
                (unsigned long long)second);
        return 1;
    }
    for(size_t i = 0; i < second; i += 4096)
        q[i] = 0xAA;

    for(size_t i = 0; i < big; i += 4096) {
        if(p[i] != (unsigned char)(i >> 12)) {
            printf("malloc4g: FAIL verify at offset %llu\n",
                    (unsigned long long)i);
            return 1;
        }
    }
    for(size_t i = 0; i < second; i += 4096) {
        if(q[i] != 0xAA) {
            printf("malloc4g: FAIL verify second at %llu\n",
                    (unsigned long long)i);
            return 1;
        }
    }

    free(p);
    free(q);
    printf("malloc4g: PASS (5GB + 1GB alloc, touched, verified, freed)\n");
    return 0;
}
