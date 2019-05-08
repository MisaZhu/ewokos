#include <trunk.h>
#include <kstring.h>
#include <stdlib.h>

int32_t trunk_init(trunk_t* trunk, uint32_t item_size) {
	if(trunk == NULL)
		return -1;
	memset(trunk, 0, sizeof(trunk_t));
	trunk->item_size = item_size;
	return 0;
}

void trunk_clear(trunk_t* trunk) {
	if(trunk == NULL)
		return;
	if(trunk->items != NULL)
		free(trunk->items);
	uint32_t tmp = trunk->item_size;
	memset(trunk, 0, sizeof(trunk_t));
	trunk->item_size = tmp;
}

#define TRUNK_STEP_SIZE 16
int32_t trunk_add(trunk_t* trunk) {
	if(trunk == NULL)
		return -1;
	
	if(trunk->size == trunk->trunk_size) {
		trunk->trunk_size += (TRUNK_STEP_SIZE * trunk->item_size);
		void* nw = malloc(trunk->trunk_size);
		if(trunk->items != NULL) {
			memcpy(nw, trunk->items, trunk->size*trunk->item_size);
			free(trunk->items);
		}
		trunk->items = nw;
	}
	trunk->size++;
	return trunk->size - 1;
}
