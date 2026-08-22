/*shared-memory helpers: allocation of server-side shm segments and
  teardown of graph/shm pairs attached to a window*/
#include <sys/shm.h>
#include "xshm.h"

void release_graph_shm(graph_t** g, void** shm, int32_t* shm_id) {
    if(g != NULL && *g != NULL) {
        graph_free(*g);
        *g = NULL;
    }

    if(shm != NULL && *shm != NULL) {
        shmdt(*shm);
        *shm = NULL;
    }

    if(shm_id != NULL && *shm_id != -1) {
        *shm_id = -1;
    }
}

static uint32_t _xserver_shm_seq = 1;

int32_t xserver_alloc_shm(uint32_t salt, int32_t size, int32_t flag, key_t* out_key) {
    for(uint32_t i = 0; i < 16; i++) {
        uint32_t seq = _xserver_shm_seq++;
        key_t key = (key_t)((salt * 2654435761u) ^ (seq * 2246822519u));
        if(key == 0)
            key = (key_t)(seq | 1u);

        int32_t shm_id = shmget(key, size, flag);
        if(shm_id != -1) {
            if(out_key != NULL)
                *out_key = key;
            return shm_id;
        }
    }
    if(out_key != NULL)
        *out_key = 0;
    return -1;
}
