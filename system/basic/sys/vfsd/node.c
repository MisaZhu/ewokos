/*
 * node.c - node allocation/hash, tree operations and path resolving.
 */
#include "vfsd.h"

vfs_node_t* _vfs_root = NULL;
map_t  _nodes_hash = NULL;
static uint32_t _next_node_id = 1;

static uint32_t vfs_alloc_node_id(void) {
    uint32_t node_id = _next_node_id++;
    if(node_id == 0) {
        node_id = _next_node_id++;
    }
    return node_id;
}

static void vfs_node_init(vfs_node_t* node) {
    memset(node, 0, sizeof(vfs_node_t));
    node->node_id = vfs_alloc_node_id();
    node->fsinfo.node = node->node_id;
    node->mount_id = -1;
    node->pending_umount = 0;
    queue_init(&node->read_wait_queue);
    queue_init(&node->write_wait_queue);
}

uint32_t vfs_get_node_id(vfs_node_t* node) {
    if(node == NULL)
        return 0;
    return node->node_id;
}

/*
 * Hash keys are formatted into a CALLER buffer: the old static buffer was
 * a data race once IPC handlers started running concurrently.
 */
static inline void node_hash_key(uint32_t node_id, char* key) {
    snprintf(key, 17, "%x", (uint32_t)node_id);
}

/* caller must hold _vfs_lock (write) */
vfs_node_t* vfsd_new_node(void) {
    vfs_node_t* ret = (vfs_node_t*)malloc(sizeof(vfs_node_t));
    if(ret == NULL)
        return NULL;
    vfs_node_init(ret);

    /* A node that is not in the hash can never be resolved again by its id,
     * which surfaces much later as a bogus ENOENT on open. */
    char key[17];
    node_hash_key(ret->node_id, key);
    if(hashmap_put(_nodes_hash, key, ret) != MAP_OK) {
        free(ret);
        return NULL;
    }
    return ret;
}

/* caller must hold _vfs_lock (read or write) */
vfs_node_t* vfs_get_node_by_id(uint32_t node_id) {
    vfs_node_t* node = NULL;
    char key[17];
    node_hash_key(node_id, key);
    hashmap_get(_nodes_hash, key, (void**)&node);
    return node;
}

/* caller must hold _vfs_lock (write) */
int32_t vfs_add_node(int32_t pid, vfs_node_t* father, vfs_node_t* node) {
    (void)pid;
    if(father == NULL || node == NULL)
        return -1;

    node->father = father;
    if(father->last_kid == NULL) {
        father->first_kid = node;
    }
    else {
        father->last_kid->next = node;
        node->prev = father->last_kid;
    }
    father->kids_num++;
    father->last_kid = node;
    return 0;
}

/* caller must hold _vfs_lock (write) */
void vfs_remove(int32_t pid, vfs_node_t* node) {
    (void)pid;
    if(node == NULL)
        return;

    vfs_node_t* father = node->father;
    if(father != NULL) {
        if(father->first_kid == node)
            father->first_kid = node->next;
        if(father->last_kid == node)
            father->last_kid = node->prev;
        father->kids_num--;
    }

    if(node->next != NULL)
        node->next->prev = node->prev;

    if(node->prev != NULL)
        node->prev->next = node->next;

    node->prev = NULL;
    node->next = NULL;
}

/* caller must hold _vfs_lock (write) */
int32_t vfsd_del_node(vfs_node_t* node) {
    if(node == NULL || node->refs > 0)
        return -1;
    /*free children*/
    vfs_node_t* c = node->first_kid;
    vfs_node_t* father = node->father;

    while(c != NULL) {
        vfs_node_t* next = c->next;
        vfsd_del_node(c);
        c = next;
    }

    if(father != NULL) {
        if(father->first_kid == node)
            father->first_kid = node->next;
        if(father->last_kid == node)
            father->last_kid = node->prev;
        father->kids_num--;
    }

    if(node->next != NULL)
        node->next->prev = node->prev;
    if(node->prev != NULL)
        node->prev->next = node->next;
    /*
     * NEVER queue_clear(..., free) these queues: their queue_item_t nodes
     * are EMBEDDED in the wait_entry_t slots of the single-malloc
     * _proc_fds_table (see wait_queue_push()), so free() on item or data
     * is an invalid interior-pointer free that corrupts vfsd's heap. This
     * fired whenever a node died with waiters still linked - routine under
     * sshd's per-connection pipe churn, since wakeup_wait_queue() leaves
     * entries queued and KEV-driven exit cleanup lags ~50ms. Detach each
     * entry via wait_queue_pop() (clears waiter->queue, so no dangling
     * pointer into the freed node survives) and wake live owners: a client
     * still blocked on this dying node must re-check and bail out instead
     * of sleeping forever; spurious wakes are level-triggered-safe.
     */
    wait_entry_t* waiter;
    while((waiter = wait_queue_pop(&node->read_wait_queue)) != NULL)
        wakeup_proc(waiter, node, VFS_EVT_CLOSE);
    while((waiter = wait_queue_pop(&node->write_wait_queue)) != NULL)
        wakeup_proc(waiter, node, VFS_EVT_CLOSE);
    char key[17];
    node_hash_key(node->node_id, key);
    hashmap_remove(_nodes_hash, key);
    free(node);
    return 0;
}

/* caller must hold _vfs_lock (write) */
int32_t set_node_info(int32_t pid, vfs_node_t* node, fsinfo_t* info) {
    (void)pid;
    if(node == NULL || info == NULL)
        return -1;
    uint32_t node_id = node->fsinfo.node;
    memcpy(&node->fsinfo, info, sizeof(fsinfo_t));
    node->fsinfo.node = node_id;
    return 0;
}

vfs_node_t* vfs_root(void) {
    return _vfs_root;
}

/*
 * Build the full path of a node into a CALLER buffer: the old static
 * return buffer was a data race between concurrent mount handlers.
 * caller must hold _vfs_lock (read or write).
 */
void vfsd_fullname(vfs_node_t* node, char* out, uint32_t out_sz) {
    str_t* s1 = str_new("");
    while(node != NULL) {
        str_t* s2 = str_new("");
        str_cpy(s2, node->fsinfo.name);
        if(strlen(CS(s1)) != 0) {
            if(node->fsinfo.name[0] != '/')
                str_addc(s2, '/');
            str_add(s2, CS(s1));
        }
        str_cpy(s1, CS(s2));
        str_free(s2);
        node = node->father;
    }

    out[0] = 0;
    strncpy(out, CS(s1), out_sz-1);
    str_free(s1);
}

/* caller must hold _vfs_lock (read or write) */
vfs_node_t* vfs_find_kid_raw(vfs_node_t* father, const char* name) {
    if(father == NULL || name == NULL || strchr(name, '/') != NULL)
        return NULL;

    vfs_node_t* node = father->first_kid;
    while(node != NULL) {
        if(strcmp(node->fsinfo.name, name) == 0)
            return node;
        node = node->next;
    }
    return NULL;
}

/*
 * Resolve an absolute/relative path to a node id. Manages _vfs_lock
 * itself: each component lookup is a short locked section; the (possibly
 * sleeping) kids-loading step runs unlocked in between. A component whose
 * directory vanished mid-walk resolves as "not found".
 * Returns true with *node_id_out set (possibly 0 for "not found") when the
 * walk completed.
 */
bool vfs_resolve_path(const char* name, uint32_t* node_id_out) {
    uint32_t cur_id = 0;

    if(node_id_out == NULL)
        return false;
    *node_id_out = 0;
    if(name == NULL)
        return false;

    pthread_rwlock_rdlock(&_vfs_lock);
    vfs_node_t* cur = _vfs_root;
    if(name[0] == '/') {
        /*go to root*/
        while(cur->father != NULL)
            cur = cur->father;
        name = name+1;
        if(name[0] == 0) {
            *node_id_out = vfs_get_node_id(cur);
            pthread_rwlock_unlock(&_vfs_lock);
            return true;
        }
    }
    cur_id = vfs_get_node_id(cur);
    pthread_rwlock_unlock(&_vfs_lock);

    char n[FS_FULL_NAME_MAX+1];
    int32_t j = 0;
    for(int32_t i=0; i<FS_FULL_NAME_MAX; i++) {
        n[i] = name[i];
        if(n[i] != 0 && n[i] != '/')
            continue;

        bool is_end = (n[i] == 0);
        n[i] = 0;

        /* make sure the current dir's kids are loaded (sleeps, unlocked) */
        vfs_ensure_kids_loaded(cur_id);

        uint32_t kid_id = 0;
        pthread_rwlock_rdlock(&_vfs_lock);
        cur = vfs_get_node_by_id(cur_id);
        if(cur != NULL) {
            vfs_node_t* kid = vfs_find_kid_raw(cur, n+j);
            if(kid != NULL)
                kid_id = vfs_get_node_id(kid);
        }
        pthread_rwlock_unlock(&_vfs_lock);

        if(is_end) {
            *node_id_out = kid_id;
            return true;
        }
        /* separator */
        if(name[i+1] == 0) { //trailing slash: return what was found
            *node_id_out = kid_id;
            return true;
        }
        if(kid_id == 0)
            return false;
        cur_id = kid_id;
        j = i+1;
    }
    return false;
}

/*
 * Permission check for accessing 'info' as process 'pid'. This is a local
 * copy of the libewoksys client helper (same logic); vfsd must not pull
 * libewoksys's vfs.o member, which defines client stubs whose names clash
 * with vfsd's internal functions.
 */
int vfsd_check_access(int pid, fsinfo_t* info, int mode) {
    procinfo_t procinfo;
    if(info == NULL || proc_info(pid, &procinfo) != 0)
        return -1;

    if(procinfo.uid <= 0) {
        if(mode == X_OK && (info->stat.mode & 0111) == 0)
            return -1;
        return 0;
    }

    int ucheck = 0400;
    int gcheck = 040;
    int acheck = 04;
    if(mode == R_OK) {
        ucheck = 0400;
        gcheck = 040;
        acheck = 04;
    }
    else if(mode == W_OK) {
        ucheck = 0200;
        gcheck = 020;
        acheck = 02;
    }
    else if(mode == X_OK) {
        ucheck = 0100;
        gcheck = 010;
        acheck = 01;
    }

    if(procinfo.uid == info->stat.uid) {
        if((info->stat.mode & ucheck) != 0)
            return 0;
    }
    else if(procinfo.gid == info->stat.gid) {
        if((info->stat.mode & gcheck) != 0)
            return 0;
    }
    else {
        if((info->stat.mode & acheck) != 0)
            return 0;
    }
    return -1;
}
