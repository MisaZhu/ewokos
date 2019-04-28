#include <sconf.h>
#include <unistd.h>
#include <kstring.h>
#include <stdlib.h>

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

sconf_t* sconf_load(const char* fname) {
	if(fname == NULL || fname[0] == 0)
		return NULL;

	sconf_t *conf = (sconf_t*)malloc(sizeof(sconf_t));
	if(conf == NULL)
		return NULL;

	int32_t fd = open(fname, O_RDONLY);
	if(fd < 0) {
		free(conf);
		return NULL;
	}
	
	int32_t i = 0;	
	int32_t it = 0;	/*item index*/
	uint8_t stat = 0; /*0 for name; 1 for value; 2 for comment*/
	sconf_item_t* item = &conf->items[0];
	while(it < S_CONF_ITEM_MAX) {
		char c;
		if(read(fd, &c, 1) <= 0) {
			it++;
			break;
		}

		if(i == 0 && is_space(c)) {
			continue;
		}
		else if(c == '#') { //comment
			if(stat == 1) {
				item->value[i] = 0;
				trim_right(item->value, i);
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
				item->value[i] = 0;
				trim_right(item->value, i);
				i = 0;
				stat = 0;
				it++;
				item = &conf->items[it];
				continue;
			}
			if(i < S_CONF_VALUE_MAX) {
				item->value[i] = c;
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

	close(fd);
	return conf;
}

void sconf_free(sconf_t* conf) {
	if(conf != NULL)
		free(conf);
}

const char* sconf_get(sconf_t *conf, const char*name) {
	if(name == NULL || conf == NULL)
		return "";

	int32_t i = 0;
	while(i < S_CONF_ITEM_MAX) {
		sconf_item_t* item = &conf->items[i++];
		if(item->name[0] == 0)
			return "";
		if(strcmp(item->name, name) == 0)
			return item->value;
	}
	return "";
}
