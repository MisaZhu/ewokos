#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <setjmp.h>
#include <ewoksys/trunkmem.h>
#include <ewoksys/vfs.h>
#include <sys/mman.h>

void ewok_longjmp(jmp_buf env, int val) {
    longjmp(env, val);
}

extern void *_sbrk(ptrdiff_t incr);
extern int _open(const char *pathname, int flags, ...);
extern int _close(int fd);
extern int _read(int fd, void *buf, size_t size);
extern int _write(int fd, const void *buf, size_t size);
extern _off_t _lseek(int fd, _off_t offset, int whence);
extern int _fstat(int fd, struct stat *st);
extern int _stat(const char *path, struct stat *st);
extern int _unlink(const char *path);
extern int _isatty(int fd);
extern pid_t _getpid(void);
extern int _gettimeofday(struct timeval *tp, void *tzvp);
extern clock_t _times(struct tms *tp);
extern int _fork(void);
extern void _exit(int err);
extern void *__heap_end;

char *__progname = "";
static char *_empty_environ[] = { NULL };
char **environ = _empty_environ;
static FILE _stdin_file = { 0 };
static FILE _stdout_file = { 1 };
static FILE _stderr_file = { 2 };
FILE *stdin = &_stdin_file;
FILE *stdout = &_stdout_file;
FILE *stderr = &_stderr_file;
char *optarg;
int optind = 1;
int opterr = 1;
int optopt;
static unsigned long prng_state = 1;

int __bss_start__ __attribute__((weak));
int __bss_end__ __attribute__((weak));

/* The mem* functions below copy/compare word-wide instead of byte-wide:
   the byte loops were the single hottest code in the whole userland (every
   IPC buffer, shm window copy and damage compare funnels through them).
   Rules honored:
   - ARM builds use -mstrict-align, so the word path only engages when both
     pointers share the same misalignment and is entered word-aligned.
   - GCC loop-distribution must not pattern-match these loops back into
     calls to themselves (x86 builds lack -fno-builtin-*), hence the
     attribute. */
#if defined(__GNUC__) && !defined(__clang__)
#define MEM_NO_IDIOM __attribute__((optimize("no-tree-loop-distribute-patterns")))
#else
#define MEM_NO_IDIOM
#endif

#define MEM_WORD_SZ   sizeof(unsigned long)
#define MEM_WORD_MASK (MEM_WORD_SZ - 1)

MEM_NO_IDIOM
void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d == NULL || s == NULL) {
        return dest;
    }

    if (n >= 2 * MEM_WORD_SZ &&
            (((uintptr_t)d ^ (uintptr_t)s) & MEM_WORD_MASK) == 0) {
        while (((uintptr_t)d & MEM_WORD_MASK) != 0) {
            *d++ = *s++;
            n--;
        }
        unsigned long *dw = (unsigned long *)d;
        const unsigned long *sw = (const unsigned long *)s;
        while (n >= 4 * MEM_WORD_SZ) {
            dw[0] = sw[0];
            dw[1] = sw[1];
            dw[2] = sw[2];
            dw[3] = sw[3];
            dw += 4;
            sw += 4;
            n -= 4 * MEM_WORD_SZ;
        }
        while (n >= MEM_WORD_SZ) {
            *dw++ = *sw++;
            n -= MEM_WORD_SZ;
        }
        d = (unsigned char *)dw;
        s = (const unsigned char *)sw;
    }

    while (n-- > 0) {
        *d++ = *s++;
    }
    return dest;
}

MEM_NO_IDIOM
void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d == s || n == 0) {
        return dest;
    }
    if (d < s || d >= s + n) {
        /* forward copy is safe (also covers all non-overlapping cases) */
        return memcpy(dest, src, n);
    }

    /* overlapping with dest above src: copy backward */
    d += n;
    s += n;
    if (n >= 2 * MEM_WORD_SZ &&
            (((uintptr_t)d ^ (uintptr_t)s) & MEM_WORD_MASK) == 0) {
        while (((uintptr_t)d & MEM_WORD_MASK) != 0) {
            *--d = *--s;
            n--;
        }
        unsigned long *dw = (unsigned long *)d;
        const unsigned long *sw = (const unsigned long *)s;
        while (n >= MEM_WORD_SZ) {
            *--dw = *--sw;
            n -= MEM_WORD_SZ;
        }
        d = (unsigned char *)dw;
        s = (const unsigned char *)sw;
    }
    while (n-- > 0) {
        *--d = *--s;
    }
    return dest;
}

MEM_NO_IDIOM
void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    unsigned char v = (unsigned char)c;

    if (p == NULL) {
        return s;
    }

    if (n >= 2 * MEM_WORD_SZ) {
        while (((uintptr_t)p & MEM_WORD_MASK) != 0) {
            *p++ = v;
            n--;
        }
        unsigned long w = v;
        w |= w << 8;
        w |= w << 16;
        /* no-op on 32-bit long, fills the high half on 64-bit; the split
           shift avoids an undefined 32-bit-long shift by 32 */
        w |= (w << 16) << 16;
        unsigned long *pw = (unsigned long *)p;
        while (n >= 4 * MEM_WORD_SZ) {
            pw[0] = w;
            pw[1] = w;
            pw[2] = w;
            pw[3] = w;
            pw += 4;
            n -= 4 * MEM_WORD_SZ;
        }
        while (n >= MEM_WORD_SZ) {
            *pw++ = w;
            n -= MEM_WORD_SZ;
        }
        p = (unsigned char *)pw;
    }

    while (n-- > 0) {
        *p++ = v;
    }
    return s;
}

MEM_NO_IDIOM
int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;

    if (n >= 2 * MEM_WORD_SZ &&
            (((uintptr_t)a ^ (uintptr_t)b) & MEM_WORD_MASK) == 0) {
        while (((uintptr_t)a & MEM_WORD_MASK) != 0) {
            if (*a != *b) {
                return (int)*a - (int)*b;
            }
            a++;
            b++;
            n--;
        }
        const unsigned long *aw = (const unsigned long *)a;
        const unsigned long *bw = (const unsigned long *)b;
        /* on the first differing word fall through: the byte tail below
           locates the differing byte and returns the correct sign */
        while (n >= MEM_WORD_SZ && *aw == *bw) {
            aw++;
            bw++;
            n -= MEM_WORD_SZ;
        }
        a = (const unsigned char *)aw;
        b = (const unsigned char *)bw;
    }

    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            return (int)a[i] - (int)b[i];
        }
    }
    return 0;
}

char *strcpy(char *dest, const char *src) {
    char *ret = dest;

    while (*src != 0) {
        *dest++ = *src++;
    }
    *dest = 0;
    return ret;
}

char *strcat(char *dest, const char *src) {
    char *ret = dest;

    while (*dest != 0) {
        ++dest;
    }
    while (*src != 0) {
        *dest++ = *src++;
    }
    *dest = 0;
    return ret;
}

char *strncat(char *dest, const char *src, size_t n) {
    char *ret = dest;
    size_t i = 0;

    while (*dest != 0) {
        ++dest;
    }
    while (i < n && src[i] != 0) {
        *dest++ = src[i++];
    }
    *dest = 0;
    return ret;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;

    for (; i < n && src[i] != 0; ++i) {
        dest[i] = src[i];
    }
    for (; i < n; ++i) {
        dest[i] = 0;
    }
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 != 0 && *s1 == *s2) {
        ++s1;
        ++s2;
    }
    return (int)((unsigned char)*s1) - (int)((unsigned char)*s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        unsigned char a = (unsigned char)s1[i];
        unsigned char b = (unsigned char)s2[i];
        if (a != b || a == 0 || b == 0) {
            return (int)a - (int)b;
        }
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (s != NULL && s[n] != 0) {
        ++n;
    }
    return n;
}

char *strchr(const char *s, int c) {
    while (*s != 0) {
        if (*s == (char)c) {
            return (char *)s;
        }
        ++s;
    }
    return ((char)c == 0) ? (char *)s : NULL;
}

char *strrchr(const char *s, int c) {
    const char *last = NULL;

    while (*s != 0) {
        if (*s == (char)c) {
            last = s;
        }
        ++s;
    }
    return ((char)c == 0) ? (char *)s : (char *)last;
}

char *strstr(const char *haystack, const char *needle) {
    size_t needle_len = strlen(needle);

    if (needle_len == 0) {
        return haystack;
    }

    while (*haystack != 0) {
        if (strncmp(haystack, needle, needle_len) == 0) {
            return haystack;
        }
        ++haystack;
    }
    return NULL;
}

char *strtok(char *str, const char *delim) {
    static char *last;
    char *token;

    if (str != NULL) {
        last = str;
    }
    if (last == NULL) {
        return NULL;
    }

    while (*last != 0 && strchr(delim, *last) != NULL) {
        ++last;
    }
    if (*last == 0) {
        last = NULL;
        return NULL;
    }

    token = last;
    while (*last != 0 && strchr(delim, *last) == NULL) {
        ++last;
    }
    if (*last != 0) {
        *last++ = 0;
    } else {
        last = NULL;
    }
    return token;
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
    char *last;
    char *token;

    if (delim == NULL || saveptr == NULL) {
        return NULL;
    }

    last = (str != NULL) ? str : *saveptr;
    if (last == NULL) {
        return NULL;
    }

    while (*last != 0 && strchr(delim, *last) != NULL) {
        ++last;
    }
    if (*last == 0) {
        *saveptr = NULL;
        return NULL;
    }

    token = last;
    while (*last != 0 && strchr(delim, *last) == NULL) {
        ++last;
    }
    if (*last != 0) {
        *last++ = 0;
        *saveptr = last;
    } else {
        *saveptr = NULL;
    }
    return token;
}

char *strdup(const char *s) {
    size_t len;
    char *dup;

    if (s == NULL) {
        return NULL;
    }
    len = strlen(s) + 1;
    dup = (char *)malloc(len);
    if (dup == NULL) {
        return NULL;
    }
    memcpy(dup, s, len);
    return dup;
}

char *strndup(const char *s, size_t n) {
    size_t len = 0;
    char *dup;

    if (s == NULL) {
        return NULL;
    }
    while (len < n && s[len] != 0) {
        ++len;
    }
    dup = (char *)malloc(len + 1);
    if (dup == NULL) {
        return NULL;
    }
    memcpy(dup, s, len);
    dup[len] = 0;
    return dup;
}

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

static int digit_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    return -1;
}

static char ascii_tolower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c - 'A' + 'a');
    }
    return c;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    unsigned long result = 0;
    int neg = 0;
    int any = 0;

    while (is_space(*s)) {
        ++s;
    }

    if (*s == '+' || *s == '-') {
        neg = (*s == '-');
        ++s;
    }

    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            base = 16;
            s += 2;
        } else if (s[0] == '0') {
            base = 8;
            ++s;
        } else {
            base = 10;
        }
    } else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }

    while (*s != 0) {
        int v = digit_value(*s);
        if (v < 0 || v >= base) {
            break;
        }
        result = result * (unsigned long)base + (unsigned long)v;
        any = 1;
        ++s;
    }

    if (endptr != NULL) {
        *endptr = (char *)(any ? s : nptr);
    }

    if (neg) {
        return (unsigned long)(-(long)result);
    }
    return result;
}

int atoi(const char *nptr) {
    int neg = 0;
    long v;

    while (is_space(*nptr)) {
        ++nptr;
    }
    if (*nptr == '+' || *nptr == '-') {
        neg = (*nptr == '-');
        ++nptr;
    }
    v = (long)strtoul(nptr, NULL, 10);
    return neg ? -(int)v : (int)v;
}

long atol(const char *nptr) {
    return strtol(nptr, NULL, 10);
}

long strtol(const char *nptr, char **endptr, int base) {
    return (long)strtoul(nptr, endptr, base);
}

long long strtoll(const char *nptr, char **endptr, int base) {
    return (long long)strtoul(nptr, endptr, base);
}

unsigned long long strtoull(const char *nptr, char **endptr, int base) {
    return (unsigned long long)strtoul(nptr, endptr, base);
}

double strtod(const char *nptr, char **endptr) {
    const char *s = nptr;
    double value = 0.0;
    double frac_scale = 1.0;
    int sign = 1;
    int saw_digit = 0;
    int exp_sign = 1;
    long exp_value = 0;

    while (is_space(*s)) {
        ++s;
    }
    if (*s == '+' || *s == '-') {
        sign = (*s == '-') ? -1 : 1;
        ++s;
    }

    while (*s >= '0' && *s <= '9') {
        value = value * 10.0 + (double)(*s - '0');
        saw_digit = 1;
        ++s;
    }

    if (*s == '.') {
        ++s;
        while (*s >= '0' && *s <= '9') {
            frac_scale *= 0.1;
            value += (double)(*s - '0') * frac_scale;
            saw_digit = 1;
            ++s;
        }
    }

    if ((*s == 'e' || *s == 'E') && saw_digit) {
        const char *exp_start = s + 1;
        if (*exp_start == '+' || *exp_start == '-') {
            exp_sign = (*exp_start == '-') ? -1 : 1;
            ++exp_start;
        }
        if (*exp_start >= '0' && *exp_start <= '9') {
            s = exp_start;
            while (*s >= '0' && *s <= '9') {
                exp_value = exp_value * 10 + (*s - '0');
                ++s;
            }
        }
    }

    if (endptr != NULL) {
        *endptr = (char *)(saw_digit ? s : nptr);
    }

    value *= (double)sign;
    while (exp_value > 0) {
        value = (exp_sign > 0) ? (value * 10.0) : (value / 10.0);
        --exp_value;
    }
    return value;
}

void srandom(unsigned int seed) {
    prng_state = (seed == 0) ? 1UL : (unsigned long)seed;
}

long random(void) {
    prng_state = prng_state * 1103515245UL + 12345UL;
    return (long)((prng_state >> 1) & 0x7fffffffUL);
}

void srand(unsigned int seed) {
    srandom(seed);
}

int rand(void) {
    return (int)random();
}

double atof(const char *nptr) {
    double value = 0.0;
    double frac = 0.1;
    int neg = 0;

    while (is_space(*nptr)) {
        ++nptr;
    }
    if (*nptr == '+' || *nptr == '-') {
        neg = (*nptr == '-');
        ++nptr;
    }

    while (*nptr >= '0' && *nptr <= '9') {
        value = value * 10.0 + (double)(*nptr - '0');
        ++nptr;
    }

    if (*nptr == '.') {
        ++nptr;
        while (*nptr >= '0' && *nptr <= '9') {
            value += (double)(*nptr - '0') * frac;
            frac *= 0.1;
            ++nptr;
        }
    }

    return neg ? -value : value;
}

int abs(int j) {
    return (j < 0) ? -j : j;
}

long labs(long j) {
    return (j < 0) ? -j : j;
}

div_t div(int numer, int denom) {
    div_t ret;
    ret.quot = numer / denom;
    ret.rem = numer % denom;
    return ret;
}

static void qsort_swap(unsigned char *a, unsigned char *b, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        unsigned char t = a[i];
        a[i] = b[i];
        b[i] = t;
    }
}

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
    unsigned char *arr = (unsigned char *)base;

    if (arr == NULL || compar == NULL || size == 0 || nmemb < 2) {
        return;
    }

    for (size_t i = 1; i < nmemb; ++i) {
        size_t j = i;
        while (j > 0) {
            unsigned char *lhs = arr + (j - 1) * size;
            unsigned char *rhs = arr + j * size;
            if (compar(lhs, rhs) <= 0) {
                break;
            }
            qsort_swap(lhs, rhs, size);
            --j;
        }
    }
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
        int (*compar)(const void *, const void *)) {
    size_t left = 0;
    size_t right = nmemb;
    const unsigned char *arr = (const unsigned char *)base;

    if (key == NULL || arr == NULL || compar == NULL || size == 0) {
        return NULL;
    }

    while (left < right) {
        size_t mid = left + (right - left) / 2;
        const void *elem = arr + mid * size;
        int cmp = compar(key, elem);
        if (cmp == 0) {
            return (void *)elem;
        }
        if (cmp < 0) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    return NULL;
}

static malloc_t compat_heap;
static int compat_heap_ready = 0;

static void *compat_heap_tail(void *arg) {
    (void)arg;
    return (__heap_end != NULL) ? __heap_end : _sbrk(0);
}

static int32_t compat_heap_expand(void *arg, int32_t pages) {
    ptrdiff_t bytes;

    (void)arg;
    if (pages <= 0) {
        return 0;
    }

    bytes = (ptrdiff_t)((size_t)pages * (size_t)compat_heap.seg_size);
    return (_sbrk(bytes) == (void *)-1) ? -1 : 0;
}

static void compat_heap_shrink(void *arg, int32_t pages) {
    ptrdiff_t bytes;

    (void)arg;
    if (pages <= 0) {
        return;
    }

    bytes = (ptrdiff_t)((size_t)pages * (size_t)compat_heap.seg_size);
    (void)_sbrk(-bytes);
}

static void compat_heap_init(void) {
    if (compat_heap_ready) {
        return;
    }

    memset(&compat_heap, 0, sizeof(compat_heap));
    compat_heap.seg_size = 4096;
    compat_heap.expand = compat_heap_expand;
    compat_heap.shrink = compat_heap_shrink;
    compat_heap.get_mem_tail = compat_heap_tail;
    compat_heap_ready = 1;
}

/*
 * mmap() and aligned allocations are backed by the regular heap, but the
 * returned pointers are not the raw heap-block heads, so trunk_free()
 * cannot release them directly. A small registry maps such pointers back
 * to their raw blocks so that free()/munmap() work as expected.
 */
struct ewok_mem_reg {
    void *ptr;   /* pointer handed to the caller */
    void *raw;   /* raw heap block head */
    size_t size; /* usable size of ptr */
    struct ewok_mem_reg *next;
};

static struct ewok_mem_reg *ewok_mem_reg_list = NULL;
static volatile int ewok_mem_reg_lock = 0;

static void ewok_reg_lock(void) {
    while (__sync_lock_test_and_set(&ewok_mem_reg_lock, 1)) {
    }
}

static void ewok_reg_unlock(void) {
    __sync_lock_release(&ewok_mem_reg_lock);
}

/* Allocate `size` bytes with `align` alignment, register the result. */
static void *ewok_reg_alloc(size_t size, size_t align) {
    void *raw;
    uintptr_t addr;
    struct ewok_mem_reg *reg;

    compat_heap_init();
    raw = trunk_malloc(&compat_heap, (uint32_t)(size + align));
    if (raw == NULL) {
        return NULL;
    }

    addr = (uintptr_t)raw;
    if (align > 0) {
        addr = (addr + (align - 1)) & ~(uintptr_t)(align - 1);
    }

    reg = (struct ewok_mem_reg *)trunk_malloc(&compat_heap,
            (uint32_t)sizeof(struct ewok_mem_reg));
    if (reg == NULL) {
        trunk_free(&compat_heap, (char *)raw);
        return NULL;
    }

    reg->ptr = (void *)addr;
    reg->raw = raw;
    reg->size = size;
    ewok_reg_lock();
    reg->next = ewok_mem_reg_list;
    ewok_mem_reg_list = reg;
    ewok_reg_unlock();
    return reg->ptr;
}

/*
 * Release `ptr` if it is a registered allocation. Returns 1 when handled,
 * 0 when `ptr` is a normal heap pointer.
 */
static int ewok_reg_try_free(void *ptr) {
    struct ewok_mem_reg **pp;
    struct ewok_mem_reg *reg;
    int handled = 0;

    ewok_reg_lock();
    for (pp = &ewok_mem_reg_list; *pp != NULL; pp = &(*pp)->next) {
        if ((*pp)->ptr == ptr) {
            reg = *pp;
            *pp = reg->next;
            ewok_reg_unlock();
            compat_heap_init();
            trunk_free(&compat_heap, (char *)reg->raw);
            trunk_free(&compat_heap, (char *)reg);
            return 1;
        }
    }
    ewok_reg_unlock();
    return handled;
}

void *malloc(size_t size) {
    void *ptr;

    if (size == 0) {
        size = 1;
    }
    if (size > UINT32_MAX) {
        errno = ENOMEM;
        return NULL;
    }

    compat_heap_init();
    ptr = trunk_malloc(&compat_heap, (uint32_t)size);
    if (ptr == NULL) {
        errno = ENOMEM;
    }
    return ptr;
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    if (ewok_reg_try_free(ptr)) {
        return;
    }

    compat_heap_init();
    trunk_free(&compat_heap, (char *)ptr);
}

/*
 * O(1), non-dereferencing heap-membership test.
 *
 * Higher-level code (e.g. litehtml, whose shared_ptr ownership model was
 * hand-ported to raw pointers) can be handed a stale slot that a heap overflow
 * or use-after-free overwrote with a small integer pointing into the binary's
 * text/data segment. Reading through such a pointer takes a data abort deep
 * inside a DOM/CSS walk. This lets the caller reject the bad pointer WITHOUT
 * dereferencing it: a live heap pointer always lies in [head, tail), and the
 * observed corruption (tiny text-segment addresses) always falls below head.
 * The bounds are read lock-free; the heap only grows upward and shrinks solely
 * by releasing free tail space, so every still-allocated object stays inside
 * [head, tail) and a momentarily stale tail can never reject a valid pointer.
 *
 * Returns 1 when p points inside the live trunk heap, 0 otherwise.
 */
int ewok_ptr_in_heap(const void *p) {
    uintptr_t addr;
    uintptr_t start;
    uintptr_t end;

    if (p == NULL || !compat_heap_ready || compat_heap.head == NULL) {
        return 0;
    }
    addr  = (uintptr_t)p;
    start = (uintptr_t)compat_heap.head;
    end   = (uintptr_t)compat_heap_tail(NULL);
    return (addr >= start && addr < end) ? 1 : 0;
}

__attribute__((noinline, optimize("O0")))
void *calloc(size_t nmemb, size_t size) {
    size_t total;
    void *ptr;

    if (size != 0 && nmemb > (SIZE_MAX / size)) {
        errno = ENOMEM;
        return NULL;
    }

    total = nmemb * size;
    ptr = malloc(total);
    if (ptr != NULL) {
        uint8_t *p = (uint8_t *)ptr;
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    void *new_ptr;
    size_t old_size;

    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    if (size > UINT32_MAX) {
        errno = ENOMEM;
        return NULL;
    }

    compat_heap_init();
    old_size = trunk_msize(&compat_heap, (char *)ptr);
    new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    memcpy(new_ptr, ptr, old_size < size ? old_size : size);
    free(ptr);
    return new_ptr;
}

int getpagesize(void) {
    return 4096;
}

/*
 * The kernel has no user-space memory mapping facility; anonymous private
 * mappings are emulated with zero-filled heap memory.
 */
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
    void *ptr;

    (void)prot;
    (void)offset;
    if (len == 0 || len > UINT32_MAX - 4096) {
        errno = EINVAL;
        return MAP_FAILED;
    }
    if (fd >= 0 || (flags & MAP_FIXED) != 0 || addr != NULL) {
        /* file-backed and fixed-address mappings are not supported */
        errno = ENODEV;
        return MAP_FAILED;
    }

    ptr = ewok_reg_alloc(len, 4096);
    if (ptr == NULL) {
        errno = ENOMEM;
        return MAP_FAILED;
    }
    memset(ptr, 0, len);
    return ptr;
}

int munmap(void *addr, size_t len) {
    (void)len;
    if (addr == NULL || addr == MAP_FAILED) {
        errno = EINVAL;
        return -1;
    }
    if (!ewok_reg_try_free(addr)) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int mprotect(void *addr, size_t len, int prot) {
    (void)len;
    (void)prot;
    if (addr == NULL) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int msync(void *addr, size_t len, int flags) {
    (void)len;
    (void)flags;
    if (addr == NULL) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int posix_madvise(void *addr, size_t len, int advice) {
    (void)len;
    if (addr == NULL || advice < POSIX_MADV_NORMAL ||
            advice > POSIX_MADV_DONTNEED) {
        return EINVAL;
    }
    return 0;
}

void *memalign(size_t alignment, size_t size) {
    /* Small alignments are already satisfied by the heap allocator. */
    if (alignment <= 8) {
        return malloc(size);
    }
    if ((alignment & (alignment - 1)) != 0) {
        errno = EINVAL;
        return NULL;
    }
    void *ptr = ewok_reg_alloc(size, alignment);
    if (ptr == NULL) {
        errno = ENOMEM;
    }
    return ptr;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    void *ptr;

    if (memptr == NULL || alignment < sizeof(void *) ||
            (alignment & (alignment - 1)) != 0) {
        return EINVAL;
    }

    if (alignment <= 8) {
        ptr = malloc(size);
    } else {
        ptr = ewok_reg_alloc(size, alignment);
    }
    if (ptr == NULL) {
        return ENOMEM;
    }
    *memptr = ptr;
    return 0;
}

void *valloc(size_t size) {
    return memalign(4096, size);
}

int rand_r(unsigned int *seedp) {
    if (seedp == NULL) {
        return rand();
    }
    *seedp = *seedp * 1103515245u + 12345u;
    return (int)((*seedp >> 16) & 0x7fff);
}

int system(const char *command) {
    /* There is no shell / fork() on this system. */
    if (command == NULL) {
        return 0;
    }
    errno = ENOSYS;
    return -1;
}

char *getenv(const char *name) {
    size_t name_len;

    if (name == NULL || environ == NULL) {
        return NULL;
    }

    name_len = strlen(name);
    for (char **ep = environ; *ep != NULL; ++ep) {
        if (strncmp(*ep, name, name_len) == 0 && (*ep)[name_len] == '=') {
            return *ep + name_len + 1;
        }
    }
    return NULL;
}

void abort(void) {
    _exit(1);
    for (;;) {
    }
}

int remove(const char *path) {
    return unlink(path);
}

int open(const char *pathname, int flags, ...) {
    return _open(pathname, flags, 0);
}

int close(int fd) {
    return _close(fd);
}

ssize_t read(int fd, void *buf, size_t count) {
    return (ssize_t)_read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return (ssize_t)_write(fd, buf, count);
}

off_t lseek(int fd, off_t offset, int whence) {
    return (off_t)_lseek(fd, (_off_t)offset, whence);
}

int fstat(int fd, struct stat *st) {
    return _fstat(fd, st);
}

int stat(const char *path, struct stat *st) {
    return _stat(path, st);
}

int unlink(const char *path) {
    return _unlink(path);
}

int isatty(int fd) {
    return _isatty(fd);
}

int fork(void) {
    return _fork();
}

int access(const char *path, int mode) {
    fsinfo_t info;

    if (vfs_get_by_name(path, &info) != 0) {
        return -1;
    }

    if (mode == F_OK) {
        return 0;
    }

    if ((mode & ~(R_OK | W_OK | X_OK)) != 0) {
        errno = EINVAL;
        return -1;
    }

    if ((mode & R_OK) != 0 && vfs_check_access(getpid(), &info, R_OK) != 0) {
        return -1;
    }
    if ((mode & W_OK) != 0 && vfs_check_access(getpid(), &info, W_OK) != 0) {
        return -1;
    }
    if ((mode & X_OK) != 0 && vfs_check_access(getpid(), &info, X_OK) != 0) {
        return -1;
    }
    return 0;
}

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 != 0 && *s2 != 0) {
        unsigned char a = (unsigned char)ascii_tolower(*s1);
        unsigned char b = (unsigned char)ascii_tolower(*s2);
        if (a != b) {
            return (int)a - (int)b;
        }
        ++s1;
        ++s2;
    }
    return (int)(unsigned char)ascii_tolower(*s1) - (int)(unsigned char)ascii_tolower(*s2);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        unsigned char a = (unsigned char)ascii_tolower(s1[i]);
        unsigned char b = (unsigned char)ascii_tolower(s2[i]);
        if (a != b || s1[i] == 0 || s2[i] == 0) {
            return (int)a - (int)b;
        }
    }
    return 0;
}

void bzero(void *s, size_t n) {
    memset(s, 0, n);
}

int getopt(int argc, char * const argv[], const char *optstring) {
    static int optpos = 1;
    char *arg;
    char *optdecl;

    if (optind >= argc || argv[optind] == NULL) {
        return -1;
    }

    arg = argv[optind];
    if (arg[0] != '-' || arg[1] == '\0') {
        return -1;
    }
    if (strcmp(arg, "--") == 0) {
        ++optind;
        optpos = 1;
        return -1;
    }

    optopt = arg[optpos++];
    optdecl = strchr(optstring, optopt);
    if (optdecl == NULL) {
        if (arg[optpos] == '\0') {
            ++optind;
            optpos = 1;
        }
        return '?';
    }

    if (optdecl[1] == ':') {
        if (arg[optpos] != '\0') {
            optarg = &arg[optpos];
            ++optind;
            optpos = 1;
        } else if (optind + 1 < argc) {
            optarg = argv[++optind];
            ++optind;
            optpos = 1;
        } else {
            optpos = 1;
            ++optind;
            return '?';
        }
    } else {
        optarg = NULL;
        if (arg[optpos] == '\0') {
            ++optind;
            optpos = 1;
        }
    }

    return optopt;
}

static const struct option *find_long_option(const char *name, size_t name_len,
        const struct option *longopts, int *index_out) {
    int i = 0;

    if (longopts == NULL) {
        return NULL;
    }

    while (longopts[i].name != NULL) {
        if (strncmp(longopts[i].name, name, name_len) == 0 &&
                longopts[i].name[name_len] == '\0') {
            if (index_out != NULL) {
                *index_out = i;
            }
            return &longopts[i];
        }
        ++i;
    }
    return NULL;
}

int getopt_long(int argc, char * const argv[], const char *optstring,
        const struct option *longopts, int *longindex) {
    char *arg;
    char *name;
    char *value;
    size_t name_len;
    const struct option *opt;
    int opt_idx = -1;

    if (optind >= argc || argv[optind] == NULL) {
        return -1;
    }

    arg = argv[optind];
    if (strcmp(arg, "--") == 0) {
        ++optind;
        return -1;
    }
    if (arg[0] != '-' || arg[1] == '\0') {
        return -1;
    }
    if (arg[1] != '-' || arg[2] == '\0') {
        return getopt(argc, argv, optstring);
    }

    name = arg + 2;
    value = strchr(name, '=');
    name_len = (value != NULL) ? (size_t)(value - name) : strlen(name);
    opt = find_long_option(name, name_len, longopts, &opt_idx);
    if (opt == NULL) {
        optopt = 0;
        ++optind;
        return '?';
    }

    if (longindex != NULL) {
        *longindex = opt_idx;
    }

    if (opt->has_arg == no_argument) {
        if (value != NULL) {
            optarg = NULL;
            ++optind;
            return '?';
        }
        optarg = NULL;
    } else if (opt->has_arg == required_argument) {
        if (value != NULL) {
            optarg = value + 1;
        } else if (optind + 1 < argc) {
            optarg = argv[++optind];
        } else {
            optarg = NULL;
            ++optind;
            return '?';
        }
    } else {
        optarg = (value != NULL) ? (value + 1) : NULL;
    }

    ++optind;
    if (opt->flag != NULL) {
        *(opt->flag) = opt->val;
        return 0;
    }
    return opt->val;
}

int gettimeofday(struct timeval *tp, void *tzvp) {
    return _gettimeofday(tp, tzvp);
}

time_t time(time_t *timer) {
    struct timeval tv;
    time_t now = (time_t)0;

    if (_gettimeofday(&tv, NULL) == 0) {
        now = (time_t)tv.tv_sec;
    }
    if (timer != NULL) {
        *timer = now;
    }
    return now;
}

clock_t times(struct tms *tp) {
    return _times(tp);
}

clock_t clock(void) {
    struct tms tm;
    return _times(&tm);
}

static void fill_tm_from_epoch(time_t epoch, struct tm *out) {
    static const int month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    long long days = (long long)epoch / 86400;
    long long rem = (long long)epoch % 86400;
    int year = 1970;
    int month = 0;
    int yday = 0;

    if (rem < 0) {
        rem += 86400;
        --days;
    }

    out->tm_hour = (int)(rem / 3600);
    rem %= 3600;
    out->tm_min = (int)(rem / 60);
    out->tm_sec = (int)(rem % 60);

    while (1) {
        int leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
        int year_days = leap ? 366 : 365;
        if (days < year_days) {
            break;
        }
        days -= year_days;
        ++year;
    }

    yday = (int)days;
    while (1) {
        int dim = month_days[month];
        int leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
        if (month == 1 && leap) {
            ++dim;
        }
        if (days < dim) {
            break;
        }
        days -= dim;
        ++month;
    }

    out->tm_year = year - 1900;
    out->tm_mon = month;
    out->tm_mday = (int)days + 1;
    out->tm_yday = yday;
    out->tm_wday = (int)((((long long)epoch / 86400) + 4) % 7);
    if (out->tm_wday < 0) {
        out->tm_wday += 7;
    }
    out->tm_isdst = 0;
}

static int is_leap_year(int year) {
    return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
}

static int month_days_for_year(int year, int month) {
    static const int month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month == 1 && is_leap_year(year)) {
        return 29;
    }
    return month_days[month];
}

struct tm *gmtime(const time_t *timer) {
    static struct tm tm_buf;
    time_t epoch = (timer != NULL) ? *timer : 0;

    fill_tm_from_epoch(epoch, &tm_buf);
    return &tm_buf;
}

/*
 * Support the simple POSIX TZ forms used by EwokOS init scripts, such as
 * "CST-8". In POSIX syntax the numeric offset is what must be added to local
 * time to obtain UTC, so local time is epoch - offset.
 */
static long parse_tz_offset_seconds(const char *tz) {
    const char *p = tz;
    char *end = NULL;
    long hours = 0;
    long minutes = 0;
    long seconds = 0;
    int sign = 1;

    if (tz == NULL || tz[0] == '\0') {
        return 0;
    }

    while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) {
        ++p;
    }

    if (*p == '\0') {
        return 0;
    }

    if (*p == '+') {
        sign = 1;
        ++p;
    }
    else if (*p == '-') {
        sign = -1;
        ++p;
    }

    if (*p < '0' || *p > '9') {
        return 0;
    }

    hours = strtol(p, &end, 10);
    if (end == p) {
        return 0;
    }
    p = end;

    if (*p == ':') {
        ++p;
        minutes = strtol(p, &end, 10);
        if (end == p) {
            minutes = 0;
        }
        p = end;
    }

    if (*p == ':') {
        ++p;
        seconds = strtol(p, &end, 10);
        if (end == p) {
            seconds = 0;
        }
    }

    return sign * (hours * 3600L + minutes * 60L + seconds);
}

struct tm *localtime(const time_t *timer) {
    static struct tm tm_buf;
    time_t epoch = (timer != NULL) ? *timer : 0;
    long tz_offset = parse_tz_offset_seconds(getenv("TZ"));

    fill_tm_from_epoch(epoch - tz_offset, &tm_buf);
    return &tm_buf;
}

struct tm *localtime_r(const time_t *timer, struct tm *result) {
    struct tm *tmp;

    if (result == NULL) {
        errno = EINVAL;
        return NULL;
    }
    tmp = localtime(timer);
    if (tmp == NULL) {
        return NULL;
    }
    *result = *tmp;
    return result;
}

struct tm *gmtime_r(const time_t *timer, struct tm *result) {
    time_t epoch;

    if (result == NULL) {
        errno = EINVAL;
        return NULL;
    }
    epoch = (timer != NULL) ? *timer : 0;
    fill_tm_from_epoch(epoch, result);
    return result;
}

char *asctime_r(const struct tm *tm, char *buf) {
    static const char wday_abbr[7][4] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const char month_abbr[12][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    if (tm == NULL || buf == NULL) {
        errno = EINVAL;
        return NULL;
    }

    int wday = tm->tm_wday;
    int mon = tm->tm_mon;
    if (wday < 0 || wday > 6) {
        wday = 0;
    }
    if (mon < 0 || mon > 11) {
        mon = 0;
    }

    snprintf(buf, 26, "%s %s%3d %.2d:%.2d:%.2d %d\n",
             wday_abbr[wday], month_abbr[mon], tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year + 1900);
    return buf;
}

char *asctime(const struct tm *tm) {
    static char asctime_buf[32];
    return asctime_r(tm, asctime_buf);
}

char *ctime_r(const time_t *timer, char *buf) {
    struct tm tm;

    if (timer == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (gmtime_r(timer, &tm) == NULL) {
        return NULL;
    }
    return asctime_r(&tm, buf);
}

char *ctime(const time_t *timer) {
    static char ctime_buf[32];
    return ctime_r(timer, ctime_buf);
}

double difftime(time_t time1, time_t time0) {
    return (double)((long long)time1 - (long long)time0);
}

void tzset(void) {
    /* TZ is re-read from the environment on every localtime() call. */
}

time_t mktime(struct tm *tm) {
    long long days = 0;
    long long seconds;
    long tz_offset;
    int year;
    int month;

    if (tm == NULL) {
        errno = EINVAL;
        return (time_t)-1;
    }

    year = tm->tm_year + 1900;
    month = tm->tm_mon;

    while (month < 0) {
        month += 12;
        --year;
    }
    while (month >= 12) {
        month -= 12;
        ++year;
    }

    if (year >= 1970) {
        for (int y = 1970; y < year; ++y) {
            days += is_leap_year(y) ? 366 : 365;
        }
    } else {
        for (int y = 1969; y >= year; --y) {
            days -= is_leap_year(y) ? 366 : 365;
        }
    }

    for (int m = 0; m < month; ++m) {
        days += month_days_for_year(year, m);
    }

    days += (long long)tm->tm_mday - 1;
    seconds = days * 86400LL +
        (long long)tm->tm_hour * 3600LL +
        (long long)tm->tm_min * 60LL +
        (long long)tm->tm_sec;

    tz_offset = parse_tz_offset_seconds(getenv("TZ"));
    seconds += tz_offset;

    fill_tm_from_epoch((time_t)(seconds - tz_offset), tm);
    tm->tm_isdst = 0;

    return (time_t)seconds;
}

static void append_char(char *buf, size_t size, size_t *pos, char c) {
    if (*pos + 1 < size) {
        buf[*pos] = c;
    }
    (*pos)++;
}

typedef enum {
    FMT_LEN_NONE = 0,
    FMT_LEN_HH,
    FMT_LEN_H,
    FMT_LEN_L,
    FMT_LEN_LL,
    FMT_LEN_Z,
    FMT_LEN_T,
    FMT_LEN_J,
    FMT_LEN_LD
} fmt_len_t;

typedef struct {
    int left;
    int plus;
    int space;
    int alt;
    int zero;
    int width;
    int precision;
    fmt_len_t length;
    char spec;
} fmt_spec_t;

static void append_repeat(char *buf, size_t size, size_t *pos, char c, int count) {
    while (count-- > 0) {
        append_char(buf, size, pos, c);
    }
}

static void append_mem(char *buf, size_t size, size_t *pos, const char *s, size_t len) {
    size_t i;

    for (i = 0; i < len; ++i) {
        append_char(buf, size, pos, s[i]);
    }
}

static size_t utoa_base(unsigned long long value, unsigned base, int upper, char *tmp) {
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    size_t len = 0;

    do {
        tmp[len++] = digits[value % base];
        value /= base;
    } while (value != 0);

    return len;
}

static void fmt_parse(const char **format, fmt_spec_t *spec) {
    const char *p = *format;

    memset(spec, 0, sizeof(*spec));
    spec->precision = -1;

    for (;;) {
        if (*p == '-') {
            spec->left = 1;
        }
        else if (*p == '+') {
            spec->plus = 1;
        }
        else if (*p == ' ') {
            spec->space = 1;
        }
        else if (*p == '#') {
            spec->alt = 1;
        }
        else if (*p == '0') {
            spec->zero = 1;
        }
        else {
            break;
        }
        ++p;
    }

    if (*p == '*') {
        spec->width = INT_MIN;
        ++p;
    }
    else {
        while (*p >= '0' && *p <= '9') {
            spec->width = spec->width * 10 + (*p - '0');
            ++p;
        }
    }

    if (*p == '.') {
        ++p;
        spec->precision = 0;
        if (*p == '*') {
            spec->precision = INT_MIN;
            ++p;
        }
        else {
            while (*p >= '0' && *p <= '9') {
                spec->precision = spec->precision * 10 + (*p - '0');
                ++p;
            }
        }
    }

    if (*p == 'h' && *(p + 1) == 'h') {
        spec->length = FMT_LEN_HH;
        p += 2;
    }
    else if (*p == 'h') {
        spec->length = FMT_LEN_H;
        ++p;
    }
    else if (*p == 'l' && *(p + 1) == 'l') {
        spec->length = FMT_LEN_LL;
        p += 2;
    }
    else if (*p == 'l') {
        spec->length = FMT_LEN_L;
        ++p;
    }
    else if (*p == 'z') {
        spec->length = FMT_LEN_Z;
        ++p;
    }
    else if (*p == 't') {
        spec->length = FMT_LEN_T;
        ++p;
    }
    else if (*p == 'j') {
        spec->length = FMT_LEN_J;
        ++p;
    }
    else if (*p == 'L') {
        /*
         * C99 long double. EwokOS targets do not link libgcc's 128-bit
         * quad-precision routines, so 'L' is accepted for compatibility and
         * the argument is consumed as a double (long double == double on arm;
         * no in-tree code passes a true quad to printf on aarch64).
         */
        spec->length = FMT_LEN_LD;
        ++p;
    }

    spec->spec = *p;
    if (*p != 0) {
        ++p;
    }
    *format = p;
}

#define FMT_GET_SIGNED(ap, length) \
    ((length) == FMT_LEN_HH ? (long long)(signed char)va_arg((ap), int) : \
     (length) == FMT_LEN_H ? (long long)(short)va_arg((ap), int) : \
     (length) == FMT_LEN_L ? (long long)va_arg((ap), long) : \
     (length) == FMT_LEN_LL ? (long long)va_arg((ap), long long) : \
     (length) == FMT_LEN_Z ? (long long)va_arg((ap), ssize_t) : \
     (length) == FMT_LEN_T ? (long long)va_arg((ap), ptrdiff_t) : \
     (length) == FMT_LEN_J ? (long long)va_arg((ap), long long) : \
     (long long)va_arg((ap), int))

#define FMT_GET_UNSIGNED(ap, length) \
    ((length) == FMT_LEN_HH ? (unsigned long long)(unsigned char)va_arg((ap), unsigned int) : \
     (length) == FMT_LEN_H ? (unsigned long long)(unsigned short)va_arg((ap), unsigned int) : \
     (length) == FMT_LEN_L ? (unsigned long long)va_arg((ap), unsigned long) : \
     (length) == FMT_LEN_LL ? (unsigned long long)va_arg((ap), unsigned long long) : \
     (length) == FMT_LEN_Z ? (unsigned long long)va_arg((ap), size_t) : \
     (length) == FMT_LEN_T ? (unsigned long long)va_arg((ap), ptrdiff_t) : \
     (length) == FMT_LEN_J ? (unsigned long long)va_arg((ap), unsigned long long) : \
     (unsigned long long)va_arg((ap), unsigned int))

static void append_formatted_string(char *buf, size_t size, size_t *pos,
        const char *s, const fmt_spec_t *spec) {
    size_t len;
    int pad;

    if (s == NULL) {
        s = "(null)";
    }
    len = strlen(s);
    if (spec->precision >= 0 && len > (size_t)spec->precision) {
        len = (size_t)spec->precision;
    }
    pad = spec->width - (int)len;
    if (!spec->left && pad > 0) {
        append_repeat(buf, size, pos, ' ', pad);
    }
    append_mem(buf, size, pos, s, len);
    if (spec->left && pad > 0) {
        append_repeat(buf, size, pos, ' ', pad);
    }
}

static void append_formatted_char(char *buf, size_t size, size_t *pos,
        char c, const fmt_spec_t *spec) {
    int pad = spec->width - 1;

    if (!spec->left && pad > 0) {
        append_repeat(buf, size, pos, ' ', pad);
    }
    append_char(buf, size, pos, c);
    if (spec->left && pad > 0) {
        append_repeat(buf, size, pos, ' ', pad);
    }
}

static void append_formatted_integer(char *buf, size_t size, size_t *pos,
        unsigned long long value, int negative, unsigned base, int upper,
        const fmt_spec_t *spec) {
    char digits[65];
    const char *prefix = "";
    size_t len = 0;
    int prefix_len = 0;
    int sign_len = 0;
    int zeroes = 0;
    int spaces = 0;
    char sign = 0;
    int precision = spec->precision;

    if ((spec->spec == 'd' || spec->spec == 'i')) {
        if (negative) {
            sign = '-';
        }
        else if (spec->plus) {
            sign = '+';
        }
        else if (spec->space) {
            sign = ' ';
        }
    }

    if (spec->spec == 'p') {
        prefix = "0x";
        prefix_len = 2;
        if (precision < 0) {
            precision = (int)(sizeof(ewokos_addr_t) * 2);
        }
    }
    else if (spec->alt && value != 0) {
        if (base == 16) {
            prefix = upper ? "0X" : "0x";
            prefix_len = 2;
        }
        else if (base == 8) {
            prefix = "0";
            prefix_len = 1;
        }
    }

    if (value == 0 && precision == 0) {
        len = 0;
        if (base == 8 && spec->alt) {
            prefix = "0";
            prefix_len = 1;
        }
    }
    else {
        len = utoa_base(value, base, upper, digits);
    }

    if (sign != 0) {
        sign_len = 1;
    }

    if (precision > (int)len) {
        zeroes = precision - (int)len;
    }
    else if (spec->zero && !spec->left && precision < 0) {
        int total = sign_len + prefix_len + (int)len;
        if (spec->width > total) {
            zeroes = spec->width - total;
        }
    }

    spaces = spec->width - sign_len - prefix_len - zeroes - (int)len;
    if (spaces < 0) {
        spaces = 0;
    }

    if (!spec->left) {
        append_repeat(buf, size, pos, ' ', spaces);
    }
    if (sign != 0) {
        append_char(buf, size, pos, sign);
    }
    if (prefix_len > 0) {
        append_mem(buf, size, pos, prefix, (size_t)prefix_len);
    }
    append_repeat(buf, size, pos, '0', zeroes);
    while (len > 0) {
        append_char(buf, size, pos, digits[--len]);
    }
    if (spec->left) {
        append_repeat(buf, size, pos, ' ', spaces);
    }
}

static void append_formatted_float(char *buf, size_t size, size_t *pos,
        double value, const fmt_spec_t *spec) {
    char out[128];
    char int_digits[32];
    size_t int_len;
    size_t out_len = 0;
    unsigned long long int_part;
    unsigned long long frac_part = 0;
    unsigned long long scale = 1;
    int precision = spec->precision >= 0 ? spec->precision : 6;
    int negative = 0;
    char sign = 0;
    int pad;
    int i;

    if (precision > 18) {
        precision = 18;
    }

    if (value != value) {
        append_formatted_string(buf, size, pos, "nan", spec);
        return;
    }
    if (value > DBL_MAX || value < -DBL_MAX) {
        if (value < 0.0) {
            append_formatted_string(buf, size, pos, "-inf", spec);
        }
        else if (spec->plus) {
            append_formatted_string(buf, size, pos, "+inf", spec);
        }
        else if (spec->space) {
            append_formatted_string(buf, size, pos, " inf", spec);
        }
        else {
            append_formatted_string(buf, size, pos, "inf", spec);
        }
        return;
    }

    if (value < 0.0) {
        negative = 1;
        value = -value;
    }
    if (negative) {
        sign = '-';
    }
    else if (spec->plus) {
        sign = '+';
    }
    else if (spec->space) {
        sign = ' ';
    }

    for (i = 0; i < precision; ++i) {
        scale *= 10ULL;
    }

    if (precision > 0) {
        double delta = 0.5;
        for (i = 0; i < precision; ++i) {
            delta /= 10.0;
        }
        value += delta;
    }
    else {
        value += 0.5;
    }

    int_part = (unsigned long long)value;
    if (precision > 0) {
        double frac = value - (double)int_part;
        if (frac < 0.0) {
            frac = 0.0;
        }
        frac_part = (unsigned long long)(frac * (double)scale);
        if (frac_part >= scale) {
            ++int_part;
            frac_part -= scale;
        }
    }

    int_len = utoa_base(int_part, 10, 0, int_digits);
    if (sign != 0) {
        out[out_len++] = sign;
    }
    while (int_len > 0) {
        out[out_len++] = int_digits[--int_len];
    }
    if (precision > 0 || spec->alt) {
        out[out_len++] = '.';
    }
    if (precision > 0) {
        for (i = precision - 1; i >= 0; --i) {
            out[out_len + i] = (char)('0' + (frac_part % 10ULL));
            frac_part /= 10ULL;
        }
        out_len += (size_t)precision;
    }
    out[out_len] = 0;

    pad = spec->width - (int)out_len;
    if (!spec->left && pad > 0) {
        char pad_char = (spec->zero ? '0' : ' ');
        if (pad_char == '0' && sign != 0) {
            append_char(buf, size, pos, sign);
            append_repeat(buf, size, pos, '0', pad);
            append_mem(buf, size, pos, out + 1, out_len - 1);
            return;
        }
        append_repeat(buf, size, pos, pad_char, pad);
    }
    append_mem(buf, size, pos, out, out_len);
    if (spec->left && pad > 0) {
        append_repeat(buf, size, pos, ' ', pad);
    }
}

/*
 * Emit an already-built number string with field-width padding. Mirrors the
 * tail of append_formatted_float so the %e/%g/%a paths share one routine.
 */
static void emit_padded(char *buf, size_t size, size_t *pos,
        const char *out, int out_len, const fmt_spec_t *spec) {
    int pad = spec->width - out_len;

    if (!spec->left && pad > 0) {
        char pad_char = spec->zero ? '0' : ' ';
        if (pad_char == '0' && out_len > 0 &&
                (out[0] == '-' || out[0] == '+' || out[0] == ' ')) {
            append_char(buf, size, pos, out[0]);
            append_repeat(buf, size, pos, '0', pad);
            append_mem(buf, size, pos, out + 1, (size_t)(out_len - 1));
            return;
        }
        append_repeat(buf, size, pos, pad_char, pad);
    }
    append_mem(buf, size, pos, out, (size_t)out_len);
    if (spec->left && pad > 0) {
        append_repeat(buf, size, pos, ' ', pad);
    }
}

/* %e / %E : [-]d.ddddde[+-]DD  (exponent always at least two digits) */
static void append_formatted_exp(char *buf, size_t size, size_t *pos,
        double value, const fmt_spec_t *spec, int upper) {
    char out[64];
    char ebuf[8];
    int out_len = 0;
    int precision = spec->precision >= 0 ? spec->precision : 6;
    int exponent = 0;
    int en = 0;
    int i;
    char sign = 0;
    char expch = upper ? 'E' : 'e';
    unsigned long long ip;
    unsigned long long fp = 0;
    unsigned long long scale = 1;

    if (precision > 17) {
        precision = 17;
    }

    if (value != value) {
        emit_padded(buf, size, pos, upper ? "NAN" : "nan", 3, spec);
        return;
    }
    if (value > DBL_MAX || value < -DBL_MAX) {
        if (value < 0.0) {
            emit_padded(buf, size, pos, upper ? "-INF" : "-inf", 4, spec);
        }
        else {
            emit_padded(buf, size, pos, upper ? "INF" : "inf", 3, spec);
        }
        return;
    }

    if (value < 0.0) {
        sign = '-';
        value = -value;
    }
    else if (spec->plus) {
        sign = '+';
    }
    else if (spec->space) {
        sign = ' ';
    }

    if (value != 0.0) {
        while (value >= 10.0) { value /= 10.0; exponent++; }
        while (value < 1.0)   { value *= 10.0; exponent--; }
    }

    {
        double delta = 0.5;
        for (i = 0; i < precision; ++i) {
            delta /= 10.0;
        }
        value += delta;
        if (value >= 10.0) { value /= 10.0; exponent++; }
    }

    ip = (unsigned long long)value;
    if (precision > 0) {
        double frac = value - (double)ip;
        for (i = 0; i < precision; ++i) {
            scale *= 10ULL;
        }
        fp = (unsigned long long)(frac * (double)scale);
        if (fp >= scale) { fp = 0; ip++; }
        if (ip >= 10ULL) { ip = 1ULL; exponent++; }
    }

    if (sign != 0) {
        out[out_len++] = sign;
    }
    out[out_len++] = (char)('0' + (int)(ip % 10ULL));
    if (precision > 0 || spec->alt) {
        out[out_len++] = '.';
    }
    for (i = precision - 1; i >= 0; --i) {
        out[out_len + i] = (char)('0' + (int)(fp % 10ULL));
        fp /= 10ULL;
    }
    out_len += precision;
    out[out_len++] = expch;
    out[out_len++] = (exponent < 0) ? '-' : '+';
    if (exponent < 0) {
        exponent = -exponent;
    }
    do {
        ebuf[en++] = (char)('0' + (exponent % 10));
        exponent /= 10;
    } while (exponent != 0);
    while (en < 2) {
        ebuf[en++] = '0';
    }
    for (i = en - 1; i >= 0; --i) {
        out[out_len++] = ebuf[i];
    }
    out[out_len] = 0;

    emit_padded(buf, size, pos, out, out_len, spec);
}

/* %a / %A : [-]0xh.hhhhp[+-]D  (IEEE-754 double bit decomposition, no libm) */
static void append_formatted_hexfloat(char *buf, size_t size, size_t *pos,
        double value, const fmt_spec_t *spec, int upper) {
    union { double d; uint64_t u; } cvt;
    const char *hexd = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char xch = upper ? 'X' : 'x';
    char pch = upper ? 'P' : 'p';
    char out[128];
    char raw[16];
    char frac[80];
    int out_len = 0;
    int fl = 0;
    int precision = spec->precision;
    int sigdigits;
    int bexp;
    int dec_exp;
    int lead;
    int i;
    char sign = 0;
    uint64_t u;
    uint64_t mant;

    if (precision > 64) {
        precision = 64;
    }

    cvt.d = value;
    u = cvt.u;
    bexp = (int)((u >> 52) & 0x7FFULL);
    mant = u & 0xFFFFFFFFFFFFFULL;

    if (bexp == 0x7FF) {
        if (mant != 0ULL) {
            emit_padded(buf, size, pos, upper ? "NAN" : "nan", 3, spec);
        }
        else if (u >> 63) {
            emit_padded(buf, size, pos, upper ? "-INF" : "-inf", 4, spec);
        }
        else {
            emit_padded(buf, size, pos, upper ? "INF" : "inf", 3, spec);
        }
        return;
    }

    if (u >> 63) {
        sign = '-';
    }
    else if (spec->plus) {
        sign = '+';
    }
    else if (spec->space) {
        sign = ' ';
    }

    if (bexp == 0) {
        lead = 0;
        dec_exp = -1022;
        if (mant == 0ULL) {
            if (sign != 0) {
                out[out_len++] = sign;
            }
            out[out_len++] = '0';
            out[out_len++] = xch;
            out[out_len++] = '0';
            if (precision > 0 || spec->alt) {
                out[out_len++] = '.';
            }
            for (i = 0; i < precision; ++i) {
                out[out_len++] = '0';
            }
            out[out_len++] = pch;
            out[out_len++] = '+';
            out[out_len++] = '0';
            out[out_len] = 0;
            emit_padded(buf, size, pos, out, out_len, spec);
            return;
        }
    }
    else {
        lead = 1;
        dec_exp = bexp - 1023;
    }

    if (precision >= 0 && precision < 13) {
        int shift = (13 - precision) * 4;
        uint64_t half = 1ULL << (shift - 1);
        uint64_t dropped = mant & ((half << 1) - 1ULL);
        mant >>= shift;
        if (dropped > half) {
            mant++;
            if (precision == 0 || mant >= (1ULL << (precision * 4))) {
                mant = 0;
                if (lead == 1) {
                    dec_exp++;
                }
                else {
                    lead = 1;
                }
            }
        }
        sigdigits = precision;
    }
    else {
        sigdigits = 13;
    }

    for (i = sigdigits - 1; i >= 0; --i) {
        raw[i] = hexd[mant & 0xFULL];
        mant >>= 4;
    }

    if (precision < 0) {
        for (i = 0; i < 13; ++i) {
            frac[fl++] = raw[i];
        }
        if (!spec->alt) {
            while (fl > 0 && frac[fl - 1] == '0') {
                fl--;
            }
        }
    }
    else {
        for (i = 0; i < sigdigits; ++i) {
            frac[fl++] = raw[i];
        }
        while (fl < precision && fl < (int)sizeof(frac)) {
            frac[fl++] = '0';
        }
    }

    if (sign != 0) {
        out[out_len++] = sign;
    }
    out[out_len++] = '0';
    out[out_len++] = xch;
    out[out_len++] = (char)('0' + lead);
    if (fl > 0 || spec->alt) {
        out[out_len++] = '.';
        for (i = 0; i < fl; ++i) {
            out[out_len++] = frac[i];
        }
    }
    out[out_len++] = pch;
    out[out_len++] = (dec_exp < 0) ? '-' : '+';
    if (dec_exp < 0) {
        dec_exp = -dec_exp;
    }
    {
        char e2[8];
        int en = 0;
        do {
            e2[en++] = (char)('0' + (dec_exp % 10));
            dec_exp /= 10;
        } while (dec_exp != 0);
        for (i = en - 1; i >= 0; --i) {
            out[out_len++] = e2[i];
        }
    }
    out[out_len] = 0;

    emit_padded(buf, size, pos, out, out_len, spec);
}

/* %g / %G : shorter of %e/%f, trailing zeros trimmed unless '#' */
static void append_formatted_g(char *buf, size_t size, size_t *pos,
        double value, const fmt_spec_t *spec, int upper) {
    char tmpbuf[256];
    size_t tpos = 0;
    fmt_spec_t sub;
    double av;
    int X = 0;
    int P;
    int len;
    int i;

    if (value != value || value > DBL_MAX || value < -DBL_MAX) {
        append_formatted_exp(buf, size, pos, value, spec, upper);
        return;
    }

    P = (spec->precision >= 0) ? spec->precision : 6;
    if (P == 0) {
        P = 1;
    }

    av = (value < 0.0) ? -value : value;
    if (av != 0.0) {
        while (av >= 10.0) { av /= 10.0; X++; }
        while (av < 1.0)   { av *= 10.0; X--; }
    }

    sub = *spec;
    sub.width = 0;
    sub.left = 0;
    sub.zero = 0;

    if (P > X && X >= -4) {
        sub.precision = P - 1 - X;
        if (sub.precision < 0) {
            sub.precision = 0;
        }
        append_formatted_float(tmpbuf, sizeof(tmpbuf), &tpos, value, &sub);
    }
    else {
        sub.precision = P - 1;
        if (sub.precision < 0) {
            sub.precision = 0;
        }
        append_formatted_exp(tmpbuf, sizeof(tmpbuf), &tpos, value, &sub, upper);
    }

    len = (int)tpos;
    if (len > (int)sizeof(tmpbuf) - 1) {
        len = (int)sizeof(tmpbuf) - 1;
    }
    tmpbuf[len] = 0;

    if (!spec->alt) {
        int epos = -1;
        int end;
        int hasdot = 0;
        for (i = 0; i < len; ++i) {
            if (tmpbuf[i] == 'e' || tmpbuf[i] == 'E') { epos = i; break; }
        }
        end = (epos >= 0) ? epos : len;
        for (i = 0; i < end; ++i) {
            if (tmpbuf[i] == '.') { hasdot = 1; break; }
        }
        if (hasdot) {
            int k = end - 1;
            int newlen;
            while (k > 0 && tmpbuf[k] == '0') {
                k--;
            }
            if (tmpbuf[k] == '.') {
                k--;
            }
            newlen = k + 1;
            if (epos >= 0) {
                int en = len - epos;
                for (i = 0; i < en; ++i) {
                    tmpbuf[newlen + i] = tmpbuf[epos + i];
                }
                newlen += en;
            }
            len = newlen;
            tmpbuf[len] = 0;
        }
    }

    emit_padded(buf, size, pos, tmpbuf, len, spec);
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    size_t pos = 0;

    if (str == NULL || size == 0) {
        size = 0;
    }

    while (*format != 0) {
        if (*format != '%') {
            append_char(str, size, &pos, *format++);
            continue;
        }

        ++format;
        if (*format == '%') {
            append_char(str, size, &pos, *format++);
            continue;
        }

        fmt_spec_t spec;
        fmt_parse(&format, &spec);
        if (spec.width == INT_MIN) {
            spec.width = va_arg(ap, int);
            if (spec.width < 0) {
                spec.left = 1;
                spec.width = -spec.width;
            }
        }
        if (spec.precision == INT_MIN) {
            spec.precision = va_arg(ap, int);
            if (spec.precision < 0) {
                spec.precision = -1;
            }
        }
        switch (spec.spec) {
        case 'c':
            append_formatted_char(str, size, &pos, (char)va_arg(ap, int), &spec);
            break;
        case 's':
            append_formatted_string(str, size, &pos, va_arg(ap, const char *), &spec);
            break;
        case 'd':
        case 'i': {
            long long v = FMT_GET_SIGNED(ap, spec.length);
            unsigned long long uv = (v < 0) ? (0ULL - (unsigned long long)v) : (unsigned long long)v;
            append_formatted_integer(str, size, &pos, uv, v < 0, 10, 0, &spec);
            break;
        }
        case 'u':
            append_formatted_integer(str, size, &pos, FMT_GET_UNSIGNED(ap, spec.length), 0, 10, 0, &spec);
            break;
        case 'o':
            append_formatted_integer(str, size, &pos, FMT_GET_UNSIGNED(ap, spec.length), 0, 8, 0, &spec);
            break;
        case 'x':
            append_formatted_integer(str, size, &pos, FMT_GET_UNSIGNED(ap, spec.length), 0, 16, 0, &spec);
            break;
        case 'X':
            append_formatted_integer(str, size, &pos, FMT_GET_UNSIGNED(ap, spec.length), 0, 16, 1, &spec);
            break;
        case 'p':
            spec.zero = 0;
            append_formatted_integer(str, size, &pos, (ewokos_addr_t)va_arg(ap, void *), 0, 16, 0, &spec);
            break;
        case 'f':
        case 'F':
            append_formatted_float(str, size, &pos, va_arg(ap, double), &spec);
            break;
        case 'e':
            append_formatted_exp(str, size, &pos, va_arg(ap, double), &spec, 0);
            break;
        case 'E':
            append_formatted_exp(str, size, &pos, va_arg(ap, double), &spec, 1);
            break;
        case 'g':
            append_formatted_g(str, size, &pos, va_arg(ap, double), &spec, 0);
            break;
        case 'G':
            append_formatted_g(str, size, &pos, va_arg(ap, double), &spec, 1);
            break;
        case 'a':
            append_formatted_hexfloat(str, size, &pos, va_arg(ap, double), &spec, 0);
            break;
        case 'A':
            append_formatted_hexfloat(str, size, &pos, va_arg(ap, double), &spec, 1);
            break;
        case 'n':
            switch (spec.length) {
            case FMT_LEN_HH: *va_arg(ap, signed char *) = (signed char)pos; break;
            case FMT_LEN_H:  *va_arg(ap, short *) = (short)pos; break;
            case FMT_LEN_L:  *va_arg(ap, long *) = (long)pos; break;
            case FMT_LEN_LL: *va_arg(ap, long long *) = (long long)pos; break;
            case FMT_LEN_Z:  *va_arg(ap, ssize_t *) = (ssize_t)pos; break;
            case FMT_LEN_J:  *va_arg(ap, long long *) = (long long)pos; break;
            case FMT_LEN_T:  *va_arg(ap, ptrdiff_t *) = (ptrdiff_t)pos; break;
            default:         *va_arg(ap, int *) = (int)pos; break;
            }
            break;
        case 0:
            --format;
            append_char(str, size, &pos, '%');
            break;
        default:
            append_char(str, size, &pos, '%');
            append_char(str, size, &pos, spec.spec);
            break;
        }
    }

    if (size != 0) {
        size_t term = (pos < size) ? pos : (size - 1);
        str[term] = 0;
    }
    return (int)pos;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

static int vsscanf_impl(const char *str, const char *format, va_list ap, const char **endptr) {
    int assigned = 0;
    const char *s = str;

    while (*format != 0) {
        int width = 0;
        int long_flag = 0;

        if (is_space(*format)) {
            while (is_space(*format)) {
                ++format;
            }
            while (is_space(*s)) {
                ++s;
            }
            continue;
        }

        if (*format != '%') {
            if (*s != *format) {
                break;
            }
            ++s;
            ++format;
            continue;
        }

        ++format;
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            ++format;
        }
        while (*format == 'l') {
            ++long_flag;
            ++format;
        }

        if (*format == 'd') {
            char *endp;
            long long v;
            while (is_space(*s)) {
                ++s;
            }
            v = (long_flag >= 2) ? strtoll(s, &endp, 10) : (long long)strtol(s, &endp, 10);
            if (endp == s) {
                break;
            }
            s = endp;
            if (long_flag >= 2) {
                *va_arg(ap, long long *) = v;
            } else if (long_flag == 1) {
                *va_arg(ap, long *) = (long)v;
            } else {
                *va_arg(ap, int *) = (int)v;
            }
            ++assigned;
        } else if (*format == 'x' || *format == 'X') {
            char *endp;
            unsigned long v;
            while (is_space(*s)) {
                ++s;
            }
            v = strtoul(s, &endp, 16);
            if (endp == s) {
                break;
            }
            s = endp;
            if (long_flag >= 1) {
                *va_arg(ap, unsigned long *) = v;
            } else {
                *va_arg(ap, int *) = (int)v;
            }
            ++assigned;
        } else if (*format == 's') {
            char *out = va_arg(ap, char *);
            int count = 0;
            while (is_space(*s)) {
                ++s;
            }
            while (*s != 0 && !is_space(*s) && (width == 0 || count < width)) {
                out[count++] = *s++;
            }
            if (count == 0) {
                break;
            }
            out[count] = 0;
            ++assigned;
        } else if (*format == 'u') {
            char *endp;
            unsigned long long v;
            while (is_space(*s)) {
                ++s;
            }
            v = (long_flag >= 2) ? strtoull(s, &endp, 10) : (unsigned long long)strtoul(s, &endp, 10);
            if (endp == s) {
                break;
            }
            s = endp;
            if (long_flag >= 2) {
                *va_arg(ap, unsigned long long *) = v;
            } else if (long_flag == 1) {
                *va_arg(ap, unsigned long *) = (unsigned long)v;
            } else {
                *va_arg(ap, unsigned int *) = (unsigned int)v;
            }
            ++assigned;
        } else if (*format == '%') {
            /* literal '%%': consumes one '%' from the input, no assignment */
            if (*s != '%') {
                break;
            }
            ++s;
        } else if (*format == 'g' || *format == 'f') {
            char *endp;
            double v;
            while (is_space(*s)) {
                ++s;
            }
            v = strtod(s, &endp);
            if (endp == s) {
                break;
            }
            s = endp;
            if (long_flag >= 1) {
                *va_arg(ap, double *) = v;
            } else {
                *va_arg(ap, float *) = (float)v;
            }
            ++assigned;
        } else {
            break;
        }

        if (*format != 0) {
            ++format;
        }
    }
    if (endptr != NULL) {
        *endptr = s;
    }
    return assigned;
}

int vsscanf(const char *str, const char *format, va_list ap) {
    va_list ap_copy;
    int ret;

    va_copy(ap_copy, ap);
    ret = vsscanf_impl(str, format, ap_copy, NULL);
    va_end(ap_copy);
    return ret;
}

int sscanf(const char *str, const char *format, ...) {
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vsscanf(str, format, ap);
    va_end(ap);
    return ret;
}

int vfscanf(FILE *stream, const char *format, va_list ap) {
    char buf[4096];
    long start;
    ssize_t nread;
    const char *endp = buf;
    va_list ap_copy;
    int ret;

    if (stream == NULL || format == NULL) {
        errno = EINVAL;
        return EOF;
    }

    start = ftell(stream);
    if (start < 0) {
        return EOF;
    }

    nread = read(stream->fd, buf, sizeof(buf) - 1);
    if (nread <= 0) {
        return EOF;
    }
    buf[nread] = 0;

    va_copy(ap_copy, ap);
    ret = vsscanf_impl(buf, format, ap_copy, &endp);
    va_end(ap_copy);

    fseek(stream, start + (long)(endp - buf), SEEK_SET);
    return ret;
}

int fscanf(FILE *stream, const char *format, ...) {
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vfscanf(stream, format, ap);
    va_end(ap);
    return ret;
}

int vsprintf(char *str, const char *format, va_list ap) {
    return vsnprintf(str, (size_t)-1, format, ap);
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vsprintf(str, format, ap);
    va_end(ap);
    return ret;
}

static ssize_t write_all_retry_compat(int fd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    size_t off = 0;

    while(off < len) {
        ssize_t wr = write(fd, p + off, len - off);
        if(wr > 0) {
            off += (size_t)wr;
            continue;
        }
        if(errno == EAGAIN || errno == EINTR) {
            /* park on the node's WR wait queue with a bounded deadline
             * instead of probing the write once per ms: the driver's wake
             * edge releases us early, an idle retry costs zero CPU. */
            vfs_block_by_fd_timeout(fd, VFS_EVT_WR, 100*1000);
            continue;
        }
        if(wr == 0 && errno == 0)
            errno = EPIPE;
        return off > 0 ? (ssize_t)off : wr;
    }
    return (ssize_t)off;
}

int vfprintf(FILE *stream, const char *format, va_list ap) {
    char stack_buf[512];
    char *buf = stack_buf;
    va_list ap_copy;
    int len;
    int fd = (stream != NULL) ? stream->fd : 1;

    va_copy(ap_copy, ap);
    len = vsnprintf(stack_buf, sizeof(stack_buf), format, ap_copy);
    va_end(ap_copy);

    if (len < 0) {
        return len;
    }
    if (len >= (int)sizeof(stack_buf)) {
        buf = (char *)malloc((size_t)len + 1);
        if (buf == NULL) {
            len = (int)sizeof(stack_buf) - 1;
            buf = stack_buf;
        }
        else {
            va_list ap_full;
            va_copy(ap_full, ap);
            vsnprintf(buf, (size_t)len + 1, format, ap_full);
            va_end(ap_full);
        }
    }
    ssize_t written = write_all_retry_compat(fd, buf, (size_t)len);
    len = (int)written;
    if (buf != stack_buf) {
        free(buf);
    }
    return len;
}

int vasprintf(char **strp, const char *format, va_list ap) {
    va_list ap_copy;
    char *buf;
    int len;

    if (strp == NULL) {
        return -1;
    }
    *strp = NULL;

    va_copy(ap_copy, ap);
    len = vsnprintf(NULL, 0, format, ap_copy);
    va_end(ap_copy);
    if (len < 0) {
        return -1;
    }

    buf = (char *)malloc((size_t)len + 1);
    if (buf == NULL) {
        return -1;
    }
    va_copy(ap_copy, ap);
    vsnprintf(buf, (size_t)len + 1, format, ap_copy);
    va_end(ap_copy);

    *strp = buf;
    return len;
}

int asprintf(char **strp, const char *format, ...) {
    va_list ap;
    int len;

    va_start(ap, format);
    len = vasprintf(strp, format, ap);
    va_end(ap);
    return len;
}

int fprintf(FILE *stream, const char *format, ...) {
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vfprintf(stream, format, ap);
    va_end(ap);
    return ret;
}

int printf(const char *format, ...) {
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vfprintf(stdout, format, ap);
    va_end(ap);
    return ret;
}

FILE *fopen(const char *path, const char *mode) {
    int flags = O_RDONLY;
    FILE *stream;

    if (path == NULL || mode == NULL) {
        errno = EINVAL;
        return NULL;
    }

    switch (mode[0]) {
    case 'w':
        flags = O_WRONLY | O_CREAT | O_TRUNC;
        break;
    case 'a':
        flags = O_WRONLY | O_CREAT | O_APPEND;
        break;
    case 'r':
    default:
        flags = O_RDONLY;
        break;
    }

    if (strchr(mode, '+') != NULL) {
        flags &= ~(O_RDONLY | O_WRONLY);
        flags |= O_RDWR;
    }

    stream = (FILE *)malloc(sizeof(FILE));
    if (stream == NULL) {
        return NULL;
    }

    stream->fd = open(path, flags, 0);
    if (stream->fd < 0) {
        free(stream);
        return NULL;
    }
    stream->eof = 0;
    stream->has_unget = 0;
    stream->unget_ch = 0;
    return stream;
}

FILE *fdopen(int fd, const char *mode) {
    FILE *stream;

    (void)mode;
    if (fd < 0) {
        errno = EINVAL;
        return NULL;
    }

    stream = (FILE *)malloc(sizeof(FILE));
    if (stream == NULL) {
        return NULL;
    }
    stream->fd = fd;
    stream->eof = 0;
    stream->has_unget = 0;
    stream->unget_ch = 0;
    return stream;
}

int fclose(FILE *stream) {
    int ret;

    if (stream == NULL) {
        errno = EINVAL;
        return EOF;
    }

    ret = close(stream->fd);
    free(stream);
    return (ret == 0) ? 0 : EOF;
}

int fseek(FILE *stream, long offset, int whence) {
    if (stream == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (lseek(stream->fd, (off_t)offset, whence) < 0) {
        return -1;
    }
    stream->eof = 0;
    stream->has_unget = 0;
    return 0;
}

long ftell(FILE *stream) {
    if (stream == NULL) {
        errno = EINVAL;
        return -1;
    }
    return (long)lseek(stream->fd, 0, SEEK_CUR);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t total;
    size_t done;
    char* p;

    if (stream == NULL || ptr == NULL || size == 0 || nmemb == 0) {
        return 0;
    }

    total = size * nmemb;
    done = 0;
    p = (char*)ptr;

    while (done < total) {
        ssize_t ret = read(stream->fd, p + done, total - done);
        if (ret <= 0) {
            if (ret == 0) {
                stream->eof = 1;
            }
            break;
        }
        done += (size_t)ret;
    }

    if (done == 0) {
        if (stream->eof == 0) {
            stream->eof = 1;
        }
        return 0;
    }

    stream->eof = 0;
    return done / size;
}

int fgetc(FILE *stream) {
    unsigned char ch;

    if (stream == NULL) {
        errno = EINVAL;
        return EOF;
    }

    if (stream->has_unget) {
        stream->has_unget = 0;
        stream->eof = 0;
        return (int)stream->unget_ch;
    }

    if (read(stream->fd, &ch, 1) != 1) {
        stream->eof = 1;
        return EOF;
    }

    stream->eof = 0;
    return (int)ch;
}

int ungetc(int c, FILE *stream) {
    if (stream == NULL || c == EOF) {
        return EOF;
    }

    stream->has_unget = 1;
    stream->unget_ch = (unsigned char)c;
    stream->eof = 0;
    return c;
}

int feof(FILE *stream) {
    if (stream == NULL) {
        return 0;
    }
    return stream->eof;
}

int fileno(FILE *stream) {
    if (stream == NULL) {
        return -1;
    }
    return stream->fd;
}

void rewind(FILE *stream) {
    if (stream == NULL) {
        return;
    }
    fseek(stream, 0, SEEK_SET);
}

char *fgets(char *s, int size, FILE *stream) {
    int i = 0;

    if (s == NULL || stream == NULL || size <= 0) {
        return NULL;
    }

    while (i < size - 1) {
        char ch;
        int got = fgetc(stream);
        if (got == EOF) {
            break;
        }
        ch = (char)got;
        s[i++] = ch;
        if (ch == '\n') {
            break;
        }
    }

    if (i == 0) {
        return NULL;
    }

    s[i] = 0;
    return s;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t total;
    ssize_t ret;

    if (stream == NULL || ptr == NULL || size == 0 || nmemb == 0) {
        return 0;
    }

    total = size * nmemb;
    ret = write(stream->fd, ptr, total);
    if (ret <= 0) {
        return 0;
    }
    return (size == 0) ? 0 : ((size_t)ret / size);
}

int ferror(FILE *stream) {
    (void)stream;
    return 0;
}

void clearerr(FILE *stream) {
    if (stream != NULL) {
        stream->eof = 0;
    }
}

int putchar(int c) {
    char ch = (char)c;
    return (write_all_retry_compat(1, &ch, 1) == 1) ? c : EOF;
}

int getchar(void) {
    unsigned char ch;
    return (read(0, &ch, 1) == 1) ? (int)ch : EOF;
}

int putc(int c, FILE *stream) {
    char ch = (char)c;
    int fd = (stream != NULL) ? stream->fd : 1;
    return (write_all_retry_compat(fd, &ch, 1) == 1) ? c : EOF;
}

int fputc(int c, FILE *stream) {
    return putc(c, stream);
}

int puts(const char *s) {
    int len = 0;
    if (s != NULL) {
        len = (int)write_all_retry_compat(1, s, strlen(s));
    }
    if(write_all_retry_compat(1, "\n", 1) != 1)
        return EOF;
    return len + 1;
}

void perror(const char *s) {
    if (s != NULL && *s != 0) {
        fprintf(stderr, "%s: %s\n", s, strerror(errno));
    } else {
        fprintf(stderr, "%s\n", strerror(errno));
    }
}

int fflush(FILE *stream) {
    (void)stream;
    return 0;
}

void setbuf(FILE *stream, char *buf) {
    (void)stream;
    (void)buf;
}
