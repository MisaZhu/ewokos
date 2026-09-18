#ifndef VERIFY_CAP_KSTRING_H
#define VERIFY_CAP_KSTRING_H

/*
 * Stub for the kernel's <kstring.h>. memset/memcpy/strlen/strncmp have built-in
 * CBMC models (and host libc equivalents), so they are only declared here.
 * sstrncpy is EwokOS-specific; cap_proofs.c provides a bounded model of it.
 */

#include <stddef.h>

void*  memset(void* s, int c, size_t n);
void*  memcpy(void* d, const void* s, size_t n);
size_t strlen(const char* s);
int    strncmp(const char* a, const char* b, size_t n);
char*  sstrncpy(char* dst, const char* src, size_t n);

#endif
