#include <kernel/kernel.h>
#include <kernel/svc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <kernel/proc.h>
#include <kernel/ipc.h>
#include <kernel/hw_info.h>
#include <kernel/kevqueue.h>
#include <mm/kalloc.h>
#include <mm/shm.h>
#include <mm/kmalloc.h>
#include <sysinfo.h>
#include <dev/uart.h>
#include <dev/timer.h>
#include <syscalls.h>
#include <kstring.h>
#include <kprintf.h>
#include <stddef.h>
#include <dev/fbinfo.h>

#ifdef FRAMEBUFFER
#include <dev/framebuffer.h>
#endif

static void sys_kprint(const char* s, int32_t len, bool tty_only) {
	(void)len;
	if(tty_only)
		uart_write(s, strlen(s));
	else
		printf(s);
}

static void sys_exit(context_t* ctx, int32_t pid, int32_t res) {
	proc_t* proc = _current_proc;
	if(pid >= 0)
		proc = proc_get(pid);

	ctx->gpr[0] = -1;
	if(proc == NULL || proc->info.owner < 0) {
		return;
	}

	if(_current_proc->info.owner <= 0 || proc->info.owner == _current_proc->info.owner) {
		ctx->gpr[0] = 0;
		proc_exit(ctx, proc, res);
	}
}

static int32_t sys_getpid(int32_t pid) {
	proc_t * proc = _current_proc;
	if(pid >= 0)
		proc = proc_get(pid);

	if(proc == NULL)
		return -1;

	if(_current_proc->info.owner > 0 && _current_proc->info.owner != proc->info.owner)
		return -1;

	proc_t* p = proc_get_proc(proc);
	if(p != NULL)
		return p->info.pid;
	return -1;
}

static int32_t sys_get_threadid(void) {
	if(_current_proc == NULL || _current_proc->info.type != PROC_TYPE_THREAD)
		return -1;
	return _current_proc->info.pid; 
}


static void sys_usleep(context_t* ctx, uint32_t count) {
	proc_usleep(ctx, count);
}

static int32_t sys_malloc(int32_t size) {
	return (int32_t)proc_malloc(size);
}

static int32_t sys_realloc(void* p, int32_t size) {
	return (int32_t)proc_realloc(p, size);
}

static void sys_free(int32_t p) {
	if(p == 0)
		return;
	proc_free((void*)p);
}

static void sys_fork(context_t* ctx) {
	proc_t *proc = kfork(PROC_TYPE_PROC);
	if(proc == NULL) {
		ctx->gpr[0] = -1;
		return;
	}

	memcpy(&proc->ctx, ctx, sizeof(context_t));
	proc->ctx.gpr[0] = 0;
	ctx->gpr[0] = proc->info.pid;
	if(proc->info.state == CREATED && _core_ready) {
		_current_proc->info.state = BLOCK;
		_current_proc->info.block_by = _core_pid;
		_current_proc->block_event = proc->info.pid;

		proc->info.state = BLOCK;
		proc->info.block_by = _core_pid;
		proc->block_event = proc->info.pid;
		schedule(ctx);
	}
}

static void sys_detach(void) {
	_current_proc->info.father_pid = 0;
}

static int32_t sys_thread(context_t* ctx, uint32_t entry, uint32_t func, int32_t arg) {
	proc_t *proc = kfork(PROC_TYPE_THREAD);
	if(proc == NULL)
		return -1;
	uint32_t sp = proc->ctx.sp;
	memcpy(&proc->ctx, ctx, sizeof(context_t));
	proc->ctx.sp = sp;
	proc->ctx.pc = entry;
	proc->ctx.lr = entry;
	proc->ctx.gpr[0] = func;
	proc->ctx.gpr[1] = arg;
	return proc->info.pid;
}

static void sys_waitpid(context_t* ctx, int32_t pid) {
	proc_waitpid(ctx, pid);
}

static void sys_load_elf(context_t* ctx, const char* cmd, void* elf, uint32_t elf_size) {
	if(elf == NULL) {
		ctx->gpr[0] = -1;
		return;
	}
	strcpy(_current_proc->info.cmd, cmd);
	if(proc_load_elf(_current_proc, elf, elf_size) != 0) {
		ctx->gpr[0] = -1;
		return;
	}

	ctx->gpr[0] = 0;
	memcpy(ctx, &_current_proc->ctx, sizeof(context_t));
}

static int32_t sys_proc_set_uid(int32_t uid) {
	if(_current_proc->info.owner > 0)	
		return -1;
	_current_proc->info.owner = uid;
	return 0;
}

static int32_t sys_proc_get_cmd(int32_t pid, char* cmd, int32_t sz) {
	proc_t* proc = proc_get(pid);
	if(proc == NULL)
		return -1;
	strncpy(cmd, proc->info.cmd, sz);
	return 0;
}

static void sys_proc_set_cmd(const char* cmd) {
	strncpy(_current_proc->info.cmd, cmd, PROC_INFO_CMD_MAX-1);
}

static void	sys_get_sysinfo(sysinfo_t* info) {
	if(info == NULL)
		return;

	strcpy(info->machine, get_hw_info()->machine);
	info->free_mem = get_free_mem_size();
	info->total_mem = get_hw_info()->phy_mem_size;
	info->shm_mem = shm_alloced_size();
	info->kernel_sec = _kernel_sec;
	info->kfs = get_hw_info()->kfs;
}

static int32_t sys_shm_alloc(uint32_t size, int32_t flag) {
	return shm_alloc(size, flag);
}

static void* sys_shm_map(int32_t id) {
	return shm_proc_map(_current_proc->info.pid, id);
}

static int32_t sys_shm_unmap(int32_t id) {
	return shm_proc_unmap(_current_proc->info.pid, id);
}

static int32_t sys_shm_ref(int32_t id) {
	return shm_proc_ref(_current_proc->info.pid, id);
}
	
static uint32_t sys_mmio_map(void) {
	if(_current_proc->info.owner > 0)
		return 0;
	hw_info_t* hw_info = get_hw_info();
	map_pages(_current_proc->space->vm, MMIO_BASE, hw_info->phy_mmio_base, hw_info->phy_mmio_base + hw_info->mmio_size, AP_RW_RW, 1);
	flush_tlb();
	return MMIO_BASE;
}
		
static int32_t sys_framebuffer_info(fbinfo_t* info) {
#ifdef FRAMEBUFFER
	if(_current_proc->info.owner > 0)
		return -1;
	fbinfo_t *fbinfo = fb_get_info();
	if(fbinfo->pointer == 0)
		return -1;
	memcpy(info, fbinfo, sizeof(fbinfo_t));
	return 0;
#else
	(void)info;
	return -1;
#endif
}
	
static int32_t sys_framebuffer_flush(void* buf, uint32_t size) {
#ifdef FRAMEBUFFER
	if(_current_proc->info.owner > 0)
		return -1;
	fbinfo_t *fbinfo = fb_get_info();
	if(fbinfo->pointer == 0)
		return -1;
	fb_dev_write(buf, size);
	return 0;
#else
	(void)buf;
	(void)size;
	return -1;
#endif
}

static uint32_t sys_kpage_map(void) {
	if(_current_proc->space->kpage != 0)
		return _current_proc->space->kpage;

	uint32_t page = (uint32_t)kalloc4k();
	map_page(_current_proc->space->vm, page, V2P(page), AP_RW_RW, 0);
	flush_tlb();
	_current_proc->space->kpage = page;
	return page;
}

static void sys_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra_data, uint32_t flags) {
	ctx->gpr[0] = proc_ipc_setup(ctx, entry, extra_data, flags);
	if((flags & IPC_NONBLOCK) == 0) {
		_current_proc->info.state = BLOCK;
		_current_proc->info.block_by = _current_proc->info.pid;
		schedule(ctx);
	}
}

static void sys_ipc_call(context_t* ctx, int32_t pid, int32_t call_id, proto_t* data) {
	ctx->gpr[0] = 0;
	if(_current_proc->info.pid == pid) {
		return;
	}
	if((call_id & 0xffff0000) != 0 && _current_proc->info.owner > 0) {
		//ipc call id > 0xffff0000 means kernel ipccall	
		return;
	}

	proc_t* proc = proc_get(pid);
	if(proc == NULL || proc->space->ipc.entry == 0)
		return;

	if(proc->info.ipc_busy) {
		ctx->gpr[0] = -1; //busy for single task , should retry
		proc_block_on(ctx, pid, (uint32_t)&proc->space->ipc);
		return;
	}
	proc->info.ipc_busy = true;

	ipc_t* ipc = proc_ipc_req(proc);
	ipc->state = IPC_BUSY;
	ipc->call_id = call_id;
	PF->init(&ipc->data, NULL, 0);
	ipc->pid_client = _current_proc->info.pid;
	ipc->pid_server = pid;
	if(data != NULL)
		PF->copy(&ipc->data, data->data, data->size);

	ctx->gpr[0] = (int32_t)ipc;
	proc_ipc_call(ctx, proc, ipc);
}

static void sys_ipc_get_return(context_t* ctx, ipc_t* ipc, proto_t* data) {
	if(ipc == NULL ||
			ipc->pid_client != _current_proc->info.pid ||
			ipc->state == IPC_IDLE) {
		ctx->gpr[0] = -2;
		return;
	}

	ctx->gpr[0] = 0;
	if(ipc->state != IPC_RETURN) {
		ctx->gpr[0] = -1;
		proc_block_on(ctx, ipc->pid_server, (uint32_t)ipc);
		return;
	}

	if(data != NULL) {
		data->total_size = data->size = ipc->data.size;
		if(data->size > 0) {
			data->data = (proto_t*)proc_malloc(ipc->data.size);
			memcpy(data->data, ipc->data.data, data->size);
		}
	}
	
	proc_t* proc = proc_get(ipc->pid_server);
	PF->clear(&ipc->data);
	proc_ipc_close(ipc);

	if(proc != NULL) {
		proc->info.ipc_busy = false;
		proc_wakeup(proc->info.pid, (uint32_t)&proc->space->ipc);
	}
}

static void sys_ipc_set_return(ipc_t* ipc, proto_t* data) {
	if(ipc == NULL || 
			_current_proc->space->ipc.entry == 0 ||
			ipc->state != IPC_BUSY) {
		return;
	}

	if(data != NULL)
		PF->copy(&ipc->data, data->data, data->size);
}

static void sys_ipc_end(context_t* ctx, ipc_t* ipc) {
	if(ipc == NULL ||
			_current_proc->space->ipc.entry == 0 ||
			ipc->state != IPC_BUSY) {
		return;
	}

	_current_proc->info.state = ipc->proc_state;
	if(ipc->proc_state == READY || ipc->proc_state == RUNNING)
		proc_ready(_current_proc);

	memcpy(ctx, &ipc->ctx, sizeof(context_t));
	ipc->state = IPC_RETURN;
	proc_wakeup(_current_proc->info.pid, (uint32_t)ipc);
	schedule(ctx);
}

static void sys_ipc_lock(void) {
	_current_proc->info.ipc_busy = true;
}

static void sys_ipc_unlock(void) {
	_current_proc->info.ipc_busy = false;
	proc_wakeup(_current_proc->info.pid, (uint32_t)&_current_proc->space->ipc);
}

static int32_t sys_ipc_get_info(ipc_t* ipc, int32_t* pid, int32_t* cmd) {
	if(ipc == NULL || _current_proc->space->ipc.entry == 0 ||
			ipc->state != IPC_BUSY) {
		return 0;
	}

	*pid = ipc->pid_client;
	*cmd = ipc->call_id;

	proto_t* ret = NULL;
	if(ipc->data.size > 0) {
		ret = (proto_t*)proc_malloc(sizeof(proto_t));
		memset(ret, 0, sizeof(proto_t));
		ret->data = proc_malloc(ipc->data.size);
		ret->total_size = ret->size = ipc->data.size;
		memcpy(ret->data, ipc->data.data, ipc->data.size);
	}
	return (int32_t)ret;
}

static int32_t sys_proc_ping(int32_t pid) {
	proc_t* proc = proc_get(pid);
	if(proc == NULL || !proc->space->ready_ping)
		return -1;
	return 0;
}

static void sys_proc_ready_ping(void) {
	_current_proc->space->ready_ping = true;
}

static kevent_t* sys_get_kevent_raw(void) {
	if(_current_proc->info.owner > 0)	
		return NULL;

	kevent_t* kev = kev_pop();
	if(kev == NULL) {
		return NULL;
	}

	kevent_t* ret = (kevent_t*)proc_malloc(sizeof(kevent_t));
	ret->type = kev->type;
	proto_t* data = ((proto_t*)kev->data);
	if(data != NULL && data->size > 0) {
		proto_t* ret_data = (proto_t*)proc_malloc(sizeof(proto_t));
		memset(ret_data, 0, sizeof(proto_t));
		ret_data->data = proc_malloc(data->size);
		ret_data->total_size = ret_data->size = data->size;
		memcpy(ret_data->data, data->data, data->size);
		ret->data = ret_data;
	}

	if(data != NULL)
		proto_free(data);
	kfree(kev);
	return ret;
}

static void sys_get_kevent(context_t* ctx) {
	ctx->gpr[0] = 0;	
	kevent_t* kev = sys_get_kevent_raw();
	if(kev == NULL) {
		proc_block_on(ctx, -1, (uint32_t)kev_init);
		return;
	}
	ctx->gpr[0] = (int32_t)kev;	
}

static void sys_proc_block(context_t* ctx, int32_t pid, uint32_t evt) {
	proc_t* proc = NULL;
	if(pid == _current_proc->info.pid)
		proc = _current_proc;
	else
		proc = proc_get_proc(proc_get(pid));

	_current_proc->info.state = BLOCK;
	_current_proc->block_event = evt;
	_current_proc->info.block_by = proc->info.pid;
	schedule(ctx);	
}

static void sys_proc_wakeup(uint32_t evt) {
	proc_t* proc = proc_get_proc(_current_proc);
	if(proc->info.block_by == proc->info.pid)
		proc->space->ipc.ctx.proc_state = READY;
	proc_wakeup(proc->info.pid, evt);
}

static void sys_core_ready(void) {
	if(_current_proc->info.owner > 0)
		return;
	_core_ready = true;
	_core_pid = _current_proc->info.pid;
}

static int32_t sys_core_pid(void) {
	return _core_pid;
}

static uint32_t _svc_tic = 0;
static int32_t sys_get_kernel_tic(void) {
	return _svc_tic;
}

#ifndef SDC
int32_t kfs_get(uint32_t index, char* name, char* data);
static int32_t sys_kfs_get(uint32_t index, char* name, char* data) {
	return kfs_get(index, name, data);
}
#else
static int32_t sys_kfs_get(uint32_t index, char* name, char* data) {
	(void)index;
	(void)name;
	(void)data;
	return -1;
}
#endif

void svc_handler(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx, int32_t processor_mode) {
	(void)processor_mode;
	_svc_tic++;

	__irq_disable();

	switch(code) {
	case SYS_EXIT:
		sys_exit(ctx, arg0, arg1);
		return;
	case SYS_MALLOC:
		ctx->gpr[0] = sys_malloc(arg0);
		return;
	case SYS_REALLOC:
		ctx->gpr[0] = sys_realloc((void*)arg0, arg1);
		return;
	case SYS_FREE:
		sys_free(arg0);
		return;
	case SYS_GET_PID:
		ctx->gpr[0] = sys_getpid(arg0);
		return;
	case SYS_GET_THREAD_ID:
		ctx->gpr[0] = sys_get_threadid();
		return;
	case SYS_USLEEP:
		sys_usleep(ctx, (uint32_t)arg0);
		return;
	case SYS_EXEC_ELF:
		sys_load_elf(ctx, (const char*)arg0, (void*)arg1, (uint32_t)arg2);
		return;
	case SYS_FORK:
		sys_fork(ctx);
		return;
	case SYS_DETACH:
		sys_detach();
		return;
	case SYS_WAIT_PID:
		sys_waitpid(ctx, arg0);
		return;
	case SYS_YIELD: 
		schedule(ctx);
		return;
	case SYS_PROC_SET_UID: 
		ctx->gpr[0] = sys_proc_set_uid(arg0);
		return;
	case SYS_PROC_GET_UID: 
		ctx->gpr[0] = _current_proc->info.owner;
		return;
	case SYS_PROC_GET_CMD: 
		ctx->gpr[0] = sys_proc_get_cmd(arg0, (char*)arg1, arg2);
		return;
	case SYS_PROC_SET_CMD: 
		sys_proc_set_cmd((const char*)arg0);
		return;
	case SYS_GET_SYSINFO:
		sys_get_sysinfo((sysinfo_t*)arg0);
		return;
	case SYS_GET_KERNEL_TIC:
		ctx->gpr[0] = sys_get_kernel_tic();
		return;
	case SYS_GET_PROCS: 
		ctx->gpr[0] = (int32_t)get_procs((int32_t*)arg0);
		return;
	case SYS_PROC_SHM_ALLOC:
		ctx->gpr[0] = sys_shm_alloc(arg0, arg1);
		return;
	case SYS_PROC_SHM_MAP:
		ctx->gpr[0] = (int32_t)sys_shm_map(arg0);
		return;
	case SYS_PROC_SHM_UNMAP:
		ctx->gpr[0] = sys_shm_unmap(arg0);
		return;
	case SYS_PROC_SHM_REF:
		ctx->gpr[0] = sys_shm_ref(arg0);
		return;
	case SYS_THREAD:
		ctx->gpr[0] = sys_thread(ctx, (uint32_t)arg0, (uint32_t)arg1, arg2);
		return;
	case SYS_KPRINT:
		sys_kprint((const char*)arg0, arg1, (bool)arg2);
		return;
	case SYS_MMIO_MAP:
		ctx->gpr[0] = sys_mmio_map();
		return;
	case SYS_KPAGE_MAP:
		ctx->gpr[0] = sys_kpage_map();
		return;
	case SYS_FRAMEBUFFER_INFO:
		ctx->gpr[0] = sys_framebuffer_info((fbinfo_t*)arg0);
		return;
	case SYS_FRAMEBUFFER_FLUSH:
		ctx->gpr[0] = sys_framebuffer_flush((void*)arg0, (uint32_t)arg1);
		return;
	case SYS_IPC_SETUP:
		sys_ipc_setup(ctx, arg0, arg1, arg2);
		return;
	case SYS_IPC_CALL:
		sys_ipc_call(ctx, arg0, arg1, (proto_t*)arg2);
		return;
	case SYS_IPC_GET_RETURN:
		sys_ipc_get_return(ctx, (ipc_t*)arg0, (proto_t*)arg1);
		return;
	case SYS_IPC_SET_RETURN:
		sys_ipc_set_return((ipc_t*)arg0, (proto_t*)arg1);
		return;
	case SYS_IPC_END:
		sys_ipc_end(ctx, (ipc_t*)arg0);
		return;
	case SYS_IPC_GET_ARG:
		ctx->gpr[0] = sys_ipc_get_info((ipc_t*)arg0, (int32_t*)arg1, (int32_t*)arg2);
		return;
	case SYS_PROC_PING:
		ctx->gpr[0] = sys_proc_ping(arg0);
		return;
	case SYS_PROC_READY_PING:
		sys_proc_ready_ping();
		return;
	case SYS_GET_KEVENT:
		sys_get_kevent(ctx);
		return;
	case SYS_WAKEUP:
		sys_proc_wakeup(arg0);
		return;
	case SYS_BLOCK:
		sys_proc_block(ctx, arg0, arg1);
		return;
	case SYS_CORE_READY:
		sys_core_ready();
		return;
	case SYS_CORE_PID:
		ctx->gpr[0] = sys_core_pid();
		return;
	case SYS_IPC_LOCK:
		sys_ipc_lock();
		return;
	case SYS_IPC_UNLOCK:
		sys_ipc_unlock();
		return;
	case SYS_KFS_GET:
		ctx->gpr[0] = sys_kfs_get(arg0, (char*)arg1, (char*)arg2);
		return;
	}
	printf("pid:%d, code(%d) error!\n", _current_proc->info.pid, code);
}

