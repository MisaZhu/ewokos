#include <dev/timer.h>
#include <kernel/irq.h>
#include <kernel/interrupt.h>
#include <kernel/system.h>
#include <kernel/schedule.h>
#include <kernel/kernel.h>
#include <kernel/proc.h>
#include <kernel/kevqueue.h>
#include <kernel/interrupt.h>
#include <kernel/core.h>
#include <kstring.h>
#include <signals.h>
#include <kernel/signal.h>
#include <kernel/hw_info.h>
#include <kernel/svc.h>
#include <kprintf.h>
#include <mm/kalloc.h>
#include <mm/mmu.h>
#include <stddef.h>
#include <stdint.h>
static uint64_t _irq_tic_last_usec = 0;
static uint32_t _irq_tic_second = 0;

static inline uint64_t irq_read_uptime_usec_stable(void) {
    volatile uint32_t* ticks32 = (volatile uint32_t*)&_kernel_info.uptime_usec;
    uint32_t hi1, lo, hi2;

    do {
        hi1 = ticks32[1];
        lo = ticks32[0];
        hi2 = ticks32[1];
    } while(hi1 != hi2);

    return ((uint64_t)hi1 << 32) | lo;
}

uint64_t irq_accounting_now_usec(void) {
    if(strstr(_sys_info.machine, "miyoo") == NULL)
        return timer_read_sys_usec();

    uint64_t base = irq_read_uptime_usec_stable();
    if(get_core_id() != 0)
        return base;

    uint64_t raw = timer_read_sys_usec();
    uint64_t last = _irq_tic_last_usec;
    if(raw > last)
        return base + (raw - last);
    return base;
}

static void dump_user_addr_words_internal(proc_t* proc, ewokos_addr_t addr, const char* tag) {
    ewokos_addr_t page_phy;
    uint32_t page_off;
    uint32_t avail;
    uint8_t* page_ptr;

    if(proc == NULL || proc->space == NULL) {
        return;
    }

    page_phy = resolve_phy_address(proc->space->vm, addr);
    if(page_phy == 0) {
        printf("%s: addr=0x%llX not mapped\n", tag, (unsigned long long)addr);
        return;
    }
    page_ptr = (uint8_t*)(uintptr_t)P2V(page_phy);
    page_off = ((uint32_t)addr) & (PAGE_SIZE - 1);
    avail = PAGE_SIZE - page_off;

    arch_dump_addr_words(page_ptr, page_off, avail, tag);
}

void dump_user_addr_words(proc_t* proc, ewokos_addr_t addr, const char* tag) {
    if(tag == NULL) {
        tag = "user";
    }
    dump_user_addr_words_internal(proc, addr, tag);
}

void dump_user_fault_words(proc_t* proc, context_t* ctx) {
    if(proc == NULL || ctx == NULL) {
        return;
    }
    dump_user_addr_words_internal(proc, ctx->sp, "user_sp");
    if(ctx->pc != ctx->sp) {
        dump_user_addr_words_internal(proc, (ewokos_addr_t)ctx->pc, "user_pc");
    }
    if(ctx->lr != 0 && ctx->lr != ctx->pc && ctx->lr != ctx->sp) {
        dump_user_addr_words_internal(proc, (ewokos_addr_t)ctx->lr, "user_lr");
    }
}

#ifdef KERNEL_SMP

void ipi_enable_all(void) {
    uint32_t i;
    for(i=0; i<_sys_info.cores; i++) {
        ipi_enable(i);
    }
}

#endif

static inline void irq_do_raw(context_t* ctx, uint32_t irq) {
    //kprintf("irq_raw: 0x%x\n", irq);
    interrupt_send(ctx, irq);
}

static inline void irq_do_timer0(context_t* ctx) {
    uint64_t usec = timer_read_sys_usec();
    uint32_t usec_gap = usec - _irq_tic_last_usec;

    _irq_tic_last_usec = usec;
    _kernel_info.uptime_usec = usec;
    _irq_tic_second += usec_gap;

    if(_irq_tic_second >= 1000000) { //SEC_TIC sec
        _kernel_info.uptime_sec++;
        _irq_tic_second = 0;
        renew_kernel_sec();
    }
    renew_kernel_tic(usec_gap);
    
    arch_irq_timer_ack();

    schedule(ctx);
}
static inline void _irq_handler(uint32_t cid, context_t* ctx) {
    uint32_t irq_raw = arch_irq_raw(ctx);
    uint32_t irq = irq_get_unified_arch(irq_raw);

    //handle irq
    if(irq > 0 && irq < IRQ_RAW_TOP) {
        irq_do_raw(ctx, irq);
    }
    else if(cid == 0 && irq == IRQ_TIMER0) {
        irq_do_timer0(ctx);
    }
    else {
#ifdef KERNEL_SMP
        ipi_clear(get_core_id());
#endif
    }
#ifdef KERNEL_SMP
    uint32_t core = get_core_id();
    if(_cpu_cores[core].need_resched != 0) {
        _cpu_cores[core].need_resched = 0;
        if(!(cid == 0 && irq == IRQ_TIMER0)) {
            schedule(ctx);
        }
    }
#endif
    irq_eoi_arch(irq_raw);
}

inline void irq_handler(context_t* ctx) {
    __irq_disable();
    arch_irq_prologue(ctx);
    proc_account_pause_current();
    uint32_t cid = get_core_id();
    kernel_lock();
    _irq_handler(cid, ctx);
    kernel_unlock();
    proc_account_resume_current();

    proc_t* cproc = get_current_proc();
    if(cproc != NULL && cproc->is_core_idle_proc) {
        arch_irq_idle();
    }
}

static int32_t copy_on_write(proc_t* proc, ewokos_addr_t v_addr) {
    if(proc == NULL || proc->space == NULL || proc->space->vm == NULL)
        return -1;
    v_addr = ALIGN_DOWN(v_addr, PAGE_SIZE);
    ewokos_addr_t phy_addr = resolve_phy_address(proc->space->vm, v_addr);
    char *page = kalloc_page();
    if(page == NULL) {
        return -1;
    }

    if(phy_addr != 0) {
        memcpy(page, (char*)P2V(phy_addr), PAGE_SIZE);
        unmap_page_ref(proc->space->vm, v_addr);
    }
    else {
        // Lazily reserved heap/stack pages have no backing page yet.
        memset(page, 0, PAGE_SIZE);
    }
    map_page_ref(proc->space->vm,
            v_addr,
            V2P(page),
            AP_RW_RW, PTE_ATTR_WRBACK);
    flush_tlb();
    return 0;
}

static inline uint8_t is_user_heap_or_stack_fault(proc_t* proc, ewokos_addr_t addr_fault) {
    if(proc == NULL || proc->space == NULL)
        return 0;

    ewokos_addr_t legel_addr_base = proc->space->rw_heap_base;
    uint8_t in_heap = (addr_fault >= legel_addr_base && addr_fault < proc->space->heap_size);
    uint8_t in_user_stack = (addr_fault >= USER_STACK_BOTTOM && addr_fault < USER_STACK_TOP);
    return (uint8_t)(in_heap || in_user_stack);
}

static inline uint8_t is_recoverable_user_data_fault(uint32_t status) {
    return arch_fault_recoverable(status);
}

/*
 * Snapshot a user proc exception and hand it to the core proc through the
 * kevent queue (KEV_PROC_CORE_DUMP) before the proc is torn down, so the crash
 * pid/pc/sp/fault address survive proc_exit. Only called for user procs; a
 * kernel-mode fault (cproc == NULL) halts instead and never reaches here.
 */
static void push_proc_core_dump(proc_t* cproc, context_t* ctx, uint32_t reason,
        uint32_t status, ewokos_addr_t fault_addr) {
    kev_core_dump_t dump;
    dump.pid = cproc->info.pid;
    dump.core = cproc->info.core;
    dump.reason = reason;
    dump.status = status;
    dump.fault_addr = fault_addr;
    dump.pc = (ewokos_addr_t)ctx->pc;
    dump.sp = (ewokos_addr_t)ctx->sp;

    /* snapshot the full register file, mirroring dump_ctx()'s arch layout */
    arch_fill_core_dump_regs(&dump, ctx);

    kev_push_core_dump(&dump);
}

/*
 * Per-core guard against a cascading fault.
 *
 * The abort handlers walk the dying proc's OWN memory before tearing it down:
 * dump_ctx(&cproc->ctx), dump_user_stack_words(), push_proc_core_dump() and
 * then proc_exit -> proc_terminate. If that proc is already corrupt (smashed
 * page tables, a bogus space, an inconsistent saved ctx - exactly the state
 * that produced the fault in the first place), one of those accesses faults
 * AGAIN.
 *
 * __irq_disable() masks IRQ/FIQ but NOT data/prefetch/undef aborts, and on ARM
 * a nested abort re-enters on the same banked abort stack, overwriting the
 * saved frame. Without a guard this recurses until the kernel stack overflows
 * - a chain crash triggered by the very handler meant to contain the fault.
 * Detecting re-entry on the same core and halting turns that unbounded cascade
 * into a single, diagnosable stop.
 */
static uint8_t _abort_in_progress[CPU_MAX_CORES];

uint32_t abort_guard_enter(const char* what) {
    uint32_t core = get_core_id();
    if(core >= CPU_MAX_CORES)
        core = 0;
    if(_abort_in_progress[core]) {
        printf("kernel: nested %s abort on core %d, halting to stop cascade\n", what, core);
        halt();
    }
    _abort_in_progress[core] = 1;
    return core;
}

void abort_guard_leave(uint32_t core) {
    if(core < CPU_MAX_CORES)
        _abort_in_progress[core] = 0;
}

/*
 * Was this fault taken while the CPU was running KERNEL code?
 *
 * abort_from_kernel() is arch specific (it reads the pre-exception program
 * status saved in the context) and now lives in each platform's
 * kernel/platform/<arch>/arch/common/src/irq.c; it is declared extern in
 * <kernel/irq.h>.
 */

void undef_abort_handler(context_t* ctx, uint32_t status) {
    (void)ctx;
    (void)status;
    __irq_disable();
    uint32_t core = abort_guard_enter("undef");
    proc_t* cproc = get_current_proc();
    if(cproc == NULL) {
        printf("_kernel, undef instrunction abort!! (core %d)\n", core);
        dump_ctx(ctx);
        halt();
    }

    if(abort_from_kernel(ctx)) {
        printf("_kernel, undef instruction abort in KERNEL mode!! (core %d) - kernel bug\n", core);
        dump_ctx(ctx);
        halt();
    }

    printf("pid: %d(%s), undef instrunction abort!! (core %d)\n", cproc->info.pid, cproc->info.cmd, core);
    dump_ctx(&cproc->ctx);

    push_proc_core_dump(cproc, ctx, KEV_CORE_DUMP_UNDEF, status, (ewokos_addr_t)ctx->pc);
    proc_exit(ctx, proc_get_proc(cproc), -1);
    abort_guard_leave(core);
}

void prefetch_abort_handler(context_t* ctx, uint32_t status) {
    (void)ctx;
    __irq_disable();
    uint32_t core = abort_guard_enter("prefetch");

    proc_t* cproc = get_current_proc();
    if(cproc == NULL) {
        printf("_kernel, prefetch abort!! (core %d)\n", core);
        dump_ctx(ctx);
        halt();
    }
    /*kprintf("handle prefetch abort: %d, status: 0x%x, addr: 0x%x\n", cproc->info.pid, status, ctx->pc);

    if(((status & 0x1D) == 0xD || //permissions fault only
        (status & 0x1F) == 0x6) && 
            ctx->pc < cproc->space->heap_size) { //in proc heap only
        if (kernel_lock_check() > 0)
            return;

        kernel_lock();
        int32_t res = copy_on_write(cproc, ctx->pc);
        kernel_unlock();
        if(res == 0)
            return;
    }
    */

    if(abort_from_kernel(ctx)) {
        printf("_kernel, prefetch abort in KERNEL mode!! (core %d) code:0x%x - kernel bug\n", core, status);
        dump_ctx(ctx);
        halt();
    }

    printf("pid: %d(%s), prefetch abort!! (core %d) code:0x%x\n", cproc->info.pid, cproc->info.cmd, core, status);
    arch_dump_prefetch_extra(ctx);
    dump_ctx(&cproc->ctx);
    arch_dump_user_fault(cproc, ctx);

    push_proc_core_dump(cproc, ctx, KEV_CORE_DUMP_PREFETCH, status, (ewokos_addr_t)ctx->pc);
    proc_exit(ctx, proc_get_proc(cproc), -1);
    abort_guard_leave(core);
}

void data_abort_handler(context_t* ctx, ewokos_addr_t addr_fault, uint32_t status) {
    (void)ctx;
    __irq_disable();
    uint32_t core = abort_guard_enter("data");
    proc_t* cproc = get_current_proc();
    if(cproc == NULL) {
        printf("_kernel, data abort!! core: %d, at: 0x%llX status: 0x%X\n", 
            get_core_id(), (unsigned long long)addr_fault, status);
        dump_ctx(ctx);
        halt();
    }
    if(abort_from_kernel(ctx)) {
        printf("_kernel, data abort in KERNEL mode!! (core %d) at: 0x%llX status: 0x%X - kernel bug\n",
            core, (unsigned long long)addr_fault, status);
        dump_ctx(ctx);
        halt();
    }
    //kprintf("handle data abort: %d, 0x%x, 0x%x\n", cproc->info.pid, status, addr_fault);

    uint32_t err = 0;
    const char* errmsg = "";
    ewokos_addr_t legel_addr_base = (cproc->space != NULL) ? cproc->space->rw_heap_base : 0;
    ewokos_addr_t legel_heap_size = (cproc->space != NULL) ? cproc->space->heap_size : 0;
    uint8_t recoverable = is_recoverable_user_data_fault(status);
    uint8_t in_user_heap_or_stack = is_user_heap_or_stack_fault(cproc, addr_fault);
    ewokos_addr_t fault_page_phy = 0;
    if(cproc->space != NULL && cproc->space->vm != NULL) {
        fault_page_phy = resolve_phy_address(cproc->space->vm, ALIGN_DOWN(addr_fault, PAGE_SIZE));
    }

    if(recoverable) {
        if(in_user_heap_or_stack) {
            if (kernel_lock_check() > 0) {
                abort_guard_leave(core);
                return;
            }

            kernel_lock();
            int32_t res = copy_on_write(cproc, addr_fault);
            kernel_unlock();
            if(res == 0) {
                abort_guard_leave(core);
                return;
            }
            err = 1;
            errmsg = "copy on write failed";
        }
        else {
            err = 2;
            errmsg = "illegel address";
        }
    }
    else {
        err = 3;
        errmsg = "access denied";
    }

    printf("\npid: %d(%s), core: %d, data abort at: 0x%llX, status: 0x%X\n", 
            cproc->info.pid, cproc->info.cmd, cproc->info.core, (unsigned long long)addr_fault, status);
    printf("\tdebug: recov=%d in_heap_or_stack=%d heap(0x%llX->0x%llX) page_phy=0x%llX\n",
            recoverable,
            in_user_heap_or_stack,
            (unsigned long long)legel_addr_base,
            (unsigned long long)legel_heap_size,
            (unsigned long long)fault_page_phy);
    if(err == 2) //illegel address
        printf("\terror: %s! heap(0x%llX->0x%llX)\n",
                errmsg,
                (unsigned long long)legel_addr_base,
                (unsigned long long)legel_heap_size);
    else
        printf("\terror: %s!\n", errmsg);

    dump_ctx(ctx);
    arch_dump_user_fault(cproc, ctx);
    push_proc_core_dump(cproc, ctx, KEV_CORE_DUMP_DATA, status, addr_fault);
    proc_exit(ctx, proc_get_proc(cproc), -1);
    abort_guard_leave(core);
}

void irq_init(void) {
    irq_init_arch();
    interrupt_init();
    _kernel_info.uptime_sec = 0;
    _kernel_info.uptime_usec = 0;
    _irq_tic_second = 0;
    _irq_tic_last_usec = timer_read_sys_usec();
    irq_enable_arch(IRQ_TIMER0);

#ifdef KERNEL_SMP
    ipi_enable_all();
#endif
}
