#include "mstr.h"
#include <string.h>
#include <stdlib.h>
#include <vprintf.h>
#include <basic_math.h>
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
		str->cstr = (char*)krealloc_raw(str->cstr, str->max, new_size);
		str->max = new_size;
	}

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
	str_t* ret = (str_t*)kmalloc(sizeof(str_t));
	ret->cstr = NULL;
	ret->max = 0;
	ret->len = 0;
	str_cpy(ret, s);
	return ret;
}

str_t* str_new_by_size(uint32_t sz) {
	str_t* ret = (str_t*)kmalloc(sizeof(str_t));
	ret->cstr = (char*)kmalloc(sz);
	ret->max = sz;
	ret->cstr[0] = 0;
	ret->len = 0;
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
		str->cstr = (char*)krealloc_raw(str->cstr, str->max, new_size);
		str->max = new_size;
	}

	strcpy(str->cstr + str->len, src);
	str->len = str->len + len;
	str->cstr[str->len] = 0;
	return str->cstr;
}

char* str_addc(str_t* str, char c) {
	uint32_t new_size = str->len + 1;
	if(str->max <= new_size) {
		new_size = str->len + STR_BUF; /*STR BUF for buffer*/
		str->cstr = (char*)krealloc_raw(str->cstr, str->max, new_size);
		str->max = new_size;
	}

	str->cstr[str->len] = c;
	str->len++;
	str->cstr[str->len] = 0;
	return str->cstr;
}

char* str_addc_int(str_t* str, int32_t i, int32_t base) {
	return str_add(str, str_from_int(i, base));
}

void str_free(str_t* str) {
	if(str == NULL)
		return;

	if(str->cstr != NULL) {
		kfree(str->cstr);
	}
	kfree(str);
}

static char _str_result[STATIC_STR_MAX+1];

const char* str_from_int(int32_t value, int32_t base) {
    // check that the base if valid
    if (base < 2 || base > 36) 
			base = 10;

    char* ptr = _str_result, *ptr1 = _str_result, tmp_char;
    int32_t tmp_value;

    do {
        tmp_value = value;
        value = div_u32(value, base);
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

const char* str_from_bool(uint8_t b) {
	return b == 0 ? "false":"true";
}

static int32_t atoi_base(const char *s, int32_t b) {
	int32_t i, result, x, error;
	for (i = result = error = x = 0; s[i]!='\0'; i++, result += x) {
		if (b==2) {
			if (!(s[i]>47&&s[i]<50)){			//rango
				error = 1;
			}
			else {
				x = s[i] - '0';
				result *= b;
			}
		}
		else if (b==8) {
			if (i==0 && s[i]=='0'){
				x=0;
			}
			else if (!(s[i]>47&&s[i]<56)) {	//rango
				error = 1;
			}
			else {
				x = s[i] - '0';
				result *= b;
			}
		}
		else if (b==10) {
			if (!(s[i]>47&&s[i]<58)) {			//rango
				error = 1;
			}
			else {
				x = s[i] - '0';
				result *= b;
			}
		}
		else if (b==16) {
			if((i==0 && s[i]=='0')||(i==1 && (s[i]=='X'||s[i]=='x'))) {
				x = 0;
			}
			else if (!((s[i]>47 && s[i]<58) || ((s[i]>64 && s[i]<71) || (s[i]>96 && s[i]<103)) )) {		//rango
				error = 1;
			}
			else {
				x = (s[i]>64 && s[i]<71)? s[i]-'7': s[i] - '0';
				x = (s[i]>96 && s[i]<103)? s[i]-'W': x;
				result *= b;
			}
		}
	}

	if (error)
		return 0;
	else
		return result;
}

int32_t str_to_int(const char* str) {
	int32_t i = 0;
	if(strstr(str, "0x") != NULL ||
			strstr(str, "0x") != NULL)
		i = (int)atoi_base(str, 16);
	else
		i = (int)atoi_base(str, 10);
	return i;
}

int32_t str_to(const char* str, char c, str_t* res, uint8_t skipspace) {
	int32_t i = 0;
	if(res != NULL)
		str_reset(res);

	while(1) {
		char offc = str[i]; 
		if(offc==0) {//the end of str
			return -1;
		}

		//skip space
		if(skipspace != 0 && (offc == ' ' || offc == '\t')) {
			i++;
			continue;
		}

		if(offc == c) 
			break;
		else if(res != NULL)
			str_addc(res, offc);
		i++;
	}

	if(skipspace != 0 && res != NULL) {
		int32_t j = res->len - 1;
		while(j >= 0) {
			char c = res->cstr[j];
			if(c == ' ' || c == '\t')
				res->cstr[j] = 0;
			j--;
		}
	}

	return i;
}
