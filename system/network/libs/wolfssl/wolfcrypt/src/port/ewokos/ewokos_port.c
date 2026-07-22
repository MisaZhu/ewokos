/* ewokos_port.c
 * wolfSSL port for EwokOS
 */

#include <wolfssl/wolfcrypt/settings.h>

#ifdef WOLFSSL_EWOKOS

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <ewoksys/sys.h>

static uint64_t g_ewok_seed_state = 0x6a09e667f3bcc909ULL;

static uint64_t ewokos_mix64(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

/*
 * EwokOS currently lacks a native /dev/random style source for wolfSSL.
 * Feed the DRBG with the best changing system state we can access at boot.
 */
int ewokos_generate_seed(unsigned char* output, unsigned int sz) {
    struct timeval tv;
    uint64_t kernel_usec = 0;
    uintptr_t stack_addr = (uintptr_t)&tv;
    uintptr_t out_addr = (uintptr_t)output;

    if(output == NULL)
        return -1;

    if(_vsyscall_info != NULL)
        kernel_usec = _vsyscall_info->kernel_usec;
    gettimeofday(&tv, NULL);

    while(sz > 0) {
        uint64_t word;
        unsigned int i;
        unsigned int chunk = sz > sizeof(word) ? (unsigned int)sizeof(word) : sz;

        g_ewok_seed_state ^= kernel_usec;
        g_ewok_seed_state ^= ((uint64_t)(uint32_t)getpid() << 32) ^ (uint32_t)tv.tv_usec;
        g_ewok_seed_state ^= ((uint64_t)(uint32_t)tv.tv_sec << 32) ^ (uint32_t)(stack_addr >> 4);
        g_ewok_seed_state ^= (uint64_t)(out_addr + sz) + 0x9e3779b97f4a7c15ULL;
        word = ewokos_mix64(g_ewok_seed_state);
        g_ewok_seed_state = ewokos_mix64(word ^ kernel_usec ^ (uint64_t)chunk);

        for(i = 0; i < chunk; ++i)
            output[i] = (unsigned char)(word >> (i * 8));

        output += chunk;
        sz -= chunk;
        stack_addr ^= (uintptr_t)&word;
        out_addr += chunk;
        kernel_usec += 0x9e3779b97f4a7c15ULL;
    }

    return 0;
}

#endif /* WOLFSSL_EWOKOS */
