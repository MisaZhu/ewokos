#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
 * Thread-local storage for EwokOS.
 *
 * The cross toolchain is built single-threaded, so it never emits a native TLS
 * relocation: every __thread and every C++ thread_local is lowered to a call to
 * libgcc's __emutls_get_address(), and libgcc's copy of that function caches
 * the buffer it mallocs inside the per-variable control object - one slot for
 * the whole process. All the tasks of a process therefore share every
 * thread_local, and whichever task wrote last decides what the others read.
 *
 * Nothing announces that failure; it just looks like nonsense from the other
 * side. Qt hits it immediately: QThreadData::current() is a thread_local
 * pointer, so the first QThreadPool worker to start hands its QThreadData to
 * the GUI thread, and from then on Qt reports every QObject as belonging to a
 * worker - it refuses to create children, refuses to start timers, and never
 * realises the window.
 *
 * The compiler reaches thread_local storage through exactly one entry point,
 * so overriding that entry point (plus __emutls_register_common(), which lives
 * in the same libgcc object and would otherwise be linked in alongside it)
 * fixes thread_local for every program on the system without changing how
 * anything is built.
 *
 * Each task gets a block holding its own array of per-variable buffers. The
 * block hangs off TPIDR_EL0, which the kernel saves and restores as part of
 * the task context (proc_switch() in kernel/kernel/src/proc.c), so finding it
 * costs one register read - no syscall, and no pid-keyed table that a recycled
 * task id could alias onto a dead thread's storage.
 *
 * Everything below is aarch64-only because that is the only architecture the
 * kernel currently carries a TLS base for. Elsewhere the file compiles to
 * nothing and libgcc's own emulation is linked, exactly as before.
 */

#if defined(__aarch64__)

/*
 * The control object the compiler emits for each thread_local, as
 * __emutls_v.<name>. The layout is fixed by the ABI libgcc uses, and the
 * compiler initialises it statically, so it has to be reproduced byte for
 * byte: size and align describe the variable, templ is its initial image (NULL
 * for zero), and loc is scratch space that belongs to whoever implements
 * __emutls_get_address(). libgcc stores the one shared buffer there; we store
 * the variable's index plus one, so 0 still means "not seen yet".
 */
typedef struct {
	uint64_t size;
	uint64_t align;
	uint64_t loc;
	void*    templ;
} emutls_object_t;

/* Per-task state, pointed to by TPIDR_EL0. */
typedef struct {
	void**   slots;   /* slots[i] is this task's buffer for variable i   */
	void**   chunks;  /* chunks[i] is the malloc it was carved out of    */
	uint32_t capacity; /* entries in both arrays                          */
	int32_t  tid;      /* cached kernel thread id, -1 until first stored  */
} emutls_block_t;

static volatile int _emutls_index_lock = 0;
static uint32_t     _emutls_object_count = 0;

static inline void* emutls_tls_get(void) {
	uintptr_t v;
	__asm__ volatile("mrs %0, tpidr_el0" : "=r"(v));
	return (void*)v;
}

static inline void emutls_tls_set(void* v) {
	__asm__ volatile("msr tpidr_el0, %0" :: "r"((uintptr_t)v));
}

/*
 * Held only across the few instructions that hand out an index. Nothing is
 * allocated under it, so it is never held across a malloc and cannot
 * participate in a lock cycle with the heap lock; spinning is cheaper here
 * than a syscall to yield.
 */
static inline void emutls_index_lock(void) {
	while(__sync_lock_test_and_set(&_emutls_index_lock, 1) != 0)
		;
}

static inline void emutls_index_unlock(void) {
	__sync_lock_release(&_emutls_index_lock);
}

static uint32_t emutls_index_of(emutls_object_t* obj) {
	uint32_t tag = (uint32_t)obj->loc;
	if(tag != 0)
		return tag - 1;

	emutls_index_lock();
	tag = (uint32_t)obj->loc;
	if(tag == 0) {
		tag = ++_emutls_object_count;
		obj->loc = tag;
	}
	emutls_index_unlock();
	return tag - 1;
}

/*
 * The calling task's block, allocated on first use. No locking: TPIDR_EL0 is
 * private to the task, so two tasks initialising at the same time are
 * initialising two different blocks.
 */
static emutls_block_t* emutls_self(void) {
	emutls_block_t* blk = (emutls_block_t*)emutls_tls_get();
	if(blk != NULL)
		return blk;

	blk = (emutls_block_t*)malloc(sizeof(emutls_block_t));
	if(blk == NULL)
		abort();
	memset(blk, 0, sizeof(emutls_block_t));
	blk->tid = -1;
	emutls_tls_set(blk);
	return blk;
}

/* Grows both arrays; only ever called by the owning task. */
static void emutls_grow(emutls_block_t* blk, uint32_t need) {
	uint32_t cap = (blk->capacity == 0) ? 8 : blk->capacity;
	while(cap < need)
		cap *= 2;

	void** slots = (void**)malloc((size_t)cap * sizeof(void*));
	if(slots == NULL)
		abort();
	void** chunks = (void**)malloc((size_t)cap * sizeof(void*));
	if(chunks == NULL) {
		free(slots);
		abort();
	}
	memset(slots, 0, (size_t)cap * sizeof(void*));
	memset(chunks, 0, (size_t)cap * sizeof(void*));

	if(blk->slots != NULL) {
		memcpy(slots, blk->slots, (size_t)blk->capacity * sizeof(void*));
		memcpy(chunks, blk->chunks, (size_t)blk->capacity * sizeof(void*));
		free(blk->slots);
		free(blk->chunks);
	}
	blk->slots = slots;
	blk->chunks = chunks;
	blk->capacity = cap;
}

void* __emutls_get_address(emutls_object_t* obj) {
	uint32_t index = emutls_index_of(obj);

	emutls_block_t* blk = emutls_self();
	if(index >= blk->capacity)
		emutls_grow(blk, index + 1);

	void* at = blk->slots[index];
	if(at != NULL)
		return at;

	/*
	 * One chunk per variable rather than one arena per task: variables are
	 * discovered in whatever order the program happens to touch them, and a
	 * shared arena would have to be realloced (moving every buffer already
	 * handed out) each time it filled. Over-allocating by align lets the
	 * buffer be aligned up inside the chunk without a second allocation -
	 * malloc's own alignment is only guaranteed to suit the basic types.
	 */
	uintptr_t align = (obj->align < sizeof(void*)) ? sizeof(void*) : obj->align;
	uintptr_t chunk = (uintptr_t)malloc((size_t)obj->size + (size_t)align);
	if(chunk == 0)
		abort();
	at = (void*)((chunk + align - 1) & ~(align - 1));

	if(obj->templ != NULL)
		memcpy(at, obj->templ, (size_t)obj->size);
	else
		memset(at, 0, (size_t)obj->size);

	blk->chunks[index] = (void*)chunk;
	blk->slots[index] = at;
	return at;
}

/*
 * Merges a tentative (common) definition into the control object, called from
 * static initialisation. Semantics are libgcc's, reproduced exactly: the widest
 * size wins and discards a narrower initialiser, the strictest alignment wins,
 * and an initialiser is kept only if it is for the size that ended up winning.
 */
void __emutls_register_common(emutls_object_t* obj, uint64_t size, uint64_t align, void* templ) {
	if(obj->size < size) {
		obj->size = size;
		obj->templ = NULL;
	}
	if(obj->align < align)
		obj->align = align;
	if(templ != NULL && obj->size == size)
		obj->templ = templ;
}

/*
 * Per-task cache of the kernel thread id, so thread_get_id()/pthread_self()
 * answers from a TPIDR_EL0 register read instead of trapping. This matters
 * because thread_get_id() is called from inside the heap lock
 * (__malloc_lock -> proc_global_lock) on every malloc/free, and it must never
 * malloc there: a malloc would re-enter the heap lock and, needing the tid
 * again, recurse without end. So only __ewok_tls_init_tid allocates, and it is
 * called solely at a thread's entry point, before that thread can take the
 * heap lock on its own behalf.
 *
 * __ewok_tls_get_tid:   the cached id, or -1 if this task has no block yet or
 *                       the id was never stored. Never allocates.
 * __ewok_tls_cache_tid: store into an EXISTING block only; a no-op when there
 *                       is none. Never allocates, so it is safe on the slow
 *                       path of thread_get_id() even under the heap lock.
 * __ewok_tls_init_tid:  store, creating the block if needed. The malloc
 *                       emutls_self() may perform is recursion-safe because
 *                       the block is published to TPIDR_EL0 only after that
 *                       malloc returns, so a re-entrant thread_get_id() still
 *                       sees no block and takes one syscall rather than trying
 *                       to create the block again.
 */
int32_t __ewok_tls_get_tid(void) {
	emutls_block_t* blk = (emutls_block_t*)emutls_tls_get();
	return (blk == NULL) ? -1 : blk->tid;
}

void __ewok_tls_cache_tid(int32_t tid) {
	emutls_block_t* blk = (emutls_block_t*)emutls_tls_get();
	if(blk != NULL)
		blk->tid = tid;
}

void __ewok_tls_init_tid(int32_t tid) {
	emutls_block_t* blk = emutls_self();
	blk->tid = tid;
}

/*
 * Clears the calling task's TLS block pointer (TPIDR_EL0). _start() calls this
 * first thing: a new image must begin with fresh thread_locals, and although
 * the kernel zeroes proc->tls_base for the new image it only reloads the
 * physical register on the next proc_switch, so immediately after exec TPIDR_EL0
 * still holds the pre-exec value - a pointer into the OLD image's heap, now
 * unmapped. Left alone, the first emutls access treats that stale pointer as a
 * live block and faults writing through it (e.g. __ewok_tls_init_tid storing
 * the tid). Clearing it makes emutls_self() allocate a fresh block instead.
 */
void __ewok_tls_reset(void) {
	emutls_tls_set(NULL);
}

/*
 * Releases everything the calling task's thread_locals occupy. Called from the
 * libc thread teardown path; the task is about to stop existing, so its
 * TPIDR_EL0 is cleared first - a block that is freed while the register still
 * points at it would be reused by the next variable this task touches.
 *
 * C++ thread_local destructors are not run: this toolchain has no
 * __cxa_thread_atexit, so nothing could have registered one.
 */
void __ewok_emutls_thread_exit(void) {
	emutls_block_t* blk = (emutls_block_t*)emutls_tls_get();
	if(blk == NULL)
		return;
	emutls_tls_set(NULL);

	for(uint32_t i = 0; i < blk->capacity; i++)
		free(blk->chunks[i]);
	free(blk->slots);
	free(blk->chunks);
	free(blk);
}

#else /* __aarch64__ */

/*
 * No other architecture has a kernel-carried TLS base yet, so there is nothing
 * per-task to hand back: libgcc's own emulation stays linked there, exactly as
 * before, and the teardown call from thread_create is a no-op.
 */
void __ewok_emutls_thread_exit(void) {
}

/*
 * The thread-id cache has nowhere to live without a kernel-carried TLS base,
 * so thread_get_id() keeps trapping on these architectures, exactly as before.
 */
int32_t __ewok_tls_get_tid(void) {
	return -1;
}

void __ewok_tls_cache_tid(int32_t tid) {
	(void)tid;
}

void __ewok_tls_init_tid(int32_t tid) {
	(void)tid;
}

void __ewok_tls_reset(void) {
}

#endif /* __aarch64__ */
