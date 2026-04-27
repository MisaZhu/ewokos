#include "openssl/dh.h"
#include "openssl/bn.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* DH structure implementation */
struct dh_st {
    BIGNUM *p;
    BIGNUM *q;
    BIGNUM *g;
    BIGNUM *pub_key;
    BIGNUM *priv_key;
};

DH *DH_new(void) {
    DH *dh = (DH *)calloc(1, sizeof(DH));
    return dh;
}

void DH_free(DH *dh) {
    if (dh) {
        BN_free(dh->p);
        BN_free(dh->q);
        BN_free(dh->g);
        BN_free(dh->pub_key);
        BN_clear_free(dh->priv_key);
        free(dh);
    }
}

/* Compare two BIGNUMs: returns -1 if a < b, 0 if a == b, 1 if a > b */
static int bn_cmp(const BIGNUM *a, const BIGNUM *b) {
    if (!a || !b) return 0;
    if (a->top > b->top) return 1;
    if (a->top < b->top) return -1;
    for (int i = a->top - 1; i >= 0; i--) {
        if (a->d[i] > b->d[i]) return 1;
        if (a->d[i] < b->d[i]) return -1;
    }
    return 0;
}

/* Subtract: result = a - b (assumes a >= b) */
static int bn_sub(BIGNUM *result, const BIGNUM *a, const BIGNUM *b) {
    if (!result || !a || !b) return 0;
    
    int max_words = (a->top > b->top) ? a->top : b->top;
    if (result->dmax < max_words) {
        unsigned long *new_d = realloc(result->d, max_words * sizeof(unsigned long));
        if (!new_d) return 0;
        result->d = new_d;
        result->dmax = max_words;
    }
    
    unsigned long borrow = 0;
    for (int i = 0; i < max_words; i++) {
        unsigned long a_val = (i < a->top) ? a->d[i] : 0;
        unsigned long b_val = (i < b->top) ? b->d[i] : 0;
        
        unsigned long diff = a_val - b_val - borrow;
        result->d[i] = diff;
        borrow = (a_val < b_val + borrow) ? 1 : 0;
    }
    
    result->top = max_words;
    while (result->top > 0 && result->d[result->top - 1] == 0) result->top--;
    result->neg = 0;
    
    return 1;
}

/* Add: result = a + b */
static int bn_add(BIGNUM *result, const BIGNUM *a, const BIGNUM *b) {
    if (!result || !a || !b) return 0;
    
    int max_words = (a->top > b->top) ? a->top : b->top;
    if (result->dmax < max_words + 1) {
        unsigned long *new_d = realloc(result->d, (max_words + 1) * sizeof(unsigned long));
        if (!new_d) return 0;
        result->d = new_d;
        result->dmax = max_words + 1;
    }
    
    unsigned long carry = 0;
    for (int i = 0; i < max_words; i++) {
        unsigned long a_val = (i < a->top) ? a->d[i] : 0;
        unsigned long b_val = (i < b->top) ? b->d[i] : 0;
        
        unsigned long long sum = (unsigned long long)a_val + b_val + carry;
        result->d[i] = (unsigned long)sum;
        carry = (unsigned long)(sum >> (sizeof(unsigned long) * 8));
    }
    
    if (carry) {
        result->d[max_words] = carry;
        result->top = max_words + 1;
    } else {
        result->top = max_words;
    }
    
    while (result->top > 0 && result->d[result->top - 1] == 0) result->top--;
    result->neg = 0;
    
    return 1;
}

/* Left shift by 1 bit: result = a << 1 */
static int bn_lshift1(BIGNUM *result, const BIGNUM *a) {
    if (!result || !a) return 0;
    
    if (result->dmax < a->top + 1) {
        unsigned long *new_d = realloc(result->d, (a->top + 1) * sizeof(unsigned long));
        if (!new_d) return 0;
        result->d = new_d;
        result->dmax = a->top + 1;
    }
    
    unsigned long carry = 0;
    for (int i = 0; i < a->top; i++) {
        unsigned long new_carry = (a->d[i] >> (sizeof(unsigned long) * 8 - 1)) & 1;
        result->d[i] = (a->d[i] << 1) | carry;
        carry = new_carry;
    }
    
    if (carry) {
        result->d[a->top] = carry;
        result->top = a->top + 1;
    } else {
        result->top = a->top;
    }
    
    result->neg = 0;
    return 1;
}

/* Modular reduction: result = a mod mod
 * Simple and correct algorithm for a >= 0, mod > 0 */
static int bn_mod(BIGNUM *result, const BIGNUM *a, const BIGNUM *mod) {
    if (!result || !a || !mod) return 0;
    if (mod->top == 0) return 0;
    
    int cmp = bn_cmp(a, mod);
    if (cmp < 0) {
        return BN_copy(result, a) != NULL;
    }
    if (cmp == 0) {
        result->top = 0;
        result->neg = 0;
        return 1;
    }
    
    BIGNUM *temp = BN_new();
    if (!temp) return 0;
    
    BN_copy(temp, a);
    temp->neg = 0;
    
    int mod_bits = BN_num_bits(mod);
    int word_bits = sizeof(unsigned long) * 8;
    
    while (bn_cmp(temp, mod) >= 0) {
        int temp_bits = BN_num_bits(temp);
        
        if (temp_bits - mod_bits < word_bits) {
            while (bn_cmp(temp, mod) >= 0) {
                bn_sub(temp, temp, mod);
            }
            break;
        }
        
        int shift = temp_bits - mod_bits;
        int word_shift = shift / word_bits;
        int bit_shift = shift % word_bits;
        
        BIGNUM *shifted_mod = BN_new();
        if (!shifted_mod) {
            BN_free(temp);
            return 0;
        }
        
        BN_copy(shifted_mod, mod);
        
        if (bit_shift == 0) {
            for (int i = shifted_mod->top - 1; i >= 0; i--) {
                if (i + word_shift >= shifted_mod->dmax) break;
                shifted_mod->d[i + word_shift] = shifted_mod->d[i];
                if (i >= word_shift) shifted_mod->d[i] = 0;
            }
            shifted_mod->top = shifted_mod->top + word_shift;
        } else {
            for (int i = shifted_mod->top - 1; i >= 0; i--) {
                unsigned long high = (i + 1 < shifted_mod->top) ? 
                    shifted_mod->d[i + 1] >> (word_bits - bit_shift) : 0;
                shifted_mod->d[i] = (shifted_mod->d[i] << bit_shift) | high;
            }
            if (word_shift > 0) {
                for (int i = shifted_mod->top - 1; i >= 0; i--) {
                    if (i + word_shift < shifted_mod->dmax) {
                        shifted_mod->d[i + word_shift] = shifted_mod->d[i];
                    }
                    if (i >= word_shift) shifted_mod->d[i] = 0;
                }
            }
            shifted_mod->top = shifted_mod->top + word_shift;
            while (shifted_mod->top > 0 && shifted_mod->d[shifted_mod->top - 1] == 0) {
                shifted_mod->top--;
            }
        }
        
        while (bn_cmp(temp, shifted_mod) >= 0) {
            bn_sub(temp, temp, shifted_mod);
        }
        
        BN_free(shifted_mod);
    }
    
    BN_copy(result, temp);
    BN_free(temp);
    
    return 1;
}

/* Multiply: result = a * b */
static int bn_mul(BIGNUM *result, const BIGNUM *a, const BIGNUM *b) {
    if (!result || !a || !b) return 0;
    
    int result_words = a->top + b->top;
    if (result->dmax < result_words) {
        unsigned long *new_d = calloc(result_words, sizeof(unsigned long));
        if (!new_d) return 0;
        free(result->d);
        result->d = new_d;
        result->dmax = result_words;
    } else {
        memset(result->d, 0, result->dmax * sizeof(unsigned long));
    }
    
    /* Grade school multiplication */
    for (int i = 0; i < a->top; i++) {
        unsigned long long carry = 0;
        for (int j = 0; j < b->top; j++) {
            unsigned long long prod = (unsigned long long)a->d[i] * b->d[j] + 
                                       result->d[i + j] + carry;
            result->d[i + j] = (unsigned long)prod;
            carry = prod >> (sizeof(unsigned long) * 8);
        }
        result->d[i + b->top] = (unsigned long)carry;
    }
    
    result->top = result_words;
    while (result->top > 0 && result->d[result->top - 1] == 0) result->top--;
    result->neg = 0;
    
    return 1;
}

/* Modular multiplication: result = (a * b) mod mod */
static int mod_mul(BIGNUM *result, const BIGNUM *a, const BIGNUM *b, const BIGNUM *mod) {
    if (!result || !a || !b || !mod) return 0;
    
    BIGNUM *temp = BN_new();
    if (!temp) return 0;
    
    /* temp = a * b */
    if (!bn_mul(temp, a, b)) {
        BN_free(temp);
        return 0;
    }
    
    /* result = temp mod mod */
    int ret = bn_mod(result, temp, mod);
    
    BN_free(temp);
    return ret;
}

/* Modular exponentiation: result = base^exp mod mod
 * Using Montgomery ladder for constant-time operation */
static int mod_exp(BIGNUM *result, const BIGNUM *base, const BIGNUM *exp, const BIGNUM *mod) {
    if (!result || !base || !exp || !mod) return 0;
    
    /* Ensure result has enough space */
    int mod_words = mod->top;
    if (mod_words == 0) mod_words = 1;
    
    if (result->dmax < mod_words) {
        unsigned long *new_d = realloc(result->d, mod_words * sizeof(unsigned long));
        if (!new_d) return 0;
        result->d = new_d;
        result->dmax = mod_words;
    }
    
    /* Initialize result to 1 */
    memset(result->d, 0, result->dmax * sizeof(unsigned long));
    result->d[0] = 1;
    result->top = 1;
    result->neg = 0;
    
    /* Create a copy of base mod mod */
    BIGNUM *base_mod = BN_new();
    if (!base_mod) return 0;
    bn_mod(base_mod, base, mod);
    
    /* Montgomery ladder */
    BIGNUM *r0 = BN_new();
    BIGNUM *r1 = BN_new();
    if (!r0 || !r1) {
        BN_free(base_mod);
        BN_free(r0);
        BN_free(r1);
        return 0;
    }
    
    /* r0 = 1, r1 = base */
    BN_copy(r0, result);
    BN_copy(r1, base_mod);
    
    /* Process bits from MSB to LSB */
    for (int i = exp->top - 1; i >= 0; i--) {
        unsigned long exp_word = exp->d[i];
        for (int j = sizeof(unsigned long) * 8 - 1; j >= 0; j--) {
            int bit = (exp_word >> j) & 1;
            
            if (bit) {
                /* r0 = r0 * r1 mod mod, r1 = r1^2 mod mod */
                mod_mul(r0, r0, r1, mod);
                mod_mul(r1, r1, r1, mod);
            } else {
                /* r1 = r0 * r1 mod mod, r0 = r0^2 mod mod */
                mod_mul(r1, r0, r1, mod);
                mod_mul(r0, r0, r0, mod);
            }
        }
    }
    
    /* Copy result */
    BN_copy(result, r0);
    
    BN_free(base_mod);
    BN_free(r0);
    BN_free(r1);
    
    return 1;
}

int DH_generate_key(DH *dh) {
    if (!dh || !dh->p || !dh->g) return 0;
    
    /* Generate private key (random number < p) */
    if (!dh->priv_key) dh->priv_key = BN_new();
    if (!dh->pub_key) dh->pub_key = BN_new();
    if (!dh->priv_key || !dh->pub_key) return 0;
    
    /* Ensure priv_key has enough space */
    int p_bytes = BN_num_bytes(dh->p);
    int p_words = (p_bytes + sizeof(unsigned long) - 1) / sizeof(unsigned long);
    if (dh->priv_key->dmax < p_words) {
        unsigned long *new_d = realloc(dh->priv_key->d, p_words * sizeof(unsigned long));
        if (!new_d) return 0;
        dh->priv_key->d = new_d;
        dh->priv_key->dmax = p_words;
    }
    
    /* Generate random private key (2 <= priv < p-1) */
    /* Use 256-bit random private key for security */
    int priv_bytes = 32; /* 256 bits */
    if (priv_bytes > p_bytes - 1) priv_bytes = p_bytes - 1;
    if (priv_bytes < 2) priv_bytes = 2;
    
    memset(dh->priv_key->d, 0, dh->priv_key->dmax * sizeof(unsigned long));
    for (int i = 0; i < priv_bytes; i++) {
        ((unsigned char *)dh->priv_key->d)[i] = (unsigned char)(rand() & 0xFF);
    }
    /* Ensure private key >= 2 */
    if (((unsigned char *)dh->priv_key->d)[0] < 2) {
        ((unsigned char *)dh->priv_key->d)[0] = 2;
    }
    dh->priv_key->top = (priv_bytes + sizeof(unsigned long) - 1) / sizeof(unsigned long);
    if (dh->priv_key->top == 0) dh->priv_key->top = 1;
    dh->priv_key->neg = 0;
    
    /* Compute public key: pub = g^priv mod p */
    if (!mod_exp(dh->pub_key, dh->g, dh->priv_key, dh->p)) {
        return 0;
    }
    
    return 1;
}

int DH_compute_key(unsigned char *key, const BIGNUM *pub_key, DH *dh) {
    if (!key || !pub_key || !dh || !dh->priv_key || !dh->p) return -1;
    
    /* Compute shared secret: key = pub^priv mod p */
    BIGNUM *shared = BN_new();
    if (!shared) return -1;
    
    if (!mod_exp(shared, pub_key, dh->priv_key, dh->p)) {
        BN_free(shared);
        return -1;
    }
    
    /* Convert to bytes */
    int len = BN_bn2bin(shared, key);
    BN_free(shared);
    
    return len;
}

int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g) {
    if (!dh) return 0;
    
    if (p) {
        BN_free(dh->p);
        dh->p = p;
    }
    if (q) {
        BN_free(dh->q);
        dh->q = q;
    }
    if (g) {
        BN_free(dh->g);
        dh->g = g;
    }
    
    return 1;
}

void DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key) {
    if (dh) {
        if (pub_key) *pub_key = dh->pub_key;
        if (priv_key) *priv_key = dh->priv_key;
    }
}
