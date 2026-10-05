.intel_syntax noprefix
.section .multiboot
.align 4
.long 0x1BADB002
.long 0x00000003
.long -(0x1BADB002 + 0x00000003)

.section .init.data,"aw"
.align 4096
.global boot_pml4
boot_pml4:
    .quad boot_pdpt + 0x003
    .fill 511, 8, 0

.align 4096
.global boot_pdpt
boot_pdpt:
    .quad boot_pd + 0x003
    .quad 0
    .quad boot_pd_high + 0x003
    /* PDPT[3]: VA 0xC0000000 区, 供内核运行时映射 AHCI ABAR 等
     * PCI MMIO(见 hw_info_arch.c x86_early_map_pci_mmio) */
    .quad boot_pd_spare + 0x003
    .fill 508, 8, 0

.align 4096
boot_pd:
    .set page, 0
    .rept 512
        .quad page + 0x083
        .set page, page + 0x200000
    .endr

.align 4096
boot_pd_high:
    .quad boot_pt_high0 + 0x003
    .fill 511, 8, 0

/* VA 0xC0000000 区备用 PD: 内核启动早期由 hw_info_arch 填入
 * 2MB 页, 映射 PCI MMIO(如 q35 的 AHCI ABAR), 内核页表(arch_vm)
 * 对同一 VA 做同样映射, 保证重映射前后地址不变 */
.align 4096
.global boot_pd_spare
boot_pd_spare:
    .fill 512, 8, 0

.align 4096
boot_pt_high0:
    .set page, 0x00100000
    .rept 512
        .quad page + 0x003
        .set page, page + 0x1000
    .endr

.align 16
gdt32:
    .quad 0x0000000000000000
    .quad 0x00af9a000000ffff
    .quad 0x00cf92000000ffff
gdt32_ptr:
    .word (3 * 8) - 1
    .quad gdt32

.section .init
.code32
.extern _boot_start
.extern _kernel_entry_c
.extern x86_runtime_init
.extern _svc_start_stack
.extern _svc_stack
.extern _bss_start
.extern _bss_end

.global __entry
__entry:
    cli
    cld
    /* rdi = bootinfo 物理地址 (BIOS-CD no-emulation 路径; BIOS 硬盘路径
     * 传 0)。hw_info 按魔数校验, 垃圾/0 均安全回退到自探测 */
    mov dword ptr [x86_uefi_bootinfo], edi    /* 分页未开: 线性 = 物理 */
    mov esp, offset _svc_start_stack
    lgdt [gdt32_ptr]
1:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz 1b
    mov dx, 0x3f8
    mov al, 'D'
    out dx, al

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
2:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz 2b
    mov dx, 0x3f8
    mov al, 'E'
    out dx, al

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
3:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz 3b
    mov dx, 0x3f8
    mov al, 'F'
    out dx, al

    mov eax, offset boot_pml4
    mov cr3, eax
4:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz 4b
    mov dx, 0x3f8
    mov al, 'G'
    out dx, al

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax
5:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz 5b
    mov dx, 0x3f8
    mov al, 'H'
    out dx, al
    jmp 0x08:long_mode_entry

.code64
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax
    lea rsp, [rip + _svc_start_stack]
    xor ebp, ebp
9:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz 9b
    mov dx, 0x3f8
    mov al, 'I'
    out dx, al

    lea rdi, [rip + _bss_start]
    lea rcx, [rip + _bss_end]
    sub rcx, rdi
    xor eax, eax
    shr rcx, 3
    rep stosq
10:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz 10b
    mov dx, 0x3f8
    mov al, 'J'
    out dx, al

    call x86_runtime_init
11:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz 11b
    mov dx, 0x3f8
    mov al, 'K'
    out dx, al
    call _boot_start
12:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz 12b
    mov dx, 0x3f8
    mov al, 'L'
    out dx, al
    lea rsp, [rip + _svc_stack]
13:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz 13b
    mov dx, 0x3f8
    mov al, 'M'
    out dx, al
    call _kernel_entry_c

halt_forever:
    cli
    hlt
    jmp halt_forever

/* ------------------------------------------------------------------
 * UEFI 入口 (boot/uefi stub 在 ExitBootServices 后进入)
 * 此时: 长模式 + 固件页表(全恒等映射), rdi = bootinfo 物理地址,
 *       段寄存器/栈仍是固件的(栈可能在 >1GB 处, 切 CR3 后不可见)。
 * 因此必须先切内核栈/段(恒等映射区内), 再切 boot_pml4 —— 与 BIOS
 * 路径的 long_mode_entry 完全同一套早期页表(恒等 0-1GB +
 * 0x80000000 -> PA 0x100000 的 2MB 高映射), 之后行为与 BIOS 一致。
 * 布局约定(boot/uefi stub 依赖): uefi_header 首个 .quad 为
 * boot_pml4 地址, uefi_entry 位于其 +8 处。
 * ------------------------------------------------------------------ */
.section .uefi,"ax"
.p2align 4
.global uefi_header
uefi_header:
    .quad boot_pml4
.global uefi_entry
uefi_entry:
    cli
    cld
    /* 暂存 bootinfo 指针, 供 hw_info_arch 消费 */
    mov qword ptr [rip + x86_uefi_bootinfo], rdi
    mov dx, 0x3fd
1:
    in al, dx
    test al, 0x20
    jz 1b
    mov dx, 0x3f8
    mov al, 'U'
    out dx, al

    /* 固件栈随 CR3 切换失效, 先换到内核 init 栈(恒等映射区) */
    lea rsp, [rip + _svc_start_stack]
    lgdt [rip + gdt32_ptr]
    lea rax, [rip + uefi_reload_cs]
    push 0x08
    push rax
    retfq
uefi_reload_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax

    /* 切到内核早期页表(与 BIOS 路径相同), 此后调用高半区代码 */
    mov rax, offset boot_pml4
    mov cr3, rax

    lea rdi, [rip + _bss_start]
    lea rcx, [rip + _bss_end]
    sub rcx, rdi
    xor eax, eax
    shr rcx, 3
    rep stosq

    call x86_runtime_init
    call _boot_start
    lea rsp, [rip + _svc_stack]
    call _kernel_entry_c

uefi_halt:
    cli
    hlt
    jmp uefi_halt

.section .init.data
.global x86_uefi_bootinfo
x86_uefi_bootinfo:
    .quad 0
