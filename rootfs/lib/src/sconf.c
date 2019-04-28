#include <sconf.h>
#include <unistd.h>
#include <kstring.h>
#include <stdlib.h>

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
	uint8_t stat = 0; /*0 for name; 1 for value*/
	sconf_item_t* item = &conf->items[0];
	while(it < S_CONF_ITEM_MAX) {
		char c;
		if(read(fd, &c, 1) <= 0) {
			it++;
			break;
		}

		if(stat == 0) {/*read name*/
			if(c == '=') {
				i = 0;
				stat = 1;
				continue;
			}
			if(i < S_CONF_NAME_MAX)
				item->name[i] = c;
			i++;
		}	
		else { /*read value*/
			if(c == '\n') {
				i = 0;
				stat = 0;
				it++;
				item = &conf->items[it];
				continue;
			}
			if(i < S_CONF_VALUE_MAX)
				item->value[i] = c;
			i++;
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
