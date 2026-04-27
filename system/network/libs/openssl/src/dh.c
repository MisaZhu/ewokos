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

    if (!BN_copy(temp, a)) {
        BN_free(temp);
        return 0;
    }
    temp->neg = 0;

    /* Simple but absolutely reliable method: repeatedly subtract mod */
    /* For efficiency, we try to subtract mod * k where k is 2^n */
    BIGNUM *current = BN_new();
    BIGNUM *temp_mod = BN_new();
    if (!current || !temp_mod) {
        if (current) BN_free(current);
        if (temp_mod) BN_free(temp_mod);
        BN_free(temp);
        return 0;
    }

    int iterations = 0;
    const int MAX_ITER = 1000000;

    while (bn_cmp(temp, mod) >= 0) {
        iterations++;
        if (iterations > MAX_ITER) {
            BN_free(temp);
            BN_free(current);
            BN_free(temp_mod);
            return 0;
        }

        if (!BN_copy(current, mod)) {
            BN_free(temp);
            BN_free(current);
            BN_free(temp_mod);
            return 0;
        }

        while (1) {
            /* First check temp >= current */
            if (bn_cmp(temp, current) < 0) {
                break;
            }

            /* Save a copy of current for checking */
            if (!BN_copy(temp_mod, current)) {
                BN_free(temp);
                BN_free(current);
                BN_free(temp_mod);
                return 0;
            }

            /* Try multiplying by 2 */
            unsigned long carry = 0;
            for (int i = 0; i < temp_mod->top; i++) {
                unsigned long new_carry = (temp_mod->d[i] >> (sizeof(unsigned long) * 8 - 1)) & 1;
                temp_mod->d[i] = (temp_mod->d[i] << 1) | carry;
                carry = new_carry;
            }
            if (carry) {
                if (temp_mod->dmax < temp_mod->top + 1) {
                    unsigned long *new_d = realloc(temp_mod->d, (temp_mod->top + 2) * sizeof(unsigned long));
                    if (!new_d) {
                        BN_free(temp);
                        BN_free(current);
                        BN_free(temp_mod);
                        return 0;
                    }
                    temp_mod->d = new_d;
                    temp_mod->dmax = temp_mod->top + 2;
                }
                temp_mod->d[temp_mod->top] = carry;
                temp_mod->top++;
            }

            /* Check if we can multiply by 2 again */
            if (bn_cmp(temp, temp_mod) >= 0) {
                /* Yes, update current */
                if (!BN_copy(current, temp_mod)) {
                    BN_free(temp);
                    BN_free(current);
                    BN_free(temp_mod);
                    return 0;
                }
            } else {
                break;
            }
        }

        /* Subtract current */
        if (!bn_sub(temp, temp, current)) {
            BN_free(temp);
            BN_free(current);
            BN_free(temp_mod);
            return 0;
        }
    }

    BN_copy(result, temp);
    BN_free(temp);
    BN_free(current);
    BN_free(temp_mod);
    return 1;
}

/* Multiply: result = a * b */
static int bn_mul(BIGNUM *result, const BIGNUM *a, const BIGNUM *b) {
    if (!result || !a || !b) return 0;

    int a_words = a->top;
    int b_words = b->top;
    int result_words = a_words + b_words;

    if (result_words == 0) {
        result->top = 0;
        result->neg = 0;
        return 1;
    }

    unsigned long *temp = calloc(result_words, sizeof(unsigned long));
    if (!temp) return 0;

    for (int i = 0; i < a_words; i++) {
        unsigned long carry = 0;
        unsigned long ai = a->d[i];
        for (int j = 0; j < b_words; j++) {
            unsigned __int128 prod = (unsigned __int128)ai * b->d[j] + temp[i + j] + carry;
            temp[i + j] = (unsigned long)prod;
            carry = (unsigned long)(prod >> 64);
        }
        temp[i + b_words] = carry;
    }

    if (result->dmax < result_words) {
        free(result->d);
        result->d = temp;
        result->dmax = result_words;
    } else {
        memcpy(result->d, temp, result_words * sizeof(unsigned long));
        free(temp);
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

    if (!bn_mul(temp, a, b)) {
        BN_free(temp);
        return 0;
    }

    int ret = bn_mod(result, temp, mod);

    BN_free(temp);
    return ret;
}

/* Compare raw temp array with BIGNUM */
static int bn_cmp_temp(const unsigned long *temp, int temp_top, const BIGNUM *b) {
    if (temp_top > b->top) return 1;
    if (temp_top < b->top) return -1;
    for (int i = temp_top - 1; i >= 0; i--) {
        if (temp[i] > b->d[i]) return 1;
        if (temp[i] < b->d[i]) return -1;
    }
    return 0;
}

/* Simple but correct modular reduction */
static int bn_mod_fast(BIGNUM *result, const BIGNUM *a, const BIGNUM *mod) {
    if (!result || !a || !mod) return 0;
    if (mod->top == 0) return 0;
    
    int cmp = bn_cmp(a, mod);
    if (cmp < 0) {
        return BN_copy(result, a) != NULL;
    }
    if (cmp == 0) {
        result->d[0] = 0;
        result->top = 1;
        result->neg = 0;
        return 1;
    }
    
    /* Create a working copy */
    int a_len = a->top;
    int mod_len = mod->top;
    unsigned long *temp = malloc((a_len + 2) * sizeof(unsigned long));
    if (!temp) return 0;
    memcpy(temp, a->d, a_len * sizeof(unsigned long));
    int temp_top = a_len;
    while (temp_top > 0 && temp[temp_top - 1] == 0) temp_top--;
    if (temp_top == 0) { temp_top = 1; temp[0] = 0; }
    
    /* Buffer for shifted mod */
    unsigned long *sm = malloc((a_len + 2) * sizeof(unsigned long));
    if (!sm) { free(temp); return 0; }
    
    int iterations = 0;
    const int MAX_ITERATIONS = 10000;
    
    /*
     * Simple long division: repeatedly subtract mod << k until temp < mod.
     */
    while (temp_top > mod_len) {
        iterations++;
        if (iterations > MAX_ITERATIONS) {
            free(sm);
            free(temp);
            return 0;
        }
        
        int shift_words = temp_top - mod_len;
        
        /* Build sm = mod << (shift_words * word_bits) */
        memset(sm, 0, (temp_top + 1) * sizeof(unsigned long));
        for (int i = 0; i < mod_len; i++) {
            sm[i + shift_words] = mod->d[i];
        }
        int sm_top = temp_top;
        while (sm_top > 0 && sm[sm_top - 1] == 0) sm_top--;
        if (sm_top == 0) sm_top = 1;
        
        /* Check if temp >= sm */
        int tcmp = 0;
        if (temp_top > sm_top) tcmp = 1;
        else if (temp_top < sm_top) tcmp = -1;
        else {
            for (int i = temp_top - 1; i >= 0; i--) {
                if (temp[i] > sm[i]) { tcmp = 1; break; }
                if (temp[i] < sm[i]) { tcmp = -1; break; }
            }
        }
        
        /* If temp < sm, try smaller shifts */
        while (tcmp < 0 && shift_words > 0) {
            shift_words--;
            memset(sm, 0, (temp_top + 1) * sizeof(unsigned long));
            for (int i = 0; i < mod_len; i++) {
                sm[i + shift_words] = mod->d[i];
            }
            sm_top = temp_top;
            while (sm_top > 0 && sm[sm_top - 1] == 0) sm_top--;
            if (sm_top == 0) sm_top = 1;
            
            /* Recheck */
            tcmp = 0;
            if (temp_top > sm_top) tcmp = 1;
            else if (temp_top < sm_top) tcmp = -1;
            else {
                for (int i = temp_top - 1; i >= 0; i--) {
                    if (temp[i] > sm[i]) { tcmp = 1; break; }
                    if (temp[i] < sm[i]) { tcmp = -1; break; }
                }
            }
        }
        
        if (tcmp < 0) {
            /* temp < mod << 0, let's break and use the old bn_mod function */
            free(sm);
            free(temp);
            return bn_mod(result, a, mod);
        }
        
        /* Subtract: temp -= sm */
        unsigned long borrow = 0;
        for (int i = 0; i < sm_top; i++) {
            unsigned long s = sm[i] + borrow;
            if (temp[i] >= s) {
                temp[i] -= s;
                borrow = 0;
            } else {
                temp[i] = temp[i] + (0xFFFFFFFFFFFFFFFF - s) + 1;
                borrow = 1;
            }
        }
        /* Handle borrow beyond sm_top */
        for (int i = sm_top; i < temp_top && borrow; i++) {
            if (temp[i] >= borrow) {
                temp[i] -= borrow;
                borrow = 0;
            } else {
                temp[i] = temp[i] + (0xFFFFFFFFFFFFFFFF - borrow) + 1;
                borrow = 1;
            }
        }
        
        while (temp_top > 0 && temp[temp_top - 1] == 0) temp_top--;
        if (temp_top == 0) { temp_top = 1; temp[0] = 0; goto done; }
    }
    
    /* Final check: repeatedly subtract mod until temp < mod */
    iterations = 0;
    while (bn_cmp_temp(temp, temp_top, mod) >= 0) {
        iterations++;
        if (iterations > MAX_ITERATIONS) {
            free(sm);
            free(temp);
            return bn_mod(result, a, mod);
        }
        
        unsigned long borrow = 0;
        for (int i = 0; i < mod_len; i++) {
            unsigned long s = mod->d[i] + borrow;
            if (temp[i] >= s) {
                temp[i] -= s;
                borrow = 0;
            } else {
                temp[i] = temp[i] + (0xFFFFFFFFFFFFFFFF - s) + 1;
                borrow = 1;
            }
        }
        /* Handle borrow beyond mod_len */
        for (int i = mod_len; i < temp_top && borrow; i++) {
            if (temp[i] >= borrow) {
                temp[i] -= borrow;
                borrow = 0;
            } else {
                temp[i] = temp[i] + (0xFFFFFFFFFFFFFFFF - borrow) + 1;
                borrow = 1;
            }
        }
        
        while (temp_top > 0 && temp[temp_top - 1] == 0) temp_top--;
        if (temp_top == 0) { temp_top = 1; temp[0] = 0; break; }
    }

done:
    /* Copy result */
    if (result->dmax < temp_top) {
        result->d = realloc(result->d, (temp_top + 4) * sizeof(unsigned long));
        result->dmax = temp_top + 4;
    }
    memcpy(result->d, temp, temp_top * sizeof(unsigned long));
    result->top = temp_top;
    result->neg = 0;
    
    free(sm);
    free(temp);
    return 1;
}

static int mod_exp(BIGNUM *result, const BIGNUM *base, const BIGNUM *exp, const BIGNUM *mod) {
    if (!result || !base || !exp || !mod) return 0;

    BIGNUM *r = BN_new();
    BIGNUM *b = BN_new();
    BIGNUM *temp = BN_new();
    if (!r || !b || !temp) {
        BN_free(r);
        BN_free(b);
        BN_free(temp);
        return 0;
    }

    r->d[0] = 1;
    r->top = 1;
    r->neg = 0;

    if (!BN_copy(b, base)) {
        BN_free(r);
        BN_free(b);
        BN_free(temp);
        return 0;
    }

    int iter = 0;
    for (int i = exp->top - 1; i >= 0; i--) {
        unsigned long exp_word = exp->d[i];
        for (int j = sizeof(unsigned long) * 8 - 1; j >= 0; j--) {
            iter++;
            
            /* Square: b = b^2 mod mod */
            if (!bn_mul(temp, b, b)) {
                BN_free(r);
                BN_free(b);
                BN_free(temp);
                return 0;
            }
            if (!bn_mod(b, temp, mod)) {
                BN_free(r);
                BN_free(b);
                BN_free(temp);
                return 0;
            }

            /* Multiply if bit is set: r = r*b mod mod */
            if ((exp_word >> j) & 1) {
                if (!bn_mul(temp, r, b)) {
                    BN_free(r);
                    BN_free(b);
                    BN_free(temp);
                    return 0;
                }
                if (!bn_mod(r, temp, mod)) {
                    BN_free(r);
                    BN_free(b);
                    BN_free(temp);
                    return 0;
                }
            }
        }
    }

    if (!BN_copy(result, r)) {
        BN_free(r);
        BN_free(b);
        BN_free(temp);
        return 0;
    }

    BN_free(r);
    BN_free(b);
    BN_free(temp);
    return 1;
}

int DH_generate_key(DH *dh) {
    if (!dh || !dh->p || !dh->g) return 0;

    if (!dh->priv_key) dh->priv_key = BN_new();
    if (!dh->pub_key) dh->pub_key = BN_new();
    if (!dh->priv_key || !dh->pub_key) return 0;

    int p_bytes = BN_num_bytes(dh->p);
    int p_words = (p_bytes + sizeof(unsigned long) - 1) / sizeof(unsigned long);

    if (dh->priv_key->dmax < p_words) {
        unsigned long *new_d = realloc(dh->priv_key->d, p_words * sizeof(unsigned long));
        if (!new_d) return 0;
        dh->priv_key->d = new_d;
        dh->priv_key->dmax = p_words;
    }

    int priv_bytes = 32;
    if (priv_bytes > p_bytes - 1) priv_bytes = p_bytes - 1;
    if (priv_bytes < 2) priv_bytes = 2;

    memset(dh->priv_key->d, 0, dh->priv_key->dmax * sizeof(unsigned long));
    for (int i = 0; i < priv_bytes; i++) {
        ((unsigned char *)dh->priv_key->d)[i] = (unsigned char)(rand() & 0xFF);
    }
    if (((unsigned char *)dh->priv_key->d)[0] < 2) {
        ((unsigned char *)dh->priv_key->d)[0] = 2;
    }
    dh->priv_key->top = (priv_bytes + sizeof(unsigned long) - 1) / sizeof(unsigned long);
    if (dh->priv_key->top == 0) dh->priv_key->top = 1;
    dh->priv_key->neg = 0;

    if (!mod_exp(dh->pub_key, dh->g, dh->priv_key, dh->p)) {
        return 0;
    }

    printf("DH_generate_key: step5 - success, pub_key->top=%d\n", dh->pub_key->top);

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
