#ifndef KCONSOLE_H
#define KCONSOLE_H

#ifdef KCONSOLE
#include <stdint.h>

typedef struct {
	struct {
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		uint32_t rotate;
	} fb;
	uint32_t font_size;
} kconsole_conf_t;

extern kconsole_conf_t _kconsole_config;

void kconsole_init(void);
void kconsole_input(const char* s);
void kconsole_close(void);
#endif

#endif