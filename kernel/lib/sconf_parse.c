#include <sconf.h>
#include <kstring.h>

static inline bool is_space(char c) {
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

static inline void trim_right(tstr_t* st) {
	char* s = (char*)st->items;
	if(s == NULL)
		return;

	while(st->size > 0) {
		if(is_space(s[st->size-1]))
			st->size--;
		else
			break;
	}
}

sconf_t* sconf_parse(const char* str, mem_funcs_t* mfs) {
	if(str == NULL || str[0] == 0)
		return NULL;

	sconf_t *conf = (sconf_t*)mfs->mlc(sizeof(sconf_t));
	if(conf == NULL)
		return NULL;
	memset(conf, 0, sizeof(sconf_t));

	int32_t i = 0;	
	int32_t it = 0;	/*item index*/
	uint8_t stat = 0; /*0 for name; 1 for value; 2 for comment*/
	sconf_item_t* item = &conf->items[0];
	item->name = tstr_new("", mfs);
	item->value = tstr_new("", mfs);
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
				tstr_addc(item->value, 0);
				it++;
				item = &conf->items[it];
				item->name = tstr_new("", mfs);
				item->value = tstr_new("", mfs);
			}
			stat = 2;
			continue;
		}
		else if(stat == 0) {/*read name*/
			if(c == '=') {
				trim_right(item->name);
				tstr_addc(item->name, 0);
				i = 0;
				stat = 1;
				continue;
			}
			else if(is_space(c)) {
				continue;
			}
			tstr_addc(item->name, c);
			i++;
		}	
		else if(stat == 1) { /*read value*/
			if(c == '\n') {
				trim_right(item->value);
				tstr_addc(item->value, 0);
				i = 0;
				stat = 0;
				it++;
				item = &conf->items[it];
				item->name = tstr_new("", mfs);
				item->value = tstr_new("", mfs);
				continue;
			}
			tstr_addc(item->value, c);
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

void sconf_free(sconf_t* conf, mem_funcs_t* mfs) {
	if(conf == NULL)
		return;
	int32_t i = 0;
	while(i < S_CONF_ITEM_MAX) {
		sconf_item_t* item = &conf->items[i++];
		if(item->name == NULL)
			break;
		tstr_free(item->name);
		tstr_free(item->value);
	}
	mfs->fr(conf);
}

const char* sconf_get(sconf_t *conf, const char*name) {
	if(name == NULL || conf == NULL)
		return "";

	int32_t i = 0;
	while(i < S_CONF_ITEM_MAX) {
		sconf_item_t* item = &conf->items[i++];
		const char* n = CS(item->name);
		if(strcmp(n, name) == 0)
			return CS(item->value);
	}
	return "";
}
