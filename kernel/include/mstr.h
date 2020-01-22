#ifndef MARIO_STRING
#define MARIO_STRING

#include <types.h>

/**
string functions.
*/
typedef struct st_str {
	char* cstr;
	uint32_t max: 16;
	uint32_t len: 16;
} str_t;

void str_reset(str_t* str);
char* str_ncpy(str_t* str, const char* src, uint32_t l);
char* str_cpy(str_t* str, const char* src);
str_t* str_new(const char* s);
str_t* str_new_by_size(uint32_t sz);
char* str_add(str_t* str, const char* src);
char* str_addc(str_t* str, char c);
char* str_add_int(str_t* str, int32_t i, int32_t base);
void str_free(str_t* str);
const char* str_from_int(int32_t i, int32_t base);
const char* str_from_bool(uint8_t b);
int32_t str_to_int(const char* str);
int32_t str_to(const char* str, char c, str_t* res, uint8_t skipspace);

#define CS(s) ((s)->cstr)

#endif
