#ifndef MARIO_UTF8
#define MARIO_UTF8

#include "mstrx.h"
#include <stdbool.h>

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

/**
utf8 string functions
*/

typedef struct st_utf8_reader {
	const char* str;
	uint32_t offset;
} utf8_reader_t;

typedef m_array_t utf8_t;

void utf8_reader_init(utf8_reader_t* reader, const char* s, uint32_t offset);
bool utf8_read(utf8_reader_t* reader, str_t* dst);

utf8_t* utf8_new(const char* s);
void utf8_free(utf8_t* utf8);
void utf8_append_raw(utf8_t* utf8, const char* s);
void utf8_append(utf8_t* utf8, const char* s);
uint32_t utf8_len(utf8_t* utf8);
str_t* utf8_at(utf8_t* utf8, uint32_t at);
void utf8_set(utf8_t* utf8, uint32_t at, const char* s);
void utf8_to_str(utf8_t* utf8, str_t* str);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
