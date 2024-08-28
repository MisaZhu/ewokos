
#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/vfs.h>
#include <tinyjson/tinyjson.h>

#include "log.h"

#define DEFAULT_WLAN_CFG	"/etc/wlan/network.json"

node_t* network_config; 

void config_init(const char* path){
	if(path == NULL)
		path = DEFAULT_WLAN_CFG;

	int sz = 0;
	char* str = (char*)vfs_readfile(path, &sz);
	if(str == NULL) {
		brcm_log("Error: %s open failed\n", path);
		return;
	}

	str[sz] = 0;

	var_t *var = json_parse(str);
	free(str);

	if(var != NULL) {
		network_config = var_find(var, "network");
		if(network_config && network_config->var->is_array)
			return;
	}
	brcm_log("Error: %s parse failed \n", path);
}

int config_match_ssid(const char* ssid){
	if(network_config)
	{
		int cnt = var_array_size(network_config->var);
		for(int i = 0; i < cnt; i++){
			node_t *n = var_array_get(network_config->var, i);
			if(n){
				node_t* s = var_find(n->var, "ssid");
				if(s && s->var->type == V_STRING){
					if(strcmp(ssid, var_get_str(s->var)) == 0)
						return i;
				}
			}
		}
	}
	return -1;
}

int config_get_priority(int idx){
	if(network_config)
	{
		node_t *n = var_array_get(network_config->var, idx);
		if(n){
			node_t* p = var_find(n->var, "priority");
			if(p && p->var->type == V_INT){
				return var_get_int(p->var);
			}
		}
	}
	return 0;
}

const char* config_get_pmk(int idx){
	if(network_config)
	{
		node_t *n = var_array_get(network_config->var, idx);
		if(n){
			node_t* p = var_find(n->var, "pmk");
			if(p && p->var->type == V_STRING){
				return var_get_str(p->var);
			}
		}
		}
	return NULL;
}

const char* config_get_passwd(int idx){
	if(network_config)
	{
		node_t *n = var_array_get(network_config->var, idx);
		if(n){
			node_t* p = var_find(n->var, "passwd");
			if(p && p->var->type == V_STRING){
				return var_get_str(p->var);
			}
		}
	}
	return NULL;
}


const char* config_get_ssid(int idx){
	if(network_config)
	{
		node_t *n = var_array_get(network_config->var, idx);
		if(n){
			node_t* p = var_find(n->var, "ssid");
			if(p && p->var->type == V_STRING){
					return var_get_str(p->var);
			}
		}
	}
	return NULL;
}