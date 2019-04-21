#include <kfile.h>
#include <kstring.h>
#include <system.h>
#include <proc.h>
#include <mm/kmalloc.h>

static int32_t _p_lock = 0;

void kf_unref(k_file_t* kf, uint32_t flags) {
	if(kf == NULL)
		return;

	CRIT_IN(_p_lock)
	if((flags & KF_WRITE) == 0) {//read mode
		if(kf->ref_r > 0)
			kf->ref_r--;
	}
	else {
		if(kf->ref_w > 0)
			kf->ref_w--;
	}
	if((kf->ref_w + kf->ref_r) == 0) { // close when ref cleared.
		km_free(kf);
	}
	CRIT_OUT(_p_lock)
}

void kf_ref(k_file_t* kf, uint32_t flags) {
	if(kf == NULL)
		return;

	CRIT_IN(_p_lock)
	if((flags & KF_WRITE) == 0) //read mode
		kf->ref_r++;
	else
		kf->ref_w++;
	CRIT_OUT(_p_lock)
}

int32_t kf_open(uint32_t node_addr, int32_t flags) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	CRIT_IN(_p_lock)
	k_file_t* kf = (k_file_t*)km_alloc(sizeof(k_file_t));	
	if(kf == NULL) {
		CRIT_OUT(_p_lock)
		return -1;
	}
	memset(kf, 0, sizeof(k_file_t));
	kf->node_addr = node_addr;
	kf_ref(kf, flags);

	int32_t i;
	for(i=0; i<FILE_MAX; i++) {
		if(proc->space->files[i].kf == NULL) {
			proc->space->files[i].kf = kf;
			proc->space->files[i].flags = flags;
			proc->space->files[i].seek = 0;
			CRIT_OUT(_p_lock)
			return i;
		}
	}

	kf_unref(kf, flags);
	CRIT_OUT(_p_lock)
	return -1;
}

void kf_close(int32_t fd) {
	process_t* proc = _current_proc;
	if(proc == NULL || fd < 0 || fd >= FILE_MAX)
		return;
	
	CRIT_IN(_p_lock)
	kf_unref(proc->space->files[fd].kf, proc->space->files[fd].flags);
	proc->space->files[fd].kf = NULL;
	proc->space->files[fd].flags = 0;
	proc->space->files[fd].seek = 0;
	CRIT_OUT(_p_lock)
}

uint32_t kf_node_addr(int32_t pid, int32_t fd) {
	CRIT_IN(_p_lock)
	process_t* proc = proc_get(pid);
	if(proc == NULL || fd < 0 || fd>= FILE_MAX) {
		CRIT_OUT(_p_lock)
		return 0;
	}

	k_file_t* kf = proc->space->files[fd].kf;
	if(kf == NULL) {
		CRIT_OUT(_p_lock)
		return 0;
	}
	return kf->node_addr;
}
