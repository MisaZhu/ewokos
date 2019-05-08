#ifndef SIMPLE_CONFIG_PARSE_H
#define SIMPLE_CONFIG_PARSE_H

#include <types.h>
#include <tstr.h>

#define S_CONF_ITEM_MAX 32

typedef struct {
	tstr_t* name;
	tstr_t* value;
} sconf_item_t;

typedef struct {
	sconf_item_t items[S_CONF_ITEM_MAX];
} sconf_t;

sconf_t* sconf_parse(const char* str, malloc_func_t mlc, free_func_t fr);
void sconf_free(sconf_t* conf, free_func_t fr);
const char* sconf_get(sconf_t *conf, const char*name);

#endif
