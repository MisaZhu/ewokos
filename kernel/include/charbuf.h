#ifndef C_BUF_H
#define C_BUF_H

#include <types.h>

#define CHAR_BUF_MAX 128

typedef struct {
	char buffer[CHAR_BUF_MAX];
	uint32_t start;
	uint32_t size;
} charbuf_t;

int32_t charbuf_push(charbuf_t *buffer, char c, uint8_t loop);
int32_t charbuf_pop(charbuf_t *buffer, char* c);

#endif
