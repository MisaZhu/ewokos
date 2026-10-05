#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/kmalloc_vm.h>
#include <mm/shm.h>
#include <mm/dma.h>
#include <kstring.h>
#include <kernel/core.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
#include <kernel/irq.h>
#include <kernel/schedule.h>
#include <kernel/kevqueue.h>
#include <dev/timer.h>
#include <kprintf.h>
#include <dev/uart.h>
#include <dev/sd.h>
#include <stddef.h>
#include <sysinfo.h>
#include <kernel/semaphore.h>



kernel_info_t _kernel_info;

/*
 * ARM32 keeps a fixed high-vector alias at INTERRUPT_VECTOR_BASE. AArch64 uses
 * VBAR_EL1 directly and points it at _sys_info.vector_base inside the mapped
 * kernel image, so no extra low alias is needed there.
 */
static void __attribute__((optimize("O0"))) copy_interrupt_table(void) {
    uint32_t *vsrc = &interrupt_table_start;
    //uint32_t *vdst = (uint32_t*)INTERRUPT_VECTOR_BASE;
    uint32_t *vdst = (uint32_t*)(_sys_info.vector_base);
    if(vsrc == vdst)
        return;
    while(vsrc < &interrupt_table_end) {
        *vdst++ = *vsrc++;
    }
}

static void set_kernel_vm(page_dir_entry_t* vm) {
    memset(vm, 0, PAGE_DIR_SIZE);
    flush_dcache();

#ifdef INTERRUPT_VECTOR_BASE
    //map interrupt vector to high(virtual) mem
    map_pages_size(vm, INTERRUPT_VECTOR_BASE, _sys_info.vector_base, PAGE_SIZE, AP_RW_D, PTE_ATTR_WRBACK);
#endif
    //map kernel image
    map_pages(vm, KERNEL_BASE, _sys_info.phy_offset, V2P(KERNEL_IMAGE_END), AP_RW_D, PTE_ATTR_WRBACK_ALLOCATE);
    //map kernel page dir
    map_pages(vm, KERNEL_PAGE_DIR_BASE, V2P(KERNEL_PAGE_DIR_BASE), V2P(KERNEL_PAGE_DIR_END), AP_RW_D, PTE_ATTR_WRBACK);

    //map kernel sys_state memory
    map_pages(vm, KERNEL_VSYSCALL_INFO_BASE, V2P(KERNEL_VSYSCALL_INFO_BASE), V2P(KERNEL_VSYSCALL_INFO_END), AP_RW_RW, PTE_ATTR_DEV);
    _kernel_info.vsyscall_info = (vsyscall_info_t*) KERNEL_VSYSCALL_INFO_BASE;

    //map kernel malloc memory
    map_pages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(KMALLOC_END), AP_RW_D, PTE_ATTR_WRBACK);
    //map allocatable memory page dir
    map_pages(vm, ALLOCABLE_PAGE_DIR_BASE, V2P(ALLOCABLE_PAGE_DIR_BASE), V2P(ALLOCABLE_PAGE_DIR_END), AP_RW_D, PTE_ATTR_WRBACK);
    //map MMIO to high(virtual) mem.
    map_pages_size(vm, _sys_info.mmio.v_base, _sys_info.mmio.phy_base, _sys_info.mmio.size, AP_RW_D, PTE_ATTR_DEV);
    arch_vm(vm);
}

static void reset_kernel_vm(void) {
    page_dir_entry_t* vm = (page_dir_entry_t*)KERNEL_PAGE_DIR_BASE;
    //map kernel malloc memory
    map_pages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(KMALLOC_END), AP_RW_D, PTE_ATTR_WRBACK);
    //map allocatable memory page dir
    map_pages(vm, ALLOCABLE_PAGE_DIR_BASE, V2P(ALLOCABLE_PAGE_DIR_BASE), V2P(ALLOCABLE_PAGE_DIR_END), AP_RW_D, PTE_ATTR_WRBACK);
    //map MMIO to high(virtual) mem.
    flush_tlb();
}


static void map_allocable_pages(page_dir_entry_t* vm) {
    //map kernel dma memory
    map_pages_size(vm, _sys_info.sys_dma.phy_base, _sys_info.sys_dma.phy_base, _sys_info.sys_dma.size, AP_RW_D, PTE_ATTR_SYS_DMA);
    //direct-map the reserved contiguous shm slab so the kernel can zero it
    //(the shm window itself lives in the private user half and is not
    //accessible from kernel/syscall context)
    if(_sys_info.shm_contig.size > 0)
        map_pages_size(vm, P2V(_sys_info.shm_contig.phy_base), _sys_info.shm_contig.phy_base, _sys_info.shm_contig.size, AP_RW_D, PTE_ATTR_NOCACHE);
    map_pages(vm,
            P2V(_sys_info.allocable_phy_mem_base),
            _sys_info.allocable_phy_mem_base,
            _sys_info.allocable_phy_mem_top,
            AP_RW_D, PTE_ATTR_WRBACK);
    flush_tlb();
}

/*
 * Install the kernel view into a fresh per-process address space. The page
 * table layout is arch specific, so the actual clone is an arch hook (declared
 * in <kernel/kernel.h>): this weak default rebuilds the full kernel mapping
 * plus the allocable direct map (used by riscv and any arch without a shared
 * high-half), while x86/aarch64/arm override it in
 * kernel/platform/<arch>/arch/common/src/kernel_arch.c to share the kernel
 * high-half tables and keep only the user half private.
 */
__attribute__((weak)) void arch_set_proc_vm(page_dir_entry_t* vm) {
    set_kernel_vm(vm);
    map_allocable_pages(vm);
}

void set_vm(page_dir_entry_t* vm) {
    arch_set_proc_vm(vm);
}

static void init_kernel_vm(void) {
    _pages_ref.max = 0;
    _kernel_info.kernel_vm = (page_dir_entry_t*)KERNEL_PAGE_DIR_BASE;
    //get kalloc(4k memblocks) ready just for kernel page tables.
    kalloc_reset();
    kalloc_append(KERNEL_PAGE_DIR_BASE+PAGE_DIR_SIZE, KERNEL_PAGE_DIR_END); 

    //switch to two-levels 4k page_size type paging
    set_kernel_vm(_kernel_info.kernel_vm);
    //Use physical address of kernel virtual memory as the new virtual memory page dir table base.
    set_translation_table_base(V2P((ewokos_addr_t)_kernel_info.kernel_vm));
}

static void init_allocable_mem(void) {
    //printf("kernel: kalloc init for allocable page dir\n");
    kalloc_append(ALLOCABLE_PAGE_DIR_BASE, ALLOCABLE_PAGE_DIR_END); 
    //printf("kernel: mapping allocable pages\n");
    map_allocable_pages(_kernel_info.kernel_vm);
    
    //_pages_ref.max = kalloc_append(P2V(_sys_info.allocable_phy_mem_base), P2V(_sys_info.allocable_phy_mem_top));
    kalloc_arch();
    _pages_ref.max = (_sys_info.allocable_phy_mem_top - _sys_info.allocable_phy_mem_base) / PAGE_SIZE;
    _pages_ref.refs = kmalloc(_pages_ref.max * sizeof(page_ref_t));	
    _pages_ref.phy_base = _sys_info.allocable_phy_mem_base;
    memset(_pages_ref.refs, 0, _pages_ref.max * sizeof(page_ref_t));
}

#ifdef KERNEL_SMP
/*
 * Resolve which logical core a secondary (AP) core binds to during bring-up.
 * Weak default keeps the detected core id; x86 overrides it in
 * kernel/platform/x86/arch/common/src/kernel_arch.c because its AP trampoline
 * passes the target logical core id explicitly.
 */
__attribute__((weak)) uint32_t arch_slave_resolve_core(uint32_t boot_core_id, uint32_t detected_cid) {
    (void)boot_core_id;
    return detected_cid;
}

void __attribute__((optimize("O0"))) _slave_kernel_entry_c(uint32_t boot_core_id) {
    /*
     * Bind this AP to its logical core (arch hook), then switch to the full
     * kernel VM before cpu_core_ready() touches LAPIC/GIC/MMIO state - the AP
     * trampoline runs on bootstrap tables that map only early kernel memory.
     */
    uint32_t cid = arch_slave_resolve_core(boot_core_id, get_core_id());
    set_translation_table_base(V2P((ewokos_addr_t)_kernel_info.kernel_vm));
    cpu_core_ready(cid);
    /*
     * The counter-access enable is a per-core control register (CNTKCTL_EL1 on
     * arm/aarch64, scounteren on riscv), so a task migrated to this AP would
     * take an EL0 trap reading the clock libc interpolates from. irq_init()
     * only covers the boot core.
     */
    arch_enable_user_cnt();
    _cpu_cores[cid].actived = true;
    flush_dcache();
    proc_t* idle_proc = _cpu_cores[cid].idle_proc;

    /*
     * Keep each AP attached to its idle task even before the first real task is
     * dispatched. The global run_usec accounting uses the idle task to infer per
     * core idle time, and the scheduler also expects an idle current task when
     * an AP wakes on IPI.
     */
    if(idle_proc != NULL) {
        idle_proc->info.state = RUNNING;
        idle_proc->info.wait_for = 0;
        set_current_proc(idle_proc);
        proc_account_resume_current();
    }

    halt();
}
#endif

static void logo(void) {
    printf(
            "-----------------------------------------------------\n"
            " ______           ______  _    _   ______  ______ \n"
            "(  ___ \\|\\     /|(  __  )| \\  / \\ (  __  )(  ___ \\\n"
            "| (__   | | _ | || |  | || (_/  / | |  | || (____\n"
            "|  __)  | |( )| || |  | ||  _  (  | |  | |(____  )\n"
            "| (___  | || || || |__| || ( \\  \\ | |__| |  ___) |\n"
            "(______/(_______)(______)|_/  \\_/ (______)\\______)\n");
}

/*
 * Boot banner + memory-layout dump. Address widths are arch specific, so the
 * whole thing is an arch hook (declared in <kernel/kernel.h>). This weak
 * default prints 32-bit addresses and is used by arm/riscv/x86; aarch64
 * overrides it in kernel/platform/aarch64/arch/common/src/kernel_arch.c to
 * print full 64-bit addresses split into hi/lo words.
 */
__attribute__((weak)) void arch_show_config(void) {
    printf("\n"
          "  machine              %s\n" 
          "  arch                 %s-%dk\n"
          "  cores                %d\n"
          "  kernel_timer_freq    %d\n"
          "  mem_offset           0x%08x\n"
          "  phy mem size         %d MB\n"
          "  usable mem size      %d MB\n"
          "  mmio_base            Phy:0x%08x V:0x%08x (%d MB)\n"
          "  kernel image         Phy:0x%08x ~ 0x%08x (%d KB)\n"
          "  vsyscall info        Phy:0x%08x ~ 0x%08x (%d KB)\n"
          "  kernel page dir      Phy:0x%08x ~ 0x%08x (%d KB)\n"
          "  allocable page dir   Phy:0x%08x ~ 0x%08x (%d MB)\n"
          "  kmalloc              Phy:0x%08x ~ 0x%08x (%d MB)\n"
          "  sys_dma_base         Phy:0x%08x ~ 0x%08x (%d MB)\n"
          "  allocable mem info   Phy:0x%08x ~ 0x%08x (%d MB)\n"
          "  max proc num         %d\n"
          "  max task total       %d\n"
          "  max task per proc    %d\n"
          "-----------------------------------------------------\n",
            _sys_info.machine,
            _sys_info.arch, PAGE_SIZE/1024,
            _kernel_config.cores,
            _kernel_config.timer_freq,
            _sys_info.phy_offset,
            _sys_info.total_phy_mem_size/(1*MB),
            _sys_info.total_usable_mem_size / (1*MB),
            _sys_info.mmio.phy_base, _sys_info.mmio.v_base, _sys_info.mmio.size/(1*MB),
            V2P(_kernel_start), V2P(_kernel_end), (_kernel_end - _kernel_start) / (1*KB),
            V2P(KERNEL_VSYSCALL_INFO_BASE), V2P(KERNEL_VSYSCALL_INFO_END), KERNEL_VSYSCALL_INFO_SIZE / (1*KB),
            V2P(KERNEL_PAGE_DIR_BASE), V2P(KERNEL_PAGE_DIR_END), KERNEL_PAGE_DIR_SIZE / (1*KB),
            V2P(ALLOCABLE_PAGE_DIR_BASE), V2P(ALLOCABLE_PAGE_DIR_END), ALLOCABLE_PAGE_DIR_SIZE / (1*MB),
            V2P(KMALLOC_BASE), V2P(KMALLOC_END), _sys_info.kmalloc_size / (1*MB),
            _sys_info.sys_dma.phy_base, _sys_info.sys_dma.phy_base+_sys_info.sys_dma.size, _sys_info.sys_dma.size/(1*MB),
            _sys_info.allocable_phy_mem_base, _sys_info.allocable_phy_mem_top, (uint32_t)(get_free_mem_size() / (1*MB)),
            _kernel_config.max_proc_num,
            _kernel_config.max_task_num,
            _kernel_config.max_task_per_proc);
}

int32_t load_init_proc(void);

/*
 * Very early per-arch entry fixup, run before the bss clear. Weak no-op
 * default; x86 overrides it in
 * kernel/platform/x86/arch/common/src/kernel_arch.c to mask interrupts.
 */
__attribute__((weak)) void arch_kernel_entry_early(void) {
}

void _kernel_entry_c(void) {
    arch_kernel_entry_early();
    //clear bss
#if defined(PAGE_SIZE_16K) || defined(PAGE_SIZE_64K)
    for(volatile char* p = _bss_start; p < (volatile char*)_bss_end; p++)
        *p = 0;
#else
    memset(_bss_start, 0, (size_t)(_bss_end - _bss_start));
#endif
    sys_info_init();

    copy_interrupt_table();

    init_kernel_vm();  

    uart_dev_init(19200);
    kout_str("\n=== ewokos booting ===\n\n");
    kout_str("kernel: init kernel malloc     ... ");
    kmalloc_init(); //init kmalloc with min size for just early stage kernel load
    kout_str("[OK]\n");

    kout_str("kernel: init sd                ... ");
    sd_init();
    kout_str("[OK]\n");

    kout_str("kernel: load kernel config     ... ");
    load_kernel_config();
    sys_info_config();
    kout_str("[OK]\n");

    uart_dev_init(_kernel_config.uart_baud);

    kout_str("kernel: remapping kernel mem   ... ");
    reset_kernel_vm();
    kmalloc_init(); //init kmalloc again with config info;
    kmalloc_vm_init(); //init kmalloc extra;
    kout_str("[OK]\n");

    //printf("kernel: init allocable memory  ... ");
    init_allocable_mem(); //init the rest allocable memory VM
    //printf("[ok] (%d MB)\n", (get_free_mem_size() / (1*MB)));

    logo();
    arch_show_config();

    kout_str("kernel: init kernel event      ... ");
    kev_init();
    kout_str("[OK]\n");

    //printf("kernel: init DMA               ... ");
    dma_init();
    //printf("[OK]\n");

    //printf("kernel: init semaphore         ... ");
    semaphore_init();
    //printf("[ok]\n");

    //printf("kernel: init irq               ... ");
    irq_init();
    //printf("[ok]\n");

    //printf("kernel: init share memory      ... ");
    shm_init();
    //printf("[ok]\n");

    //printf("kernel: init processes table   ... ");
    if(procs_init() != 0)
        halt();
    //printf("[ok] (%d)\n", _kernel_config.max_proc_num);

    printf("kernel: loading init ... ");
    if(load_init_proc() != 0)  {
        printf("[failed!]\n");
        halt();
    }
    printf("[ok]\n");

#ifdef __x86_64__
	extern void console_handoff(void);
    console_handoff();
#endif

    kfork_core_halt(0);
    if(_cpu_cores[0].idle_proc != NULL) {
        _cpu_cores[0].idle_proc->info.state = RUNNING;
        _cpu_cores[0].idle_proc->info.wait_for = 0;
        set_current_proc(_cpu_cores[0].idle_proc);
        proc_account_resume_current();
    }
#ifdef KERNEL_SMP
    _cpu_cores[0].actived = true;
    kernel_lock_init();
    printf("kernel: start cores ... 0");
    for(uint32_t i=1; i<_sys_info.cores; i++) {
        _cpu_cores[i].actived = false;
        kfork_core_halt(i);
        flush_dcache();
        start_core(i);
        while(!_cpu_cores[i].actived) {
            //continue;
            _delay_msec(10);
        }
        printf(" %d", i);
    }
    printf("\n");
#endif

    //printf("kernel: set timer(fps): %6d ... ", _kernel_config.timer_freq);
    timer_set_interval(0, _kernel_config.timer_freq); 
    //printf("[ok]\n");
    //printf("kernel: start init process     ...\n"
        //   "---------------------------------------------------\n");

    __irq_enable();
    halt();

    kfree_page(_kernel_info.vsyscall_info);
}
