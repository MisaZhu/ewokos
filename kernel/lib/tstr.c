#include <tstr.h>
#include <kstring.h>

tstr_t* tstr_new(const char* src, malloc_func_t mlc, free_func_t fr) {
	tstr_t* tstr = (tstr_t*)mlc(sizeof(tstr_t));
	trunk_init(tstr, 1, mlc, fr);
	if(src != NULL)
		tstr_cpy(tstr, src);
	return tstr;
}

void tstr_free(tstr_t* tstr) {
	if(tstr != NULL) {
		free_func_t fr = tstr->fr;
		trunk_clear(tstr);
		fr(tstr);
	}
}

inline const char* tstr_addc(tstr_t* tstr, char c) {
	int32_t at = trunk_add(tstr);
	if(at < 0)
		return "";
	((char*)tstr->items)[at]	= c;
	if(c == 0)
		tstr->size--;
	return (const char*)tstr->items;
}

const char* tstr_add(tstr_t* tstr, const char* add) {
	while(*add != 0) {
		tstr_addc(tstr, *add);
		add++;
	}
	tstr_addc(tstr, 0);
	return (const char*)tstr->items;
}

const char* tstr_cpy(tstr_t* tstr, const char* add) {
	tstr_empty(tstr);
	return tstr_add(tstr, add);
}

const char* tstr_cstr(tstr_t* tstr) {
	if(tstr == NULL || tstr->items == NULL)
		return "";
	return (const char*)tstr->items; 
}

const char* tstr_rev(tstr_t* tstr) {
	if(tstr == NULL || tstr->items == NULL)
		return "";

	char* s = (char*)tstr->items; 
	int32_t h = 0;
	int32_t t = ((int32_t)tstr->size)-1;
	while(h < t) {
		char c = s[h];
		s[h] = s[t];
		s[t] = c;
		h++;
		t--;
	}
	return s;
}

void tstr_empty(tstr_t* tstr) {
	if(tstr == NULL || tstr->items == NULL)
		return;
	((char*)tstr->items)[0] = 0; 
	tstr->size = 0;
}

tstr_t* tstr_dump(tstr_t* src) {
	if(src == NULL)
		return NULL;
	tstr_t* ret = tstr_new(tstr_cstr(src), src->mlc, src->fr);
	return ret;
}
