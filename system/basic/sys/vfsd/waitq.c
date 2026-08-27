/*
 * waitq.c - wait queues and node event wakeup.
 */
#include "vfsd.h"

/* caller must hold _vfs_lock (write) */
void wakeup_proc(wait_entry_t* waiter, vfs_node_t* node, int32_t events) {
    (void)events;
    if(waiter == NULL)
        return;
    if(waiter->pid < 0 || (uint32_t)waiter->pid >= _max_proc_table_num)
        return;
    if(proc_check_uuid(waiter->pid, waiter->uuid) == 0)
        return;
    proc_wakeup_by(waiter->pid, vfs_get_node_id(node));
}

/* caller must hold _vfs_lock (read or write) */
wait_entry_t* get_wait_entry(int32_t pid, bool wr) {
    if(pid < 0 || (uint32_t)pid >= _max_proc_table_num)
        return NULL;
    if(wr)
        return &_proc_fds_table[pid].write_waiter;
    return &_proc_fds_table[pid].read_waiter;
}

/* caller must hold _vfs_lock (read or write) */
int32_t get_tracked_owner_pid(int32_t pid) {
    if(pid < 0 || (uint32_t)pid >= _max_proc_table_num)
        return pid;
    int32_t owner = _proc_fds_table[pid].owner_pid;
    if(owner <= 0)
        owner = vfs_fd_owner_pid(pid);
    return owner;
}

/* caller must hold _vfs_lock (write) */
static void wait_queue_detach(queue_t* q, wait_entry_t* waiter) {
    queue_item_t* it;
    bool linked;

    if(q == NULL || waiter == NULL)
        return;
    if(waiter->queue != q)
        return;

    it = &waiter->item;
    linked = (q->head == it) || (q->tail == it) ||
        (it->prev != NULL) || (it->next != NULL);
    if(!linked) {
        waiter->queue = NULL;
        waiter->node_id = 0;
        memset(it, 0, sizeof(queue_item_t));
        return;
    }

    if(it->next != NULL)
        it->next->prev = it->prev;
    if(it->prev != NULL)
        it->prev->next = it->next;

    if(q->head == it)
        q->head = it->next;
    if(q->tail == it)
        q->tail = it->prev;
    if(q->num > 0)
        q->num--;

    memset(it, 0, sizeof(queue_item_t));
    waiter->queue = NULL;
    waiter->node_id = 0;
}

/* caller must hold _vfs_lock (write) */
void wait_queue_remove_entry(wait_entry_t* waiter) {
    if(waiter == NULL || waiter->queue == NULL)
        return;
    wait_queue_detach(waiter->queue, waiter);
}

/* caller must hold _vfs_lock (write) */
wait_entry_t* wait_queue_pop(queue_t* q) {
    if(q == NULL || q->head == NULL)
        return NULL;

    wait_entry_t* waiter = (wait_entry_t*)q->head->data;
    if(waiter == NULL)
        return NULL;
    wait_queue_detach(q, waiter);
    return waiter;
}

/* caller must hold _vfs_lock (write) */
static void wait_queue_push(queue_t* q, wait_entry_t* waiter, uint32_t node_id) {
    queue_item_t* it;

    if(q == NULL || waiter == NULL)
        return;

    if(waiter->queue == q && waiter->node_id == node_id)
        return;
    if(waiter->queue != NULL)
        wait_queue_remove_entry(waiter);

    it = &waiter->item;
    memset(it, 0, sizeof(queue_item_t));
    it->data = waiter;

    if(q->tail == NULL) {
        q->head = q->tail = it;
    }
    else {
        q->tail->next = it;
        it->prev = q->tail;
        q->tail = it;
    }
    q->num++;
    waiter->queue = q;
    waiter->node_id = node_id;
}

/* caller must hold _vfs_lock (write) */
static void wakeup_wait_queue(queue_t* q, vfs_node_t* node, int32_t events) {
    if(q == NULL)
        return;

    /*
     * Iterate the queue and wake each registered process WITHOUT removing
     * it. Popping entries here creates a race on SMP: the wakeup can be
     * consumed by an unrelated block (e.g. IPC-return wait with token=0)
     * before the process reaches its intended proc_block_by(node). If the
     * entry was already popped, the process ends up blocked with no waiter
     * on any queue and no pending sticky wake – hung forever.
     *
     * Leaving entries in place is safe: the process always calls
     * vfs_unblock() to remove itself once it observes the event, and
     * wait_queue_push() short-circuits if the entry is already present.
     */
    queue_item_t* it = q->head;
    while(it != NULL) {
        wait_entry_t* waiter = (wait_entry_t*)it->data;
        it = it->next;
        if(waiter != NULL)
            wakeup_proc(waiter, node, events);
    }
}

/* caller must hold _vfs_lock (write) */
void enqueue_waiter(queue_t* q, int32_t pid, uint32_t uuid, bool wr, uint32_t node_id) {
    wait_entry_t* waiter = get_wait_entry(pid, wr);
    if(q == NULL || waiter == NULL)
        return;

    /*
     * VFS_BLOCK is on the shell hot path. Reuse the per-proc waiter instead of
     * malloc()ing a fresh wait record plus queue node for every block/unblock
     * round-trip; allocator churn here shows up as a visible IPC timeout.
     */
    waiter->pid = pid;
    waiter->uuid = uuid;
    wait_queue_push(q, waiter, node_id);
}

/* caller must hold _vfs_lock (write) */
void do_node_wakeup(vfs_node_t* node, int events) {
    if(node == NULL)
        return;

    node->events |= events;

    queue_t* qr = NULL, *qw = NULL;
    if((events & VFS_EVT_RD) != 0 ||
            events == VFS_EVT_CLOSE || events == VFS_EVT_ERR || events == VFS_EVT_NVAL)
        qr = &node->read_wait_queue;
    if((events & VFS_EVT_WR) != 0 ||
            events == VFS_EVT_CLOSE || events == VFS_EVT_ERR || events == VFS_EVT_NVAL)
        qw = &node->write_wait_queue;

    /*
     * Duplicated/fork-inherited descriptors can leave multiple processes
     * blocked on the same node event. Waking only one waiter lets it consume
     * the sticky edge and can strand sibling waiters forever.
     */
    wakeup_wait_queue(qr, node, events);
    wakeup_wait_queue(qw, node, events);
}
