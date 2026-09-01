#include <ewoksys/shm_pipe.h>
#include <string.h>

/*
 * SPSC lock-free ring buffer helpers for the shared-memory pipe.
 * The on-shm layout (shm_pipe_t) is declared in ewoksys/shm_pipe.h;
 * both vfs.c (libc side) and piped (server side) call into these.
 *
 * Memory ordering: the side producing data releases its position update
 * after copying payload; the consuming side acquires the producer's
 * position before reading payload.
 */

int32_t shm_pipe_readable(shm_pipe_t* ring) {
	int32_t wp = __atomic_load_n(&ring->write_pos, __ATOMIC_ACQUIRE);
	int32_t rp = __atomic_load_n(&ring->read_pos, __ATOMIC_RELAXED);
	return wp - rp;
}

int32_t shm_pipe_writable(shm_pipe_t* ring) {
	int32_t wp = __atomic_load_n(&ring->write_pos, __ATOMIC_RELAXED);
	int32_t rp = __atomic_load_n(&ring->read_pos, __ATOMIC_ACQUIRE);
	return ring->capacity - (wp - rp);
}

int32_t shm_pipe_read(shm_pipe_t* ring, void* buf, int32_t size) {
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

int32_t shm_pipe_write(shm_pipe_t* ring, const void* buf, int32_t size) {
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

void shm_pipe_init(shm_pipe_t* ring, int32_t node_id, int32_t shm_id) {
	memset(ring, 0, sizeof(shm_pipe_t));
	ring->capacity = SHM_PIPE_DATA_SIZE;
	ring->node_id = node_id;
	ring->shm_id = shm_id;
}
