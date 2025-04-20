#ifndef TEXT_CONTENT_H
#define TEXT_CONTENT_H

#include <stdint.h>
#include <stddef.h>
#include "textgrid.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	textchar_t* content;
	uint32_t num;
	uint32_t total;
} text_content_t;

extern text_content_t* text_content_new(void);
extern void text_content_free(text_content_t* text_content);
extern int text_content_push(text_content_t* text_content, textchar_t* tc);

#ifdef __cplusplus
}
#endif

#endif
