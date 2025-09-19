#include <ewoksys/mstr.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


/** str functions.-----------------------------*/

#define STR_BUF 32
#define STATIC_STR_MAX 64

void str_reset(str_t* str) {
	if(str->cstr == NULL) {
		str->cstr = (char*)malloc(STR_BUF);
		if(str->cstr == NULL) {
			str->max = 0;
			str->len = 0;	
			return;
		}
		str->max = STR_BUF;
	}
	str->cstr[0] = 0;
	str->len = 0;	
}

char* str_detach(str_t* str) {
	char* ret = str->cstr;
	str->cstr = NULL;
	str_free(str);
	return ret;
}

char* str_ncpy(str_t* str, const char* src, uint32_t l) {
	if(src == NULL || src[0] == 0 || l == 0) {
		str_reset(str);
		return str->cstr;
	}

	uint32_t len = (uint32_t)strlen(src);
	if(len > l)
		len = l;
	len++;

	uint32_t new_size = len;
	if(str->max <= new_size) {
		new_size = len + STR_BUF; /*STR BUF for buffer*/
		str->cstr = realloc(str->cstr, new_size);
		str->max = new_size;
	}
	len--;

	strncpy(str->cstr, src, len);
	str->cstr[len] = 0;
	str->len = len;
	return str->cstr;
}

char* str_cpy(str_t* str, const char* src) {
	str_ncpy(str, src, 0x0FFFF);
	return str->cstr;
}

str_t* str_new(const char* s) {
	str_t* ret = (str_t*)malloc(sizeof(str_t));
	if(ret == NULL)
		return NULL;
	ret->cstr = NULL;
	ret->max = 0;
	ret->len = 0;
	str_cpy(ret, s);
	return ret;
}

str_t* str_new_by_size(uint32_t sz) {
	str_t* ret = (str_t*)malloc(sizeof(str_t));
	if(ret == NULL)
		return NULL;
	ret->len = 0;

	ret->cstr = (char*)malloc(sz);
	if(ret->cstr == NULL) {
		ret->max = 0;
		return ret;
	}

	ret->max = sz;
	ret->cstr[0] = 0;
	return ret;
}

char* str_add(str_t* str, const char* src) {
	if(src == NULL || src[0] == 0) {
		return str->cstr;
	}

	uint32_t len = (uint32_t)strlen(src);
	uint32_t new_size = str->len + len + 1;
	if(str->max <= new_size) {
		new_size = str->len + len + STR_BUF; /*STR BUF for buffer*/
		str->cstr = realloc(str->cstr, new_size);
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
		str->cstr = realloc(str->cstr, new_size);
		str->max = new_size;
	}

	str->cstr[str->len] = c;
	str->len++;
	str->cstr[str->len] = 0;
	return str->cstr;
}

char* str_addc_int(str_t* str, int i, int base) {
	return str_add(str, str_from_int(i, base));
}

char* str_addc_float(str_t* str, float f) {
	return str_add(str, str_from_float(f));
}

void str_free(str_t* str) {
	if(str == NULL)
		return;

	if(str->cstr != NULL) {
		free(str->cstr);
	}
	free(str);
}

static char _str_result[STATIC_STR_MAX+1];

const char* str_from_int(int value, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) 
			base = 10;

    char* ptr = _str_result, *ptr1 = _str_result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value = (value / base);
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
    return _str_result;
}

const char* str_from_bool(bool b) {
	return b ? "true":"false";
}

const char* str_from_float(float i) {
#ifdef M_FLOAT
	snprintf(_str_result, STATIC_STR_MAX-1, "%f", i);
#else
	(void)i;
	strcpy(_str_result, "0.0");
#endif
	return _str_result;
}

int str_to_int(const char* str) {
	int i = 0;
	if(strstr(str, "0x") != NULL ||
			strstr(str, "0x") != NULL)
		i = (int)strtoul(str, NULL, 16);
	else
		i = (int)strtoul(str, NULL,  10);
	return i;
}

bool str_to_bool(const char* str) {
	if(strcmp(str, "true") == 0 ||
			strcmp(str, "TRUE") == 0)
		return true;
	return false;
}

float str_to_float(const char* str) {
	return atof(str);
}

int str_to(const char* str, char c, str_t* res, uint8_t skipspace) {
	int i = 0;
	if(res != NULL)
		str_reset(res);

	if(skipspace != 0) {
		while(1) {
			char offc = str[i]; 
			if(offc == 0) //the end of str
				return -1;
			if(offc != ' ' && offc != '\t')
				break;
			i++;
		}
	}

	while(1) {
		char offc = str[i]; 
		if(offc == 0) //the end of str
			return -1;
		if(offc == c) 
			break;
		else if(res != NULL)
			str_addc(res, offc);
		i++;
	}

	if(skipspace != 0 && res != NULL) {
		int j = res->len - 1;
		while(j >= 0) {
			char c = res->cstr[j];
			if(c == ' ' || c == '\t')
				res->cstr[j] = 0;
			else {
				res->len = j+1;
				break;
			}
			j--;
		}
	}

	return i;
}

/*
static void outc(char c, void* p) {
	str_t* buf = (str_t*)p;
	str_addc(buf, c);
}

str_t* str_format(str_t* str, const char *format, ...) {
	str_reset(str);
	va_list ap;
	va_start(ap, format);
	vprintf(outc, str, format, ap);
	va_end(ap);
	return str;
}

str_t* str_format_new(const char *format, ...) {
	str_t* str = str_new(NULL);
	va_list ap;
	va_start(ap, format);
	vprintf(outc, str, format, ap);
	va_end(ap);
	return str;
}
*/

#ifdef __cplusplus
}
#endif

