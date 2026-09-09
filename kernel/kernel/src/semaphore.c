#include <kernel/semaphore.h>
#include <kernel/proc.h>
#include <kernel/schedule.h>
#include <kstring.h>
#include <stddef.h>

#define SEMAPHORE_MAX 1024

/*
 * Wait queue.
 *
 * One flat table shared by every semaphore rather than a per-semaphore array:
 * SEMAPHORE_MAX is 1024 and this table lives in kernel BSS, so giving each
 * semaphore its own queue would cost hundreds of KB for contention that does
 * not exist. An entry is live only for as long as its task is parked, so the
 * table's occupancy equals the number of tasks blocked on semaphores right
 * now, system wide.
 *
 * seq gives strict FIFO across arrivals, which survives slot reuse: a waiter
 * released without a permit keeps its slot (and therefore its seq) across the
 * round trip to userspace and back, so it re-parks in the same position rather
 * than at the tail.
 */
#define SEM_WAITER_MAX 256

typedef struct {
	int32_t  sem_id;   /* 1-based id waited on; 0 marks the slot free */
	int32_t  pid;      /* task parked on it */
	uint32_t seq;      /* arrival order */
	int8_t   granted;  /* permit already handed to this waiter */
} sem_waiter_t;

static semaphore_t _semaphores[SEMAPHORE_MAX];
static sem_waiter_t _waiters[SEM_WAITER_MAX];
static uint32_t _waiter_seq = 0;

/*
 * Block/wake token for a semaphore.
 *
 * proc_block_by()/proc_wakeup_by() scope a non-zero token so an edge meant for
 * something else cannot release the block: a VFS node wake carries a small node
 * id, and this is a kernel virtual address, so the two namespaces never collide
 * and a semaphore wait cannot be broken by unrelated node traffic. Generic
 * token-0 wakes (ipc returns, signals) still release it. That is a spurious
 * wake: the enter path reports SEM_RES_RETRY, the task keeps its slot and
 * re-parks on the next syscall.
 *
 * Never 0, which proc_wakeup_by() would read as "generic".
 */
static inline ewokos_addr_t sem_token(int32_t idx) {
	return (ewokos_addr_t)&_semaphores[idx];
}

static inline void sem_refresh_occupied(int32_t idx) {
	_semaphores[idx].occupied =
		(_semaphores[idx].count <= 0) ? SEM_OCCUPIED : SEM_IDLE;
}

static inline bool sem_valid_idx(int32_t idx) {
	return (idx >= 0 && idx < SEMAPHORE_MAX && _semaphores[idx].creater_pid != -1);
}

static inline bool sem_is_binary(int32_t idx) {
	return (_semaphores[idx].max_count == 1);
}

/* All _waiters[] helpers below are called with the proc lock held. */

static int32_t sem_waiter_add_locked(uint32_t sem_id, int32_t pid) {
	for(int32_t i=0; i<SEM_WAITER_MAX; i++) {
		if(_waiters[i].sem_id != 0)
			continue;
		_waiters[i].sem_id = (int32_t)sem_id;
		_waiters[i].pid = pid;
		_waiters[i].seq = _waiter_seq++;
		_waiters[i].granted = 0;
		return i;
	}
	return -1;
}

/*
 * The slot this task already holds on this semaphore, or -1.
 *
 * A task appears here at most once: entries are keyed by (sem_id, task pid) and
 * a task is either parked or running, never both. Finding an entry while the
 * task is running therefore means one of two things - a permit was granted to
 * it and has not been collected yet, or it was released without one and came
 * back to re-park. semaphore_enter_raw() tells the two apart by .granted.
 */
static int32_t sem_waiter_find_locked(uint32_t sem_id, int32_t pid) {
	for(int32_t i=0; i<SEM_WAITER_MAX; i++) {
		if(_waiters[i].sem_id == (int32_t)sem_id && _waiters[i].pid == pid)
			return i;
	}
	return -1;
}

/*
 * pid is checked as well as the index: when a semaphore is torn down the queue
 * is purged from under the parked tasks, and a slot index a woken task still
 * holds can have been reused by another waiter in the meantime. Without the
 * check that task would delete someone else's place in the queue.
 */
static void sem_waiter_del_locked(int32_t slot, int32_t pid) {
	if(slot < 0 || slot >= SEM_WAITER_MAX)
		return;
	if(_waiters[slot].sem_id == 0 || _waiters[slot].pid != pid)
		return;
	_waiters[slot].sem_id = 0;
	_waiters[slot].pid = -1;
	_waiters[slot].granted = 0;
}

/* Oldest ungranted waiter for this semaphore, or -1. */
static int32_t sem_waiter_head_locked(int32_t idx) {
	int32_t best = -1;
	uint32_t best_seq = 0;
	int32_t sem_id = idx + 1;

	for(int32_t i=0; i<SEM_WAITER_MAX; i++) {
		if(_waiters[i].sem_id != sem_id || _waiters[i].granted)
			continue;
		if(best < 0 || _waiters[i].seq < best_seq) {
			best = i;
			best_seq = _waiters[i].seq;
		}
	}
	return best;
}

/*
 * Give one available permit straight to the oldest waiter.
 *
 * Hand-off rather than "bump count and wake someone": the permit moves to a
 * named task before it is woken, so a third thread that arrives in between
 * cannot steal it and send the woken waiter back to sleep. This is what makes
 * the queue FIFO in practice and not merely in intent.
 *
 * The slot is deliberately NOT removed here. proc_wakeup_by() only makes the
 * waiter runnable - it cannot reach into a parked task's saved context and
 * rewrite its syscall return value, and schedule() has already handed this
 * kernel stack to somebody else. So the permit stays parked in the queue as
 * .granted until the woken task re-enters and collects it. Deleting the slot
 * here would leave the permit nowhere for that task to find it.
 */
static void sem_handoff_locked(int32_t idx) {
	while(_semaphores[idx].count > 0) {
		int32_t slot = sem_waiter_head_locked(idx);
		if(slot < 0)
			return;

		proc_t* waiter = proc_get(_waiters[slot].pid);
		if(waiter == NULL) {
			/*
			 * Died while queued and never got reaped. Drop it and try the
			 * next waiter; leaving it would park the permit here forever.
			 */
			sem_waiter_del_locked(slot, _waiters[slot].pid);
			continue;
		}

		_semaphores[idx].count--;
		_waiters[slot].granted = 1;
		if(sem_is_binary(idx))
			_semaphores[idx].occupied_pid = _waiters[slot].pid;
		sem_refresh_occupied(idx);
		proc_wakeup_by(waiter, sem_token(idx));
		return;
	}
}

/*
 * Release every task queued on a semaphore that is going away, so they re-check
 * creater_pid and return SEM_RES_ERROR instead of sleeping on a dead lock.
 */
static void sem_waiter_purge_locked(int32_t idx) {
	int32_t sem_id = idx + 1;

	for(int32_t i=0; i<SEM_WAITER_MAX; i++) {
		if(_waiters[i].sem_id != sem_id)
			continue;
		int32_t pid = _waiters[i].pid;
		sem_waiter_del_locked(i, pid);
		proc_t* waiter = proc_get(pid);
		if(waiter != NULL)
			proc_wakeup_by(waiter, sem_token(idx));
	}
}

static void sem_reset_locked(int32_t idx) {
	sem_waiter_purge_locked(idx);
	_semaphores[idx].creater_pid = -1;
	_semaphores[idx].occupied_pid = -1;
	_semaphores[idx].occupied = SEM_IDLE;
	_semaphores[idx].count = 0;
	_semaphores[idx].max_count = 1;
}

static bool sem_try_acquire_locked(int32_t idx, int32_t pid) {
	if(_semaphores[idx].count <= 0)
		return false;
	_semaphores[idx].count--;
	if(sem_is_binary(idx))
		_semaphores[idx].occupied_pid = pid;
	sem_refresh_occupied(idx);
	return true;
}

void semaphore_init(void) {
	for(int32_t i=0; i<SEMAPHORE_MAX; i++) {
		_semaphores[i].occupied = SEM_IDLE;
		_semaphores[i].creater_pid = -1;
		_semaphores[i].occupied_pid = -1;
		_semaphores[i].count = 0;
		_semaphores[i].max_count = 1;
	}
	for(int32_t i=0; i<SEM_WAITER_MAX; i++) {
		_waiters[i].sem_id = 0;
		_waiters[i].pid = -1;
		_waiters[i].seq = 0;
		_waiters[i].granted = 0;
	}
	_waiter_seq = 0;
}

static int32_t sem_alloc_raw(int32_t count, int32_t max_count) {
	proc_t* proc = proc_get_proc(get_current_proc());
	if(proc == NULL)
		return 0;
	if(count < 0)
		count = 0;

	proc_lock_enter();
	for(int32_t i=0; i<SEMAPHORE_MAX; i++) {
		if(_semaphores[i].creater_pid != -1)
			continue;
		_semaphores[i].creater_pid = proc->info.pid;
		_semaphores[i].count = count;
		_semaphores[i].max_count = max_count;
		_semaphores[i].occupied_pid = -1;
		sem_refresh_occupied(i);
		proc_lock_leave();
		return i+1;
	}
	proc_lock_leave();
	return 0;
}

int32_t semaphore_alloc(void) {
	return sem_alloc_raw(1, 1);
}

int32_t semaphore_alloc_count(int32_t count) {
	return sem_alloc_raw(count, SEMAPHORE_COUNT_MAX);
}

void semaphore_clear(int32_t pid) {
	proc_lock_enter();
	for(int32_t i=0; i<SEMAPHORE_MAX; i++) {
		if(_semaphores[i].creater_pid == pid)
			sem_reset_locked(i);
	}
	proc_lock_leave();
}

/*
 * Release every semaphore currently OCCUPIED by the given task pid, and drop
 * its places in any wait queue, without touching ownership (creater_pid).
 * semaphore_clear() only runs when a whole PROC dies, but semaphore occupancy
 * is per-task: a THREAD that terminates while holding a lock (multi_task IPC
 * worker killed mid-handler by a fatal fault, etc.) would otherwise leave it
 * occupied forever, and every other thread of the surviving proc would wait on
 * a lock nobody can ever release.
 *
 * Only binary semaphores carry an owner, so only those are recovered; a POSIX
 * counting semaphore drained to zero by a task that then dies is not "held" by
 * it and must be left alone.
 */
void semaphore_clear_occupied(int32_t pid) {
	proc_lock_enter();

	/*
	 * Queue entries first. One that had already been granted a permit must
	 * give the permit back, or it leaves with it and the semaphore stays
	 * drained even though nobody holds the lock.
	 */
	for(int32_t i=0; i<SEM_WAITER_MAX; i++) {
		if(_waiters[i].sem_id == 0 || _waiters[i].pid != pid)
			continue;

		int32_t idx = _waiters[i].sem_id - 1;
		bool granted = (_waiters[i].granted != 0);
		sem_waiter_del_locked(i, pid);

		if(!granted || !sem_valid_idx(idx))
			continue;
		if(_semaphores[idx].count < _semaphores[idx].max_count)
			_semaphores[idx].count++;
		if(_semaphores[idx].occupied_pid == pid)
			_semaphores[idx].occupied_pid = -1;
		sem_refresh_occupied(idx);
		sem_handoff_locked(idx);
	}

	for(int32_t i=0; i<SEMAPHORE_MAX; i++) {
		if(!sem_valid_idx(i) || !sem_is_binary(i))
			continue;
		if(_semaphores[i].occupied != SEM_OCCUPIED ||
				_semaphores[i].occupied_pid != pid)
			continue;
		_semaphores[i].count = _semaphores[i].max_count;
		_semaphores[i].occupied = SEM_IDLE;
		_semaphores[i].occupied_pid = -1;
		sem_handoff_locked(i);
	}
	proc_lock_leave();
}

void semaphore_free(uint32_t sem_id) {
	if(sem_id == 0 || sem_id > SEMAPHORE_MAX)
		return;
	int32_t idx = (int32_t)sem_id - 1;

	proc_t* cproc = proc_get_proc(get_current_proc());
	if(cproc == NULL)
		return;

	proc_lock_enter();
	/* creater_pid is the owner PROC, not the task, so compare like for like. */
	if(_semaphores[idx].creater_pid == cproc->info.pid)
		sem_reset_locked(idx);
	proc_lock_leave();
}

/*
 * Take a permit, parking the task until one is available.
 *
 * Straight-line by necessity, never a loop: see the comment above the park.
 *
 * timeout_usec == 0 waits indefinitely; the task is released by a grant (which
 * its next enter collects) or by a spurious generic wake, reported as
 * SEM_RES_RETRY so libc re-issues without burning a yield. A non-zero budget
 * stages SEM_RES_TIMEOUT instead: the absolute deadline belongs to the caller
 * (POSIX passes a wall-clock abstime, the kernel only has a boot-relative tic),
 * so it recomputes what is left and calls again.
 */
static void semaphore_enter_raw(context_t* ctx, uint32_t sem_id, uint32_t timeout_usec) {
	ctx->gpr[0] = SEM_RES_ERROR;

	if(sem_id == 0 || sem_id > SEMAPHORE_MAX)
		return;
	int32_t idx = (int32_t)sem_id - 1;

	proc_t* cproc = get_current_proc();
	if(cproc == NULL)
		return;
	if(!sem_valid_idx(idx))
		return;

	int32_t pid = cproc->info.pid;
	int32_t slot;

	proc_lock_enter();
	if(!sem_valid_idx(idx)) {
		/* freed, or its owner died, since the unlocked check above */
		proc_lock_leave();
		return;
	}

	slot = sem_waiter_find_locked(sem_id, pid);
	if(slot >= 0 && _waiters[slot].granted) {
		/* the permit handed to us by sem_handoff_locked() while we slept */
		sem_waiter_del_locked(slot, pid);
		proc_lock_leave();
		ctx->gpr[0] = SEM_RES_ACQUIRED;
		return;
	}

	if(sem_try_acquire_locked(idx, pid)) {
		if(slot >= 0)
			sem_waiter_del_locked(slot, pid);
		proc_lock_leave();
		ctx->gpr[0] = SEM_RES_ACQUIRED;
		return;
	}

	/*
	 * No permit. Reuse a slot we already hold rather than queueing a second
	 * one: a re-park after a spurious wake keeps its seq, and therefore its
	 * place in the queue, instead of being demoted to the tail.
	 */
	if(slot < 0) {
		slot = sem_waiter_add_locked(sem_id, pid);
		if(slot < 0) {
			proc_lock_leave();
			ctx->gpr[0] = SEM_RES_OCCUPIED;
			return;
		}
	}
	proc_lock_leave();

	/*
	 * A hijacked single-task ipc service context must never park - the
	 * saved-state restore machine would be stranded, exactly as in
	 * sys_proc_block(). multi_task worker threads are independent contexts
	 * and do park here. Hand the cpu over and report OCCUPIED so the caller
	 * retries instead of deadlocking the server.
	 */
	if(proc_ipc_sync_serving(cproc)) {
		/*
		 * The slot was exposed while the lock was dropped above, so a quit()
		 * on another core may have granted it a permit already: count is
		 * decremented and, on a binary mutex, occupied_pid names this task.
		 * Deleting the slot without looking would leak that permit and leave
		 * the mutex occupied by a task that never took it. Consuming the
		 * grant as ACQUIRED is safe - only PARKING is forbidden in a serving
		 * context, holding a lock is not. Re-key the slot before trusting
		 * .granted: a teardown purge in the same window can have freed it
		 * for reuse by another waiter.
		 */
		proc_lock_enter();
		bool granted = (_waiters[slot].sem_id == (int32_t)sem_id &&
				_waiters[slot].pid == pid && _waiters[slot].granted != 0);
		sem_waiter_del_locked(slot, pid);
		proc_lock_leave();
		if(granted) {
			ctx->gpr[0] = SEM_RES_ACQUIRED;
			return;
		}
		ctx->gpr[0] = SEM_RES_OCCUPIED;
		schedule(ctx);
		return;
	}

	/*
	 * The verdict is staged in ctx BEFORE the park, and nothing here may
	 * touch ctx afterwards.
	 *
	 * schedule() does not come back to this task. proc_switch() copies this
	 * frame into cproc->ctx, overwrites ctx with the NEXT task's registers
	 * and returns, so the syscall epilogue eret's into that task - and any
	 * statement written after proc_block_by() runs on its behalf. A trailing
	 * ctx->gpr[0] = res there does not deliver our result, it silently
	 * replaces somebody else's syscall return value, and a loop around the
	 * park re-blocks a task that is already BLOCK. The blocked task resumes
	 * in USERSPACE, at its syscall return point, whenever it is next
	 * scheduled - the same contract proc_usleep() and sys_proc_block()
	 * already follow.
	 *
	 * Parking with the "released without a permit" answer already staged is
	 * what makes that safe: if the wake carried a grant, the next enter
	 * finds the slot and upgrades to ACQUIRED; if it was spurious, the slot
	 * is still ours and we re-park in place.
	 */
	ctx->gpr[0] = (timeout_usec > 0) ? SEM_RES_TIMEOUT : SEM_RES_RETRY;

	if(timeout_usec > 0)
		proc_block_by_timeout(ctx, cproc, sem_token(idx), timeout_usec);
	else
		proc_block_by(ctx, cproc, sem_token(idx));
}

void semaphore_enter(context_t* ctx, uint32_t sem_id) {
	semaphore_enter_raw(ctx, sem_id, 0);
}

void semaphore_enter_timeout(context_t* ctx, uint32_t sem_id, uint32_t timeout_usec) {
	semaphore_enter_raw(ctx, sem_id, timeout_usec);
}

/*
 * Non-blocking: take a permit if one is free, otherwise report OCCUPIED and
 * leave the caller running. This used to alias semaphore_enter(), which made
 * pthread_mutex_trylock() and sem_trywait() block - the opposite of what a
 * try* call promises.
 *
 * It also settles this task's wait-queue entry, which a timed park leaves
 * behind: the deadline verdict was staged before the block and the task
 * resumed in userspace, so no kernel path ever got the chance to clean up
 * after it. A granted slot is collected as ACQUIRED - that is how
 * pthread_mutex_timedlock() and sem_timedwait() avoid stranding a permit that
 * landed just after they gave up - and an ungranted one is dropped, because a
 * task running this syscall is by definition not parked and a later quit()
 * must not hand a permit to it.
 */
void semaphore_try_enter(context_t* ctx, uint32_t sem_id) {
	ctx->gpr[0] = SEM_RES_ERROR;

	if(sem_id == 0 || sem_id > SEMAPHORE_MAX)
		return;
	int32_t idx = (int32_t)sem_id - 1;

	proc_t* cproc = get_current_proc();
	if(cproc == NULL)
		return;

	int32_t pid = cproc->info.pid;

	proc_lock_enter();
	if(!sem_valid_idx(idx)) {
		proc_lock_leave();
		return;
	}

	int32_t slot = sem_waiter_find_locked(sem_id, pid);
	if(slot >= 0) {
		bool granted = (_waiters[slot].granted != 0);
		sem_waiter_del_locked(slot, pid);
		if(granted) {
			proc_lock_leave();
			ctx->gpr[0] = SEM_RES_ACQUIRED;
			return;
		}
	}

	ctx->gpr[0] = sem_try_acquire_locked(idx, pid) ?
		SEM_RES_ACQUIRED : SEM_RES_OCCUPIED;
	proc_lock_leave();
}

/*
 * Permits available right now, or -1 if there is no such semaphore. count is
 * never negative, so -1 is an unambiguous "no such semaphore" - this is what
 * libc's sem_getvalue() reports.
 */
int32_t semaphore_get_count(uint32_t sem_id) {
	if(sem_id == 0 || sem_id > SEMAPHORE_MAX)
		return -1;
	int32_t idx = (int32_t)sem_id - 1;

	proc_lock_enter();
	int32_t count = sem_valid_idx(idx) ? _semaphores[idx].count : -1;
	proc_lock_leave();
	return count;
}

/*
 * Return a permit and hand it to the oldest waiter.
 *
 * There is deliberately no owner check. A counting semaphore has to be postable
 * by a task that never waited (sem_post), and pthread_cond_signal/broadcast post
 * into a waiter queue they do not own. What stops an unmatched quit() from
 * corrupting a mutex is the max_count ceiling instead: on a binary semaphore
 * count is already 1 when it is free, so the quit is refused rather than quietly
 * making the lock admit two holders.
 */
int32_t semaphore_quit(uint32_t sem_id) {
	if(sem_id == 0 || sem_id > SEMAPHORE_MAX)
		return -1;
	int32_t idx = (int32_t)sem_id - 1;

	proc_t* cproc = get_current_proc();
	if(cproc == NULL)
		return -1;

	proc_lock_enter();
	if(!sem_valid_idx(idx) ||
			_semaphores[idx].count >= _semaphores[idx].max_count) {
		proc_lock_leave();
		return -1;
	}

	_semaphores[idx].count++;
	if(sem_is_binary(idx) && _semaphores[idx].occupied_pid == cproc->info.pid)
		_semaphores[idx].occupied_pid = -1;
	sem_refresh_occupied(idx);
	sem_handoff_locked(idx);
	proc_lock_leave();
	return 0;
}
