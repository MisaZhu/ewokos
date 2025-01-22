#ifndef ARRAY_H
#define ARRAY_H

#define array_push(array, value)                                              \
    do {                                                                      \
        (array) = array_hold((array), 1, sizeof(*(array)));                   \
        (array)[array_length(array) - 1] = (value);                           \
    } while (0);

void* array_hold(void* array, int count, int item_size);
int array_length(void* array);
void array_free(void* array);

#endif