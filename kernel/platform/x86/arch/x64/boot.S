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
boot_pdpt:
    .quad boot_pd + 0x003
    .quad 0
    .quad boot_pd_high + 0x003
    .fill 509, 8, 0

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
    .quad 0x00af92000000ffff
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
