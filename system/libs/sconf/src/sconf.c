#include <sconf.h>
#include <sys/vfs.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <mstr.h>

static inline int is_space(char c) {
	if(c == ' ' || c == '\t' || c == '\r' || c == '\n')
		return 1;
	return 0;
}

static inline void trim_right(str_t* st) {
	char* s = (char*)st->cstr;
	if(s == NULL)
		return;

	while(st->len > 0) {
		if(is_space(s[st->len-1])) {
			st->cstr[st->len-1] = 0;
			st->len--;
		}
		else
			break;
	}
}

sconf_t* sconf_parse(const char* str) {
	if(str == NULL || str[0] == 0)
		return NULL;

	sconf_t *conf = (sconf_t*)malloc(sizeof(sconf_t));
	if(conf == NULL)
		return NULL;
	memset(conf, 0, sizeof(sconf_t));

	int32_t i = 0;	
	int32_t it = 0;	/*item index*/
	uint8_t stat = 0; /*0 for name; 1 for value; 2 for comment*/
	sconf_item_t* item = &conf->items[0];
	item->name = str_new("");
	item->value = str_new("");
	while(it < S_CONF_ITEM_MAX) {
		char c = *str;
		str++;
		if(c == 0) {
			it++;
			break;
		}
		if(i == 0 && is_space(c)) {
			continue;
		}
		else if(c == '#') { //comment
			if(stat == 1) {
				trim_right(item->value);
				str_addc(item->value, 0);
				it++;
				item = &conf->items[it];
				item->name = str_new("");
				item->value = str_new("");
			}
			stat = 2;
			continue;
		}
		else if(stat == 0) {/*read name*/
			if(c == '=') {
				trim_right(item->name);
				str_addc(item->name, 0);
				i = 0;
				stat = 1;
				continue;
			}
			else if(is_space(c)) {
				continue;
			}
			str_addc(item->name, c);
			i++;
		}	
		else if(stat == 1) { /*read value*/
			if(c == '\n') {
				trim_right(item->value);
				str_addc(item->value, 0);
				i = 0;
				stat = 0;
				it++;
				item = &conf->items[it];
				item->name = str_new("");
				item->value = str_new("");
				continue;
			}
			str_addc(item->value, c);
			i++;
		}
		else { //comment
			if(c == '\n') {
				i = 0;
				stat = 0;
			}
		}
	}
	return conf;
}

void sconf_free(sconf_t* conf) {
	if(conf == NULL)
		return;
	int32_t i = 0;
	while(i < S_CONF_ITEM_MAX) {
		sconf_item_t* item = &conf->items[i++];
		if(item->name == NULL)
			break;
		str_free(item->name);
		str_free(item->value);
	}
	free(conf);
}

sconf_item_t* sconf_get_at(sconf_t *conf, int index) {
	if(index >= S_CONF_ITEM_MAX)
		return NULL;
	return &conf->items[index];
}

const char* sconf_get(sconf_t *conf, const char*name) {
	if(name == NULL || conf == NULL)
		return "";

	int32_t i = 0;
	while(i < S_CONF_ITEM_MAX) {
		sconf_item_t* item = &conf->items[i++];
		if(item->name == NULL || item->name->cstr == NULL)
			continue;
		const char* n = item->name->cstr;
		if(strcmp(n, name) == 0) {
			if(item->value->cstr == NULL)
				return "";
			return item->value->cstr;
		}
	}
	return "";
}

sconf_t* sconf_load(const char* fname) {
	int size;
	char* str = vfs_readfile(fname, &size);
	if(str == NULL || size == 0)
		return NULL;
	sconf_t* ret = sconf_parse(str);
	free(str);
	return ret;
}
