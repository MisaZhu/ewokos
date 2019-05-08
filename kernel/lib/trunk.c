#include <trunk.h>
#include <kstring.h>

int32_t trunk_init(trunk_t* trunk, uint32_t item_size, malloc_func_t mlc, free_func_t fr) {
	if(trunk == NULL)
		return -1;
	memset(trunk, 0, sizeof(trunk_t));
	trunk->item_size = item_size;
	trunk->mlc = mlc;
	trunk->fr = fr;
	return 0;
}

void trunk_clear(trunk_t* trunk) {
	if(trunk == NULL)
		return;
	if(trunk->items != NULL)
		trunk->fr(trunk->items);
	uint32_t tmp = trunk->item_size;
	memset(trunk, 0, sizeof(trunk_t));
	trunk->item_size = tmp;
}

#define TRUNK_STEP_SIZE 32
int32_t trunk_add(trunk_t* trunk) {
	if(trunk == NULL)
		return -1;
	
	if(trunk->size == trunk->trunk_size) {
		trunk->trunk_size += (TRUNK_STEP_SIZE * trunk->item_size);
		void* nw = trunk->mlc(trunk->trunk_size);
		if(trunk->items != NULL) {
			memcpy(nw, trunk->items, trunk->size*trunk->item_size);
			trunk->fr(trunk->items);
		}
		trunk->items = nw;
	}
	trunk->size++;
	return trunk->size - 1;
}

