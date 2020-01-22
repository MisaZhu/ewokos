#ifndef ENV_H
#define ENV_H

#include <mstr.h>

typedef struct {
	str_t* name;
	str_t* value;
} env_t;

void    init_global(void);
int32_t set_global(const char* name, const char* value);
int32_t get_global(const char* name, char* value, uint32_t size);

#endif
