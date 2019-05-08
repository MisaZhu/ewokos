#ifndef TRUNK_H
#define TRUNK_H

#include <types.h>

typedef struct {
	uint32_t item_size;
	uint32_t size;
	uint32_t trunk_size;
	void *items;
} trunk_t;

int32_t trunk_init(trunk_t* trunk, uint32_t item_size);
void trunk_clear(trunk_t* trunk);
int32_t trunk_add(trunk_t* trunk);

#endif
