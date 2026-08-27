#include <kernel/kernel.h>
#include <kernel/interrupt.h>
#include <kernel/svc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <kernel/proc.h>
#include <kernel/ipc.h>
#include <kernel/hw_info.h>
#include <kernel/semaphore.h>
#include <kernel/kevqueue.h>
#include <kernel/signal.h>
#include <kernel/core.h>
#include <mm/kalloc.h>
#include <mm/shm.h>
#include <mm/dma.h>
#include <mm/kmalloc.h>
#include <sysinfo.h>
#include <dev/timer.h>
#include <syscalls.h>
#include <kstring.h>
#include <signals.h>
#include <kprintf.h>
#include <stddef.h>

#ifndef PTE_ATTR_FRAMEBUFFER
#define PTE_ATTR_FRAMEBUFFER PTE_ATTR_DEV
#endif

static uint32_t _svc_counter[SYS_CALL_NUM];
static uint32_t _svc_total;
#ifdef KERNEL_SMP
static int32_t _svc_stat_spin = 0;
#endif

static inline void svc_stat_lock(void) {
#ifdef KERNEL_SMP
    mcore_lock(&_svc_stat_spin);
#endif
}

static inline void svc_stat_unlock(void) {
#ifdef KERNEL_SMP
    mcore_unlock(&_svc_stat_spin);
#endif
}

static inline void svc_account(int32_t code) {
    if(code < 0 || code >= SYS_CALL_NUM)
        return;
    svc_stat_lock();
    _svc_total++;
    _svc_counter[code]++;
    svc_stat_unlock();
}

static inline bool svc_is_query_fastpath(int32_t code) {
    switch(code) {
    case SYS_GET_PID:
    case SYS_GET_THREAD_ID:
    case SYS_PROC_GET_UID:
    case SYS_PROC_GET_GID:
    case SYS_PROC_GET_CMD:
    case SYS_IPC_PING:
    case SYS_CORE_PID:
    case SYS_GET_SYS_INFO:
    case SYS_GET_VSYSCALL_INFO:
    case SYS_GET_PROC:
    case SYS_GET_PROCS_NUM:
    case SYS_GET_PROCS:
        return true;
    default:
        return false;
    }
}

static void sys_kprint(const char* s, uint32_t len) {
    kout(s, len);
}

static void sys_exit(context_t* ctx, int32_t res) {
    ctx->gpr[0] = 0;
    proc_t* cproc = get_current_proc();
    proc_exit(ctx, cproc, res);
}

static int32_t sys_signal_setup(ewokos_addr_t entry) {
    return proc_signal_setup(entry);
}

static void sys_signal(context_t* ctx, int32_t pid, int32_t sig) {
    ctx->gpr[0] = -1;
    proc_t* proc = proc_get(pid);
    proc_t* cproc = get_current_proc();
    if((cproc->info.uid > 0 &&
            cproc->info.uid != proc->info.uid) ||
            proc->info.uid < 0) {
        return;
    }

    ctx->gpr[0] = 0;
    if((sig == SYS_SIG_KILL || sig == SYS_SIG_STOP) &&
            proc != cproc &&
            proc->info.state != RUNNING &&
            proc->info.state != READY) {
        proc_exit(ctx, proc, 0);
        return;
    }
    proc_signal_send(ctx, proc, sig, true);
}

static void sys_signal_end(context_t* ctx) {
    proc_signal_end(ctx);
}

static int32_t sys_getpid(int32_t pid) {
    return proc_get_root_pid_visible(pid);
}

static int32_t sys_get_thread_id(void) {
    return proc_get_current_thread_id_safe();
}

static void sys_usleep(context_t* ctx, uint32_t count) {
    proc_t * cproc = get_current_proc();
    if(cproc->info.type == TASK_TYPE_PROC && cproc->space->interrupt.state != INTR_STATE_IDLE)
        return;

    /*
     * Only the context actually serving an ipc request may take the
     * "schedule instead of true sleep" fast path (single-task mode only;
     * see proc_ipc_sync_serving()). In multi_task mode the owner proc
     * never serves requests itself, so its main loop and regular worker
     * threads (e.g. netd's net_thread) must still enter real
     * proc_usleep().
     */
    if(cproc->info.type == TASK_TYPE_PROC && proc_ipc_sync_serving(cproc)) {
        schedule(ctx);
        return;
    }

    proc_usleep(ctx, count);
}

static ewokos_addr_t sys_malloc(int32_t size) {
    return (ewokos_addr_t)proc_malloc(get_current_proc(), size);
}

static ewokos_addr_t sys_msize(void) {
    return proc_msize(get_current_proc());
}

static void sys_free(ewokos_addr_t p) {
    if(p == 0)
        return;
    proc_free(get_current_proc());
}

static void sys_fork(context_t* ctx) {
    proc_t *proc;
    proc = kfork(ctx, TASK_TYPE_PROC);
    if(proc == NULL) {
        ctx->gpr[0] = -1;
        return;
    }

    memcpy(&proc->ctx, ctx, sizeof(context_t));
    proc->ctx.gpr[0] = 0;
    ctx->gpr[0] = proc->info.pid;

    if(proc->info.state == CREATED && _core_proc_ready) {
        proc->info.state = BLOCK;

        proc_t* cproc = get_current_proc();
        cproc->info.state = BLOCK;
        cproc->ctx.gpr[0] = proc->info.pid;
        schedule(ctx);
    }
}

static void sys_detach(void) {
    proc_t* cproc = get_current_proc();
    cproc->info.father_pid = 0;
}

static void sys_thread(context_t* ctx, ewokos_addr_t entry, ewokos_addr_t func, ewokos_addr_t arg) {
    ctx->gpr[0] = -1;
    proc_t *proc = kfork(ctx, TASK_TYPE_THREAD);
    if(proc == NULL)
        return;
    ctx->gpr[0] = proc->info.pid;

    proc->ctx.pc = entry;
    proc->ctx.lr = entry;
    proc->ctx.gpr[0] = func;
    proc->ctx.gpr[1] = arg;
}

static void sys_waitpid(context_t* ctx, int32_t pid) {
    proc_waitpid(ctx, pid);
}

static void sys_load_elf(context_t* ctx, const char* cmd, void* elf, uint32_t elf_size) {
    if(elf == NULL) {
        printf("Panic: load elf content is NULL!\n");
        ctx->gpr[0] = -1;
        return;
    }

    if(strlen(cmd) >= PROC_INFO_MAX_CMD_LEN) {
        printf("Panic: proc cmd line too long!\n");
        ctx->gpr[0] = -1;
        return;
    }

    proc_t* cproc = get_current_proc();
    strcpy(cproc->info.cmd, cmd);
    if(proc_load_elf(cproc, elf, elf_size) != 0) {
        ctx->gpr[0] = -1;
        return;
    }

    ctx->gpr[0] = 0;
    memcpy(ctx, &cproc->ctx, sizeof(context_t));
}

static int32_t sys_proc_set_uid(int32_t uid) {
    proc_t* cproc = get_current_proc();
    if(cproc->info.uid > 0)	
        return -1;
    cproc->info.uid = uid;
    return 0;
}

static int32_t sys_proc_set_gid(int32_t gid) {
    proc_t* cproc = get_current_proc();
    if(cproc->info.uid > 0)	
        return -1;
    cproc->info.gid = gid;
    return 0;
}

static int32_t sys_proc_get_cmd(int32_t pid, char* cmd, int32_t sz) {
    return proc_get_cmd_safe(pid, cmd, sz);
}

static void sys_proc_set_cmd(const char* cmd) {
    proc_t* cproc = get_current_proc();
    sstrncpy(cproc->info.cmd, cmd, PROC_INFO_MAX_CMD_LEN-1);
}

static int32_t	sys_get_sys_info(sys_info_t* info) {
    if(info == NULL)
        return -1;
    memcpy(info, &_sys_info, sizeof(sys_info_t));
    info->max_proc_num = _kernel_config.max_proc_num;
    info->max_task_num = _kernel_config.max_task_num;
    info->max_task_per_proc = _kernel_config.max_task_per_proc;
    proc_get_core_runtime_stats(info->core_procs, info->core_idles, info->core_kernels, _sys_info.cores);
    return 0;
}

static int32_t	sys_get_sys_state(sys_state_t* info) {
    if(info == NULL)
        return -1;

    info->mem.free = get_free_mem_size();
    info->mem.kfree = kmalloc_free_size();
    info->mem.shared = shm_alloced_size();
    info->kernel_usec = _kernel_info.uptime_usec;
    svc_stat_lock();
    info->svc_total = _svc_total;
    memcpy(info->svc_counter, _svc_counter, SYS_CALL_NUM*4);
    svc_stat_unlock();
    return 0;
}

static vsyscall_info_t* sys_get_vsyscall_info(void) {
    return _kernel_info.vsyscall_info;
}

static int32_t sys_shm_get(int32_t id, uint32_t size, int32_t flag) {
    return (int32_t)shm_get(id, size, flag);
}

static void* sys_shm_map(int32_t id) {
    proc_t* cproc = proc_get_proc(get_current_proc());
    void* ret = shm_proc_map(cproc, id);
    return ret;
}

static int32_t sys_shm_unmap(void* p) {
    proc_t* cproc = proc_get_proc(get_current_proc());
    return shm_proc_unmap(cproc, p);
}
        
static ewokos_addr_t sys_dma_alloc(int32_t dma_block_id, uint32_t size) {
    proc_t* cproc = proc_get_proc(get_current_proc());
    if(cproc->info.uid > 0)
        return 0;

    ewokos_addr_t paddr = dma_alloc(dma_block_id, cproc->info.pid, size);
    if(paddr == 0)
        return 0;
    ewokos_addr_t vaddr = dma_v_addr(dma_block_id, paddr);
    if(vaddr == 0)
        return 0;

    map_pages_size(cproc->space->vm, vaddr, paddr, size, AP_RW_RW, PTE_ATTR_NOCACHE);
    flush_tlb();
    return vaddr;
}

static void sys_dma_free(int32_t dma_block_id, ewokos_addr_t vaddr) {
    proc_t* cproc = proc_get_proc(get_current_proc());
    if(cproc->info.uid > 0)
        return;

    ewokos_addr_t paddr = dma_phy_addr(dma_block_id, vaddr);
    if(paddr == 0)
        return;
    uint32_t size = dma_size(dma_block_id, cproc->info.pid, paddr);
    dma_free(dma_block_id, cproc->info.pid, paddr);

    unmap_pages(cproc->space->vm, vaddr, size/PAGE_SIZE);
    flush_tlb();
}

static int32_t sys_dma_set(ewokos_addr_t phy_base, uint32_t size, bool shared) {
    proc_t* cproc = get_current_proc();
    if(cproc == NULL)
        return -1;
    return dma_set(cproc->info.pid, phy_base, phy_base, size, shared);
}

static ewokos_addr_t sys_dma_phy(int32_t dma_block_id, ewokos_addr_t vaddr) {
    return dma_phy_addr(dma_block_id, vaddr);
}

static ewokos_addr_t sys_shm_contig_phy(int32_t shm_id, ewokos_addr_t vaddr) {
    return shm_contig_phy_addr(shm_id, vaddr);
}

static ewokos_addr_t sys_mem_map(ewokos_addr_t vaddr, ewokos_addr_t paddr, uint32_t size) {
    proc_t* cproc = proc_get_proc(get_current_proc());
    uint32_t attr;
    int32_t is_normal_ram;
    if(cproc->info.uid > 0)
        return 0;

    if(size == 0)
        return 0;

    /*
     * The sys_dma pool is reserved driver memory carved out below the
     * allocable heap. dma_alloc() maps its buffer only into the caller,
     * so another root daemon (g2dd attaching a client's dma canvas) must
     * be able to map the same physical range into itself. Same NOCACHE
     * attribute as sys_dma_alloc() to avoid cache aliasing between the
     * two mappings.
     */
    if(paddr >= _sys_info.sys_dma.phy_base &&
            (paddr + size) <= (_sys_info.sys_dma.phy_base + _sys_info.sys_dma.size)) {
        size = ALIGN_UP(size, PAGE_SIZE);
        map_pages_size(cproc->space->vm, vaddr, paddr, size, AP_RW_RW, PTE_ATTR_NOCACHE);
        flush_tlb();
        return vaddr;
    }

    /*allocatable memory can only mapped by kernel,
    userspace can map upper address such as MMIO/FRAMEBUFFER... */
    if(check_mem_map_arch(paddr, size) != 0) {
        return 0;
    }
    size = ALIGN_UP(size, PAGE_SIZE);

    /*
     * RAM (framebuffer) -> Normal Non-Cacheable: writes bypass cache
     * so the HVS DMA sees them in DRAM; unlike Device-nGnRE the CPU
     * may combine consecutive stores into burst transactions.
     * MMIO / reserved carveouts -> Device-nGnRE: strict ordering for
     * register or firmware-owned regions.
     */
    attr = PTE_ATTR_DEV;
    is_normal_ram = mem_map_is_normal_ram_arch(paddr, size);
    if(is_normal_ram)
        attr = PTE_ATTR_NOCACHE;
    map_pages_size(cproc->space->vm, vaddr, paddr, size, AP_RW_RW, attr);
    flush_tlb();
    return vaddr;
}

static void sys_ipc_setup(context_t* ctx, ewokos_addr_t entry, ewokos_addr_t extra_data, uint32_t flags) {
    ctx->gpr[0] = proc_ipc_setup(ctx, entry, extra_data, flags);
}

/*
 * The core logic of SYS_IPC_CALL / GET_RETURN / GET_ARG / SET_RETURN /
 * END / DISABLE / ENABLE lives in kernel/ipc.c (proc_ipc_*); the syscall
 * table below dispatches into those directly.
 */

static int32_t sys_proc_ping(int32_t pid) {
    return proc_get_ready_ping_safe(pid);
}

static void sys_proc_priority(int32_t pid, uint32_t priority) {
    proc_t* cproc = get_current_proc();
    if(cproc->info.uid > 0)
        return;

    proc_t* proc = proc_get(pid);
    if(proc == NULL)
        return;
    proc->info.priority = priority;
}

static void sys_proc_ready_ping(void) {
    proc_t* proc = proc_get_proc(get_current_proc());
    proc->space->ready_ping = true;
}

static void sys_get_kevent(context_t* ctx, kevent_t* kev) {
    ctx->gpr[0] = -1;
    if(kev_pop(kev) != 0) {
        //proc_block_on(ctx, -1, (uint32_t)kev_init);
        return;
    }
    ctx->gpr[0] = 0;
}

static proc_block_event_t* get_block_evt(proc_t* proc, uint32_t event) {
    if(proc == NULL || event == 0)
        return NULL;

    for(int32_t i=0; i<BLOCK_EVT_MAX; i++) {
        proc_block_event_t* block_evt = &proc->space->block_events[i];
        if(block_evt->event == event)
            return block_evt;
    }
    return NULL;
}

static void set_block_evt(proc_t* proc, uint32_t event) {
    if(proc == NULL || event == 0)
        return;

    for(int32_t i=0; i<BLOCK_EVT_MAX; i++) {
        proc_block_event_t* block_evt = &proc->space->block_events[i];
        if(block_evt->event == 0) {
            block_evt->event = event;
            block_evt->refs = 0;
            break;
        }
    }
}

static void sys_proc_block(context_t* ctx, ewokos_addr_t token) {
    proc_t* cproc = get_current_proc();
    if(cproc == NULL)
        return;

    /*
     * Don't block the context while it synchronously serves an ipc request
     * (single-task mode only - there the server's main context is hijacked
     * and blocking it would strand the restore state machine). multi_task
     * worker threads are independent contexts and are allowed to block; the
     * client simply keeps waiting, and the watchdog freezes while parked.
     */
    if(proc_ipc_sync_serving(cproc)) {
        schedule(ctx);
        return;
    }

    proc_block_by(ctx, cproc, token);
}

static void sys_proc_wakeup(context_t* ctx, int32_t pid, ewokos_addr_t token) {
    (void)ctx;
    proc_t* cproc = proc_get_proc(get_current_proc());
    if(cproc->info.uid > 0)
        return;
    proc_t* proc = proc_get(pid);
    proc_wakeup_by(proc, token);
}

static void sys_core_proc_ready(void) {
    proc_t* cproc = get_current_proc();
    if(cproc->info.uid > 0)
        return;
    _core_proc_ready = true;
    proc_set_core_pid_safe(cproc->info.pid);
}

static int32_t sys_core_proc_pid(void) {
    return proc_get_core_pid_safe();
}

static int32_t sys_get_kernel_tic(uint32_t* sec, uint32_t* hi, uint32_t* low) {
    if(sec != NULL)
        *sec = _kernel_info.uptime_sec;
    if(hi != NULL) 
        *hi = _kernel_info.uptime_usec >> 32;
    if(low != NULL)
        *low = _kernel_info.uptime_usec & 0xffffffff;
    return 0;
}

static int32_t sys_interrupt_setup(uint32_t interrupt, ewokos_addr_t entry, ewokos_addr_t data) {
    proc_t * cproc = get_current_proc();
    if(cproc->info.uid > 0)
        return -1;
    return interrupt_setup(cproc, interrupt, entry, data);
}

static void sys_interrupt_end(context_t* ctx) {
    interrupt_end(ctx);
}

static inline void sys_soft_int(context_t* ctx, int32_t to_pid, ewokos_addr_t entry, ewokos_addr_t data) {
    ctx->gpr[0] = 0;
    proc_t* proc = proc_get_proc(get_current_proc());
    if(proc->info.uid > 0)
        ctx->gpr[0] = -2;
    interrupt_soft_send(ctx, to_pid, entry, data);
}

static inline int32_t sys_proc_uuid(int32_t pid) {
    proc_t* proc = proc_get(pid);
    if(proc == NULL)
        return 0;
    return proc->info.uuid;
}

static inline void sys_mmio_rw(ewokos_addr_t arg0, uint32_t arg1, uint32_t arg2, context_t* ctx){
        volatile uint32_t *reg = (volatile uint32_t *)arg0;
        uint32_t val = arg1;
        uint32_t mask = arg2;

        if(arg0 >= MMIO_BASE && arg0 < (MMIO_BASE + _sys_info.mmio.size)) {
        *reg &= ~(mask);
        *reg |= (val & mask);
        ctx->gpr[0] = *reg;
    }
}

static inline void _svc_handler(int32_t code, ewokos_addr_t arg0, ewokos_addr_t arg1, ewokos_addr_t arg2, context_t* ctx) {
    switch(code) {
    case SYS_EXIT:
        sys_exit(ctx, arg0);
        return;
    case SYS_SIGNAL_SETUP:
        sys_signal_setup(arg0);
        return;
    case SYS_SIGNAL:
        sys_signal(ctx, arg0, arg1);
        return;
    case SYS_SIGNAL_END:
        sys_signal_end(ctx);
        return;
    case SYS_MALLOC_EXPAND:
        ctx->gpr[0] = sys_malloc(arg0);
        return;
    case SYS_MALLOC_SIZE:
        ctx->gpr[0] = sys_msize();
        return;
    case SYS_FREE:
        sys_free(arg0);
        return;
    case SYS_GET_PID:
        ctx->gpr[0] = sys_getpid(arg0);
        return;
    case SYS_GET_THREAD_ID:
        ctx->gpr[0] = sys_get_thread_id();
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
        ctx->gpr[0] = proc_get_current_uid_safe();
        return;
    case SYS_PROC_SET_GID: 
        ctx->gpr[0] = sys_proc_set_gid(arg0);
        return;
    case SYS_PROC_GET_GID: 
        ctx->gpr[0] = proc_get_current_gid_safe();
        return;
    case SYS_PROC_GET_CMD: 
        ctx->gpr[0] = sys_proc_get_cmd(arg0, (char*)arg1, arg2);
        return;
    case SYS_PROC_SET_CMD: 
        sys_proc_set_cmd((const char*)arg0);
        return;
    case SYS_GET_SYS_INFO:
        ctx->gpr[0] = sys_get_sys_info((sys_info_t*)arg0);
        return;
    case SYS_GET_SYS_STATE:
        ctx->gpr[0] = sys_get_sys_state((sys_state_t*)arg0);
        return;
    case SYS_GET_VSYSCALL_INFO:
        ctx->gpr[0] = (ewokos_addr_t)sys_get_vsyscall_info();
        return;
    case SYS_GET_PROC: 
        ctx->gpr[0] = (int32_t)get_proc(arg0, (procinfo_t*)arg1);
        return;
    case SYS_GET_PROCS_NUM: 
        ctx->gpr[0] = (int32_t)get_procs_num();
        return;
    case SYS_GET_PROCS: 
        ctx->gpr[0] = (int32_t)get_procs(arg0, (procinfo_t*)arg1);
        return;
    case SYS_PROC_SHM_GET:
        ctx->gpr[0] = (int32_t)sys_shm_get(arg0, arg1, arg2);
        return;
    case SYS_PROC_SHM_MAP:
        ctx->gpr[0] = (ewokos_addr_t)sys_shm_map(arg0);
        return;
    case SYS_PROC_SHM_UNMAP:
        ctx->gpr[0] = sys_shm_unmap((void*)arg0);
        return;
    case SYS_SHM_CONTIG_PHY_ADDR:
        ctx->gpr[0] = sys_shm_contig_phy(arg0, (ewokos_addr_t)arg1);
        return;
    case SYS_THREAD:
        sys_thread(ctx, arg0, arg1, arg2);
        return;
    case SYS_KPRINT:
        sys_kprint((const char*)arg0, arg1);
        return;
    case SYS_MEM_MAP:
        ctx->gpr[0] = sys_mem_map(arg0, arg1, (uint32_t)arg2);
        return;
    case SYS_DMA_ALLOC:
        ctx->gpr[0] = sys_dma_alloc(arg0, (uint32_t)arg1);
        return;
    case SYS_DMA_FREE:
        sys_dma_free(arg0, arg1);
        return;
    case SYS_DMA_SET:
        ctx->gpr[0] = sys_dma_set((ewokos_addr_t)arg0, (uint32_t)arg1, arg2);
        return;
    case SYS_DMA_PHY_ADDR:
        ctx->gpr[0] = sys_dma_phy(arg0, (ewokos_addr_t)arg1);
        return;
    case SYS_IPC_SETUP:
        sys_ipc_setup(ctx, arg0, arg1, arg2);
        return;
    case SYS_IPC_CALL:
        proc_ipc_call(ctx, arg0, arg1, (proto_t*)arg2);
        return;
    case SYS_IPC_GET_RETURN:
        proc_ipc_get_return(ctx, arg0, (uint32_t)arg1, (proto_t*)arg2);
        return;
    case SYS_IPC_SET_RETURN:
        proc_ipc_set_return((uint32_t)arg0, (proto_t*)arg1);
        return;
    case SYS_IPC_END:
        proc_ipc_end(ctx);
        return;
    case SYS_IPC_GET_ARG:
        ctx->gpr[0] = proc_ipc_get_arg((uint32_t)arg0, (int32_t*)arg1, (proto_t*)arg2);
        return;
    case SYS_IPC_PING:
        ctx->gpr[0] = sys_proc_ping(arg0);
        return;
    case SYS_IPC_READY:
        sys_proc_ready_ping();
        return;
    case SYS_GET_KEVENT:
        sys_get_kevent(ctx, (kevent_t*)arg0);
        return;
    case SYS_WAKEUP:
        sys_proc_wakeup(ctx, arg0, (uint32_t)arg1);
        return;
    case SYS_BLOCK:
        sys_proc_block(ctx, (uint32_t)arg0);
        return;
    case SYS_CORE_READY:
        sys_core_proc_ready();
        return;
    case SYS_CORE_PID:
        ctx->gpr[0] = sys_core_proc_pid();
        return;
    case SYS_IPC_DISABLE:
        ctx->gpr[0] = proc_ipc_disable();
        return;
    case SYS_IPC_ENABLE:
        proc_ipc_enable();
        return;
    case SYS_INTR_SETUP:
                ctx->gpr[0] = sys_interrupt_setup((uint32_t)arg0, arg1, arg2);
        return;
    case SYS_INTR_END:
        sys_interrupt_end(ctx);
        return;
    case SYS_SEMAPHORE_ALLOC:
        ctx->gpr[0] = semaphore_alloc();
        return;
    case SYS_SEMAPHORE_FREE:
        semaphore_free(arg0);
        return;
    case SYS_SEMAPHORE_ENTER:
        semaphore_enter(ctx, arg0);
        return;
    case SYS_SEMAPHORE_TRY_ENTER:
        semaphore_try_enter(ctx, arg0);
        return;
    case SYS_SEMAPHORE_QUIT:
        ctx->gpr[0] = semaphore_quit(arg0);
        return;
    case SYS_SOFT_INT:
        sys_soft_int(ctx, arg0, arg1, arg2);
        return;
    case SYS_V2P:
        ctx->gpr[0] = V2P(arg0);
        return;
    case SYS_P2V:
        ctx->gpr[0] = P2V(arg0);
        return;
    case SYS_MMIO_RW:
                sys_mmio_rw(arg0, (uint32_t)arg1, (uint32_t)arg2, ctx);
        return;
    case SYS_PROC_PRIORITY:
        sys_proc_priority(arg0, (uint32_t)arg1);
        return;
    }
}

inline void svc_handler(int32_t code, ewokos_addr_t arg0, ewokos_addr_t arg1, ewokos_addr_t arg2, context_t* ctx) {
    __irq_disable();
    svc_account(code);
    if(svc_is_query_fastpath(code)) {
        _svc_handler(code, arg0, arg1, arg2, ctx);
        return;
    }
    kernel_lock();
    _svc_handler(code, arg0, arg1, arg2, ctx);
    kernel_unlock();
}
