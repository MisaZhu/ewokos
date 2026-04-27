#include "openssl/hmac.h"
#include "openssl/sha.h"
#include <string.h>

unsigned char *HMAC(const EVP_MD *evp_md, const void *key, int key_len,
                    const unsigned char *d, size_t n, unsigned char *md,
                    unsigned int *md_len) {
    static unsigned char static_md[64];
    unsigned char k_ipad[64], k_opad[64];
    unsigned char tk[32];
    int i;
    
    if (!md) md = static_md;
    
    /* For SHA-256, block size is 64 bytes */
    int block_size = 64;
    int md_size = 32; /* SHA-256 output size */
    
    /* If key is longer than block_size, hash it */
    if (key_len > block_size) {
        SHA256((const unsigned char *)key, key_len, tk);
        key = tk;
        key_len = md_size;
    }
    
    /* Create padded keys */
    memset(k_ipad, 0, block_size);
    memset(k_opad, 0, block_size);
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);
    
    /* XOR with ipad and opad */
    for (i = 0; i < block_size; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }
    
    /* HMAC = hash(key_opad || hash(key_ipad || message)) */
    SHA256_CTX ctx;
    
    /* Inner hash */
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, k_ipad, block_size);
    SHA256_Update(&ctx, d, n);
    SHA256_Final(md, &ctx);
    
    /* Outer hash */
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, k_opad, block_size);
    SHA256_Update(&ctx, md, md_size);
    SHA256_Final(md, &ctx);
    
    if (md_len) *md_len = md_size;
    
    return md;
}
