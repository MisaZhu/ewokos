#ifndef XSHM_H
#define XSHM_H

#include <stdint.h>
#include <ewoksys/ipc.h> //key_t
#include <graph/graph.h>

void release_graph_shm(graph_t** g, void** shm, int32_t* shm_id);
int32_t xserver_alloc_shm(uint32_t salt, int32_t size, int32_t flag, key_t* out_key);

#endif
