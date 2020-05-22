#ifndef SIMPLE_CONFIG_H
#define SIMPLE_CONFIG_H

#include <mstr.h>

#define S_CONF_ITEM_MAX 32

typedef struct {
	str_t* name;
	str_t* value;
} sconf_item_t;

typedef struct {
	sconf_item_t items[S_CONF_ITEM_MAX];
} sconf_t;

sconf_t* sconf_parse(const char* str);
void sconf_free(sconf_t* conf);
const char* sconf_get(sconf_t *conf, const char*name);
sconf_item_t* sconf_get_at(sconf_t *conf, int index);
sconf_t* sconf_load(const char* fname);

#endif
