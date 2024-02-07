#ifndef C_BUF_H
#define C_BUF_H

#include <ewoksys/ewokdef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char* buffer;
	uint32_t buf_size;
	uint32_t start;
	uint32_t size;
} charbuf_t;

charbuf_t* charbuf_new(uint32_t buf_size);
int32_t    charbuf_push(charbuf_t *buffer, char c, bool loop);
int32_t    charbuf_pop(charbuf_t *buffer, char* c);
void       charbuf_free(charbuf_t *buffer);

#ifdef __cplusplus 
}
#endif

#endif
