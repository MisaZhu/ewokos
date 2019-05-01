#include <semaphore.h>
#include <system.h>
#include <proc.h>

static inline uint32_t semaphore_phy_addr(int32_t* s) {
	uint32_t paddr = resolve_phy_address(_current_proc->space->vm, (uint32_t)s);
	return paddr;
}

static int32_t _p_lock = 0;

int32_t semaphore_init(int32_t *s) {
	if(s == NULL)
		return -1;
	CRIT_IN(_p_lock)
	*s = 0;
	CRIT_OUT(_p_lock)
	return 0;
}

int32_t semaphore_close(int32_t* s) {
	CRIT_IN(_p_lock)
	uint32_t paddr = semaphore_phy_addr(s);
	proc_unblock(paddr);
	*s = -1;
	CRIT_OUT(_p_lock)
	return 0;
}

int32_t semaphore_lock(int32_t* s) {
	CRIT_IN(_p_lock)
	if(s == NULL || (*s) < 0) {
		CRIT_OUT(_p_lock)
		return 0;
	}

	uint32_t paddr = semaphore_phy_addr(s);
	
	while(true) {
		if(*s > 0) {/*still locked by other process, put current process to block*/
			_current_proc->state = BLOCK;
			_current_proc->slept_by = paddr;
			CRIT_OUT(_p_lock)
			//schedule();
			return -1;
		}	
		else {
			(*s)++;
			break;
		}
	}	
	CRIT_OUT(_p_lock)
	return 0;
}

int32_t semaphore_unlock(int32_t* s) {
	CRIT_IN(_p_lock)
	if(s == NULL || (*s) <= 0) {
		CRIT_OUT(_p_lock)
		return 0;
	}
	(*s)--;
	if((*s) == 0) {/*if all locks cleared, wake up processed blocked by this semaphore*/
		uint32_t paddr = semaphore_phy_addr(s);
		proc_unblock(paddr);
	}
	CRIT_OUT(_p_lock)
	return 0;
}
