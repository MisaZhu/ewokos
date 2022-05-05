#ifndef KCONSOLE_H
#define KCONSOLE_H

#include <graph/graph.h>

void kconsole_init(void);
void kconsole_input(const char* s);
void kconsole_close(void);

extern graph_t* _fb_g;

#endif