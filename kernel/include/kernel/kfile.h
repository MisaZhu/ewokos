#ifndef KFILE_H
#define KFILE_H

#include <types.h>

typedef struct {
	uint32_t node;
	uint32_t wr;
	uint32_t seek; 
} kfile_t;

#endif
