#ifndef SHM_PIPE_H
#define SHM_PIPE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Shared-memory pipe ring buffer.
 *
 * Allocated as shared memory accessible by both the reader and writer
 * processes. Data transfer happens entirely in userspace via the ring
 * buffer — no IPC to vfsd is needed for read/write operations.
 *
 * Blocking/wakeup uses direct kernel syscalls (proc_block_by / proc_wakeup_by)
 * scoped to the VFS node_id token.
 *
 * Design: Single-Producer Single-Consumer (SPSC) lock-free ring buffer.
 * - write_pos is only modified by the writer
 * - read_pos is only modified by the reader
 * - Memory ordering via __atomic builtins ensures visibility across cores
 *
 * Only the on-shm layout lives here (both sides map it directly); the ring
 * helpers are implemented in ewoksys/src/shm_pipe.c.
 */

#define SHM_PIPE_HEADER_SIZE  64
#define SHM_PIPE_PAGE_SIZE    (4096*32)
#define SHM_PIPE_DATA_SIZE    (SHM_PIPE_PAGE_SIZE - SHM_PIPE_HEADER_SIZE)

typedef struct {
	volatile int32_t write_pos;      /* ring write position (only writer modifies) */
	volatile int32_t read_pos;       /* ring read position (only reader modifies) */
	volatile int32_t writer_closed;  /* set to 1 when all writers close */
	volatile int32_t reader_closed;  /* set to 1 when all readers close */
	volatile int32_t writer_pid;     /* current writer PID (for wakeup by reader) */
	volatile int32_t reader_pid;     /* current reader PID (for wakeup by writer) */
	int32_t node_id;                 /* VFS node ID (blocking token) */
	int32_t shm_id;                  /* shared memory ID */
	int32_t capacity;                /* ring data capacity */
	/*
	 * Number of poll waiters currently registered inside piped for this pipe.
	 * Peers check it before emitting a PIPE_EDGE notification: data edges only
	 * need IPC while somebody can actually be woken by them, so a plain
	 * reader/writer pair stays at zero IPC for its whole lifetime.
	 */
	volatile int32_t has_poll_waiters;
	int32_t _reserved[6];            /* pad header to 64 bytes */
	char data[SHM_PIPE_DATA_SIZE];   /* circular ring buffer */
} shm_pipe_t;

/*
 * Ring buffer helpers (implemented in shm_pipe.c).
 */
void shm_pipe_init(shm_pipe_t* ring, int32_t node_id, int32_t shm_id);
int32_t shm_pipe_readable(shm_pipe_t* ring);
int32_t shm_pipe_writable(shm_pipe_t* ring);
int32_t shm_pipe_read(shm_pipe_t* ring, void* buf, int32_t size);
int32_t shm_pipe_write(shm_pipe_t* ring, const void* buf, int32_t size);

#ifdef __cplusplus
}
#endif

#endif
