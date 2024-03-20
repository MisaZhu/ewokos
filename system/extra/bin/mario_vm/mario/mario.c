#include "mario.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef MRCIO_THREAD
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**======platform porting functions======*/
void* (*_malloc)(uint32_t size) = NULL;
void  (*_free)(void* p) = NULL;
void  (*_out_func)(const char*) = NULL;

/**======memory functions======*/

void *_realloc(void* p, uint32_t old_size, uint32_t new_size) {
	void *np = _malloc(new_size);
	if(p != NULL && old_size > 0) {
		memcpy(np, p, old_size);
		_free(p);
	}
	return np;
}

/**======debug functions======*/
bool _m_debug = false;

static inline void dout(const char* s) {
	if(_out_func != NULL)
		_out_func(s);
}

inline void mario_debug(const char* s) {
	if(_m_debug)
		dout(s);
}

/**======array functions======*/

#define ARRAY_BUF 16

inline void array_init(m_array_t* array) { 
	array->items = NULL; 
	array->size = 0; 
	array->max = 0; 
}

m_array_t* array_new() {
	m_array_t* ret = (m_array_t*)_malloc(sizeof(m_array_t));
	array_init(ret);
	return ret;
}

inline void array_add(m_array_t* array, void* item) {
	uint32_t new_size = array->size + 1; 
	if(array->max <= new_size) { 
		new_size = array->size + ARRAY_BUF;
		array->items = (void**)_realloc(array->items, array->max*sizeof(void*), new_size*sizeof(void*)); 
		array->max = new_size; 
	} 
	array->items[array->size] = item; 
	array->size++; 
	array->items[array->size] = NULL; 
}

inline void array_add_head(m_array_t* array, void* item) {
	uint32_t new_size = array->size + 1; 
	if(array->max <= new_size) { 
		new_size = array->size + ARRAY_BUF;
		array->items = (void**)_realloc(array->items, array->max*sizeof(void*), new_size*sizeof(void*)); 
		array->max = new_size; 
	} 
	int32_t i;
	for(i=array->size; i>0; i--) {
		array->items[i] = array->items[i-1];
	}
	array->items[0] = item; 
	array->size++; 
	array->items[array->size] = NULL; 
}

void* array_add_buf(m_array_t* array, void* s, uint32_t sz) {
	void* item = _malloc(sz);
	if(s != NULL)
		memcpy(item, s, sz);
	array_add(array, item);
	return item;
}

inline void* array_get(m_array_t* array, uint32_t index) {
	if(array->items == NULL || index >= array->size)
		return NULL;
	return array->items[index];
}

inline void* array_set(m_array_t* array, uint32_t index, void* p) {
	if(array->items == NULL || index >= array->size)
		return NULL;
	array->items[index] = p;
	return p;
}

inline void* array_head(m_array_t* array) {
	if(array->items == NULL || array->size == 0)
		return NULL;
	return array->items[0];
}

inline void* array_remove(m_array_t* array, uint32_t index) { //remove out but not free
	if(index >= array->size)
		return NULL;

	void *p = array->items[index];
	uint32_t i;
	for(i=index; (i+1)<array->size; i++) {
		array->items[i] = array->items[i+1];	
	}

	array->size--;
	array->items[array->size] = NULL;
	return p;
}

inline void array_del(m_array_t* array, uint32_t index, free_func_t fr) { // remove out and free.
	void* p = array_remove(array, index);
	if(p != NULL) {
		if(fr != NULL)
			fr(p);
		else
			_free(p);
	}
}

inline void array_remove_all(m_array_t* array) { //remove all items bot not free them.
	if(array->items != NULL) {
		_free(array->items);
		array->items = NULL;
	}
	array->max = array->size = 0;
}

inline void array_free(m_array_t* array, free_func_t fr) {
	array_clean(array, fr);
	_free(array);
}

inline void array_clean(m_array_t* array, free_func_t fr) { //remove all items and free them.
	if(array->items != NULL) {
		uint32_t i;
		for(i=0; i<array->size; i++) {
			void* p = array->items[i];
			if(p != NULL) {
				if(fr != NULL)
					fr(p);
				else
					_free(p);
			}
		}
		_free(array->items);
		array->items = NULL;
	}
	array->max = array->size = 0;
}

/**======string functions======*/

#define mstr_BUF 16

void mstr_reset(mstr_t* str) {
	if(str->cstr == NULL) {
		str->cstr = (char*)_malloc(mstr_BUF);
		str->max = mstr_BUF;
	}

	str->cstr[0] = 0;
	str->len = 0;	
}

char* mstr_ncpy(mstr_t* str, const char* src, uint32_t l) {
	if(src == NULL || src[0] == 0 || l == 0) {
		mstr_reset(str);
		return str->cstr;
	}

	uint32_t len = (uint32_t)strlen(src);
	if(len > l)
		len = l;

	uint32_t new_size = len;
	if(str->max <= new_size) {
		new_size = len + mstr_BUF; /*STR BUF for buffer*/
		str->cstr = (char*)_realloc(str->cstr, str->max, new_size);
		str->max = new_size;
	}

	strncpy(str->cstr, src, len);
	str->cstr[len] = 0;
	str->len = len;
	return str->cstr;
}

char* mstr_cpy(mstr_t* str, const char* src) {
	mstr_ncpy(str, src, 0x0FFFF);
	return str->cstr;
}

mstr_t* mstr_new(const char* s) {
	mstr_t* ret = (mstr_t*)_malloc(sizeof(mstr_t));
	ret->cstr = NULL;
	ret->max = 0;
	ret->len = 0;
	mstr_cpy(ret, s);
	return ret;
}

mstr_t* mstr_new_by_size(uint32_t sz) {
	mstr_t* ret = (mstr_t*)_malloc(sizeof(mstr_t));
	ret->cstr = (char*)_malloc(sz);
	ret->max = sz;
	ret->cstr[0] = 0;
	ret->len = 0;
	return ret;
}

char* mstr_append(mstr_t* str, const char* src) {
	if(src == NULL || src[0] == 0) {
		return str->cstr;
	}

	uint32_t len = (uint32_t)strlen(src);
	uint32_t new_size = str->len + len;
	if(str->max <= new_size) {
		new_size = str->len + len + mstr_BUF; /*STR BUF for buffer*/
		str->cstr = (char*)_realloc(str->cstr, str->max, new_size);
		str->max = new_size;
	}

	strcpy(str->cstr + str->len, src);
	str->len = str->len + len;
	str->cstr[str->len] = 0;
	return str->cstr;
}

char* mstr_add(mstr_t* str, char c) {
	uint32_t new_size = str->len + 1;
	if(str->max <= new_size) {
		new_size = str->len + mstr_BUF; /*STR BUF for buffer*/
		str->cstr = (char*)_realloc(str->cstr, str->max, new_size);
		str->max = new_size;
	}

	str->cstr[str->len] = c;
	str->len++;
	str->cstr[str->len] = 0;
	return str->cstr;
}

char* mstr_add_int(mstr_t* str, int i, int base) {
	return mstr_append(str, mstr_from_int(i, base));
}

char* mstr_add_float(mstr_t* str, float f) {
	return mstr_append(str, mstr_from_float(f));
}

void mstr_free(mstr_t* str) {
	if(str == NULL)
		return;

	if(str->cstr != NULL) {
		_free(str->cstr);
	}
	_free(str);
}

static char _mstr_result[STATIC_mstr_MAX+1];

const char* mstr_from_int(int value, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) 
			base = 10;

    char* ptr = _mstr_result, *ptr1 = _mstr_result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return _mstr_result;
}

const char* mstr_from_bool(bool b) {
	return b ? "true":"false";
}

const char* mstr_from_float(float i) {
	snprintf(_mstr_result, STATIC_mstr_MAX-1, "%f", i);
	return _mstr_result;
}

int mstr_to_int(const char* str) {
	int i = 0;
	if(strstr(str, "0x") != NULL ||
			strstr(str, "0x") != NULL)
		i = (int)strtol(str, NULL, 16);
	else
		i = (int)strtol(str, NULL, 10);
	return i;
}

float mstr_to_float(const char* str) {
	return 0.0;//atof(str);
}

void mstr_split(const char* str, char c, m_array_t* array) {
	int i = 0;
	char offc = str[i];
	while(true) {
		if(offc == c || offc == 0) {
			char* p = (char*)_malloc(i+1);
			memcpy(p, str, i+1);
			p[i] = 0;
			array_add(array, p);
			if(offc == 0)
				break;

			str = str +  i + 1;
			i = 0;
			offc = str[i]; 
		}
		else {
			i++;
			offc = str[i]; 
		}
	}
}

int mstr_to(const char* str, char c, mstr_t* res, bool skipspace) {
	int i = 0;
	mstr_reset(res);

	while(true) {
		char offc = str[i]; 
		if(offc==0) {//the end of str
			return -1;
		}

		//skip space
		if(skipspace && (offc == ' ' || offc == '\t')) {
			i++;
			continue;
		}

		if(offc == c) 
			break;
		else
			mstr_add(res, offc);
		i++;
	}

	if(skipspace) {
		int j = res->len - 1;
		while(j >= 0) {
			char c = res->cstr[j];
			if(c == ' ' || c == '\t')
				res->cstr[j] = 0;
			j--;
		}
	}

	return i;
}

/**======utf8 functions======*/

#define isASCII(b)  ((b & 0x80) == 0)

void utf8_reader_init(utf8_reader_t* reader, const char* s, uint32_t offset) {
	if(reader == NULL)
		return;
	
	reader->str = s;
	reader->offset = offset;
}

/**Read single word with UTF-8 encode
*/
bool utf8_read(utf8_reader_t* reader, mstr_t* dst) {
	if(reader == NULL || reader->str == NULL)
		return false;
	const char* src = reader->str;
	mstr_reset(dst);

	uint8_t b;
	b = src[reader->offset++];
	if(b == 0)//end of input
		return false; 

	mstr_add(dst, b);
	if(!isASCII(b)) { //not ASCII
		uint8_t count = 0;
		if((b >> 4) == 0x0E) { //3 bytes encode like UTF-8 Chinese
			count = 2;
		}
		else if((b >> 3) == 0x1E) { //4 bytes encode
			count = 3;
		}
		else if((b >> 2) == 0x3E) { //5 bytes encode
			count = 4;
		}
		else if((b >> 1) == 0x7E) { //6 bytes encode
			count = 5;
		}

		while(count > 0) {
			b = src[reader->offset++];
			if(b == 0)
				return false; //wrong encode.
			mstr_add(dst, b);
			count--;
		}
	}
	return true;
}

utf8_t* utf8_new(const char* s) {
	utf8_t* ret = (utf8_t*)_malloc(sizeof(utf8_t));
	array_init(ret);
	utf8_reader_t reader;
	utf8_reader_init(&reader, s, 0);
	while(true) {
		mstr_t* str = mstr_new("");
		if(!utf8_read(&reader, str)) {
			mstr_free(str);
			break;
		}
		array_add(ret, str);
	}
	return ret;
}

void utf8_free(utf8_t* utf8) {
	if(utf8 == NULL)
		return;
	array_clean(utf8, (free_func_t)mstr_free);
	_free(utf8);
}

uint32_t utf8_len(utf8_t* utf8) {
	if(utf8 == NULL)
		return 0;
	return utf8->size;
}

mstr_t* utf8_at(utf8_t* utf8, uint32_t at) {
	if(utf8 == NULL || at >= utf8_len(utf8))
		return NULL;
	return (mstr_t*)array_get(utf8, at);
}

void utf8_set(utf8_t* utf8, uint32_t at, const char* s) {
	if(s == NULL || s[0]  == 0) {
		array_del(utf8, at, (free_func_t)mstr_free);
		return;
	}

	mstr_t* str = utf8_at(utf8, at);
	if(str == NULL)
		return;
	mstr_cpy(str, s);
}

void utf8_append_raw(utf8_t* utf8, const char* s) {
	if(utf8 == NULL || s == NULL || s[0] == 0)
		return;

	utf8_t* u = utf8_new(s);
	uint32_t len = utf8_len(u);
	uint32_t i;
	for(i=0; i<len; ++i) {
		mstr_t* s = utf8_at(u, i);
		if(s == NULL)
			break;
		array_add(utf8, s);
	}
	array_remove_all(u); //don't free items in 'u' coz all moved to 'utf8'
	utf8_free(u);
}

void utf8_append(utf8_t* utf8, const char* s) {
	if(utf8 == NULL || s == NULL || s[0] == 0)
		return;
	array_add(utf8, mstr_new(s));
}

void utf8_to_str(utf8_t* utf8, mstr_t* str) {
	mstr_reset(str);
	if(utf8 == NULL)
		return;

	uint32_t len = utf8_len(utf8);
	uint32_t i;
	for(i=0; i<len; ++i) {
		mstr_t* s = utf8_at(utf8, i);
		if(s == NULL)
			break;
		mstr_append(str, s->cstr);
	}
}

/**======lex functions======*/

bool is_whitespace(unsigned char ch) {
	if(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
		return true;
	return false;
}

bool is_space(unsigned char ch) {
	if(ch == ' ' || ch == '\t' || ch == '\r')
		return true;
	return false;
}

bool is_numeric(unsigned char ch) {
	if(ch >= '0' && ch <= '9')
		return true;
	return false;
}

bool is_number(const char* cstr) {
	int  i= 0;
	while(cstr[i] != 0) {
		if (is_numeric(cstr[i]) == false)
			return false;
		i++;
	}
	return true;
}

bool is_hexadecimal(unsigned char ch) {
	if(((ch>='0') && (ch<='9')) ||
		((ch>='a') && (ch<='f')) ||
		((ch>='A') && (ch<='F')))
			return true;
	return false;
}

bool is_alpha(unsigned char ch) {
	if(((ch>='a') && (ch<='z')) ||
		((ch>='A') && (ch<='Z')) ||
		ch == '_')
		return true;
	return false;
}

bool is_alpha_num(const char* cstr) {
	if (cstr[0] == 0){
		return true;
	}
	if (is_alpha(cstr[0]) == 0){
		return false;
	}

	int  i= 0;
	while(cstr[i] != 0) {
		if (is_alpha(cstr[i]) == false || is_numeric(cstr[i]) == true){
			return false;
		}
		i++;
	}
	return true;
}

void lex_get_nextch(lex_t* lex) {
	lex->curr_ch = lex->next_ch;
	if (lex->data_pos < lex->data_end){
		lex->next_ch = lex->data[lex->data_pos];
	}else{
		lex->next_ch = 0;
	}
	lex->data_pos++;
}

void lex_skip_whitespace(lex_t* lex) {
	while (lex->curr_ch && is_whitespace(lex->curr_ch)){
		lex_get_nextch(lex);
	}
}

void lex_skip_space(lex_t* lex) {
	while (lex->curr_ch && is_space(lex->curr_ch)){
		lex_get_nextch(lex);
	}
}

//only take first 1~2 chars from start
bool lex_skip_comments_line(lex_t* lex, const char* start) {
	if(start[0] == 0)
		return false;

	if ((lex->curr_ch==start[0] && (start[1] == 0 || lex->next_ch==start[1]))) {
		while (lex->curr_ch && lex->curr_ch!='\n'){
			lex_get_nextch(lex);
		}
		if(start[1] != 0)
			lex_get_nextch(lex);
		return true;
	}
	return false;
}

//only take first 2 chars from start and end.
bool lex_skip_comments_block(lex_t* lex, const char* start, const char* end) {
	if(start[0] == 0 || start[1] == 0 || end[0] == 0 || end[1] == 0)
		return false;

	if (lex->curr_ch==start[0] && lex->next_ch==start[1]) {
		while (lex->curr_ch && (lex->curr_ch!=end[0] || lex->next_ch!=end[1])) {
			lex_get_nextch(lex);
		}
		lex_get_nextch(lex);
		lex_get_nextch(lex);
		return true;
	}	
	return false;
}

void lex_reset(lex_t* lex) {
	lex->data_pos = lex->data_start;
	lex->tk_start   = 0;
	lex->tk_end     = 0;
	lex->tk_last_end = 0;
	lex->tk  = LEX_EOF;
	mstr_reset(lex->tk_str);
	lex_get_nextch(lex);
	lex_get_nextch(lex);
}

void lex_init(lex_t * lex, const char* input) {
	lex->data = input;
	lex->data_start = 0;
	lex->data_end = (int)strlen(lex->data);
	lex->tk_str = mstr_new("");
	lex_reset(lex);
}

void lex_release(lex_t* lex) {
	mstr_free(lex->tk_str);
	lex->tk_str = NULL;
}

void lex_token_start(lex_t* lex) {
	// record beginning of this token(pre-read 2 chars );
	lex->tk_start = lex->data_pos-2;
}

void lex_token_end(lex_t* lex) {
	lex->tk_last_end = lex->tk_end;
	lex->tk_end = lex->data_pos-3;
}

void lex_get_char_token(lex_t* lex) {
	lex->tk = lex->curr_ch;
	if (lex->curr_ch) 
		lex_get_nextch(lex);
}

void lex_get_basic_token(lex_t* lex) {
	// tokens
	if (is_alpha(lex->curr_ch)) { //  IDs
		while (is_alpha(lex->curr_ch) || is_numeric(lex->curr_ch)) {
			mstr_add(lex->tk_str, lex->curr_ch);
			lex_get_nextch(lex);
		}
		lex->tk = LEX_ID;
	} else if (is_numeric(lex->curr_ch)) { // _numbers
		bool isHex = false;
		if (lex->curr_ch=='0') {
			mstr_add(lex->tk_str, lex->curr_ch);
			lex_get_nextch(lex);
		}
		if (lex->curr_ch=='x') {
			isHex = true;
			mstr_add(lex->tk_str, lex->curr_ch);
			lex_get_nextch(lex);
		}
		lex->tk = LEX_INT;

		while (is_numeric(lex->curr_ch) || (isHex && is_hexadecimal(lex->curr_ch))) {
			mstr_add(lex->tk_str, lex->curr_ch);
			lex_get_nextch(lex);
		}
		if (!isHex && lex->curr_ch=='.' && is_numeric(lex->next_ch)) {
			lex->tk = LEX_FLOAT;
			mstr_add(lex->tk_str, '.');
			lex_get_nextch(lex);
			while (is_numeric(lex->curr_ch)) {
				mstr_add(lex->tk_str, lex->curr_ch);
				lex_get_nextch(lex);
			}
		}
		// do fancy e-style floating point
		if (!isHex && (lex->curr_ch=='e'||lex->curr_ch=='E')) {
			lex->tk = LEX_FLOAT;
			mstr_add(lex->tk_str, lex->curr_ch);
			lex_get_nextch(lex);
			if (lex->curr_ch=='-') {
				mstr_add(lex->tk_str, lex->curr_ch);
				lex_get_nextch(lex);
			}
			while (is_numeric(lex->curr_ch)) {
				mstr_add(lex->tk_str, lex->curr_ch);
				lex_get_nextch(lex);
			}
		}
	} else if (lex->curr_ch=='"') {
		// strings...
		lex_get_nextch(lex);
		while (lex->curr_ch && lex->curr_ch!='"') {
			if (lex->curr_ch == '\\') {
				lex_get_nextch(lex);
				switch (lex->curr_ch) {
					case 'n' : mstr_add(lex->tk_str, '\n'); break;
					case 'r' : mstr_add(lex->tk_str, '\r'); break;
					case 't' : mstr_add(lex->tk_str, '\t'); break;
					case '"' : mstr_add(lex->tk_str, '\"'); break;
					case '\\' : mstr_add(lex->tk_str, '\\'); break;
					default: mstr_add(lex->tk_str, lex->curr_ch);
				}
			} else {
				mstr_add(lex->tk_str, lex->curr_ch);
			}
			lex_get_nextch(lex);
		}
		lex_get_nextch(lex);
		lex->tk = LEX_STR;
	}
}

void lex_get_pos(lex_t* lex, int* line, int *col, int pos) {
	if (pos<0) 
		pos= lex->tk_last_end;

	int l = 1;
	int c  = 1;
	int i;
	for (i=0; i<pos; i++) {
		char ch;
		if (i < lex->data_end){
			ch = lex->data[i];
		}else{
			ch = 0;
		}

		c++;
		if (ch=='\n') {
			l++;
			c = 1;
		}
	}
	*line = l;
	*col = c;
}

typedef enum {
  // reserved words
#define LEX_R_LIST_START LEX_R_IF
  LEX_R_FUNCTION,
  LEX_R_TRUE,
  LEX_R_FALSE,
  LEX_R_NULL,
  LEX_R_UNDEFINED,
  LEX_R_LIST_END /* always the last entry */
} JSON_LEX_TYPES;

static void lex_js_get_js_str(lex_t* lex) {
	// js style strings 
	lex_get_nextch(lex);
	while (lex->curr_ch && lex->curr_ch!='\'') {
		if (lex->curr_ch == '\\') {
			lex_get_nextch(lex);
			switch (lex->curr_ch) {
				case 'n' : mstr_add(lex->tk_str, '\n'); break;
				case 'a' : mstr_add(lex->tk_str, '\a'); break;
				case 'r' : mstr_add(lex->tk_str, '\r'); break;
				case 't' : mstr_add(lex->tk_str, '\t'); break;
				case '\'' : mstr_add(lex->tk_str, '\''); break;
				case '\\' : mstr_add(lex->tk_str, '\\'); break;
				case 'x' : { // hex digits
										 char buf[3] = "??";
										 lex_get_nextch(lex);
										 buf[0] = lex->curr_ch;
										 lex_get_nextch(lex);
										 buf[1] = lex->curr_ch;
										 mstr_add(lex->tk_str, (char)strtol(buf,0,16));
									 } break;
				default: if (lex->curr_ch>='0' && lex->curr_ch<='7') {
									 // octal digits
									 char buf[4] = "???";
									 buf[0] = lex->curr_ch;
									 lex_get_nextch(lex);
									 buf[1] = lex->curr_ch;
									 lex_get_nextch(lex);
									 buf[2] = lex->curr_ch;
									 mstr_add(lex->tk_str, (char)strtol(buf,0,8));
								 } else
									 mstr_add(lex->tk_str, lex->curr_ch);
			}
		} else {
			mstr_add(lex->tk_str, lex->curr_ch);
		}
		lex_get_nextch(lex);
	}
	lex_get_nextch(lex);
	lex->tk = LEX_STR;
}

static void lex_js_get_reserved_word(lex_t *lex) {
	if (strcmp(lex->tk_str->cstr, "function") == 0)  lex->tk = LEX_R_FUNCTION;
	else if (strcmp(lex->tk_str->cstr, "true") == 0)      lex->tk = LEX_R_TRUE;
	else if (strcmp(lex->tk_str->cstr, "false") == 0)     lex->tk = LEX_R_FALSE;
	else if (strcmp(lex->tk_str->cstr, "null") == 0)      lex->tk = LEX_R_NULL;
	else if (strcmp(lex->tk_str->cstr, "undefined") == 0) lex->tk = LEX_R_UNDEFINED;
}

static void lex_js_get_next_token(lex_t* lex) {
	lex->tk = LEX_EOF;
	mstr_reset(lex->tk_str);

	lex_skip_whitespace(lex);
	if(lex_skip_comments_line(lex, "//")) {
		lex_js_get_next_token(lex);
		return;
	}
	if(lex_skip_comments_block(lex, "/*", "*/")) {
		lex_js_get_next_token(lex);
		return;
	}

	lex_token_start(lex);
	lex_get_basic_token(lex);

	if (lex->tk == LEX_ID) { //  IDs
		lex_js_get_reserved_word(lex);
	} 
	else if(lex->tk == LEX_EOF) {
		if (lex->curr_ch=='\'') {
			// js style strings 
			lex_js_get_js_str(lex);
		} 
		else {
			lex_get_char_token(lex);
		}
	}

	lex_token_end(lex);
}

static bool lex_js_chkread(lex_t* lex, uint32_t expected_tk) {
	if (lex->tk != expected_tk) {
		return false;
	}
	lex_js_get_next_token(lex);
	return true;
}

static var_t* json_parse_factor(vm_t* vm, lex_t *l) {
	if (l->tk==LEX_R_TRUE) {
		lex_js_chkread(l, LEX_R_TRUE);
		return var_new_int(vm, 1);
	}
	else if (l->tk==LEX_R_FALSE) {
		lex_js_chkread(l, LEX_R_FALSE);
		return var_new_int(vm, 0);
	}
	else if (l->tk==LEX_R_NULL) {
		lex_js_chkread(l, LEX_R_NULL);
		return var_new(vm);
	}
	else if (l->tk==LEX_R_UNDEFINED) {
		lex_js_chkread(l, LEX_R_UNDEFINED);
		return var_new(vm);
	}
	else if (l->tk==LEX_INT) {
		int i = atoi(l->tk_str->cstr);
		lex_js_chkread(l, LEX_INT);
		return var_new_int(vm, i);
	}
	else if (l->tk==LEX_FLOAT) {
		float f = 0.0;//atof(l->tk_str->cstr);
		lex_js_chkread(l, LEX_FLOAT);
		return var_new_float(vm, f);
	}
	else if (l->tk==LEX_STR) {
		mstr_t* s = mstr_new(l->tk_str->cstr);
		lex_js_chkread(l, LEX_STR);
		var_t* ret = var_new_str(vm, s->cstr);
		mstr_free(s);
		return ret;
	}
	else if(l->tk==LEX_R_FUNCTION) {
		lex_js_chkread(l, LEX_R_FUNCTION);
		//TODO
		mario_debug("[debug] Error: Can not parse json function item!\n");
		return var_new(vm);
	}
	else if (l->tk=='[') {
		/* JSON-style array */
		var_t* arr = var_new_array(vm);
		lex_js_chkread(l, '[');
		while (l->tk != ']') {
			var_t* v = json_parse_factor(vm, l);
			var_add(arr, "", v);
			if (l->tk != ']') 
				lex_js_chkread(l, ',');
		}
		lex_js_chkread(l, ']');
		return arr;
	}
	else if (l->tk=='{') {
		lex_js_chkread(l, '{');
		var_t* obj = var_new_obj(vm, NULL, NULL);
		while(l->tk != '}') {
			mstr_t* id = mstr_new(l->tk_str->cstr);
			if(l->tk == LEX_STR)
				lex_js_chkread(l, LEX_STR);
			else
				lex_js_chkread(l, LEX_ID);

			lex_js_chkread(l, ':');
			var_t* v = json_parse_factor(vm, l);
			var_add(obj, id->cstr, v);
			mstr_free(id);
			if(l->tk != '}')
				lex_js_chkread(l, ',');
		}
		lex_js_chkread(l, '}');
		return obj;
	}
	return var_new(vm);
}

var_t* json_parse(vm_t* vm, const char* str) {
	lex_t lex;
	lex_init(&lex, str);
	lex_js_get_next_token(&lex);

	var_t* ret = json_parse_factor(vm, &lex);
	lex_release(&lex);
	return ret;
}

/**======bytecode functions======*/

#define BC_BUF_SIZE  3232


uint32_t bc_getstrindex(bytecode_t* bc, const char* str) {
	uint32_t sz = bc->mstr_table.size;
	uint32_t i;
	if(str == NULL || str[0] == 0)
		return OFF_MASK;

	for(i=0; i<sz; ++i) {
		char* s = (char*)bc->mstr_table.items[i];
		if(s != NULL && strcmp(s, str) == 0)
			return i;
	}

	uint32_t len = (uint32_t)strlen(str);
	char* p = (char*)_malloc(len + 1);
	memcpy(p, str, len+1);
	array_add(&bc->mstr_table, p);
	return sz;
}	

void bc_init(bytecode_t* bc) {
	bc->cindex = 0;
	bc->code_buf = NULL;
	bc->buf_size = 0;
	array_init(&bc->mstr_table);
}

void bc_release(bytecode_t* bc) {
	array_clean(&bc->mstr_table, NULL);
	if(bc->code_buf != NULL)
		_free(bc->code_buf);
}

void bc_add(bytecode_t* bc, PC ins) {
	if(bc->cindex >= bc->buf_size) {
		bc->buf_size = bc->cindex + BC_BUF_SIZE;
		PC *new_buf = (PC*)_malloc(bc->buf_size*sizeof(PC));

		if(bc->cindex > 0 && bc->code_buf != NULL) {
			memcpy(new_buf, bc->code_buf, bc->cindex*sizeof(PC));
			_free(bc->code_buf);
		}
		bc->code_buf = new_buf;
	}

	bc->code_buf[bc->cindex] = ins;
	bc->cindex++;
}
	
PC bc_reserve(bytecode_t* bc) {
	bc_add(bc, INS(INSTR_NIL, OFF_MASK));
  return bc->cindex-1;
}

PC bc_bytecode(bytecode_t* bc, opr_code_t instr, const char* str) {
	opr_code_t r = instr;
	uint32_t i = OFF_MASK;

	if(str != NULL && str[0] != 0)
		i = bc_getstrindex(bc, str);

	return INS(r, i);
}
	
PC bc_gen_int(bytecode_t* bc, opr_code_t instr, int32_t i) {
	PC ins = bc_bytecode(bc, instr, "");
	bc_add(bc, ins);
	bc_add(bc, i);
	return bc->cindex;
}

PC bc_gen_short(bytecode_t* bc, opr_code_t instr, int32_t s) {
	PC ins = bc_bytecode(bc, instr, "");
	ins = (ins&0xFFF0000) | (s&OFF_MASK);
	bc_add(bc, ins);
	return bc->cindex;
}
	
PC bc_gen_str(bytecode_t* bc, opr_code_t instr, const char* str) {
	uint32_t i = 0;
	float f = 0.0;
	const char* s = str;

	if(instr == INSTR_INT) {
		if(strstr(str, "0x") != NULL ||
				strstr(str, "0x") != NULL)
			i = (uint32_t)strtoul(str, NULL, 16);
		else
			i = (uint32_t)strtol(str, NULL, 10);
		s = NULL;
	}
	else if(instr == INSTR_FLOAT) {
		f = 0.0;//atof(str);
		s = NULL;
	}
	
	PC ins = bc_bytecode(bc, instr, s);
	bc_add(bc, ins);

	if(instr == INSTR_INT) {
		if(i < OFF_MASK) //short int
			bc->code_buf[bc->cindex-1] = INS(INSTR_INT_S, i);
		else 	
			bc_add(bc, i);
	}
	else if(instr == INSTR_FLOAT) {
		memcpy(&i, &f, sizeof(PC));
		bc_add(bc, i);
	}
	return bc->cindex;
}

PC bc_gen(bytecode_t* bc, opr_code_t instr) {
	return bc_gen_str(bc, instr, "");
}

void bc_remove_instr(bytecode_t* bc, PC from, uint32_t num) {
	PC off = from+num;
	while(off < bc->cindex) {
		bc->code_buf[off-num] = bc->code_buf[off];
		off++;
	}
	bc->cindex -= num;
}

void bc_set_instr(bytecode_t* bc, PC anchor, opr_code_t op, PC target) {
	if(target == ILLEGAL_PC)
		target = bc->cindex;

	int offset = target > anchor ? (target-anchor) : (anchor-target);
	PC ins = INS(op, offset);
	bc->code_buf[anchor] = ins;
}

PC bc_add_instr(bytecode_t* bc, PC anchor, opr_code_t op, PC target) {
	if(target == ILLEGAL_PC)
		target = bc->cindex;

	int offset = target > anchor ? (target-anchor) : (anchor-target);
	PC ins = INS(op, offset);
	bc_add(bc, ins);
	return bc->cindex;
} 

/**======var functions======*/
load_m_func_t _load_m_func = NULL;

static var_t* var_clone(var_t* v) {
	switch(v->type) { //basic types
		case V_INT:
			return var_new_int(v->vm, var_get_int(v));
		case V_FLOAT:
			return var_new_float(v->vm, var_get_int(v));
		case V_STRING:
			return var_new_str(v->vm, var_get_str(v));
		/*case V_BOOL:
			return var_new_bool(v->vm, var_get_bool(v));
		case V_NULL:
			return var_new_null(v->vm);
		case V_UNDEF:
			return var_new(v->vm);*/
		default:
			break;
	}
	//object types
	return v; 
}

node_t* node_new(vm_t* vm, const char* name, var_t* var) {
	node_t* node = (node_t*)_malloc(sizeof(node_t));
	memset(node, 0, sizeof(node_t));

	node->magic = 1;
	uint32_t len = (uint32_t)strlen(name);
	node->name = (char*)_malloc(len+1);
	memcpy(node->name, name, len+1);
	if(var != NULL)
		//node->var = var_ref(var_clone(var));
		node->var = var_ref(var);
	else
		node->var = var_ref(var_new(vm));
	return node;
}

static inline bool var_empty(var_t* var) {
	if(var == NULL || var->status <= V_ST_GC_FREE)
		return true;
	return false;
}

void node_free(void* p) {
	node_t* node = (node_t*)p;
	if(node == NULL)
		return;

	_free(node->name);
	if(!var_empty(node->var)) {
		var_unref(node->var);
	}
	_free(node);
}

static inline bool node_empty(node_t* node) {
	if(node == NULL || var_empty(node->var))
		return true;
	return false;
}

inline var_t* node_replace(node_t* node, var_t* v) {
	var_t* old = node->var;
	//node->var = var_ref(var_clone(v));
	node->var = var_ref(v);
	var_unref(old);
	return v;
}

inline void var_remove_all(var_t* var) {
	/*free children*/
	array_clean(&var->children, node_free);
}

static inline node_t* var_find_raw(var_t* var, const char*name) {
	if(var_empty(var))
		return NULL;

	uint32_t i;
	for(i=0; i<var->children.size; i++) {
		node_t* node = (node_t*)var->children.items[i];
		if(node != NULL && node->name != NULL) {
			if(strcmp(node->name, name) == 0) {
				return node;
			}
		}
	}
	return NULL;
}

node_t* var_add(var_t* var, const char* name, var_t* add) {
	node_t* node = NULL;

	if(name[0] != 0) 
		node = var_find_raw(var, name);

	if(node == NULL) {
		node = node_new(var->vm, name, add);
		array_add(&var->children, node);
	}
	else if(add != NULL)
		node_replace(node, add);

	return node;
}

node_t* var_add_head(var_t* var, const char* name, var_t* add) {
	return var_add(var, name, add);
}

inline node_t* var_find(var_t* var, const char*name) {
	node_t* node = var_find_raw(var, name);
	if(node_empty(node))
		return NULL;
	return node;
}

inline var_t* var_find_var(var_t* var, const char*name) {
	node_t* node = var_find(var, name);
	if(node == NULL)
		return NULL;
	return node->var;
}

inline node_t* var_find_create(var_t* var, const char*name) {
	node_t* n = var_find(var, name);
	if(n != NULL)
		return n;
	n = var_add(var, name, NULL);
	return n;
}

node_t* var_get(var_t* var, int32_t index) {
	node_t* node = (node_t*)array_get(&var->children, index);
	if(node_empty(node))
		return NULL;
	return node;
}

node_t* var_array_get(var_t* var, int32_t index) {
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return NULL;

	int32_t i;
	for(i=arr_var->children.size; i<=index; i++) {
		var_add(arr_var, "", NULL);
	}

	node_t* node = (node_t*)array_get(&arr_var->children, index);
	if(node_empty(node))
		return NULL;
	return node;
}

var_t* var_array_get_var(var_t* var, int32_t index) {
	node_t* n = var_array_get(var, index);
	if(n != NULL)
		return n->var;
	return NULL;
}

node_t* var_array_set(var_t* var, int32_t index, var_t* set_var) {
	node_t* node = var_array_get(var, index);
	if(node != NULL)
		node_replace(node, set_var);
	return node;
}

node_t* var_array_add(var_t* var, var_t* add_var) {
	node_t* ret = NULL;
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var != NULL)
		ret = var_add(arr_var, "", add_var);
	return ret;
}

node_t* var_array_add_head(var_t* var, var_t* add_var) {
	node_t* ret = NULL;
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var != NULL)
		ret = var_add_head(arr_var, "", add_var);
	return ret;
}

uint32_t var_array_size(var_t* var) {
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return 0;
	return arr_var->children.size;
}

void var_array_reverse(var_t* arr) {
	uint32_t sz = var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz/2; ++i) {
		node_t* n1 = var_array_get(arr, i);
		node_t* n2 = var_array_get(arr, sz-i-1);
		if(n1 != NULL && n2 != NULL) {
			var_t* tmp = n1->var;
			n1->var = n2->var;
			n2->var = tmp;
		}
	}
}

node_t* var_array_remove(var_t* var, int32_t index) {
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return NULL;
	return (node_t*)array_remove(&arr_var->children, index);
}

void var_array_del(var_t* var, int32_t index) {
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return;
	array_del(&arr_var->children, index, node_free);
}

inline void var_clean(var_t* var) {
	if(var_empty(var))
		return;
	var->status = V_ST_FREE; //mark as freed for avoid dead loop

	if(var->on_destroy != NULL) {
		var->on_destroy(var);
	}

	/*free children*/
	if(var->children.size > 0)
		var_remove_all(var);	

	/*free value*/
	if(var->value != NULL) {
		if(var->free_func != NULL) 
			var->free_func(var->value);
		else
			_free(var->value);
		var->value = NULL;
	}

	var_t* next = var->next; //backup next
	var_t* prev = var->prev; //backup prev
	memset(var, 0, sizeof(var_t));
	var->next = next;
	var->prev = prev;
}

static inline void add_to_free(var_t* var) {
	vm_t* vm = var->vm;
	var->status = V_ST_FREE;
	if(vm->free_vars != NULL)
		vm->free_vars->prev = var;
	var->next = vm->free_vars;
	vm->free_vars = var;
	vm->free_vars_num++;
}

#define GC_BUFFER 128
static void gc(vm_t* vm, bool force);
static inline void add_to_gc(var_t* var) {
	vm_t* vm = var->vm;
	var->prev = vm->gc_vars_tail;
	if(vm->gc_vars_tail != NULL)
		vm->gc_vars_tail->next = var;
	else {
		vm->gc_vars = var;
	}
	var->next = NULL;
	vm->gc_vars_tail = var;
	var->status = V_ST_GC;
	vm->gc_vars_num++;

	if(vm->gc_vars_num > GC_BUFFER)
		gc(vm, false);
}

static inline var_t* get_from_free(vm_t* vm) {
	if(vm->is_doing_gc)
		return NULL;

	var_t* var = vm->free_vars;
	if(var != NULL) {
		vm->free_vars = var->next;
		if(vm->free_vars != NULL)
			vm->free_vars->prev = NULL;
		if(vm->free_vars_num > 0)
			vm->free_vars_num--;
	}
	return var;
}

static inline void remove_from_gc(var_t* var) {
	vm_t* vm = var->vm;
	if(var->prev != NULL)
		var->prev->next = var->next;
	else
		vm->gc_vars = var->next;

	if(var->next != NULL)
		var->next->prev = var->prev;
	else
		vm->gc_vars_tail = var->prev;

	var->prev = var->next = NULL;
	if(var->vm->gc_vars_num > 0)
		var->vm->gc_vars_num--;
}

static inline void gc_mark(var_t* var, bool mark) {
 	if(var_empty(var))
 		return;

 	var->gc_marking = true;
 	uint32_t i;
 	for(i=0; i<var->children.size; i++) {
 		node_t* node = (node_t*)var->children.items[i];
 		if(!node_empty(node)) {
 			node->var->gc_marked = mark;
			if(node->var->gc_marking == false) {
				gc_mark(node->var, mark);
			}
 		}
 	}
 	var->gc_marking = false;
}

static inline void gc_mark_cache(vm_t* vm, bool mark) {
#ifdef MARIO_CACHE
	uint32_t i;
	for(i=0; i<vm->var_cache_used; ++i) {
		var_t* v = vm->var_cache[i];
		gc_mark(v, mark);
	}
#endif
}

static inline void gc_mark_stack(vm_t* vm, bool mark) {
	int i = vm->stack_top-1;
	while(i>=0) {
		void *p = vm->stack[i];
		i--;
		if(p == NULL)
			continue;

		int8_t magic = *(int8_t*)p;
		var_t* v = NULL;
		if(magic == 0) { //var
			v = (var_t*)p;
		}
		else {//node
			node_t* node = (node_t*)p;
			if(node != NULL)
				v = node->var;
		}

		gc_mark(v, mark);
	}
}

static inline void gc_mark_isignal(vm_t* vm, bool mark) {
#ifdef MARIO_THREAD
	isignal_t* sig = vm->isignal_head;
	while(sig != NULL) {
		gc_mark(sig->handle_func, mark);
		gc_mark(sig->obj, mark);
		sig = sig->next;
	}
#endif
}

static inline void var_free(void* p) {
	var_t* var = (var_t*)p;
	if(var_empty(var))
		return;

	vm_t* vm = var->vm;
	uint32_t status = var->status; //store status of variable
	//clean var.
	var_clean(var);
	var->type = V_UNDEF;
	var->vm = vm;

	if(status == V_ST_GC) { //if in gc_vars list
		if(vm->is_doing_gc) { // if is doing gc, change status to GC_FREE for moving to free_vars list later.
			var->status = V_ST_GC_FREE;
		}
		else { //not doing gc, move to free_vars list immediately.
			remove_from_gc(var);
			add_to_free(var);
		}
	}
	else if(status != V_ST_FREE) {
		add_to_free(var);
	}
}

inline var_t* var_ref(var_t* var) {
	++var->refs;
	if(var->status == V_ST_GC) {
		/*remove from vm->gc_vars list.*/
		remove_from_gc(var);
		var->status = V_ST_REF;
	}
	return var;
}

inline void var_unref(var_t* var) {
	if(var_empty(var))
		return;

	if(var->refs > 0)
		--var->refs;

	if(var->refs == 0) {
		/*referenced count is 0, means this variable not be referenced anymore,
		free it immediately.*/
		var_free(var);
	}
	else if(var->status == V_ST_REF) { 
		/*referenced count not 0, means this variable still be referenced,
		add to vm->gc_vars list for rooted checking.*/
		add_to_gc(var);
	}
}

static inline void gc_vars(vm_t* vm) {
	gc_mark(vm->root, true); //mark all rooted vars
	gc_mark_stack(vm, true); //mark all stacked vars
	gc_mark_isignal(vm, true); //mark all interrupt signal vars
	gc_mark_cache(vm, true); //mark all cached vars

	var_t* v = vm->gc_vars;
	//first step: free unmarked vars
	while(v != NULL) {
		var_t* next = v->next;
		if(v->status == V_ST_GC && v->gc_marked == false) {
			var_free(v);
		}
		v = next;
	}

	gc_mark(vm->root, false); //unmark all rooted vars
	gc_mark_stack(vm, false); //unmark all stacked vars
	gc_mark_isignal(vm, false); //unmark all interrupt signal vars
	gc_mark_cache(vm, false); //unmark all cached vars

	//second step: move freed var to free_var_list for reusing.
	v = vm->gc_vars;
	while(v != NULL) {
		var_t* next = v->next;
		if(v->status == V_ST_GC_FREE) {
			remove_from_gc(v);	
			add_to_free(v);
		}
		v = next;
	}
}

static inline void gc_free_vars(vm_t* vm, uint32_t buffer_size) {
	var_t* v = vm->free_vars;
	while(v != NULL) {
		var_t* vtmp = v->next;
		_free(v);
		v = vtmp;
		vm->free_vars = v;
		vm->free_vars_num--;
		if(vm->free_vars_num <= buffer_size)
			break;
	}
}

static inline void gc(vm_t* vm, bool force) {
	if(vm->is_doing_gc)
		return;
	if(!force && vm->gc_vars_num < vm->gc_buffer_size)
		return;
	mario_debug("[debug] do gc ......");
	vm->is_doing_gc = true;
	gc_vars(vm);
	gc_free_vars(vm, force ? 0:vm->gc_free_buffer_size);
	vm->is_doing_gc = false;
	mario_debug(" gc done.\n");
}

static const char* get_typeof(var_t* var) {
	switch(var->type) {
		case V_UNDEF:
			return "undefined";
		case V_INT:
		case V_FLOAT:
			return "number";
		case V_BOOL: 
			return "boolean";
		case V_STRING: 
			return "string";
		case V_NULL: 
			return "null";
		case V_OBJECT: 
			return var->is_func ? "function": "object";
	}
	return "undefined";
}

inline var_t* var_new(vm_t* vm) {
	var_t* var = get_from_free(vm);
	if(var == NULL) {
        uint32_t sz = sizeof(var_t);
		var = (var_t*)_malloc(sz);
	}

	memset(var, 0, sizeof(var_t));
	var->type = V_UNDEF;
	var->vm = vm;
	var->status = V_ST_REF;
	return var;
}

inline var_t* var_new_block(vm_t* vm) {
	var_t* var = var_new_obj(vm, NULL, NULL);
	return var;
}

inline var_t* var_new_array(vm_t* vm) {
	var_t* var = var_new_obj(vm, NULL, NULL);
	var->is_array = 1;
	var_t* members = var_new_obj(vm, NULL, NULL);
	var_add(var, "_ARRAY_", members);
	return var;
}

inline var_t* var_new_int(vm_t* vm, int i) {
	var_t* var = var_new(vm);
	var->type = V_INT;
	var->value = _malloc(sizeof(int));
	*((int*)var->value) = i;
	return var;
}

inline var_t* var_new_null(vm_t* vm) {
	var_t* var = var_new(vm);
	var->type = V_NULL;
	return var;
}

inline var_t* var_new_bool(vm_t* vm, bool b) {
	var_t* var = var_new(vm);
	var->type = V_BOOL;
	var->value = _malloc(sizeof(int));
	*((int*)var->value) = b;
	return var;
}

inline var_t* var_new_obj(vm_t* vm, void*p, free_func_t fr) {
	var_t* var = var_new(vm);
	var->type = V_OBJECT;
	var->value = p;
	var->free_func = fr;
	return var;
}

inline var_t* var_new_float(vm_t* vm, float i) {
	var_t* var = var_new(vm);
	var->type = V_FLOAT;
	var->value = _malloc(sizeof(float));
	*((float*)var->value) = i;
	return var;
}

var_t* var_get_prototype(var_t* var) {
	return get_obj(var, PROTOTYPE);
}

inline var_t* var_new_str(vm_t* vm, const char* s) {
	var_t* var = var_new(vm);
	var->type = V_STRING;
	var->size = (uint32_t)strlen(s);
	var->value = _malloc(var->size + 1);
	memcpy(var->value, s, var->size + 1);
	return var;
}

inline var_t* var_new_str2(vm_t* vm, const char* s, uint32_t len) {
	var_t* var = var_new(vm);
	var->type = V_STRING;
	var->size = (uint32_t)strlen(s);
	if(var->size > len)
		var->size = len;
	var->value = _malloc(var->size + 1);
	memcpy(var->value, s, var->size + 1);
	((char*)(var->value))[var->size] = 0;
	return var;
}

inline const char* var_get_str(var_t* var) {
	if(var == NULL || var->value == NULL)
		return "";
	
	return (const char*)var->value;
}

inline var_t* var_set_str(var_t* var, const char* v) {
	if(v == NULL)
		return var;

	var->type = V_STRING;
	if(var->value != NULL)
		_free(var->value);
	uint32_t len = (uint32_t)strlen(v)+1;
	var->value = _malloc(len);
	memcpy(var->value, v, len);
	return var;
}

inline bool var_get_bool(var_t* var) {
	if(var == NULL || var->value == NULL)
		return false;
	int i = (int)(*(int*)var->value);
	return i==0 ? false:true;
}

inline int var_get_int(var_t* var) {
	if(var == NULL || var->value == NULL)
		return 0;
	if(var->type == V_FLOAT)	
		return (int)(*(float*)var->value);
	return *(int*)var->value;
}

inline var_t* var_set_int(var_t* var, int v) {
	var->type = V_INT;
	if(var->value != NULL)
		_free(var->value);
	var->value = _malloc(sizeof(int));
	*((int*)var->value) = v;
	return var;
}

inline float var_get_float(var_t* var) {
	if(var == NULL || var->value == NULL)
		return 0.0;
	
	if(var->type == V_INT)	
		return (float)(*(int*)var->value);
	return *(float*)var->value;
}

inline var_t* var_set_float(var_t* var, float v) {
	var->type = V_FLOAT;
	if(var->value != NULL)
		_free(var->value);
	var->value = _malloc(sizeof(int));
	*((float*)var->value) = v;
	return var;
}

inline func_t* var_get_func(var_t* var) {
	if(var == NULL || var->value == NULL)
		return NULL;
	
	return (func_t*)var->value;
}

static void get_m_str(const char* str, mstr_t* ret) {
	mstr_reset(ret);
	mstr_add(ret, '"');
	/*
	while(*str != 0) {
		switch (*str) {
			case '\\': mstr_append(ret, "\\\\"); break;
			case '\n': mstr_append(ret, "\\n"); break;
			case '\r': mstr_append(ret, "\\r"); break;
			case '\a': mstr_append(ret, "\\a"); break;
			case '"':  mstr_append(ret, "\\\""); break;
			default: mstr_add(ret, *str);
		}
		str++;
	}
	*/
	mstr_append(ret, str);
	mstr_add(ret, '"');
}

void var_to_str(var_t* var, mstr_t* ret) {
	mstr_reset(ret);
	if(var == NULL) {
		mstr_cpy(ret, "undefined");
		return;
	}

	switch(var->type) {
	case V_INT:
		mstr_cpy(ret, mstr_from_int(var_get_int(var), 10));
		break;
	case V_FLOAT:
		mstr_cpy(ret, mstr_from_float(var_get_float(var)));
		break;
	case V_STRING:
		mstr_cpy(ret, var_get_str(var));
		break;
	case V_OBJECT:
		var_to_json_str(var, ret, 0);
		break;
	case V_BOOL:
		mstr_cpy(ret, var_get_int(var) == 1 ? "true":"false");
		break;
	case V_NULL:
		mstr_cpy(ret, "null");
		break;
	default:
		mstr_cpy(ret, "undefined");
		break;
	}
}

static void get_parsable_str(var_t* var, mstr_t* ret) {
	mstr_reset(ret);

	mstr_t* s = mstr_new("");
	var_to_str(var, s);
	if(var->type == V_STRING)
		get_m_str(s->cstr, ret);
	else
		mstr_cpy(ret, s->cstr);

	mstr_free(s);
}

static void append_json_spaces(mstr_t* ret, int level) {
	int spaces;
	for (spaces = 0; spaces<=level; ++spaces) {
        mstr_add(ret, ' '); mstr_add(ret, ' ');
	}
}

static bool _done_arr_inited = false;
void var_to_json_str(var_t* var, mstr_t* ret, int level) {
	mstr_reset(ret);

	uint32_t i;

	//check if done to avoid dead recursion
	static m_array_t done;
	if(level == 0) {
		if(!_done_arr_inited) {		
			array_init(&done);
			_done_arr_inited = true;
		}
		array_remove_all(&done);
	}
	if(var->type == V_OBJECT) {
		uint32_t sz = done.size;
		for(i=0; i<sz; ++i) {
			if(done.items[i] == var) { //already done before.
				mstr_cpy(ret, "{}");
				if(level == 0)
					array_remove_all(&done);
				return;
			}
		}
		array_add(&done, var);
	}

	if (var->is_array) {
		mstr_add(ret, '[');
		uint32_t len = var_array_size(var);
		if (len>100) len=100; // we don't want to get stuck here!

		uint32_t i;
		for (i=0;i<len;i++) {
			node_t* n = var_array_get(var, i);

			mstr_t* s = mstr_new("");
			var_to_json_str(n->var, s, level);
			mstr_append(ret, s->cstr);
			mstr_free(s);

			if (i<len-1) 
				mstr_append(ret, ", ");
		}
		mstr_add(ret, ']');
	}
	else if (var->is_func) {
		mstr_append(ret, "function (");
		// get list of parameters
		int sz = 0;
		if(var->value != NULL) {
			func_t* func = var_get_func(var);
			sz = func->args.size;
			int i=0;
			for(i=0; i<sz; ++i) {
				mstr_append(ret, (const char*)func->args.items[i]);
				if ((i+1) < sz) {
					mstr_append(ret, ", ");
				}
			}
		}
		// add function body
		mstr_append(ret, ") {}");
		//return;
	}
	else if (var->type == V_OBJECT) {
		// children - handle with bracketed list
		int sz = (int)var->children.size;
		if(sz > 0)
			mstr_append(ret, "{\n");
		else
			mstr_append(ret, "{");

		int i;
		bool had = false;
		for(i=0; i<sz; ++i) {
			node_t* n = var_get(var, i);
			if(strcmp(n->name, "prototype") == 0)
				continue;
			if(had)
				mstr_append(ret, ",\n");
			had = true;
			append_json_spaces(ret, level);
			mstr_add(ret, '"');
			mstr_append(ret, n->name);
			mstr_add(ret, '"');
			mstr_append(ret, ": ");

			mstr_t* s = mstr_new("");
			var_to_json_str(n->var, s, level+1);
			mstr_append(ret, s->cstr);
			mstr_free(s);
		}
		if(sz > 0) {
			mstr_add(ret, '\n');
		}
	
		append_json_spaces(ret, level - 1);	
		mstr_add(ret, '}');
	} 
	else {
		// no children or a function... just write value directly
		mstr_t* s = mstr_new("");
		get_parsable_str(var, s);
		mstr_append(ret, s->cstr);
		mstr_free(s);
	}

	if(level == 0) {
		array_remove_all(&done);
	}
}

static inline var_t* vm_load_var(vm_t* vm, const char* name, bool create) {
	node_t* n = vm_load_node(vm, name, create);
	if(n != NULL)
		return n->var;
	return NULL;
}

static inline void vm_load_basic_classes(vm_t* vm) {
	vm->var_String = vm_load_var(vm, "String", false);
	vm->var_Array = vm_load_var(vm, "Array", false);
	vm->var_Number = vm_load_var(vm, "Number", false);
}

/** var cache for const value --------------*/

#ifdef MARIO_CACHE

static void var_cache_init(vm_t* vm) {
	uint32_t i;
	for(i=0; i<vm->var_cache_used; ++i) {
		vm->var_cache[i] = NULL;
	}
	vm->var_cache_used = 0;	
}

static void var_cache_free(vm_t* vm) {
	uint32_t i;
	for(i=0; i<vm->var_cache_used; ++i) {
		var_t* v = vm->var_cache[i];
		var_unref(v);
		vm->var_cache[i] = NULL;
	}
	vm->var_cache_used = 0;	
}

/*
static int32_t var_cache(vm_t* vm, var_t* v) {
	if(vm->var_cache_used >= VAR_CACHE_MAX)
		return -1;
	vm->var_cache[vm->var_cache_used] = var_ref(v);
	vm->var_cache_used++;
	return vm->var_cache_used-1;
}

static bool try_cache(vm_t* vm, PC* ins, var_t* v) {
	if((*ins) & INSTR_OPT_CACHE) {
		int index = var_cache(vm, v); 
		if(index >= 0) 
			*ins = INS(INSTR_CACHE, index);
		return true;
	}

	*ins = (*ins) | INSTR_OPT_CACHE;
	return false;
}
*/

#endif

/**======Interrupt functions======*/

inline void vm_push(vm_t* vm, var_t* var) {  
	var_ref(var);
	if(vm->stack_top < VM_STACK_MAX) {
		vm->stack[vm->stack_top++] = var; 
	} 
}

inline void vm_push_node(vm_t* vm, node_t* node) {
	var_ref(node->var); 
	if(vm->stack_top < VM_STACK_MAX)
		vm->stack[vm->stack_top++] = node;
}

static inline var_t* vm_pop2(vm_t* vm) {
	void *p = NULL;
	vm->stack_top--;
	p = vm->stack[vm->stack_top];
	int8_t magic = *(int8_t*)p;
	var_t* v = NULL;
	//var
	if(magic == 0) {
		v = (var_t*)p;
	}
	else {
		//node
		node_t* node = (node_t*)p;
		if(!node_empty(node)) {
			v = node->var;
		}
	}

	if(var_empty(v))
		return NULL;
	return v;
}

/*#define vm_pop2(vm) ({ \
	(vm)->stack_top--; \
	void* p = (vm)->stack[(vm)->stack_top]; \
	int8_t magic = *(int8_t*)p; \
	var_t* ret = NULL; \
	if(magic == 0) { \
		ret = (var_t*)p; \
	} \
	else { \
	node_t* node = (node_t*)p; \
	if(node != NULL) \
		ret = node->var; \
	else \
		ret = NULL; \
	} \
	ret; \
})
*/

static inline bool vm_pop(vm_t* vm) {
	if(vm->stack_top == 0)
		return false;

	vm->stack_top--;
	void *p = vm->stack[vm->stack_top];
	int8_t magic = *(int8_t*)p;
	var_t* v = NULL;
	if(magic == 0) { //var
		v = (var_t*)p;
	}
	else { //node
		node_t* node = (node_t*)p;
		if(!node_empty(node)) {
			v = node->var;
		}
	}

	if(!var_empty(v))
		var_unref(v);
	return true;
}

static inline node_t* vm_pop2node(vm_t* vm) {
	void *p = NULL;
	/*if(vm->stack_top <= 0) 
		return NULL;
	*/

	vm->stack_top--;
	p = vm->stack[vm->stack_top];
	int8_t magic = *(int8_t*)p;
	if(magic != 1) {//not node!
		return NULL;
	}

	return (node_t*)p;
}

static var_t* vm_stack_pick(vm_t* vm, int depth) {
	int index = vm->stack_top - depth;
	if(index < 0)
		return NULL;

	void* p = vm->stack[index];
	var_t* ret = NULL;
	if(p == NULL)
		return ret;

	int8_t magic = *(int8_t*)p;
	if(magic == 1) {//node
		node_t* node = (node_t*)p;
		ret = node->var;
	}
	else {
		ret = (var_t*)p;
	}

	vm->stack_top--;
	int i;
	for(i=index; i<vm->stack_top; ++i) {
		vm->stack[i] = vm->stack[i+1];
	}
	return ret;
}

//scope of vm runing
typedef struct st_scope {
	struct st_scope* prev;
	var_t* var;
	PC pc_start; // continue anchor for loop
	PC pc; // try cache anchor , or break anchor for loop
	uint32_t is_func: 8;
	uint32_t is_block: 8;
	uint32_t is_try: 8;
	uint32_t is_loop: 8;
	//continue and break anchor for loop(while/for)
} scope_t;

#define vm_get_scope(vm) (scope_t*)array_tail((vm)->scopes)
static inline var_t* vm_get_scope_var(vm_t* vm) {
	var_t* ret = vm->root;
	scope_t* sc = (scope_t*)array_tail(vm->scopes);
	if(sc != NULL && !var_empty(sc->var))
		ret = sc->var;
	return ret;
}


static scope_t* scope_new(var_t* var) {
	scope_t* sc = (scope_t*)_malloc(sizeof(scope_t));
	sc->prev = NULL;

	if(var != NULL)
		sc->var = var_ref(var);
	sc->pc = 0;
	sc->pc_start = 0;
	sc->is_block = false;
	sc->is_func = false;
	sc->is_try = false;
	sc->is_loop = false;
	return sc;
}

/*static scope_t* scope_clone(scope_t* src) {
	scope_t* sc = (scope_t*)_malloc(sizeof(scope_t));
	memcpy(sc, src, sizeof(scope_t));
	if(src->var != NULL)
		sc->var = var_ref(src->var);
	sc->prev = NULL;
	return sc;
}
*/

static void scope_free(void* p) {
	scope_t* sc = (scope_t*)p;
	if(sc == NULL)
		return;
	if(sc->var != NULL)
		var_unref(sc->var);
	_free(sc);
}
/*#define vm_get_scope_var(vm, skipBlock) ({ \
	scope_t* sc = (scope_t*)array_tail((vm)->scopes); \
	if((skipBlock) && sc != NULL && sc->var->type == V_BLOCK) \
		sc = sc->prev; \
	var_t* ret; \
	if(sc == NULL) \
		ret = (vm)->root; \
	else \
		ret = sc->var; \
	ret; \
})
*/

static void vm_push_scope(vm_t* vm, scope_t* sc) {
	scope_t* prev = NULL;
	if(vm->scopes->size > 0)
		prev = (scope_t*)array_tail(vm->scopes);
	array_add(vm->scopes, sc);	
	sc->prev = prev;
}

static PC vm_pop_scope(vm_t* vm) {
	if(vm->scopes->size <= 0)
		return 0;

	PC pc = 0;
	scope_t* sc = vm_get_scope(vm);
	if(sc == NULL)
		return 0;

	if(sc->is_func)
		pc = sc->pc;
	array_del(vm->scopes, vm->scopes->size-1, scope_free);
	gc(vm, false);
	return pc;
}

node_t* vm_find(vm_t* vm, const char* name) {
	var_t* var = vm_get_scope_var(vm);
	if(var_empty(var))
		return NULL;
	return var_find(var, name);	
}

node_t* vm_find_in_class(var_t* var, const char* name) {
	var_t* proto = var_get_prototype(var);
	while(proto != NULL) {
		node_t* ret = NULL;
		ret = var_find(proto, name);
		if(ret != NULL) {
			ret = var_add(var, name, var_clone(ret->var));
			return ret;
		}
		proto = var_get_prototype(proto);
	}
	return NULL;
}

bool var_instanceof(var_t* var, var_t* proto) {
	var_t* v = var_get_prototype(proto);
	if(v != NULL)
		proto = v;
		
	var_t* protov = var_get_prototype(var);
	while(protov != NULL) {
		if(protov == proto)
			return true;
		protov = var_get_prototype(protov);
	}
	return false;
}

node_t* find_member(var_t* obj, const char* name) {
	node_t* node = var_find(obj, name);
	if(node != NULL)
		return node;

	return vm_find_in_class(obj, name);
}

static inline node_t* vm_find_in_closure(var_t* closure, const char* name) {
	int sz = var_array_size(closure);
	int i;
	node_t* ret = NULL;
	for(i=0; i<sz; ++i) {
		var_t* var = var_array_get_var(closure, i);
		ret = var_find(var, name);
		if(ret != NULL)
			break;
	}
	return ret;
}

#define CLOSURE "__closure"

static inline node_t* vm_find_in_scopes(vm_t* vm, const char* name) {
	node_t* ret = NULL;
	scope_t* sc = vm_get_scope(vm);
	if(sc != NULL && sc->is_func) {
		var_t* closure = get_obj(sc->var, CLOSURE);
		if(closure != NULL) {
			ret = vm_find_in_closure(closure, name);	
			if(ret != NULL)
				return ret;
		}
	}
	
	while(sc != NULL) {
		if(!var_empty(sc->var)) {
			ret = var_find(sc->var, name);
			if(ret != NULL)
				return ret;
			
			var_t* obj = get_obj(sc->var, THIS);
			if(obj != NULL) {
				ret = find_member(obj, name);
				if(ret != NULL)
					return ret;
			}
		}
		sc = sc->prev;
	}

	return var_find(vm->root, name);
}

static inline var_t* vm_this_in_scopes(vm_t* vm) {
	node_t* n = vm_find_in_scopes(vm, THIS);
	if(n == NULL)
		return NULL;
	return n->var;
}

inline node_t* vm_load_node(vm_t* vm, const char* name, bool create) {
	var_t* var = vm_get_scope_var(vm);

	node_t* n;
	if(var != NULL) {
		n = find_member(var, name);
	}
	else {
		n = vm_find_in_scopes(vm, name);
	}

	if(n == NULL)
		n =  vm_find_in_scopes(vm, name);	

	if(n != NULL && n->var != NULL && n->var->status != V_ST_FREE)
		return n;
	/*
	mario_debug("Warning: '");	
	mario_debug(name);
	mario_debug("' undefined!\n");	
	*/
	if(!create)
		return NULL;

	if(var == NULL)
		return NULL;

	n =var_add(var, name, NULL);	
	return n;
}

/*static void var_clone_members(var_t* var, var_t* src) {
	//clone member varibles.
	uint32_t i;
	for(i=0; i<src->children.size; i++) {
		node_t* node = (node_t*)src->children.items[i];
		if(node != NULL && !node->var->is_func && node->name[0] != 0) { //don't clone functions.
			if(strcmp(node->name, THIS) == 0 ||
					strcmp(node->name, PROTOTYPE) == 0 ||
					strcmp(node->name, CONSTRUCTOR) == 0) 
				continue;
			var_add(var, node->name, node->var);	
		}
	}
}
*/

void var_set_prototype(var_t* var, var_t* proto) {
	if(var == NULL || proto == NULL)
		return;
	var_add(var, PROTOTYPE, proto);
}

void var_from_prototype(var_t* var, var_t* proto) {
	if(var == NULL || proto == NULL)
		return;
	var_set_prototype(var, proto);
//	var_clone_members(var, proto);
}

void var_instance_from(var_t* var, var_t* src) {
	if(var == NULL || src == NULL)
		return;

	var_t* proto = var_get_prototype(src);
	if(proto == NULL)
		proto = src;
	var_from_prototype(var, proto);
}

static void var_set_father(var_t* var, var_t* father) {
	if(var == NULL)
		return;

	var_t* proto = var_get_prototype(var);
	if(proto == NULL)
		return;

	var_t* super_proto = NULL;
	if(father != NULL) 
		super_proto = var_get_prototype(father);
	if(super_proto == NULL)
		return;

	var_set_prototype(proto, super_proto);
}

static inline var_t* var_build_basic_prototype(vm_t* vm, var_t* var) {
	var_t* protoV = var_get_prototype(var);
	if(protoV != NULL)
		return var;

	var_t* cls_var = NULL;
	if(var->type == V_STRING) { //get basic native class
		cls_var  = vm->var_String;
	}
	else if(var->is_array) {
		cls_var  = vm->var_Array;
	}
	else if(var->type == V_INT || var->type == V_FLOAT) {
		cls_var  = vm->var_Number;
	}

	if(cls_var != NULL) {
		protoV = var_get_prototype(cls_var); //set prototype of var
		var_set_prototype(var, protoV);
	}
	return var;
}

//for function.
static func_t* func_new() {
	func_t* func = (func_t*)_malloc(sizeof(func_t));
	if(func == NULL)
		return NULL;
	
	func->native = NULL;
	func->regular = true;
	func->pc = 0;
	func->data = NULL;
	func->owner = NULL;
	array_init(&func->args);
	return func;
}

static void func_free(void* p) {
	func_t* func = (func_t*)p;
	array_clean(&func->args, NULL);
	_free(p);
}

static var_t* var_new_func(vm_t* vm, func_t* func) {
	var_t* var = var_new_obj(vm, NULL, NULL);
	var->is_func = 1;
	var->free_func = func_free;
	var->value = func;

	var_t* proto = var_new_obj(vm, NULL, NULL);
	var_set_prototype(var, proto);
	//var_add(proto, CONSTRUCTOR, var);
	return var;
}

static var_t* find_func(vm_t* vm, var_t* obj, const char* fname) {
	//try full name with arg_num
	node_t* node = NULL;
	if(obj != NULL) {
		node = find_member(obj, fname);
	}
	if(node == NULL) {
		node = vm_find_in_scopes(vm, fname);
	}

	if(node != NULL && node->var != NULL && node->var->type == V_OBJECT) {
		return node->var;
	}
	return NULL;
}

static var_t* func_get_closure(var_t* var) {
	return get_obj(var, CLOSURE);
}

static void func_mark_closure(vm_t* vm, var_t* func) { //try mark function closure
	if(vm->scopes->size <=0)
		return;
	if(func_get_closure(func) != NULL)
		return;

	var_t* closure = var_new_array(vm);
	int i;
	bool mark = false;
	for(i=0; i<vm->scopes->size; ++i) {
		scope_t* sc = (scope_t*)array_get(vm->scopes, i);
		if(sc->is_func) { //enter closure
			mark = true;
			var_add(func, CLOSURE, closure);
		}
		if(mark)
			var_array_add(closure, sc->var);
	}
	if(!mark)
		var_unref(closure); //not a closure.
}

static bool func_call(vm_t* vm, var_t* obj, var_t* func_var, int arg_num) {
	var_t *env = var_new_obj(vm, NULL, NULL);
	var_t* args = var_new_array(vm);
	var_add(env, "arguments", args);
	func_t* func = var_get_func(func_var);
	if(obj == NULL) {
		//obj = vm->root;
	}
	else {
		var_add(env, THIS, obj);
	}
	
	var_t* closure = func_get_closure(func_var);
	if(closure != NULL)
		var_add(env, CLOSURE, closure);

	int32_t i;
	for(i=arg_num; i>func->args.size; i--) {
		var_t* v = vm_pop2(vm);
		var_array_add(args, v);
		var_unref(v);
	}

	for(i=(int32_t)func->args.size-1; i>=0; i--) {
		const char* arg_name = (const char*)array_get(&func->args, i);
		var_t* v = NULL;
		if(i >= arg_num) {
			v = var_new(vm);
			var_ref(v);
		}
		else {
			v = vm_pop2(vm);	
		}	
		if(v != NULL) {
			var_array_add(args, v);
			var_add(env, arg_name, v);
			var_unref(v);
		}
	}
	var_array_reverse(args); // reverse the args array coz stack index.

	if(func->owner != NULL) {
		var_t* super_v = var_get_prototype(func->owner);
		if(super_v != NULL)
			var_add(env, SUPER, super_v);
	}

	var_t* ret = NULL;
	vm_push(vm, env); //avoid for gc
	if(func->native != NULL) { //native function
		ret = func->native(vm, env, func->data);
		if(ret == NULL)
			ret = var_new(vm);
	}
	else {
		scope_t* sc = scope_new(env);
		sc->pc = vm->pc;
		sc->is_func = true;
		vm_push_scope(vm, sc);

		//script function
		vm->pc = func->pc;
		if(vm_run(vm)) { //with function return;
			ret = vm_pop2(vm);
			ret->refs--;
		}
	}
	var_ref(ret);
	vm_pop(vm);
	ret->refs--;
	vm_push(vm, ret);
	return true;
}

static var_t* func_def(vm_t* vm, bool regular, bool is_static) {
	func_t* func = func_new();
	func->regular = regular;
	func->is_static = is_static;
	while(true) {
		PC ins = vm->bc.code_buf[vm->pc++];
		opr_code_t instr = OP(ins);
		uint32_t offset = OFF(ins);
		if(instr == INSTR_JMP) {
			func->pc = vm->pc;
			vm->pc = vm->pc + offset - 1;
			break;
		}

		const char* s = bc_getstr(&vm->bc, offset);
		if(s == NULL)
			break;
		array_add_buf(&func->args, (void*)s, (uint32_t)strlen(s) + 1);
	}

	var_t* ret = var_new_func(vm, func);
	return ret;
}

static inline void math_op(vm_t* vm, opr_code_t op, var_t* v1, var_t* v2) {
	/*if(v1->value == NULL || v2->value == NULL) {
		vm_push(vm, var_new());
		return;
	}	
	*/

	//do int
	if(v1->type == V_INT && v2->type == V_INT) {
		int i1, i2, ret = 0;
		i1 = *(int*)v1->value;
		i2 = *(int*)v2->value;

		switch(op) {
			case INSTR_PLUS: 
			case INSTR_PLUSEQ: 
				ret = (i1 + i2);
				break; 
			case INSTR_MINUS: 
			case INSTR_MINUSEQ: 
				ret = (i1 - i2);
				break; 
			case INSTR_DIV: 
			case INSTR_DIVEQ: 
				ret = (i1 / i2);
				break; 
			case INSTR_MULTI: 
			case INSTR_MULTIEQ: 
				ret = (i1 * i2);
				break; 
			case INSTR_MOD: 
			case INSTR_MODEQ: 
				ret = i1 % i2;
				break; 
			case INSTR_RSHIFT: 
				ret = i1 >> i2;
				break; 
			case INSTR_LSHIFT: 
				ret = i1 << i2;
				break; 
			case INSTR_AND: 
				ret = i1 & i2;
				break; 
			case INSTR_OR: 
				ret = i1 | i2;
				break; 
		}

		var_t* v;
		if(op == INSTR_PLUSEQ || 
				op == INSTR_MINUSEQ ||
				op == INSTR_DIVEQ ||
				op == INSTR_MULTIEQ ||
				op == INSTR_MODEQ)  {
			v = v1;
			*(int*)v->value = ret;
		}
		else {
			v = var_new_int(vm, ret);
		}
		vm_push(vm, v);
		return;
	}

	//do float
	if(v1->type == V_FLOAT || v2->type == V_FLOAT) {
		float f1, f2, ret = 0.0;

		if(v1->type == V_FLOAT)
			f1 = *(float*)v1->value;
		else //INT
			f1 = (float) *(int*)v1->value;

		if(v2->type == V_FLOAT)
			f2 = *(float*)v2->value;
		else //INT
			f2 = (float) *(int*)v2->value;

		switch(op) {
			case INSTR_PLUS: 
			case INSTR_PLUSEQ: 
				ret = (f1 + f2);
				break; 
			case INSTR_MINUS: 
			case INSTR_MINUSEQ: 
				ret = (f1 - f2);
				break; 
			case INSTR_DIV: 
			case INSTR_DIVEQ: 
				ret = (f1 / f2);
				break; 
			case INSTR_MULTI: 
			case INSTR_MULTIEQ: 
				ret = (f1 * f2);
				break; 
		}

		var_t* v;
		if(op == INSTR_PLUSEQ || 
				op == INSTR_MINUSEQ ||
				op == INSTR_DIVEQ ||
				op == INSTR_MULTIEQ ||
				op == INSTR_MODEQ)  {
			v = v1;
			*(float*)v->value = ret;
		}
		else {
			v = var_new_float(vm, ret);
		}
		vm_push(vm, v);
		return;
	}

	//do string + 
	if(op == INSTR_PLUS || op == INSTR_PLUSEQ) {
		mstr_t* s = mstr_new((const char*)v1->value);
		mstr_t* json = mstr_new("");
		var_to_str(v2, json);
		mstr_append(s, json->cstr);
		mstr_free(json);

		var_t* v;
		if(op == INSTR_PLUSEQ) {
			v = v1;
			char* p = (char*)v->value;
			v->value = _malloc(s->len+1);
			memcpy(v->value, s->cstr, s->len+1);
			if(p != NULL)
				_free(p);
		}
		else {
			v = var_new_str(vm, s->cstr);
		}
		mstr_free(s);
		vm_push(vm, v);
	}
}

static inline void compare(vm_t* vm, opr_code_t op, var_t* v1, var_t* v2) {
    if(v1->type == V_OBJECT) {
        bool i = false;
        switch(op) {
        case INSTR_EQ:
        case INSTR_TEQ:
            i = (v1 == v2);
            break;
        case INSTR_NEQ:
        case INSTR_NTEQ:
            i = (v1 != v2);
            break;
        }
        if(i)
            vm_push(vm, vm->var_true);
        else
            vm_push(vm, vm->var_false);
        return;
    }
    
	//do int
	if(v1->type == V_INT && v2->type == V_INT) {
		register int i1, i2;
		i1 = *(int*)v1->value;
		i2 = *(int*)v2->value;

		bool i = false;
		switch(op) {
			case INSTR_EQ: 
			case INSTR_TEQ:
				i = (i1 == i2);
				break; 
			case INSTR_NEQ: 
			case INSTR_NTEQ:
				i = (i1 != i2);
				break; 
			case INSTR_LES: 
				i = (i1 < i2);
				break; 
			case INSTR_GRT: 
				i = (i1 > i2);
				break; 
			case INSTR_LEQ: 
				i = (i1 <= i2);
				break; 
			case INSTR_GEQ: 
				i = (i1 >= i2);
				break; 
		}
		if(i)
			vm_push(vm, vm->var_true);
		else
			vm_push(vm, vm->var_false);
		return;
	}

	
	register float f1, f2;
	if(v1->value == NULL)
		f1 = 0.0;
	else if(v1->type == V_FLOAT)
		f1 = *(float*)v1->value;
	else //INT
		f1 = (float) *(int*)v1->value;

	if(v2->value == NULL)
		f2 = 0.0;
	else if(v2->type == V_FLOAT)
		f2 = *(float*)v2->value;
	else //INT
		f2 = (float) *(int*)v2->value;

	bool i = false;
	if(v1->type == v2->type || 
			((v1->type == V_INT || v1->type == V_FLOAT) &&
			(v2->type == V_INT || v2->type == V_FLOAT))) {
		if(v1->type == V_STRING) {
			switch(op) {
				case INSTR_EQ: 
				case INSTR_TEQ:
					i = (strcmp((const char*)v1->value, (const char*)v2->value) == 0);
					break; 
				case INSTR_NEQ: 
				case INSTR_NTEQ:
					i = (strcmp((const char*)v1->value, (const char*)v2->value) != 0);
					break;
			}
		}
		else if(v1->type == V_NULL) {
			switch(op) {
				case INSTR_EQ: 
				case INSTR_TEQ:
					i = (v2->type == V_NULL);
					break; 
				case INSTR_NEQ: 
				case INSTR_NTEQ:
					i = (v2->type != V_NULL);
					break;
			}
		}
		else if(v1->type == V_INT || v1->type == V_FLOAT) {
			switch(op) {
				case INSTR_EQ: 
				case INSTR_TEQ:
					i = (f1 == f2);
					break; 
				case INSTR_NEQ: 
				case INSTR_NTEQ:
					i = (f1 != f2);
					break; 
				case INSTR_LES: 
					i = (f1 < f2);
					break; 
				case INSTR_GRT: 
					i = (f1 > f2);
					break; 
				case INSTR_LEQ: 
					i = (f1 <= f2);
					break; 
				case INSTR_GEQ: 
					i = (f1 >= f2);
					break; 
			}
		}
	}
	else if(op == INSTR_NEQ || op == INSTR_NTEQ) {
		i = true;
	}

	if(i)
		vm_push(vm, vm->var_true);
	else
		vm_push(vm, vm->var_false);
}

void do_get(vm_t* vm, var_t* v, const char* name) {
	if(v->type == V_STRING && strcmp(name, "length") == 0) {
		int len = (int)strlen(var_get_str(v));
		vm_push(vm, var_new_int(vm, len));
		return;
	}
	else if(v->is_array && strcmp(name, "length") == 0) {
		int len = var_array_size(v);
		vm_push(vm, var_new_int(vm, len));
		return;
	}	

	node_t* n = find_member(v, name);
	if(n != NULL) {
		/*if(n->var->type == V_FUNC) {
			func_t* func = var_get_func(n->var);
			if(!func->regular) { //class get/set function.
				func_call(vm, v, funC);
				return;
			}
		}
		*/
	}
	else {
		if(v->type == V_UNDEF)
			v->type = V_OBJECT;

		if(v->type == V_OBJECT) {
			n = var_add(v, name, NULL);
		}
		else {
			mario_debug("[debug] Can not get member '");
			mario_debug(name);
			mario_debug("'!\n");
			n = node_new(vm, name, NULL);
			vm_push(vm, var_new(vm));
			return;
		}
	}

	vm_push_node(vm, n);
}

static void do_extends(vm_t* vm, var_t* cls_var, const char* super_name) {
	node_t* n = vm_find_in_scopes(vm, super_name);
	if(n == NULL) {
		mario_debug("[debug] Super Class '");
		mario_debug(super_name);
		mario_debug("' not found!\n");
		return;
	}

	var_set_father(cls_var, n->var);
}

/** create object by classname or function */
var_t* new_obj(vm_t* vm, const char* name, int arg_num) {
	var_t* obj = NULL;
	node_t* n = vm_load_node(vm, name, false); //load class;

	if(n == NULL || n->var->type != V_OBJECT) {
		mario_debug("[debug] Error: There is no class: '");
		mario_debug(name);
		mario_debug("'!\n");
		return NULL;
	}

	obj = var_new_obj(vm, NULL, NULL);
	var_instance_from(obj, n->var);
	var_t* protoV = var_get_prototype(obj);
	var_t* constructor = NULL;

	if(n->var->is_func) { // new object built by function call
		constructor = n->var;
	}
	else {
		constructor = var_find_var(protoV, CONSTRUCTOR);
	}

	if(constructor != NULL) {
		func_call(vm, obj, constructor, arg_num);
		obj = vm_pop2(vm);
		obj->refs--;
	}
	return obj;
}

static int parse_func_name(const char* full, mstr_t* name) {
	const char* pos = strchr(full, '$');
	int args_num = 0;
	if(pos != NULL) {
		args_num = atoi(pos+1);
		if(name != NULL)
			mstr_ncpy(name, full, (uint32_t)(pos-full));	
	}
	else {
		if(name != NULL)
			mstr_cpy(name, full);
	}
	return args_num;
}

/** create object and try constructor */
static bool do_new(vm_t* vm, const char* full) {
	mstr_t* name = mstr_new("");
	int arg_num = parse_func_name(full, name);

	var_t* obj = new_obj(vm, name->cstr, arg_num);
	mstr_free(name);

	if(obj == NULL)
		return false;
	vm_push(vm, obj);
	return true;
}

var_t* call_m_func(vm_t* vm, var_t* obj, var_t* func, var_t* args) {
	//push args to stack.
	int arg_num = 0;
	if(args != NULL) {
		arg_num = args->children.size;
		int i;
		for(i=0; i<arg_num; i++) {
			var_t* v = ((node_t*)args->children.items[i])->var;
			vm_push(vm, v);
		}
	}

	while(vm->is_doing_gc);
	func_call(vm, obj, func, arg_num);
	return vm_pop2(vm);
}

var_t* call_m_func_by_name(vm_t* vm, var_t* obj, const char* func_name, var_t* args) {
	node_t* func = find_member(obj, func_name);
	if(func == NULL || func->var->is_func == 0) {
		mario_debug("[debug] Interrupt function '");
		mario_debug(func_name);
		mario_debug("' not defined!\n");
		return NULL;
	}
	return call_m_func(vm, obj, func->var, args);
}

#ifdef MARIO_THREAD
/**
interrupter
*/

#define MAX_ISIGNAL 128

static bool interrupt_raw(vm_t* vm, var_t* obj, const char* func_name, var_t* func, const char* msg) {
	while(vm->interrupted) { } // can not interrupt another interrupter.

	pthread_mutex_lock(&vm->thread_lock);
	if(vm->isignal_num >= MAX_ISIGNAL) {
		mario_debug("[debug] Too many interrupt signals!\n");
		pthread_mutex_unlock(&vm->thread_lock);
		return false;
	}

	isignal_t* is = (isignal_t*)_malloc(sizeof(isignal_t));
	if(is == NULL) {
		mario_debug("[debug] Interrupt signal input error!\n");
		pthread_mutex_unlock(&vm->thread_lock);
		return false;
	}

	is->next = NULL;
	is->prev = NULL;
	//is->obj = var_ref(obj);
	is->obj = obj;

	if(func != NULL)
		is->handle_func = var_ref(func);
	else
		is->handle_func = NULL;

	if(func_name != NULL && func_name[0] != 0) 
		is->handle_func_name = mstr_new(func_name);
	else
		is->handle_func_name = NULL;

	if(msg != NULL)
		is->msg = mstr_new(msg);
	else
		is->msg = NULL;

	if(vm->isignal_tail == NULL) {
		vm->isignal_head = is;
		vm->isignal_tail = is;
	}
	else {
		vm->isignal_tail->next = is;
		is->prev = vm->isignal_tail;
		vm->isignal_tail = is;
	}

	vm->isignal_num++;
	pthread_mutex_unlock(&vm->thread_lock);
	return true;
}

bool interrupt_by_name(vm_t* vm, var_t* obj, const char* func_name, const char* msg) {
	return interrupt_raw(vm, obj, func_name, NULL, msg);
}

bool interrupt(vm_t* vm, var_t* obj, var_t* func, const char* msg) {
	return interrupt_raw(vm, obj, NULL, func, msg);
}

void try_interrupter(vm_t* vm) {
	if(vm->isignal_head == NULL || vm->interrupted) {
		return;
	}

	pthread_mutex_lock(&vm->thread_lock);
	vm->interrupted = true;

	isignal_t* sig = vm->isignal_head;
		
	var_t* func = NULL;

	while(sig != NULL) {
		if(sig->handle_func != NULL) {
			func = sig->handle_func;
			break;
		}
		else if(sig->handle_func_name != NULL) {
			//if func undefined yet, keep it and try next sig
			node_t* func_node = find_member(sig->obj, sig->handle_func_name->cstr);
			if(func_node != NULL && func_node->var->is_func) { 
				func = var_ref(func_node->var);
				break;
			}
		}
		sig = sig->next;
	}

	if(func == NULL || sig == NULL) {
		vm->interrupted = false;
		pthread_mutex_unlock(&vm->thread_lock);
		return;
	}
	
	var_t* args = var_new(vm);
	if(sig->msg != NULL) {
		if(sig->msg->cstr == NULL)
			var_add(args, "", var_new_str(vm, ""));
		else
			var_add(args, "", var_new_str(vm, sig->msg->cstr));
		mstr_free(sig->msg);
	}

	vm_push(vm, args);
	var_t* ret = call_m_func(vm, sig->obj, func, args);
	vm_pop(vm);
	//var_unref(args);

	if(ret != NULL)
		var_unref(ret);

	//pop this sig from queue.
	if(sig->prev == NULL) 
		vm->isignal_head = sig->next;
	else 
		sig->prev->next = sig->next;

	if(sig->next == NULL)
		vm->isignal_tail = sig->prev;
	else
		sig->next->prev = sig->prev;

	//var_unref(sig->obj);
	var_unref(func);
	if(sig->handle_func_name != NULL)
		mstr_free(sig->handle_func_name);
	_free(sig);
	vm->isignal_num--;
	vm->interrupted = false;
	pthread_mutex_unlock(&vm->thread_lock);
}

#else

bool interrupt(vm_t* vm, var_t* obj, var_t* func, const char* msg) {
	return false;
}	

bool interrupt_by_name(vm_t* vm, var_t* obj, const char* func_name, const char* msg) {
	return false;
}	

#endif

/*****************/

var_t* vm_new_class(vm_t* vm, const char* cls) {
	var_t* cls_var = vm_load_var(vm, cls, true);
	cls_var->type = V_OBJECT;
	if(var_get_prototype(cls_var) == NULL)
		var_set_prototype(cls_var, var_new_obj(vm, NULL, NULL));
	return cls_var;
}

void vm_terminate(vm_t* vm) {
	while(true) { //clear stack
		if(!vm_pop(vm))
			break;
	}

	while(true) { //clear scopes
		scope_t* sc = vm_get_scope(vm);
		if(sc == NULL) break;
		vm_pop_scope(vm);
	}
	vm->pc = vm->bc.cindex;
}

static void do_include(vm_t* vm, const char* jsname) {
	if(_load_m_func == NULL)
		return;
	//check if included or not.
	int i;
	for(i=0; i<vm->included.size; i++) {
		mstr_t* jsn = (mstr_t*)array_get(&vm->included, i);
		if(strcmp(jsn->cstr, jsname) == 0)
			return;
	}

	mstr_t* js = _load_m_func(vm, jsname);
	if(js == NULL) {
		mario_debug("[debug] Error: include file '");
		mario_debug(jsname);
		mario_debug("' not found!\n");
		return;
	}
	array_add(&vm->included, mstr_new(jsname));

	PC pc = vm->pc;
	vm_load_run(vm, js->cstr);
	mstr_free(js);
	vm->pc = pc;
}

bool vm_run(vm_t* vm) {
	//int32_t scDeep = vm->scopes.size;
	register PC code_size = vm->bc.cindex;
	register PC* code = vm->bc.code_buf;

	do {
		register PC ins = code[vm->pc++];
		register opr_code_t instr = OP(ins);
		register uint32_t offset = OFF(ins);

		if(instr == INSTR_END)
			break;
		
		switch(instr) {
			case INSTR_JMP: 
			{
				vm->pc = vm->pc + offset - 1;
				break;
			}
			case INSTR_JMPB: 
			{
				vm->pc = vm->pc - offset - 1;
				break;
			}
			case INSTR_NJMP: 
			case INSTR_NJMPB: 
			{
				var_t* v = vm_pop2(vm);
				if(v->type == V_UNDEF ||
						v->value == NULL ||
						*(int*)(v->value) == 0) {
					if(instr == INSTR_NJMP) 
						vm->pc = vm->pc + offset - 1;
					else
						vm->pc = vm->pc - offset - 1;
				}
				var_unref(v);
				break;
			}
			case INSTR_LOAD: 
			case INSTR_LOADO: 
			{
				bool loaded = false;
				if(offset == vm->this_strIndex) {
					var_t* thisV = vm_this_in_scopes(vm);
					if(thisV != NULL) {
						vm_push(vm, thisV);	
						loaded = true;	
					}
				}
				if(!loaded) {
					const char* s = bc_getstr(&vm->bc, offset);
					node_t* n = NULL;
					if(instr == INSTR_LOAD) {
						n = vm_load_node(vm, s, true); //load variable, create if not exist.
						vm_push_node(vm, n);
					}
					else { // LOAD obj
						n = vm_load_node(vm, s, true); //load variable, warning if not exist.
						vm_push_node(vm, n);
						if(n->var->type != V_OBJECT) {
							mario_debug("[debug] Warning: object or class '");
							mario_debug(s);
							mario_debug("' undefined, object created.\n");
						}
					}
				}
				break;
			}
			case INSTR_LES: 
			case INSTR_EQ: 
			case INSTR_NEQ: 
			case INSTR_TEQ:
			case INSTR_NTEQ:
			case INSTR_GRT: 
			case INSTR_LEQ: 
			case INSTR_GEQ: 
			{
				var_t* v2 = vm_pop2(vm);
				var_t* v1 = vm_pop2(vm);
				compare(vm, instr, v1, v2);
				var_unref(v1);
				var_unref(v2);
				break;
			}
			case INSTR_NIL: 
			{	
				break; 
			}
			case INSTR_BLOCK: 
			case INSTR_LOOP: 
			case INSTR_TRY: 
			{
				scope_t* sc = NULL;
				sc = scope_new(var_new_block(vm));
				sc->is_block = true;
				if(instr == INSTR_LOOP) {
					sc->is_loop = true;
					sc->pc_start = vm->pc+1;
					sc->pc = vm->pc+2;
				}
				else if(instr == INSTR_TRY) {
					sc->is_try = true;
					sc->pc = vm->pc+1;
				}
				vm_push_scope(vm, sc);
				break;
			}
			case INSTR_BLOCK_END: 
			case INSTR_LOOP_END: 
			case INSTR_TRY_END: 
			{
				vm_pop_scope(vm);
				break;
			}
			case INSTR_BREAK: 
			{
				while(true) {
					scope_t* sc = vm_get_scope(vm);
					if(sc == NULL) {
						mario_debug("[debug] Error: 'break' not in any loop!\n");
						vm_terminate(vm);
						break;
					}
					if(sc->is_loop) {
						vm->pc = sc->pc;
						break;
					}
					vm_pop_scope(vm);
				}
				break;
			}
			case INSTR_CONTINUE:
			{
				while(true) {
					scope_t* sc = vm_get_scope(vm);
					if(sc == NULL) {
						mario_debug("[debug] Error: 'continue' not in any loop!\n");
						vm_terminate(vm);
						break;
					}
					if(sc->is_loop) {
						vm->pc = sc->pc_start;
						break;
					}
					vm_pop_scope(vm);
				}
				break;
			}
			#ifdef MARIO_CACHE
			case INSTR_CACHE: 
			{	
				var_t* v = vm->var_cache[offset];
				vm_push(vm, v);
				break;
			}
			#endif
			case INSTR_TRUE: 
			{
				vm_push(vm, vm->var_true);
				break;
			}
			case INSTR_FALSE: 
			{
				vm_push(vm, vm->var_false);
				break;
			}
			case INSTR_NULL: 
			{
				vm_push(vm, vm->var_null);
				break;
			}
			case INSTR_UNDEF: 
			{
				var_t* v = var_new(vm);	
				vm_push(vm, v);
				break;
			}
			case INSTR_POP: 
			{
				vm_pop(vm);
				break;
			}
			case INSTR_NEG: 
			{
				var_t* v = vm_pop2(vm);
				if(v->type == V_INT) {
					int n = *(int*)v->value;
					n = -n;
					vm_push(vm, var_new_int(vm, n));
				}
				else if(v->type == V_FLOAT) {
					float n = *(float*)v->value;
					n = -n;
					vm_push(vm, var_new_float(vm, n));
				}
				var_unref(v);
				break;
			}
			case INSTR_NOT: 
			{
				var_t* v = vm_pop2(vm);
				bool i = false;
				if(v->type == V_UNDEF || *(int*)v->value == 0)
					i = true;
				var_unref(v);
				vm_push(vm, i ? vm->var_true:vm->var_false);
				break;
			}
			case INSTR_AAND: 
			case INSTR_OOR: 
			{
				var_t* v2 = vm_pop2(vm);
				var_t* v1 = vm_pop2(vm);
				bool r = false;
				int i1 = *(int*)v1->value;
				int i2 = *(int*)v2->value;

				if(instr == INSTR_AAND)
					r = (i1 != 0) && (i2 != 0);
				else
					r = (i1 != 0) || (i2 != 0);
				vm_push(vm, r ? vm->var_true:vm->var_false);

				var_unref(v1);
				var_unref(v2);
				break;
			}
			case INSTR_PLUS: 
			case INSTR_RSHIFT: 
			case INSTR_LSHIFT: 
			case INSTR_AND: 
			case INSTR_OR: 
			case INSTR_PLUSEQ: 
			case INSTR_MULTIEQ: 
			case INSTR_DIVEQ: 
			case INSTR_MODEQ: 
			case INSTR_MINUS: 
			case INSTR_MINUSEQ: 
			case INSTR_DIV: 
			case INSTR_MULTI: 
			case INSTR_MOD:
			{
				var_t* v2 = vm_pop2(vm);
				var_t* v1 = vm_pop2(vm);
				math_op(vm, instr, v1, v2);
				var_unref(v1);
				var_unref(v2);
				break;
			}
			case INSTR_MMINUS_PRE: 
			{
				var_t* v = vm_pop2(vm);
				int *i = (int*)v->value;
				if(i != NULL) {
					(*i)--;
					if((ins & INSTR_OPT_CACHE) == 0) {
						if(OP(code[vm->pc]) != INSTR_POP) { 
							vm_push(vm, v);
						}
						else { 
							code[vm->pc] = INSTR_NIL;
							code[vm->pc-1] |= INSTR_OPT_CACHE; 
						}
					}
					else { //skip the nil if cached
						vm->pc++;
					}
				}
				else {
					vm_push(vm, v);
				}
				var_unref(v);
				break;
			}
			case INSTR_MMINUS: 
			{
				var_t* v = vm_pop2(vm);
				int *i = (int*)v->value;
				if(i != NULL) {
					if((ins & INSTR_OPT_CACHE) == 0) {
						var_t* v2 = var_new_int(vm, *i);
						if(OP(code[vm->pc]) != INSTR_POP) {
							vm_push(vm, v2);
						}
						else { 
							code[vm->pc] = INSTR_NIL; 
							code[vm->pc-1] |= INSTR_OPT_CACHE;
							var_unref(v2);
						}
					}
					else { //skip the nil if cached
						vm->pc++;
					}
					(*i)--;
				}
				else {
					vm_push(vm, v);
				}
				var_unref(v);
				break;
			}
			case INSTR_PPLUS_PRE: 
			{
				var_t* v = vm_pop2(vm);
				int *i = (int*)v->value;
				if(i != NULL) {
					(*i)++;
					if((ins & INSTR_OPT_CACHE) == 0) {
						if(OP(code[vm->pc]) != INSTR_POP) { 
							vm_push(vm, v);
						}
						else { 
							code[vm->pc] = INSTR_NIL; 
							code[vm->pc-1] |= INSTR_OPT_CACHE; 
						}
					}
					else { //skip the nil if cached
						vm->pc++;
					}
				}
				else {
					vm_push(vm, v);
				}
				var_unref(v);
				break;
			}
			case INSTR_PPLUS: 
			{
				var_t* v = vm_pop2(vm);
				int *i = (int*)v->value;
				if(i != NULL) {
					if((ins & INSTR_OPT_CACHE) == 0) {
						var_t* v2 = var_new_int(vm, *i);
						if(OP(code[vm->pc]) != INSTR_POP) {
							vm_push(vm, v2);
						}
						else { 
							code[vm->pc] = INSTR_NIL;
							code[vm->pc-1] |= INSTR_OPT_CACHE; 
							var_unref(v2);
						}
					}
					else { //skip the nil if cached
						vm->pc++;
					}
					(*i)++;
				}
				else {
					vm_push(vm, v);
				}
				var_unref(v);
				break;
			}
			case INSTR_RETURN:  //return without value
			case INSTR_RETURNV: 
			{ //return with value
				if(instr == INSTR_RETURN) {//return without value, push "this" to stack
					var_t* thisV = vm_this_in_scopes(vm);
					if(thisV != NULL)
						vm_push(vm, thisV);
					else
						vm_push(vm, var_new(vm));
				}
				else { //return with value;
					var_t* ret = vm_pop2(vm); // if node in stack, just restore the var value.
					vm_push(vm, ret);
					var_unref(ret);
				}

				while(true) {
					scope_t* sc = vm_get_scope(vm);
					if(sc == NULL) break;
					if(sc->is_func) {
						vm->pc = sc->pc;
						vm_pop_scope(vm);
						break;
					}
					vm_pop_scope(vm);
				}
				return true;
			}
			case INSTR_VAR:
			{
				const char* s = bc_getstr(&vm->bc, offset);
				node_t *node = vm_find(vm, s);
				if(node != NULL) { //find just in current scope
					mario_debug("[debug] Warning: '");
					mario_debug(s);
					mario_debug("' has already existed!\n");
				}
				else {
					var_t* v = vm_get_scope_var(vm);
					if(v != NULL) {
						node = var_add(v, s, NULL);
					}
				}
				break;
			}
			case INSTR_LET:
			case INSTR_CONST: 
			{
				const char* s = bc_getstr(&vm->bc, offset);
				var_t* v = vm_get_scope_var(vm);
				node_t *node = var_find(v, s);
				if(node != NULL) { //find just in current scope
					mario_debug("[debug] Error: let '");
					mario_debug(s);
					mario_debug("' has already existed!\n");
					vm_terminate(vm);
				}
				else {
					node = var_add(v, s, NULL);
					if(node != NULL && instr == INSTR_CONST)
						node->be_const = true;
				}
				break;
			}
			case INSTR_INT:
			{
				var_t* v = var_new_int(vm, (int)code[vm->pc++]);
				/*#ifdef MARIO_CACHE
				if(try_cache(vm, &code[vm->pc-2], v))
					code[vm->pc-1] = INSTR_NIL;
				#endif
				*/
				vm_push(vm, v);
				break;
			}
			case INSTR_INT_S:
			{
				var_t* v = var_new_int(vm, offset);
				/*#ifdef MARIO_CACHE
				try_cache(vm, &code[vm->pc-1], v);
				#endif
				*/
				vm_push(vm, v);
				break;
			}
			case INSTR_FLOAT: 
			{
				var_t* v = var_new_float(vm, *(float*)(&code[vm->pc++]));
				/*#ifdef MARIO_CACHE
				if(try_cache(vm, &code[vm->pc-2], v))
					code[vm->pc-1] = INSTR_NIL;
				#endif
				*/
				vm_push(vm, v);
				break;
			}
			case INSTR_STR: 
			{
				const char* s = bc_getstr(&vm->bc, offset);
				var_t* v = var_new_str(vm, s);
				/*
				#ifdef MARIO_CACHE	
				try_cache(vm, &code[vm->pc-1], v);
				#endif
				*/
				vm_push(vm, v);
				break;
			}
			case INSTR_ASIGN: 
			{
				var_t* v = vm_pop2(vm);
				node_t* n = vm_pop2node(vm);
				bool modi = (!n->be_const || n->var->type == V_UNDEF);
				var_unref(n->var);
				if(modi) 
					node_replace(n, v);
				else {
					mario_debug("[debug] Can not change a const variable: '");
					mario_debug(n->name);
					mario_debug("'!\n");
				}

				if((ins & INSTR_OPT_CACHE) == 0) {
					if(OP(code[vm->pc]) != INSTR_POP) {
						vm_push(vm, n->var);
					}
					else { 
						code[vm->pc] = INSTR_NIL;
						code[vm->pc-1] |= INSTR_OPT_CACHE;
					}
				}
				else { //skip the nil if cached
					vm->pc++;
				}
				var_unref(v);
				break;
			}
			case INSTR_GET: 
			{
				const char* s = bc_getstr(&vm->bc, offset);
				var_t* v = vm_pop2(vm);
				var_build_basic_prototype(vm, v);
				do_get(vm, v, s);
				var_unref(v);
				break;
			}
			case INSTR_NEW: 
			{
				const char* s = bc_getstr(&vm->bc, offset);
				if(!do_new(vm, s)) 
					vm_terminate(vm);
				break;
			}
			case INSTR_CALL: 
			case INSTR_CALLO: 
			{
				var_t* func = NULL;
				var_t* obj = NULL;
				bool unrefObj = false;
				const char* s = bc_getstr(&vm->bc, offset);
				mstr_t* name = mstr_new("");
				int arg_num = parse_func_name(s, name);
				var_t* sc_var = vm_get_scope_var(vm);
				
				if(instr == INSTR_CALLO) {
					obj = vm_stack_pick(vm, arg_num+1);
					var_build_basic_prototype(vm, obj);
					unrefObj = true;
				}
				else {
					obj = vm_this_in_scopes(vm);
					func = find_func(vm, sc_var, name->cstr);
				}

				if(func == NULL && obj != NULL)
					func = find_func(vm, obj, name->cstr);

				if(func != NULL && !func->is_func) { //constructor like super()
					var_t* constr = var_find_var(func, CONSTRUCTOR);	
					if(constr == NULL) {
						var_t* protoV = var_get_prototype(func);
						if(protoV != NULL) 
							func = var_find_var(protoV, CONSTRUCTOR);	
						else
							func = NULL;
					}
					else {
						func = constr;
					}
				}

				if(func != NULL) {
					func_call(vm, obj, func, arg_num);
				}
				else {
					while(arg_num > 0) {
						vm_pop(vm);
						arg_num--;
					}
					vm_push(vm, var_new(vm));
					mario_debug("[debug] Error: can not find function '");
					mario_debug(s);
					mario_debug("'!\n");
				}
				mstr_free(name);

				if(unrefObj && obj != NULL)
					var_unref(obj);

				//check and do interrupter.
				#ifdef MARIO_THREAD
				try_interrupter(vm);
				#endif
				break;
			}
			case INSTR_MEMBER: 
			case INSTR_MEMBERN: 
			{
				const char* s = (instr == INSTR_MEMBER ? "" :  bc_getstr(&vm->bc, offset));
				var_t* v = vm_pop2(vm);
				if(v == NULL) 
					v = var_new(vm);
				
				var_t *var = vm_get_scope_var(vm);
				if(var->is_array) {
					var_array_add(var, v);
				}
				else {
					if(v->is_func) {
						func_t* func = (func_t*)v->value;
						func->owner = var;
					}
					var_add(var, s, v);
				}
				var_unref(v);
				break;
			}
			case INSTR_FUNC: 
			case INSTR_FUNC_STC: 
			case INSTR_FUNC_GET: 
			case INSTR_FUNC_SET: 
			{
				var_t* v = func_def(vm, 
						(instr == INSTR_FUNC ? true:false),
						(instr == INSTR_FUNC_STC ? true:false));
				if(v != NULL) {
					func_mark_closure(vm, v);
					vm_push(vm, v);
				}
				break;
			}
			case INSTR_OBJ:
			case INSTR_ARRAY: 
			{
				var_t* obj;
				if(instr == INSTR_OBJ) {
					obj = var_new_obj(vm, NULL, NULL);
					var_set_prototype(obj, var_get_prototype(vm->var_Object));
				}
				else
					obj = var_new_array(vm);
				scope_t* sc = scope_new(obj);
				vm_push_scope(vm, sc);
				break;
			}
			case INSTR_ARRAY_END: 
			case INSTR_OBJ_END: 
			{
				var_t* obj = vm_get_scope_var(vm);
				vm_push(vm, obj); //that actually means currentObj->ref() for push and unref for unasign.
				vm_pop_scope(vm);
				break;
			}
			case INSTR_ARRAY_AT: 
			{
				var_t* v2 = vm_pop2(vm);
				var_t* v1 = vm_pop2(vm);
				node_t* n = NULL;
				if(v2->type == V_STRING) {
					const char* s = var_get_str(v2);
					n = var_find(v1, s);
				}
				else {
					int at = var_get_int(v2);
					n = var_array_get(v1, at);
				}
				if(n != NULL) 
					vm_push_node(vm, n);
				else
					vm_push(vm, var_new(vm));
				var_unref(v1);
				var_unref(v2);
				break;
			}
			case INSTR_CLASS: 
			{
				const char* s =  bc_getstr(&vm->bc, offset);
				var_t* cls_var = vm_new_class(vm, s);
				//read extends
				ins = code[vm->pc];
				instr = OP(ins);
				if(instr == INSTR_EXTENDS) {
					vm->pc++;
					offset = OFF(ins);
					s =  bc_getstr(&vm->bc, offset);
					do_extends(vm, cls_var, s);
				}

				var_t* protoV = var_get_prototype(cls_var);
				scope_t* sc = scope_new(protoV);
				vm_push_scope(vm, sc);
				break;
			}
			case INSTR_CLASS_END: 
			{
				var_t* var = vm_get_scope_var(vm);
				vm_push(vm, var);
				vm_pop_scope(vm);
				break;
			}
			case INSTR_INSTOF: 
			{
				var_t* v2 = vm_pop2(vm);
				var_t* v1 = vm_pop2(vm);
				bool res = var_instanceof(v1, v2);
				var_unref(v2);
				var_unref(v1);
				vm_push(vm, var_new_bool(vm, res));
				break;
			}
			case INSTR_TYPEOF: 
			{
				var_t* var = vm_pop2(vm);
				var_t* v = var_new_str(vm, get_typeof(var));
				vm_push(vm, v);
				break;
			}
			case INSTR_INCLUDE: 
			{
				var_t* v = vm_pop2(vm);
				do_include(vm, var_get_str(v));
				var_unref(v);
				break;
			}
			case INSTR_THROW: 
			{
				while(true) {
					scope_t* sc = vm_get_scope(vm);
					if(sc == NULL) {
						mario_debug("[debug] Error: 'throw' not in any try...catch!\n");
						vm_terminate(vm);
						break;
					}
					if(sc->is_try) {
						vm->pc = sc->pc;
						break;
					}
					vm_pop_scope(vm);
				}
				break;
			}
			case INSTR_CATCH: 
			{
				const char* s = bc_getstr(&vm->bc, offset);
				var_t* v = vm_pop2(vm);
				var_t* sc_var = vm_get_scope_var(vm);
				var_add(sc_var, s, v);
				var_unref(v);
				break;
			}
		}
	}
	while(vm->pc < code_size && !vm->terminated);
	return false;
}

bool vm_load(vm_t* vm, const char* s) {
	if(vm->compiler == NULL)
		return false;

	if(vm->bc.cindex > 0) {
		//vm->bc.cindex--;
		vm->pc = vm->bc.cindex;
	}
	return vm->compiler(&vm->bc, s);
}

bool vm_load_run(vm_t* vm, const char* s) {
	bool ret = false;
	if(vm_load(vm, s)) {
		vm_run(vm);
		ret = true;
	}
	return ret;
}

bool vm_load_run_native(vm_t* vm, const char* s) {
	bool ret = false;
	PC old = vm->pc;

	if(vm_load(vm, s)) {
		vm_run(vm);
		ret = true;
	}

	vm->pc = old;
	return ret;
}

typedef struct st_native_init {
	void (*func)(void*);
	void *data;
} native_init_t;

void vm_close(vm_t* vm) {
	vm->terminated = true;
	if(vm->on_close != NULL)
		vm->on_close(vm);

	int i;
	for(i=0; i<vm->close_natives.size; i++) {
		native_init_t* it = (native_init_t*)array_get(&vm->close_natives, i);
		it->func(it->data);
	}
	array_clean(&vm->close_natives, NULL);
	var_unref(vm->var_true);
	var_unref(vm->var_false);
	var_unref(vm->var_null);


	#ifdef MARIO_THREAD
	pthread_mutex_destroy(&vm->thread_lock);
	#endif

	#ifdef MARIO_CACHE
	var_cache_free(vm);
	#endif

	array_free(vm->scopes, scope_free);
	vm->scopes = NULL;
	array_clean(&vm->init_natives, NULL);

	array_clean(&vm->included, (free_func_t)mstr_free);	

	var_unref(vm->root);
	bc_release(&vm->bc);
	vm->stack_top = 0;

	gc(vm, true);
	_free(vm);
}	

/**======native extends functions======*/

void vm_reg_init(vm_t* vm, void (*func)(void*), void* data) {
	native_init_t* it = (native_init_t*)_malloc(sizeof(native_init_t));
	it->func = func;
	it->data = data;
	array_add(&vm->init_natives, it);
}

void vm_reg_close(vm_t* vm, void (*func)(void*), void* data) {
	native_init_t* it = (native_init_t*)_malloc(sizeof(native_init_t));
	it->func = func;
	it->data = data;
	array_add(&vm->close_natives, it);
}

node_t* vm_reg_var(vm_t* vm, var_t* cls, const char* name, var_t* var, bool be_const) {
	var_t* cls_var = vm->root;
	if(cls != NULL) {
		cls_var = var_get_prototype(cls);
	}

	node_t* node = var_add(cls_var, name, var);
	node->be_const = be_const;
	return node;
}

node_t* vm_reg_native(vm_t* vm, var_t* cls, const char* decl, native_func_t native, void* data) {
	var_t* cls_var = vm->root;
	if(cls != NULL) {
		cls_var = var_get_prototype(cls);
	}

	mstr_t* name = mstr_new("");
	mstr_t* arg = mstr_new("");

	func_t* func = func_new();
	func->native = native;
	func->data = data;

	const char *off = decl;
	//read name
	while(*off != '(') { 
		if(*off != ' ') //skip spaces
			mstr_add(name, *off);
		off++; 
	}
	off++; 

	while(*off != 0) {
		if(*off == ',' || *off == ')') {
			if(arg->len > 0)
				array_add_buf(&func->args, arg->cstr, arg->len+1);
			mstr_reset(arg);
		}
		else if(*off != ' ') //skip spaces
			mstr_add(arg, *off);

		off++; 
	} 
	mstr_free(arg);

	var_t* var = var_new_func(vm, func);
	node_t* node = var_add(cls_var, name->cstr, var);
	mstr_free(name);

	return node;
}

node_t* vm_reg_static(vm_t* vm, var_t* cls, const char* decl, native_func_t native, void* data) {
	node_t* n = vm_reg_native(vm, cls, decl, native, data);
	func_t* func = var_get_func(n->var);
	func->is_static = true;
	return n;
}

const char* get_str(var_t* var, const char* name) {
	var_t* v = get_obj(var, name);
	return v == NULL ? "" : var_get_str(v);
}

int get_int(var_t* var, const char* name) {
	var_t* v = get_obj(var, name);
	return v == NULL ? 0 : var_get_int(v);
}

bool get_bool(var_t* var, const char* name) {
	var_t* v = get_obj(var, name);
	return v == NULL ? 0 : var_get_bool(v);
}

float get_float(var_t* var, const char* name) {
	var_t* v = get_obj(var, name);
	return v == NULL ? 0 : var_get_float(v);
}

var_t* get_obj(var_t* var, const char* name) {
	//if(strcmp(name, THIS) == 0)
	//	return var;
	node_t* n = var_find(var, name);
	if(n == NULL)
		return NULL;
	return n->var;
}

void* get_raw(var_t* var, const char* name) {
	var_t* v = get_obj(var, name);
	if(v == NULL)
		return NULL;
	return v->value;
}

var_t* get_obj_member(var_t* env, const char* name) {
	var_t* obj = get_obj(env, THIS);
	if(obj == NULL)
		return NULL;
	return var_find_var(obj, name);
}

var_t* set_obj_member(var_t* env, const char* name, var_t* var) {
	var_t* obj = get_obj(env, THIS);
	if(obj == NULL)
		return NULL;
	var_add(obj, name, var);
	return var;
}

var_t* get_func_args(var_t* env) {
	return get_obj(env, "arguments");
}

uint32_t get_func_args_num(var_t* env) {
	var_t* args = get_func_args(env);
	return var_array_size(args);
}

var_t* get_func_arg(var_t* env, uint32_t index) {
	var_t* args = get_func_args(env);
	return var_array_get_var(args, index);
}

int get_func_arg_int(var_t* env, uint32_t index) {
	return var_get_int(get_func_arg(env, index));
}

bool get_func_arg_bool(var_t* env, uint32_t index) {
	return var_get_bool(get_func_arg(env, index));
}

float get_func_arg_float(var_t* env, uint32_t index) {
	return var_get_float(get_func_arg(env, index));
}

const char* get_func_arg_str(var_t* env, uint32_t index) {
	return var_get_str(get_func_arg(env, index));
}

vm_t* vm_from(vm_t* vm) {
	vm_t* ret = vm_new(vm->compiler);
	ret->gc_buffer_size = vm->gc_buffer_size;
	ret->gc_free_buffer_size = vm->gc_free_buffer_size;
  vm_init(ret, vm->on_init, vm->on_close);
	return ret;
}

static var_t* native_debug(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	var_t* args = get_func_args(env); 
	mstr_t* ret = mstr_new("");
	mstr_t* str = mstr_new("");
	uint32_t sz = var_array_size(args);
	uint32_t i;
	for(i=0; i<sz; ++i) {
		node_t* n = var_array_get(args, i);
		if(n != NULL) {
			var_to_str(n->var, str);
			if(i > 0)
				mstr_add(ret, ' ');
			mstr_append(ret, str->cstr);
		}
	}
	mstr_free(str);
	mstr_add(ret, '\n');
	dout(ret->cstr);
	mstr_free(ret);
	return NULL;
}

/**yield */
static var_t* native_yield(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data; (void)env;
	return NULL;
}

#define GC_FREE_BUFFER 128
vm_t* vm_new(bool compiler(bytecode_t *bc, const char* input)) {
	vm_t* vm = (vm_t*)_malloc(sizeof(vm_t));
	memset(vm, 0, sizeof(vm_t));

	vm->compiler = compiler;
	vm->terminated = false;
	vm->pc = 0;
	vm->this_strIndex = 0;
	vm->gc_buffer_size = GC_BUFFER;
	vm->gc_free_buffer_size = GC_FREE_BUFFER;
	vm->stack_top = 0;

	bc_init(&vm->bc);
	vm->this_strIndex = bc_getstrindex(&vm->bc, THIS);

	vm->scopes = array_new();

	#ifdef MARIO_CACHE
	var_cache_init(vm);
	#endif

	#ifdef MARIO_THREAD
	pthread_mutex_init(&vm->thread_lock, NULL);

	vm->isignal_head = NULL;
	vm->isignal_tail = NULL;
	vm->isignal_num = 0;
	vm->interrupted = false;
	#endif

	array_init(&vm->included);	
	array_init(&vm->close_natives);	
	array_init(&vm->init_natives);	
	
	vm->root = var_new_obj(vm, NULL, NULL);
	var_ref(vm->root);
	vm->var_true = var_new_bool(vm, true);
	//var_add(vm->root, "", vm->var_true);
	var_ref(vm->var_true);
	vm->var_false = var_new_bool(vm, false);
	//var_add(vm->root, "", vm->var_false);
	var_ref(vm->var_false);
	vm->var_null = var_new_null(vm);
	//var_add(vm->root, "", vm->var_null);
	var_ref(vm->var_null);

	vm->var_Object = vm_new_class(vm, "Object");
	vm_reg_static(vm, NULL, "yield()", native_yield, NULL);
	vm_reg_static(vm, NULL, "debug()", native_debug, NULL);
	return vm;
}

void vm_init(vm_t* vm,
		void (*on_init)(struct st_vm* vm),
		void (*on_close)(struct st_vm* vm)) {
	_done_arr_inited = false;
	vm->on_init = on_init;
	vm->on_close = on_close;
	
	if(vm->on_init != NULL)
		vm->on_init(vm);

	int i;
	for(i=0; i<vm->init_natives.size; i++) {
		native_init_t* it = (native_init_t*)array_get(&vm->init_natives, i);
		it->func(it->data);
	}

	vm_load_basic_classes(vm);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

