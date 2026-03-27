#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <types.h>

void config_init(const char* path);
int config_match_ssid(const char* ssid);
int config_get_priority(int idx);
const char* config_get_pmk(int idx);
const char* config_get_ssid(int idx);
const char* config_get_passwd(int idx);


#endif