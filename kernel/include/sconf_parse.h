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

sconf_t* sconf_parse(const char* str, mem_funcs_t* mfs);
void sconf_free(sconf_t* conf, mem_funcs_t* mfs);
const char* sconf_get(sconf_t *conf, const char*name);

#endif
