#include <kernel/env.h>
#include <kstring.h>
#include <kernel/proc.h>

#define GLOBAL_MAX 64

static env_t _global_envs[GLOBAL_MAX];

void init_global(void) {
	int32_t i;
	for(i=0; i<GLOBAL_MAX; i++) {
		memset(&_global_envs[i], 0, sizeof(env_t));
	}
}

int32_t set_global(const char* name, const char* value) {
	env_t* env = NULL;	
	int32_t i=0;
	for(i=0; i<GLOBAL_MAX; i++) {
		if(_global_envs[i].name == NULL ||
				strcmp(CS(_global_envs[i].name), name) == 0) {
			env = &_global_envs[i];
			if(env->name == NULL) {
				env->name = str_new(name);
			}
			break;
		}
	}
	if(env == NULL)
		return -1;
	if(env->value == NULL)
		env->value = str_new(value);
	else
		str_cpy(env->value, value);
	return 0;
}

static inline env_t* find_env(const char* name) {
	int32_t i=0;
	for(i=0; i<GLOBAL_MAX; i++) {
		if(_global_envs[i].name == NULL)
			break;
		if(strcmp(CS(_global_envs[i].name), name) == 0)
			return &_global_envs[i];
	}
	return NULL;
}

int32_t get_global(const char* name, char* value, uint32_t size) {
	env_t* env = find_env(name);
	if(env == NULL)
		return -1;
	strncpy(value, CS(env->value), size);
	return 0;
}
