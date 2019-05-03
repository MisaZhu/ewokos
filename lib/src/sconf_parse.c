#include <sconf.h>
#include <kstring.h>

static inline bool is_space(char c) {
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

static inline void trim_right(char* s, int32_t i) {
	i--;
	while(i >= 0) {
		if(is_space(s[i]))
			s[i--] = 0;
		else
			break;
	}
}

#define CONFV_MAX 1024

sconf_t* sconf_parse(const char* str, malloc_func_t mlc) {
	if(str == NULL || str[0] == 0)
		return NULL;

	sconf_t *conf = (sconf_t*)mlc(sizeof(sconf_t));
	if(conf == NULL)
		return NULL;
	memset(conf, 0, sizeof(sconf_t));

	int32_t i = 0;	
	int32_t it = 0;	/*item index*/
	uint8_t stat = 0; /*0 for name; 1 for value; 2 for comment*/
	char value[CONFV_MAX];
	sconf_item_t* item = &conf->items[0];
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
				value[i] = 0;
				trim_right(value, i);
				item->value = (char*)mlc(strlen(value)+1);
				strcpy(item->value, value);
				it++;
				item = &conf->items[it];
			}
			stat = 2;
			continue;
		}
		else if(stat == 0) {/*read name*/
			if(c == '=') {
				item->name[i] = 0;
				i = 0;
				stat = 1;
				continue;
			}
			else if(is_space(c)) {
				continue;
			}
			else if(i < S_CONF_NAME_MAX) {
				item->name[i] = c;
				i++;
			}
		}	
		else if(stat == 1) { /*read value*/
			if(c == '\n') {
				value[i] = 0;
				trim_right(value, i);
				item->value = (char*)mlc(strlen(value)+1);
				strcpy(item->value, value);
				i = 0;
				stat = 0;
				it++;
				item = &conf->items[it];
				continue;
			}
			if(i < CONFV_MAX) {
				value[i] = c;
				i++;
			}
		}
		else { //comment
			if(c == '\n') {
				i = 0;
				stat = 0;
			}
		}
	}
	if(it < S_CONF_ITEM_MAX) 
		conf->items[it].name[0] = 0;	
	return conf;
}

void sconf_free(sconf_t* conf, free_func_t fr) {
	if(conf == NULL)
		return;
	int32_t i = 0;
	while(i < S_CONF_ITEM_MAX) {
		sconf_item_t* item = &conf->items[i++];
		if(item->name[0] != 0 && item->value != NULL)
			fr(item->value);
	}
	fr(conf);
}

const char* sconf_get(sconf_t *conf, const char*name) {
	if(name == NULL || conf == NULL)
		return "";

	int32_t i = 0;
	while(i < S_CONF_ITEM_MAX) {
		sconf_item_t* item = &conf->items[i++];
		if(item->name[0] == 0 || item->value == NULL)
			return "";
		if(strcmp(item->name, name) == 0)
			return item->value;
	}
	return "";
}
