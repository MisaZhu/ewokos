
#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/vfs.h>
#include <tinyjson/tinyjson.h>

#include "log.h"

#define DEFAULT_WLAN_CFG	"/etc/wlan/network.json"

json_node_t* network_config; 

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

	json_var_t *var = json_parse_str(str);
	free(str);

	if(var != NULL) {
		network_config = json_var_find(var, "network");
		if(network_config && network_config->var->json_is_array)
			return;
	}
	brcm_log("Error: %s parse failed \n", path);
}

int config_match_ssid(const char* ssid){
	if(network_config)
	{
		int cnt = json_var_array_size(network_config->var);
		for(int i = 0; i < cnt; i++){
			json_node_t *n = json_var_array_get(network_config->var, i);
			if(n){
				json_node_t* s = json_var_find(n->var, "ssid");
				if(s && s->var->type == JSON_V_STRING){
					if(strcmp(ssid, json_var_get_str(s->var)) == 0)
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
		json_node_t *n = json_var_array_get(network_config->var, idx);
		if(n){
			json_node_t* p = json_var_find(n->var, "priority");
			if(p && p->var->type == JSON_V_INT){
				return json_var_get_int(p->var);
			}
		}
	}
	return 0;
}

const char* config_get_pmk(int idx){
	if(network_config)
	{
		json_node_t *n = json_var_array_get(network_config->var, idx);
		if(n){
			json_node_t* p = json_var_find(n->var, "pmk");
			if(p && p->var->type == JSON_V_STRING){
				return json_var_get_str(p->var);
			}
		}
		}
	return NULL;
}

const char* config_get_passwd(int idx){
	if(network_config)
	{
		json_node_t *n = json_var_array_get(network_config->var, idx);
		if(n){
			json_node_t* p = json_var_find(n->var, "passwd");
			if(p && p->var->type == JSON_V_STRING){
				return json_var_get_str(p->var);
			}
		}
	}
	return NULL;
}


const char* config_get_ssid(int idx){
	if(network_config)
	{
		json_node_t *n = json_var_array_get(network_config->var, idx);
		if(n){
			json_node_t* p = json_var_find(n->var, "ssid");
			if(p && p->var->type == JSON_V_STRING){
					return json_var_get_str(p->var);
			}
		}
	}
	return NULL;
}