#ifndef MD5_H
#define MD5_H

#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

void md5_encode(const uint8_t *initial_msg, uint32_t initial_len, uint8_t *digest); 
const char* md5_encode_str(const uint8_t *initial_msg, uint32_t initial_len);

#ifdef __cplusplus
}
#endif

#endif
