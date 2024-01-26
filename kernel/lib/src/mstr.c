#include <mstr.h>
#include <string.h>

#include <stdarg.h>
#include <mm/kmalloc.h>

/** str functions.-----------------------------*/

#define STR_BUF 16
#define STATIC_STR_MAX 64

void str_reset(str_t* str) {
	if(str->cstr == NULL) {
		str->cstr = (char*)kmalloc(STR_BUF);
		str->max = STR_BUF;
	}

	str->cstr[0] = 0;
	str->len = 0;	
}

char* str_ncpy(str_t* str, const char* src, uint32_t l) {
	if(src == NULL || src[0] == 0 || l == 0) {
		str_reset(str);
		return str->cstr;
	}

	uint32_t len = (uint32_t)strlen(src);
	if(len > l)
		len = l;

	uint32_t new_size = len;
	if(str->max <= new_size) {
		new_size = len + STR_BUF; /*STR BUF for buffer*/
		kfree(str->cstr);
		str->cstr = (char*)kmalloc(new_size);
		str->max = new_size;
	}

	sstrncpy(str->cstr, src, len);
	str->cstr[len] = 0;
	str->len = len;
	return str->cstr;
}

char* str_cpy(str_t* str, const char* src) {
	str_ncpy(str, src, 0x0FFFF);
	return str->cstr;
}

str_t* str_new(const char* s) {
	str_t* ret = (str_t*)kmalloc(sizeof(str_t));
	ret->cstr = NULL;
	ret->max = 0;
	ret->len = 0;
	str_cpy(ret, s);
	return ret;
}

char* str_add(str_t* str, const char* src) {
	if(src == NULL || src[0] == 0) {
		return str->cstr;
	}

	uint32_t len = (uint32_t)strlen(src);
	uint32_t new_size = str->len + len;
	if(str->max <= new_size) {
		new_size = str->len + len + STR_BUF; /*STR BUF for buffer*/
		char* old = str->cstr;
		str->cstr = (char*)kmalloc(new_size);
		sstrncpy(str->cstr, old, str->len);
		kfree(old);
		str->max = new_size;
	}

	strcpy(str->cstr + str->len, src);
	str->len = str->len + len;
	str->cstr[str->len] = 0;
	return str->cstr;
}

char* str_addc(str_t* str, char c) {
	if(c == 0) {
		str->cstr[str->len] = 0;
		return str->cstr;
	}

	uint32_t new_size = str->len + 1;
	if(str->max <= new_size) {
		new_size = str->len + STR_BUF; /*STR BUF for buffer*/
		char* old = str->cstr;
		str->cstr = (char*)kmalloc(new_size);
		sstrncpy(str->cstr, old, str->len);
		kfree(old);
		str->max = new_size;
	}

	str->cstr[str->len] = c;
	str->len++;
	str->cstr[str->len] = 0;
	return str->cstr;
}

void str_free(str_t* str) {
	if(str == NULL)
		return;

	if(str->cstr != NULL) {
		kfree(str->cstr);
	}
	kfree(str);
}
