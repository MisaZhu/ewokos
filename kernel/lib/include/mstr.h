#ifndef MARIO_STRING
#define MARIO_STRING

#include <stdint.h>

/**
string functions.
*/
typedef struct st_str {
	char* cstr;
	uint32_t max;
	uint32_t len;
} str_t;

void str_reset(str_t* str);
char* str_ncpy(str_t* str, const char* src, uint32_t l);
char* str_cpy(str_t* str, const char* src);
str_t* str_new(const char* s);
char* str_add(str_t* str, const char* src);
char* str_addc(str_t* str, char c);
void str_free(str_t* str);

#define CS(s) ((s)->cstr)

#endif
