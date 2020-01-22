#ifndef MARIO_LEX
#define MARIO_LEX

#include <types.h>
#include <marray.h>
#include <mstr.h>
#include <mstrx.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Script Lex. -----------------------------*/

typedef enum {
	LEX_EOF  =  0,
  LEX_ID = 256,
	LEX_INT,
  LEX_FLOAT,
  LEX_STR,
	LEX_BASIC_END
} lex_basic_type_t;

typedef struct st_lex {
	const char* data;

	int32_t data_pos;
	int32_t data_start, data_end;
	char curr_ch, next_ch;

	uint32_t tk;
	str_t* tk_str;
	int32_t tk_start, tk_end, tk_last_end;
} lex_t;

bool is_whitespace(unsigned char ch);

bool is_space(unsigned char ch);

bool is_numeric(unsigned char ch);

bool is_number(const char* cstr);

bool is_hexadecimal(unsigned char ch);

bool is_alpha(unsigned char ch);

bool is_alpha_num(const char* cstr);

void lex_get_nextch(lex_t* lex);

void lex_reset(lex_t* lex);

void lex_init(lex_t * lex, const char* input);

void lex_release(lex_t* lex);

void lex_skip_whitespace(lex_t* lex);

void lex_skip_space(lex_t* lex);

bool lex_skip_comments_line(lex_t* lex, const char* start);

bool lex_skip_comments_block(lex_t* lex, const char* start, const char* end);

void lex_token_start(lex_t* lex); 

void lex_token_end(lex_t* lex); 

void lex_get_char_token(lex_t* lex); 

void lex_get_basic_token(lex_t* lex); 

void lex_get_pos(lex_t* lex, int* line, int *col, int pos);

#ifdef __cplusplus
}
#endif

#endif
