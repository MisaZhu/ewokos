#ifndef SIMPLE_CONFIG_H
#define SIMPLE_CONFIG_H

#include <types.h>

#define S_CONF_NAME_MAX 32
#define S_CONF_VALUE_MAX 128
#define S_CONF_ITEM_MAX 32

typedef struct {
	char name[S_CONF_NAME_MAX];
	char value[S_CONF_VALUE_MAX];
} sconf_item_t;

typedef struct {
	sconf_item_t items[S_CONF_ITEM_MAX];
} sconf_t;

sconf_t* sconf_load(const char* fname);
void sconf_free(sconf_t* conf);
const char* sconf_get(sconf_t *conf, const char*name);

#endif
