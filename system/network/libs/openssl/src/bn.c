#include "openssl/bn.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* BIGNUM structure implementation - defined in header */

#define BN_BITS_PER_WORD (sizeof(unsigned long) * 8)

BIGNUM *BN_new(void) {
    BIGNUM *a = (BIGNUM *)calloc(1, sizeof(BIGNUM));
    if (a) {
        a->dmax = 4;
        a->d = (unsigned long *)calloc(a->dmax, sizeof(unsigned long));
        if (!a->d) {
            free(a);
            return NULL;
        }
    }
    return a;
}

void BN_free(BIGNUM *a) {
    if (a) {
        if (a->d) {
            memset(a->d, 0, a->dmax * sizeof(unsigned long));
            free(a->d);
        }
        free(a);
    }
}

void BN_clear_free(BIGNUM *a) {
    BN_free(a);
}

int BN_set_word(BIGNUM *a, unsigned long w) {
    if (!a) return 0;
    
    if (a->dmax < 1) {
        a->d = realloc(a->d, sizeof(unsigned long));
        a->dmax = 1;
    }
    
    a->d[0] = w;
    a->top = w ? 1 : 0;
    a->neg = 0;
    
    return 1;
}

unsigned long BN_get_word(const BIGNUM *a) {
    if (!a || a->top == 0) return 0;
    return a->d[0];
}

int BN_hex2bn(BIGNUM **a, const char *str) {
    if (!a || !str) return 0;
    
    if (!*a) {
        *a = BN_new();
        if (!*a) return 0;
    }
    
    BIGNUM *bn = *a;
    int len = strlen(str);
    int bytes = (len + 1) / 2;
    
    /* Ensure enough space */
    int needed_words = (bytes + sizeof(unsigned long) - 1) / sizeof(unsigned long);
    if (bn->dmax < needed_words) {
        int new_dmax = needed_words + 4;
        unsigned long *new_d = realloc(bn->d, new_dmax * sizeof(unsigned long));
        if (!new_d) return 0;
        bn->d = new_d;
        bn->dmax = new_dmax;
    }
    
    memset(bn->d, 0, bn->dmax * sizeof(unsigned long));
    
    /* Parse hex string */
    for (int i = 0; i < len; i++) {
        char c = str[i];
        int val;
        if (c >= '0' && c <= '9') val = c - '0';
        else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
        else continue;

        int hex_digit_pos = len - 1 - i;
        int byte_pos = hex_digit_pos / 2;
        int nibble = hex_digit_pos % 2;

        int word_pos = byte_pos / sizeof(unsigned long);
        int byte_in_word = byte_pos % sizeof(unsigned long);

        if (nibble == 0)
            bn->d[word_pos] |= ((unsigned long)val) << (byte_in_word * 8);
        else
            bn->d[word_pos] |= ((unsigned long)val) << (byte_in_word * 8 + 4);
    }
    
    bn->top = (bytes + sizeof(unsigned long) - 1) / sizeof(unsigned long);
    while (bn->top > 0 && bn->d[bn->top - 1] == 0) bn->top--;
    
    return len;
}

char *BN_bn2hex(const BIGNUM *a) {
    if (!a || a->top == 0) {
        char *ret = malloc(2);
        if (ret) {
            ret[0] = '0';
            ret[1] = '\0';
        }
        return ret;
    }
    
    int bytes = a->top * sizeof(unsigned long);
    char *ret = malloc(bytes * 2 + 1);
    if (!ret) return NULL;
    
    int pos = 0;
    int leading = 1;
    
    for (int i = a->top - 1; i >= 0; i--) {
        unsigned long val = a->d[i];
        for (int j = sizeof(unsigned long) - 1; j >= 0; j--) {
            unsigned char byte = (val >> (j * 8)) & 0xFF;
            if (leading && byte == 0) continue;
            leading = 0;
            sprintf(ret + pos, "%02X", byte);
            pos += 2;
        }
    }
    
    if (pos == 0) {
        ret[0] = '0';
        ret[1] = '\0';
    } else {
        ret[pos] = '\0';
    }
    
    return ret;
}

int BN_bn2bin(const BIGNUM *a, unsigned char *to) {
    if (!a || !to) return 0;
    
    int len = 0;
    
    for (int i = a->top - 1; i >= 0; i--) {
        unsigned long val = a->d[i];
        for (int j = sizeof(unsigned long) - 1; j >= 0; j--) {
            if (len == 0 && i == 0 && j == 0 && val == 0) continue;
            to[len++] = (val >> (j * 8)) & 0xFF;
        }
    }
    
    if (len == 0) {
        len = 1;
        to[0] = 0;
    }
    
    return len;
}

BIGNUM *BN_bin2bn(const unsigned char *s, int len, BIGNUM *ret) {
    if (!s || len < 0) return NULL;

    if (!ret) {
        ret = BN_new();
        if (!ret) return NULL;
    }

    int words = (len + sizeof(unsigned long) - 1) / sizeof(unsigned long);
    if (ret->dmax < words) {
        ret->dmax = words + 4;
        ret->d = realloc(ret->d, ret->dmax * sizeof(unsigned long));
    }

    memset(ret->d, 0, ret->dmax * sizeof(unsigned long));

    for (int i = 0; i < len; i++) {
        int word = i / sizeof(unsigned long);
        int byte = i % sizeof(unsigned long);
        ret->d[word] |= ((unsigned long)s[len - 1 - i]) << (byte * 8);
    }

    ret->top = words;
    while (ret->top > 0 && ret->d[ret->top - 1] == 0) ret->top--;
    ret->neg = 0;

    return ret;
}

int BN_num_bytes(const BIGNUM *a) {
    if (!a || a->top == 0) return 0;
    return a->top * sizeof(unsigned long);
}

int BN_num_bits(const BIGNUM *a) {
    if (!a || a->top == 0) return 0;
    
    int bits = (a->top - 1) * BN_BITS_PER_WORD;
    unsigned long val = a->d[a->top - 1];
    
    while (val) {
        bits++;
        val >>= 1;
    }
    
    return bits;
}

int BN_cmp(const BIGNUM *a, const BIGNUM *b) {
    if (!a || !b) return 0;
    if (a->top != b->top) {
        return (a->top > b->top) ? 1 : -1;
    }
    for (int i = a->top - 1; i >= 0; i--) {
        if (a->d[i] > b->d[i]) return 1;
        if (a->d[i] < b->d[i]) return -1;
    }
    return 0;
}

BIGNUM *BN_copy(BIGNUM *a, const BIGNUM *b) {
    if (!a || !b) return NULL;
    
    if (a->dmax < b->top) {
        int new_dmax = b->top + 4;
        unsigned long *new_d = realloc(a->d, new_dmax * sizeof(unsigned long));
        if (!new_d) return NULL;
        a->d = new_d;
        a->dmax = new_dmax;
    }
    
    memcpy(a->d, b->d, b->top * sizeof(unsigned long));
    a->top = b->top;
    a->neg = b->neg;
    
    return a;
}

/* BN_CTX implementation */
struct bignum_ctx {
    BIGNUM *bn[16];
    int used;
};

BN_CTX *BN_CTX_new(void) {
    BN_CTX *ctx = calloc(1, sizeof(BN_CTX));
    return ctx;
}

void BN_CTX_free(BN_CTX *ctx) {
    if (ctx) {
        for (int i = 0; i < ctx->used; i++) {
            BN_free(ctx->bn[i]);
        }
        free(ctx);
    }
}
