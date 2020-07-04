#ifndef MSTR_X_H
#define MSTR_X_H

#include <sys/mstr.h>
#include <marray.h>

#ifdef __cplusplus
extern "C" {
#endif

void str_split(const char* str, char c, m_array_t* array);

#ifdef __cplusplus
}
#endif

#endif
