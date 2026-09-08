#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>

#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>
#include <ewoksys/kernel_tic.h>

#include "task.h"
#include "netd.h"
#include "stack/net.h"
#include "stack/util.h"

/*
 * stack/sock.c entry points (signatures match stack/sock.h; that header is
 * not included here because netd.h already pulls in sys/socket.h, which
 * provides the conflicting SOCK_xxx and sockaddr definitions the wire
 * protocol uses).
 */
extern int sock_open(int domain, int type, int protocol);
extern int sock_close(int id);
extern ssize_t sock_recvfrom(int id, void *buf, size_t n, struct sockaddr *addr, int *addrlen);
extern ssize_t sock_sendto(int id, const void *buf, size_t n, const struct sockaddr *addr, int addrlen);
extern int sock_bind(int id, const struct sockaddr *addr, int addrlen);
extern int sock_listen(int id, int backlog);
extern int sock_accept(int id, struct sockaddr *addr, int *addrlen);
extern int sock_connect(int id, const struct sockaddr *addr, int addrlen);
extern ssize_t sock_recv(int id, void *buf, size_t n);
extern ssize_t sock_send(int id, const void *buf, size_t n);
extern int sock_setsockopt(int id, int level, int optname, const void *optval, int optlen);
extern int sock_getsockopt(int id, int level, int optname, void *optval, int *optlen);
extern int sock_readable(int id);
extern int sock_poll_readable(int id);
extern int sock_poll_writable(int id);
extern int sock_connect_pending(int id);
extern int sock_get_rcvtimeo(int sock, struct timeval *timeout);
extern int sock_id_from_tcp_desc(int tcp_desc);
extern int sock_id_from_udp_desc(int udp_desc);
extern void sock_init_maps(void);

/* Clamp for the per-request recv scratch buffer: matches the 32KB TCP
 * receive window, so one SOCK_RECV drains a full window in one IPC. */
#define TASK_IO_BUF_SIZE (1024 * 32)
/* How often armed recv()/recvfrom() deadlines are swept. */
#define TASK_TIMEOUT_TICK_US 200000
/* Bounded drain of concurrent handlers before a socket id is closed. */
#define TASK_DRAIN_POLL_US   1000
#define TASK_DRAIN_POLL_MAX  1000

/*
 * Wall clock used for the SO_RCVTIMEO deadlines. The stack uses the same
 * source (kernel_tic) so the two never disagree.
 */
static void task_now(struct timeval *tv) {
    uint64_t usec = 0;
    kernel_tic(NULL, &usec);
    tv->tv_sec = usec / 1000000;
    tv->tv_usec = usec % 1000000;
}

static int task_deadline_expired(const struct timeval *deadline) {
    struct timeval now;

    task_now(&now);
    if (now.tv_sec != deadline->tv_sec)
        return now.tv_sec > deadline->tv_sec;
    return now.tv_usec >= deadline->tv_usec;
}

pthread_mutex_t task_list_lock;
net_task_t *task_list = NULL;

#ifndef SOCKS_MAX
#define SOCKS_MAX 128
#endif
static net_task_t* sock_to_task[SOCKS_MAX];
static uint32_t task_total_created = 0;
static uint32_t task_total_freed = 0;
static uint32_t task_active_count = 0;

/*
 * Deferred VFS wakeup delivery.
 *
 * vfs_wakeup() is a blocking reverse IPC into vfsd. The stack-side wakeup
 * paths (tcp_segment_arrives()/tcp_timer()/tcp_pcb_release() and the UDP
 * input path) run WHILE HOLDING the global stack mutex. If that IPC has to
 * wait (vfsd busy, or vfsd itself blocked in an ipc_call into netd, e.g. a
 * clear_zombie FS_CMD_CLOSE), the stack mutex stays held for the whole wait.
 * pthread_mutex_lock() is a try+yield spin (semaphore_enter), so every other
 * netd thread that touches the stack then burns CPU in "rdy": netd stops
 * answering IPC, and vfsd's call into netd never completes either -- a
 * netd<->vfsd livelock. Queue the wakeup here instead; the protocol loop
 * (intr_step() on the main thread) issues the IPC via task_flush_wakeups()
 * with no netd lock held, right after each packet/timer round. Entries
 * coalesce per VFS node, which also rate-limits per-segment wakeup storms
 * into single edges.
 */
#define WAKEUP_QUEUE_MAX 64
typedef struct {
    uint32_t node;
    uint32_t events;
} pending_wakeup_t;
static pending_wakeup_t wakeup_queue[WAKEUP_QUEUE_MAX];
static uint32_t wakeup_queue_num = 0;
static pthread_mutex_t wakeup_queue_lock;

static void task_queue_vfs_wakeup(uint32_t node, uint32_t events) {
    if (node == 0 || events == 0)
        return;
    pthread_mutex_lock(&wakeup_queue_lock);
    uint32_t i;
    for (i = 0; i < wakeup_queue_num; i++) {
        if (wakeup_queue[i].node == node) {
            wakeup_queue[i].events |= events;
            break;
        }
    }
    if (i == wakeup_queue_num) {
        if (wakeup_queue_num < WAKEUP_QUEUE_MAX) {
            wakeup_queue[i].node = node;
            wakeup_queue[i].events = events;
            wakeup_queue_num++;
        } else {
            /* Unreachable in practice: entries coalesce per node. Merge
             * rather than drop. */
            wakeup_queue[0].events |= events;
        }
    }
    pthread_mutex_unlock(&wakeup_queue_lock);
}

/*
 * Called by intr_step() (main thread) once per protocol round, outside the
 * stack mutex and every other netd lock, so the blocking vfs_wakeup() IPC
 * can never wedge the stack. Wakeups queued by the round itself are
 * delivered before the loop sleeps; wakeups queued by pool workers
 * (e.g. the SO_RCVTIMEO sweeper) ride the next round.
 */
void task_flush_wakeups(void) {
    pending_wakeup_t batch[WAKEUP_QUEUE_MAX];
    uint32_t num;

    pthread_mutex_lock(&wakeup_queue_lock);
    num = wakeup_queue_num;
    if (num > 0) {
        memcpy(batch, wakeup_queue, num * sizeof(pending_wakeup_t));
        wakeup_queue_num = 0;
    }
    pthread_mutex_unlock(&wakeup_queue_lock);
    for (uint32_t i = 0; i < num; i++) {
        vfs_wakeup(batch[i].node, (int)batch[i].events);
    }
}

static int task_owner_pid(int from_pid) {
    int owner_pid = proc_getpid(from_pid);

    if (owner_pid > 0)
        return owner_pid;
    return from_pid;
}

static void task_list_add(net_task_t *task) {
    pthread_mutex_lock(&task_list_lock);
    task->prev = NULL;
    if (task_list == NULL) {
        task_list = task;
        task->next = NULL;
    } else {
        net_task_t *t = task_list;

        while (t->next != NULL) {
            t = t->next;
        }
        t->next = task;
        task->prev = t;
        task->next = NULL;
    }
    pthread_mutex_unlock(&task_list_lock);
}

static void task_list_remove(net_task_t *task) {
    if (task == NULL)
        return;

    pthread_mutex_lock(&task_list_lock);
    if (task == task_list)
        task_list = task->next;

    if (task->prev)
        task->prev->next = task->next;
    if (task->next)
        task->next->prev = task->prev;
    task->next = NULL;
    task->prev = NULL;
    pthread_mutex_unlock(&task_list_lock);
}

void start_task(void) {
    /*
     * Initialize the O(1) per-connection wakeup maps: sock_to_task[] here
     * and the desc-to-sock reverse maps inside the stack.
     */
    memset(sock_to_task, 0, sizeof(sock_to_task));
    sock_init_maps();

    memset(wakeup_queue, 0, sizeof(wakeup_queue));
    wakeup_queue_num = 0;
    /*
     * Explicit init before the first enqueue: the lazy semaphore_alloc in
     * pthread_mutex_lock() races when two threads hit the first lock at once.
     */
    pthread_mutex_init(&wakeup_queue_lock, NULL);

    /*
     * SO_RCVTIMEO sweeper. Must be registered before net_run(); start_task()
     * runs before setup() in main(), so this is the right place.
     */
    struct timeval timeout_interval = {0, TASK_TIMEOUT_TICK_US};
    if (net_timer_register("TASK Timeout", timeout_interval, task_timeout_check) == -1) {
        slog("netd: task timeout timer register failed\n");
    }
}

net_task_t *task_find_live_by_node(uint32_t node) {
    /*
     * Resolve a still-LIVE task by its VFS node. Anonymous /dev/net0 opens
     * each get a unique, monotonically allocated node id from vfsd, and
     * release_task() removes the task from task_list (under task_list_lock)
     * BEFORE destroying its lock and freeing it, so a hit here is guaranteed
     * valid. The task's own lock is taken before the list lock is dropped:
     * the task can leave the list the instant we release task_list_lock, but
     * it cannot be freed while task->lock is held (release_task() drains the
     * inflight count under that lock first).
     *
     * Never resolve tasks through fsinfo.data: a duplicated/stale FS_CMD_CLOSE
     * re-seeds that blob with a pointer to an already-freed task.
     */
    if (node == 0)
        return NULL;

    pthread_mutex_lock(&task_list_lock);
    net_task_t *t = task_list;
    while (t != NULL) {
        if ((uint32_t)t->node == node) {
            if (pthread_mutex_lock(&t->lock) != 0) {
                pthread_mutex_unlock(&task_list_lock);
                return NULL;
            }
            pthread_mutex_unlock(&task_list_lock);
            return t;
        }
        t = t->next;
    }
    pthread_mutex_unlock(&task_list_lock);
    return NULL;
}

net_task_t *create_task(int fd, int from_pid, int node) {
    net_task_t *task = malloc(sizeof(net_task_t));
    if (task == NULL) {
        errno = ENOMEM;
        slog("netd: create_task malloc failed fd=%d from_pid=%d node=%d active=%u created=%u freed=%u\n",
                fd, from_pid, node, task_active_count, task_total_created,
                task_total_freed);
        return NULL;
    }
    memset(task, 0, sizeof(net_task_t));
    pthread_mutex_init(&task->lock, NULL);
    task->fd = fd;
    task->node = node;
    task->from_pid = task_owner_pid(from_pid);
    task->sock = -1;
    task->refs = 1;
    task->inflight = 0;
    task->fin_sock = -1;
    task->closing = false;
    task->dead = false;
    task->is_listener = false;
    task_list_add(task);
    pthread_mutex_lock(&task_list_lock);
    task_total_created++;
    task_active_count++;
    pthread_mutex_unlock(&task_list_lock);
    return task;
}

/*
 * Final accounting + memory release. Callers must guarantee nothing can
 * still reach @task: off task_list, not in sock_to_task[], and no inflight
 * operation left (or handed off to the last task_end_op()).
 */
static void task_destroy(net_task_t *task) {
    pthread_mutex_lock(&task_list_lock);
    if (task_active_count > 0)
        task_active_count--;
    task_total_freed++;
    pthread_mutex_unlock(&task_list_lock);
    pthread_mutex_destroy(&task->lock);
    free(task);
}

/*
 * Enter an operation on the task identified by @node: resolve it, refuse
 * closing tasks, bump the inflight count and snapshot the bound socket id.
 * Returns with task->lock RELEASED; the inflight count keeps the task alive
 * (release_task()/SOCK_CLOSE drain it before freeing/closing). Every
 * successful call must be paired with task_end_op().
 */
static net_task_t *task_begin_op(uint32_t node, int *sock) {
    net_task_t *task = task_find_live_by_node(node);
    if (task == NULL)
        return NULL;
    if (task->closing) {
        pthread_mutex_unlock(&task->lock);
        return NULL;
    }
    task->inflight++;
    if (sock != NULL)
        *sock = task->sock;
    pthread_mutex_unlock(&task->lock);
    return task;
}

static void task_end_op(net_task_t *task) {
    pthread_mutex_lock(&task->lock);
    task->inflight--;
    /*
     * Zombie handoff (see release_task()): the teardown drain timed out
     * because an operation was stalled -- typically task_write() riding out
     * wl0 TX backpressure inside net_tx_flush() at sshd-transfer end. The
     * LAST op out owns the deferred close and free. release_task() freeing
     * the task and destroying task->lock while this thread still held the
     * pointer was a use-after-free on a destroyed mutex: the reused
     * semaphore/heap block corrupted live locks and wedged or crashed netd
     * right after the transfer's close.
     */
    bool last = (task->dead && task->inflight == 0);
    int fin_sock = last ? task->fin_sock : -1;
    pthread_mutex_unlock(&task->lock);
    if (!last)
        return;
    /* Outside task->lock: sock_close() takes the stack mutex and the lock
     * order is stack mutex -> task_list_lock -> task->lock. */
    if (fin_sock >= 0)
        sock_close(fin_sock);
    task_destroy(task);
}

/*
 * Bind @task to @sock, updating the O(1) sock_to_task wakeup map. The map
 * and the binding must change atomically (lock order: task_list_lock ->
 * task->lock) or a stack wakeup could route through a stale entry.
 */
static void task_bind_sock(net_task_t *task, int sock) {
    pthread_mutex_lock(&task_list_lock);
    pthread_mutex_lock(&task->lock);
    if (task->sock >= 0 && task->sock < SOCKS_MAX &&
            sock_to_task[task->sock] == task)
        sock_to_task[task->sock] = NULL;
    task->sock = sock;
    task->is_listener = false;
    task->rcv_deadline_set = false;
    task->rcv_timeout_pending = false;
    if (sock >= 0 && sock < SOCKS_MAX)
        sock_to_task[sock] = task;
    pthread_mutex_unlock(&task->lock);
    pthread_mutex_unlock(&task_list_lock);
}

/*
 * Detach and return the bound socket id (or -1). Leaves the task unbound so
 * no later operation can reach the (soon closed) id, and a reused stack slot
 * can never be double-closed through this task.
 */
static int task_detach_sock(net_task_t *task) {
    int sock;

    pthread_mutex_lock(&task_list_lock);
    pthread_mutex_lock(&task->lock);
    sock = task->sock;
    task->sock = -1;
    task->is_listener = false;
    task->rcv_deadline_set = false;
    if (sock >= 0 && sock < SOCKS_MAX && sock_to_task[sock] == task)
        sock_to_task[sock] = NULL;
    pthread_mutex_unlock(&task->lock);
    pthread_mutex_unlock(&task_list_lock);
    return sock;
}

/*
 * Wait (bounded) until no handler beyond the calling one(s) still operates
 * on @task. Must run before sock_close(): a concurrent recv/send holds a
 * snapshot of the old socket id, and closing it early would let the stack
 * hand that id to an unrelated new connection mid-operation.
 */
static void task_wait_other_ops(net_task_t *task, int self_ops) {
    for (int i = 0; i < TASK_DRAIN_POLL_MAX; i++) {
        pthread_mutex_lock(&task->lock);
        int busy = task->inflight > self_ops;
        pthread_mutex_unlock(&task->lock);
        if (!busy)
            return;
        proc_usleep(TASK_DRAIN_POLL_US);
    }
    slog("netd: task node=%d close drain timed out\n", task->node);
}

/*
 * SO_RCVTIMEO handling for blocking recv()/recvfrom().
 *
 * The client parks in vfs_block_by_fd(VFS_EVT_RD) after a VFS_ERR_RETRY, and
 * vfs_get_poll_events() REPLACES the sticky RD bit with live dev_poll state,
 * so the timeout cannot be delivered as a plain wakeup edge: the retried
 * request itself must observe it. task_timeout_check() latches
 * rcv_timeout_pending (reported as RD by task_poll_events()); the retried
 * recv consumes the latch here and completes with ETIMEDOUT.
 *
 * Returns true when the current recv attempt must fail with ETIMEDOUT;
 * otherwise (re)arms the deadline for the sweeper. sock_get_rcvtimeo() is
 * called before task->lock on purpose: leaf stack state must never be read
 * while holding a task lock the timer path also takes.
 */
static bool task_recv_should_timeout(net_task_t *task, int sock) {
    struct timeval timeout;
    bool expire = false;
    bool has_timeout = (sock >= 0 && sock_get_rcvtimeo(sock, &timeout) == 0);

    pthread_mutex_lock(&task->lock);
    if (task->rcv_timeout_pending) {
        task->rcv_timeout_pending = false;
        task->rcv_deadline_set = false;
        expire = true;
    } else if (task->rcv_deadline_set && task_deadline_expired(&task->rcv_deadline)) {
        task->rcv_deadline_set = false;
        expire = true;
    } else if (has_timeout && !task->rcv_deadline_set) {
        task_now(&task->rcv_deadline);
        task->rcv_deadline.tv_sec += timeout.tv_sec;
        task->rcv_deadline.tv_usec += timeout.tv_usec;
        if (task->rcv_deadline.tv_usec >= 1000000) {
            task->rcv_deadline.tv_sec += task->rcv_deadline.tv_usec / 1000000;
            task->rcv_deadline.tv_usec %= 1000000;
        }
        task->rcv_deadline_set = true;
    }
    pthread_mutex_unlock(&task->lock);
    return expire;
}

/* A recv()/recvfrom() completed (data, EOF or hard error): disarm. */
static void task_recv_done(net_task_t *task) {
    pthread_mutex_lock(&task->lock);
    task->rcv_deadline_set = false;
    task->rcv_timeout_pending = false;
    pthread_mutex_unlock(&task->lock);
}

/*
 * Socket fcntl commands, executed inline on the calling IPC pool worker.
 * Blocking semantics: a command that cannot complete yet returns
 * VFS_ERR_RETRY with @out untouched; the client library then parks in
 * vfs_block_by_fd() (RD for accept/recv/recvfrom, WR for connect/send) until
 * a stack wakeup or the timeout sweeper raises the matching edge.
 *
 * The reply payload layouts are part of the wire protocol with
 * libs/socket/src/socket.c and must not change.
 */
int task_cntl(uint32_t node, int from_pid, int cmd, proto_t *in, proto_t *out) {
    (void)from_pid;
    int sock = -1;
    net_task_t *task = task_begin_op(node, &sock);
    if (task == NULL)
        return -1;

    int res = 0;
    int ret = -1;
    int sock_errno = 0;
    int32_t size, optlen;
    int32_t addrlen = sizeof(struct sockaddr);
    struct sockaddr addr;
    struct sockaddr *paddr;
    char *data, *optval;
    char optbuf[64];
    char *buf;

    switch (cmd) {
    case SOCK_OPEN: {
        int domain = proto_read_int(in);
        int type = proto_read_int(in);
        int protocol = proto_read_int(in);
        sock = sock_open(domain, type, protocol);
        task_bind_sock(task, sock);
        PF->addi(out, sock);
        break;
    }
    case SOCK_LINK:
        /* Bind this (freshly opened) vfs node to an existing socket id:
         * used to wrap the connection returned by accept(). */
        sock = proto_read_int(in);
        task_bind_sock(task, sock);
        PF->addi(out, 0);
        break;
    case SOCK_BIND:
        paddr = (struct sockaddr*)proto_read(in, &addrlen);
        ret = (paddr == NULL) ? -1 : sock_bind(sock, paddr, addrlen);
        PF->addi(out, ret);
        break;
    case SOCK_LISTEN:
        size = proto_read_int(in);
        ret = sock_listen(sock, size);
        if (ret == 0) {
            pthread_mutex_lock(&task->lock);
            task->is_listener = true;
            pthread_mutex_unlock(&task->lock);
        }
        PF->addi(out, ret);
        break;
    case SOCK_ACCEPT:
        if (sock < 0) {
            PF->addi(out, -1);
            PF->addi(out, EBADF);
            break;
        }
        /* Never park a pool worker inside the stack: report RETRY and let
         * the backlog wakeup (VFS_EVT_RD) restart the blocked client. */
        if (!sock_readable(sock)) {
            res = VFS_ERR_RETRY;
            break;
        }
        errno = 0;
        ret = sock_accept(sock, &addr, &addrlen);
        sock_errno = errno;
        if (ret < 0 && sock_errno == 0)
            sock_errno = EAGAIN;
        if (ret < 0 && (sock_errno == EAGAIN || sock_errno == EINTR)) {
            /* Raced with a concurrent accept() on the same listener. */
            res = VFS_ERR_RETRY;
            break;
        }
        PF->addi(out, ret);
        if (ret >= 0)
            PF->add(out, &addr, addrlen);
        PF->addi(out, ret < 0 ? sock_errno : 0);
        break;
    case SOCK_CONNECT:
        if (sock < 0) {
            PF->addi(out, -1);
            PF->addi(out, EBADF);
            break;
        }
        paddr = (struct sockaddr*)proto_read(in, &addrlen);
        if (paddr == NULL) {
            PF->addi(out, -1);
            PF->addi(out, EINVAL);
            break;
        }
        /* sock_connect() is re-entrant: EAGAIN while the SYN handshake is
         * in flight, 0 once ESTABLISHED. The client blocks on VFS_EVT_WR;
         * task_poll_events() suppresses WR until the handshake resolves. */
        errno = 0;
        ret = sock_connect(sock, paddr, addrlen);
        sock_errno = errno;
        if (ret < 0 && sock_errno == 0)
            sock_errno = EAGAIN;
        if (ret < 0 && (sock_errno == EAGAIN || sock_errno == EINTR)) {
            res = VFS_ERR_RETRY;
            break;
        }
        PF->addi(out, ret);
        PF->addi(out, ret < 0 ? sock_errno : 0);
        break;
    case SOCK_SEND:
        if (sock < 0) {
            PF->addi(out, -1);
            PF->addi(out, EBADF);
            break;
        }
        data = proto_read(in, &size);
        if (data != NULL && size > 0) {
            errno = 0;
            ret = sock_send(sock, data, size);
            sock_errno = errno;
            if (ret < 0 && sock_errno == 0)
                sock_errno = EAGAIN;
            if (ret < 0 && (sock_errno == EAGAIN || sock_errno == EINTR)) {
                /* Send window full: the ACK-driven WR wakeup re-arms us. */
                res = VFS_ERR_RETRY;
                break;
            }
        } else {
            /* Nothing to send: report 0 bytes written, not a phantom -1
             * that the client would treat as EAGAIN and retry forever. */
            ret = 0;
            sock_errno = 0;
        }
        PF->addi(out, ret);
        PF->addi(out, ret < 0 ? sock_errno : 0);
        break;
    case SOCK_SENDTO:
        /* Datagram sends never re-arm: errors go straight back. */
        if (sock < 0) {
            PF->addi(out, -1);
            PF->addi(out, EBADF);
            break;
        }
        data = proto_read(in, &size);
        paddr = (struct sockaddr*)proto_read(in, &addrlen);
        if (data == NULL || paddr == NULL) {
            ret = -1;
            sock_errno = EINVAL;
        } else {
            errno = 0;
            ret = sock_sendto(sock, data, size, paddr, addrlen);
            sock_errno = errno;
            if (ret < 0 && sock_errno == 0)
                sock_errno = (ret == -17) ? EBADF : EIO;
        }
        PF->addi(out, ret);
        PF->addi(out, ret < 0 ? sock_errno : 0);
        break;
    case SOCK_RECV:
    case SOCK_RECVFROM:
        if (sock < 0) {
            PF->addi(out, -1);
            PF->addi(out, EBADF);
            break;
        }
        if (!sock_readable(sock)) {
            if (task_recv_should_timeout(task, sock)) {
                PF->addi(out, -1);
                PF->addi(out, ETIMEDOUT);
                break;
            }
            res = VFS_ERR_RETRY;
            break;
        }
        size = proto_read_int(in);
        if (size > TASK_IO_BUF_SIZE)
            size = TASK_IO_BUF_SIZE;
        if (size < 0)
            size = 0;
        buf = malloc(size > 0 ? size : 1);
        if (buf == NULL) {
            PF->addi(out, -1);
            PF->addi(out, ENOMEM);
            break;
        }
        errno = 0;
        if (cmd == SOCK_RECV)
            ret = sock_recv(sock, buf, size);
        else
            ret = sock_recvfrom(sock, buf, size, &addr, &addrlen);
        sock_errno = errno;
        if (ret < 0 && sock_errno == 0)
            sock_errno = EAGAIN;
        if (ret < 0 && (sock_errno == EAGAIN || sock_errno == EINTR)) {
            /* Raced with another reader draining the data. */
            free(buf);
            if (task_recv_should_timeout(task, sock)) {
                PF->addi(out, -1);
                PF->addi(out, ETIMEDOUT);
                break;
            }
            res = VFS_ERR_RETRY;
            break;
        }
        task_recv_done(task);
        /* Same non-blocking window-update ACK flush as task_read(); do it
         * before building the reply so the ACK reaches the wire ahead of our
         * own IPC return. */
        if (ret > 0)
            net_tx_tryflush();
        PF->addi(out, ret);
        if (ret > 0) {
            if (cmd == SOCK_RECVFROM) {
                PF->addi(out, addrlen);
                PF->add(out, buf, ret);
                PF->add(out, &addr, addrlen);
            } else {
                PF->add(out, buf, ret);
            }
        }
        PF->addi(out, ret < 0 ? sock_errno : 0);
        free(buf);
        break;
    case SOCK_SETOPT: {
        int level = proto_read_int(in);
        int optname = proto_read_int(in);
        optval = proto_read(in, &optlen);
        ret = (optval == NULL) ? -1 : sock_setsockopt(sock, level, optname, optval, optlen);
        PF->addi(out, ret);
        break;
    }
    case SOCK_GETOPT: {
        int level = proto_read_int(in);
        int optname = proto_read_int(in);
        optlen = proto_read_int(in);
        if (optlen > (int32_t)sizeof(optbuf))
            optlen = sizeof(optbuf);
        ret = sock_getsockopt(sock, level, optname, optbuf, &optlen);
        PF->addi(out, ret);
        if (ret == 0) {
            PF->addi(out, optlen);
            PF->add(out, optbuf, optlen);
        }
        break;
    }
    case SOCK_CLOSE: {
        /*
         * shutdown()-style close of the bound socket while the vfs node
         * stays open. Detach first so no new operation can reach the id,
         * then drain concurrent handlers (they snapshotted the old id)
         * before the id returns to the stack's free pool.
         */
        int old = task_detach_sock(task);
        if (old < 0) {
            ret = -17;
        } else {
            task_wait_other_ops(task, 1);
            ret = sock_close(old);
        }
        PF->addi(out, ret);
        break;
    }
    default:
        PF->addi(out, -1);
        break;
    }

    task_end_op(task);
    return res;
}

int task_read(uint32_t node, char* buf, int size) {
    int sock = -1;
    net_task_t *task = task_begin_op(node, &sock);
    if (task == NULL) {
        errno = EBADF;
        return -1;
    }

    int ret;
    if (sock < 0) {
        errno = EBADF;
        ret = -1;
    } else if (!sock_readable(sock)) {
        /* Client parks on VFS_EVT_RD; stack wakeups re-arm it. */
        ret = VFS_ERR_RETRY;
    } else {
        errno = 0;
        ret = sock_recv(sock, buf, size);
        if (ret < 0 && (errno == 0 || errno == EAGAIN || errno == EINTR))
            ret = VFS_ERR_RETRY; /* raced with another reader */
    }

    /*
     * tcp_receive() grows rcv.wnd and emits the matching window-update ACK,
     * which ether_tap only coalesces into its TX batch. Left alone, that ACK
     * sits until the next intr_step() round runs net_tx_flush() -- a whole
     * cadence period of added RTT on every single read, which is precisely
     * the stall the peer's sender sees on a download. Push it out now so the
     * window reopens on the wire immediately.
     *
     * tryflush, not flush: this runs on an IPC worker, and the blocking flush
     * can park up to ETHER_TAP_TX_WAIT_MS on POLLOUT while holding the device
     * lock the main thread needs to drain inbound frames. A congested ACK
     * simply rides the next round.
     */
    if (ret > 0)
        net_tx_tryflush();

    int saved_errno = errno;
    task_end_op(task);
    errno = saved_errno;
    return ret;
}

int task_write(uint32_t node, const char* buf, int size) {
    int sock = -1;
    net_task_t *task = task_begin_op(node, &sock);
    if (task == NULL) {
        errno = EBADF;
        return -1;
    }

    int ret;
    if (sock < 0) {
        errno = EBADF;
        ret = -1;
    } else if (size <= 0) {
        ret = 0;
    } else {
        errno = 0;
        ret = sock_send(sock, buf, size);
        if (ret < 0 && (errno == 0 || errno == EAGAIN || errno == EINTR)) {
            /* Send window full: the ACK-driven WR wakeup re-arms. Partial
             * writes return the accepted byte count; libc loops on it. */
            ret = VFS_ERR_RETRY;
        }
        /* Burst boundary: push any coalesced tap frames out now instead of
         * waiting for the next batch-full or interrupt-loop flush. */
        net_tx_flush();
    }

    int saved_errno = errno;
    task_end_op(task);
    errno = saved_errno;
    return ret;
}

/*
 * Live poll state for FS_CMD_POLL / vfs_get_poll_events(). vfsd REPLACES the
 * sticky RD/WR bits with whatever this returns, so everything a blocked
 * client may be waiting on must be derivable from live state here.
 */
uint32_t task_poll_events(uint32_t node) {
    uint32_t events = 0;
    int sock;
    bool timeout_rd;

    net_task_t *task = task_find_live_by_node(node);
    if (task == NULL)
        return 0;
    if (task->closing) {
        pthread_mutex_unlock(&task->lock);
        return 0;
    }
    task->inflight++;
    sock = task->sock;
    /* A latched SO_RCVTIMEO expiry must surface as an RD edge: the retried
     * recv consumes it and completes with ETIMEDOUT. */
    timeout_rd = task->rcv_timeout_pending;
    pthread_mutex_unlock(&task->lock);

    if (sock < 0) {
        /* No socket bound yet: never block writers on an unbound node. */
        events = VFS_EVT_WR;
    } else {
        /*
         * tcp_poll_readable()/tcp_poll_writable() take the stack mutex with a
         * trylock and return -1 when it is held elsewhere -- e.g. the main
         * thread is mid packet-burst during a bulk download. -1 means
         * "readiness UNKNOWN right now", NOT "buffer empty" / "window closed".
         * vfsd REPLACES the sticky RD/WR bits with this result, so the old
         * `> 0` test folded -1 into "not ready" and ERASED a pending edge that
         * already-buffered data (or an opened window) had earned. The parked
         * reader then slept forever with a full receive buffer -- the xBrowser
         * stall right after a large HTTP response, where tens of KB sat
         * undrained in netd and no further segment arrived to re-raise the
         * edge. Treat any nonzero (ready OR unknown) as ready: a spurious wake
         * costs one blocking recv()/send() -- which takes the mutex properly,
         * sees the true state, and re-parks via VFS_ERR_RETRY if genuinely not
         * ready -- whereas a dropped edge hangs the connection permanently.
         */
        int pr = sock_poll_readable(sock);
        if (timeout_rd || pr != 0)
            events |= VFS_EVT_RD;
        /*
         * tcp_poll_writable() already reports 1 while a connect() is still
         * in SYN_SENT/SYN_RECEIVED; publishing WR then busy-spins the
         * blocked connect() retry loop. Suppress WR until the handshake
         * resolves (the stack raises a WR edge on establishment).
         */
        if (sock_poll_writable(sock) != 0 && !sock_connect_pending(sock))
            events |= VFS_EVT_WR;
    }

    task_end_op(task);
    return events;
}

/*
 * Periodic SO_RCVTIMEO sweeper (stack net-timer: runs with the stack mutex
 * held, which is exactly our lock order stack -> task_list -> task). Expired
 * deadlines are latched into rcv_timeout_pending and an RD edge is raised so
 * the parked client retries and collects ETIMEDOUT.
 */
void task_timeout_check(void) {
    net_task_t *task;

    pthread_mutex_lock(&task_list_lock);
    for (task = task_list; task != NULL; task = task->next) {
        uint32_t wake_node = 0;
        pthread_mutex_lock(&task->lock);
        if (!task->closing && task->rcv_deadline_set &&
                task_deadline_expired(&task->rcv_deadline)) {
            task->rcv_deadline_set = false;
            task->rcv_timeout_pending = true;
            wake_node = (uint32_t)task->node;
        }
        pthread_mutex_unlock(&task->lock);
        /* Deferred: the wakeup queue lock is a leaf, safe under the list
         * lock; the reverse IPC itself happens on the flusher thread. */
        if (wake_node > 0)
            task_queue_vfs_wakeup(wake_node, VFS_EVT_RD);
    }
    pthread_mutex_unlock(&task_list_lock);
}

/*
 * Route a stack event on @sock_id to the owning vfs node. Callers hold the
 * global stack mutex, so the wakeup must go through the deferred queue (see
 * task_queue_vfs_wakeup).
 */
static int task_wakeup_sock(int sock_id, uint32_t events) {
    uint32_t wake_node = 0;

    if (sock_id < 0 || sock_id >= SOCKS_MAX || events == 0)
        return 0;

    pthread_mutex_lock(&task_list_lock);
    net_task_t *task = sock_to_task[sock_id];
    if (task != NULL) {
        pthread_mutex_lock(&task->lock);
        if (!task->closing && task->sock == sock_id && task->node > 0)
            wake_node = (uint32_t)task->node;
        pthread_mutex_unlock(&task->lock);
    }
    pthread_mutex_unlock(&task_list_lock);

    if (wake_node == 0)
        return 0;
    task_queue_vfs_wakeup(wake_node, events);
    return 1;
}

int task_wakeup_tcp_readers(int tcp_desc) {
    if (tcp_desc < 0)
        return 0;
    return task_wakeup_sock(sock_id_from_tcp_desc(tcp_desc), VFS_EVT_RD);
}

int task_wakeup_tcp_writers(int tcp_desc) {
    if (tcp_desc < 0)
        return 0;
    return task_wakeup_sock(sock_id_from_tcp_desc(tcp_desc), VFS_EVT_WR);
}

int task_wakeup_udp_readers(int udp_desc) {
    if (udp_desc < 0)
        return 0;
    return task_wakeup_sock(sock_id_from_udp_desc(udp_desc), VFS_EVT_RD);
}

int task_wakeup_raw_readers(int sock_id) {
    return task_wakeup_sock(sock_id, VFS_EVT_RD);
}

/*
 * Final teardown, called from network_close() once the refcount hit zero.
 * The caller already set task->closing under task->lock, so no new handler
 * can begin an operation; whatever is still inflight is drained here before
 * the socket is closed and the memory freed.
 */
void release_task(net_task_t *task) {
    if (task == NULL)
        return;

    /* Off the list first: node lookups miss from here on. */
    task_list_remove(task);

    int fin_sock = task_detach_sock(task);
    task_wait_other_ops(task, 0);

    /*
     * Re-check under the lock: the bounded drain above can time out while a
     * handler is legitimately stalled -- task_write()'s net_tx_flush() rides
     * wl0 TX backpressure for seconds at the end of an sshd upload, exactly
     * when the client's close() lands here. Destroying task->lock and
     * freeing the task while that handler still holds the pointer made its
     * task_end_op() lock a destroyed mutex in freed memory: the recycled
     * semaphore id / heap block corrupted live locks (stack mutex included)
     * and netd stopped responding for good. Hand the deferred sock_close +
     * free to the LAST task_end_op() instead; new operations cannot start
     * (closing is set and the task is off the list), so inflight only
     * decreases.
     */
    pthread_mutex_lock(&task->lock);
    if (task->inflight > 0) {
        task->dead = true;
        task->fin_sock = fin_sock;
        pthread_mutex_unlock(&task->lock);
        slog("netd: task node=%d release deferred to last inflight op\n",
             task->node);
        return;
    }
    pthread_mutex_unlock(&task->lock);

    /* sock_close() runs the TCP graceful close; under IPC_MULTI_TASK that
     * only occupies this one pool worker. */
    if (fin_sock >= 0)
        sock_close(fin_sock);

    /*
     * Do NOT publish VFS_EVT_CLOSE here. release_task() only runs once
     * netd-side refs hit zero, i.e. every client has already closed its fd;
     * vfsd's own close path (proc_file_close -> do_node_wakeup(CLOSE) and
     * vfsd_del_node waiter drain) wakes every still-blocked client before
     * this deferred wakeup could even ride the next protocol round. In the
     * documented double-close race the vfs node can still be ALIVE when this
     * runs: latching CLOSE into node->events then poisons it forever
     * (nothing ever clears sticky CLOSE/ERR/NVAL), turning every later
     * vfs_block_by_fd()/poll() on that node into a no-sleep livelock that
     * floods vfsd with IPCs and freezes the whole system. A blocked client
     * can never observe this edge as its sole wake source (it holds an fd,
     * so refs > 0, so release cannot fire), so dropping it is safe.
     */

    task_destroy(task);
}
