#include "text_content.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

text_content_t* text_content_new(void) {
	text_content_t* text_content = (text_content_t*)malloc(sizeof(text_content_t));
	if(text_content == NULL)
		return NULL;
	memset(text_content, 0, sizeof(text_content_t));
	return text_content;
}

void text_content_free(text_content_t* text_content) {
	if(text_content == NULL)
		return;
	if(text_content->content)
		free(text_content->content);
	free(text_content);
}

#define EXPAND_LEN  32

int text_content_push(text_content_t* text_content, textchar_t* tc) {
	if(text_content == NULL)
		return -1;
	
	if(text_content->content == NULL || text_content->num >= text_content->total) {
		uint32_t sz = (text_content->num + EXPAND_LEN) * sizeof(textchar_t);
		text_content->content = (textchar_t*)realloc(text_content->content, sz);
		if(text_content->content == NULL)
			return -1;
		text_content->total = text_content->num + EXPAND_LEN;
	}

	text_content->content[text_content->num] = *tc;
	text_content->num++;
	return 0;
}

#ifdef __cplusplus
}
#endif
