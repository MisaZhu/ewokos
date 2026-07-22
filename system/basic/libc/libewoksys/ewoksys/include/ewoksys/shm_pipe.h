#ifndef SHM_PIPE_H
#define SHM_PIPE_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Shared-memory pipe ring buffer.
 *
 * Allocated as a single page (4096 bytes) of shared memory accessible by both
 * the reader and writer processes. Data transfer happens entirely in userspace
 * via the ring buffer — no IPC to vfsd is needed for read/write operations.
 *
 * Blocking/wakeup uses direct kernel syscalls (proc_block_by / proc_wakeup_by)
 * scoped to the VFS node_id token.
 *
 * Design: Single-Producer Single-Consumer (SPSC) lock-free ring buffer.
 * - write_pos is only modified by the writer
 * - read_pos is only modified by the reader
 * - Memory ordering via __atomic builtins ensures visibility across cores
 */

#define SHM_PIPE_HEADER_SIZE  64
#define SHM_PIPE_PAGE_SIZE    4096
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
	int32_t _reserved[7];            /* pad header to 64 bytes */
	char data[SHM_PIPE_DATA_SIZE];   /* circular ring buffer */
} shm_pipe_t;

/*
 * Ring buffer helpers — inline for zero-overhead in the hot path.
 */

static inline int32_t shm_pipe_readable(shm_pipe_t* ring) {
	int32_t wp = __atomic_load_n(&ring->write_pos, __ATOMIC_ACQUIRE);
	int32_t rp = __atomic_load_n(&ring->read_pos, __ATOMIC_RELAXED);
	return wp - rp;
}

static inline int32_t shm_pipe_writable(shm_pipe_t* ring) {
	int32_t wp = __atomic_load_n(&ring->write_pos, __ATOMIC_RELAXED);
	int32_t rp = __atomic_load_n(&ring->read_pos, __ATOMIC_ACQUIRE);
	return ring->capacity - (wp - rp);
}

static inline int32_t shm_pipe_read(shm_pipe_t* ring, void* buf, int32_t size) {
	int32_t avail = shm_pipe_readable(ring);
	if(avail <= 0)
		return 0;
	if(size > avail)
		size = avail;

	int32_t rp = ring->read_pos;
	int32_t offset = rp % ring->capacity;
	int32_t first = ring->capacity - offset;
	if(first >= size) {
		memcpy(buf, ring->data + offset, size);
	} else {
		memcpy(buf, ring->data + offset, first);
		memcpy((char*)buf + first, ring->data, size - first);
	}

	__atomic_store_n(&ring->read_pos, rp + size, __ATOMIC_RELEASE);
	return size;
}

static inline int32_t shm_pipe_write(shm_pipe_t* ring, const void* buf, int32_t size) {
	int32_t space = shm_pipe_writable(ring);
	if(space <= 0)
		return 0;
	if(size > space)
		size = space;

	int32_t wp = ring->write_pos;
	int32_t offset = wp % ring->capacity;
	int32_t first = ring->capacity - offset;
	if(first >= size) {
		memcpy(ring->data + offset, buf, size);
	} else {
		memcpy(ring->data + offset, buf, first);
		memcpy(ring->data, (const char*)buf + first, size - first);
	}

	__atomic_store_n(&ring->write_pos, wp + size, __ATOMIC_RELEASE);
	return size;
}

static inline void shm_pipe_init(shm_pipe_t* ring, int32_t node_id, int32_t shm_id) {
	memset(ring, 0, sizeof(shm_pipe_t));
	ring->capacity = SHM_PIPE_DATA_SIZE;
	ring->node_id = node_id;
	ring->shm_id = shm_id;
}

#ifdef __cplusplus
}
#endif

#endif
