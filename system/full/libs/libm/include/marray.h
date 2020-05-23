#ifndef MARIO_ARRAY
#define MARIO_ARRAY

#include <stdint.h>
#include <stddef.h>

/**
array functions.
*/
typedef struct st_array {
	void** items;
	uint32_t max: 16;
	uint32_t size: 16;
} m_array_t;

typedef void (*free_func_t)(void* p);

m_array_t* array_new(void);
void array_free(m_array_t* array, free_func_t fr);
void array_init(m_array_t* array);
void array_add(m_array_t* array, void* item);
void array_add_head(m_array_t* array, void* item);
void* array_add_buf(m_array_t* array, void* s, uint32_t sz);
void* array_get(m_array_t* array, uint32_t index);
void* array_set(m_array_t* array, uint32_t index, void* p);
void* array_tail(m_array_t* array);
void* array_head(m_array_t* array);
void* array_remove(m_array_t* array, uint32_t index);
void array_del(m_array_t* array, uint32_t index, free_func_t fr);
void array_remove_all(m_array_t* array);
void array_clean(m_array_t* array, free_func_t fr);
#define array_tail(array) (((array)->items == NULL || (array)->size == 0) ? NULL: (array)->items[(array)->size-1]);

#endif
