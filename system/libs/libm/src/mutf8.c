#include "mutf8.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define isASCII(b)  ((b & 0x80) == 0)

void utf8_reader_init(utf8_reader_t* reader, const char* s, uint32_t offset) {
	if(reader == NULL)
		return;
	
	reader->str = s;
	reader->offset = offset;
}

/**Read single word with UTF-8 encode
*/
bool utf8_read(utf8_reader_t* reader, str_t* dst) {
	if(reader == NULL || reader->str == NULL)
		return false;
	const char* src = reader->str;
	str_reset(dst);

	uint8_t b;
	b = src[reader->offset++];
	if(b == 0)//end of input
		return false; 

	str_addc(dst, b);
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
			str_addc(dst, b);
			count--;
		}
	}
	return true;
}

utf8_t* utf8_new(const char* s) {
	utf8_t* ret = (utf8_t*)malloc(sizeof(utf8_t));
	array_init(ret);
	utf8_reader_t reader;
	utf8_reader_init(&reader, s, 0);
	while(true) {
		str_t* str = str_new("");
		if(!utf8_read(&reader, str)) {
			str_free(str);
			break;
		}
		array_add(ret, str);
	}
	return ret;
}

void utf8_free(utf8_t* utf8) {
	if(utf8 == NULL)
		return;
	array_clean(utf8, (free_func_t)str_free);
	free(utf8);
}

uint32_t utf8_len(utf8_t* utf8) {
	if(utf8 == NULL)
		return 0;
	return utf8->size;
}

str_t* utf8_at(utf8_t* utf8, uint32_t at) {
	if(utf8 == NULL || at >= utf8_len(utf8))
		return NULL;
	return (str_t*)array_get(utf8, at);
}

void utf8_set(utf8_t* utf8, uint32_t at, const char* s) {
	if(s == NULL || s[0]  == 0) {
		array_del(utf8, at, (free_func_t)str_free);
		return;
	}

	str_t* str = utf8_at(utf8, at);
	if(str == NULL)
		return;
	str_cpy(str, s);
}

void utf8_append_raw(utf8_t* utf8, const char* s) {
	if(utf8 == NULL || s == NULL || s[0] == 0)
		return;

	utf8_t* u = utf8_new(s);
	uint32_t len = utf8_len(u);
	uint32_t i;
	for(i=0; i<len; ++i) {
		str_t* s = utf8_at(u, i);
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
	array_add(utf8, str_new(s));
}

void utf8_to_str(utf8_t* utf8, str_t* str) {
	str_reset(str);
	if(utf8 == NULL)
		return;

	uint32_t len = utf8_len(utf8);
	uint32_t i;
	for(i=0; i<len; ++i) {
		str_t* s = utf8_at(utf8, i);
		if(s == NULL)
			break;
		str_add(str, s->cstr);
	}
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

