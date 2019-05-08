#ifndef TRUNK_STR_H
#define TRUNK_STR_H

#include <trunk.h>

typedef trunk_t tstr_t;

tstr_t* tstr_new(const char* src, malloc_func_t mlc, free_func_t fr);
void tstr_free(tstr_t* tstr);
const char* tstr_addc(tstr_t* tstr, char c);
const char* tstr_add(tstr_t* tstr, const char* add);
const char* tstr_cpy(tstr_t* tstr, const char* str);
const char* tstr_rev(tstr_t* tstr);
void tstr_empty(tstr_t* tstr);
tstr_t* tstr_dump(tstr_t* src);
const char* tstr_cstr(tstr_t* tstr);

#define CS(tstr) tstr_cstr(tstr)

#endif
