.intel_syntax noprefix
.equ X86_MAX_CORES, 8
.equ X86_RUNTIME_TSS_SIZE, 104
.equ X86_RUNTIME_IRQ_STACK_SIZE, 0x4000
.extern x86_smp_mapping_init
.extern x86_apic_id_to_core_id
.section .data
.align 16
runtime_gdt:
    .quad 0x0000000000000000
    .quad 0x00af9a000000ffff
    .quad 0x00cf92000000ffff
    .quad 0x00cff2000000ffff
    .quad 0x00affa000000ffff
    .quad 0x0000000000000000
    .quad 0x0000000000000000
runtime_gdt_ptr:
    .word (7 * 8) - 1
    .quad runtime_gdt

.align 16
runtime_tss:
    .zero X86_RUNTIME_TSS_SIZE * X86_MAX_CORES

.section .bss
.align 16
runtime_irq_stack:
    .zero X86_RUNTIME_IRQ_STACK_SIZE * X86_MAX_CORES

.section .text

.global x86_runtime_init
x86_runtime_init:
    push rbx
    sub rsp, 8
    call x86_smp_mapping_init
    mov eax, 1
    xor ecx, ecx
    cpuid
    mov eax, ebx
    shr eax, 24
    and eax, 0xff
    mov edi, eax
    call x86_apic_id_to_core_id

    lea rdx, [rip + runtime_tss]
    mov ecx, eax
    imul rcx, X86_RUNTIME_TSS_SIZE
    add rdx, rcx

    lea rcx, [rip + runtime_irq_stack]
    imul rax, X86_RUNTIME_IRQ_STACK_SIZE
    add rcx, rax
    add rcx, X86_RUNTIME_IRQ_STACK_SIZE - 16
    mov qword ptr [rdx + 4], rcx
    mov word ptr [rdx + 102], X86_RUNTIME_TSS_SIZE

    mov rax, cr0
    and eax, 0xfffffffb
    or eax, 0x2
    mov cr0, rax
    clts

    mov rax, cr4
    or eax, 0x600
    mov cr4, rax
    fninit

    lea rax, [rip + runtime_gdt + 40]
    mov word ptr [rax + 0], X86_RUNTIME_TSS_SIZE - 1
    mov word ptr [rax + 2], dx
    shr rdx, 16
    mov byte ptr [rax + 4], dl
    mov byte ptr [rax + 5], 0x89
    mov byte ptr [rax + 6], 0x00
    shr rdx, 8
    mov byte ptr [rax + 7], dl
    shr rdx, 8
    mov dword ptr [rax + 8], edx
    mov dword ptr [rax + 12], 0

    lea rax, [rip + runtime_gdt_ptr]
    lgdt [rax]
    lea rax, [rip + 1f]
    push 0x08
    push rax
    lretq
1:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov ax, 0x28
    ltr ax
    add rsp, 8
    pop rbx
    ret

.global __core_id
__core_id:
    push rbx
    mov eax, 1
    xor ecx, ecx
    cpuid
    mov eax, ebx
    shr eax, 24
    and eax, 0xff
    pop rbx
    ret

.global __cpu_cores
__cpu_cores:
    push rbx
    mov eax, 1
    xor ecx, ecx
    cpuid
    mov eax, ebx
    shr eax, 16
    and eax, 0xff
    test eax, eax
    jnz 1f
    mov eax, 1
1:
    pop rbx
    ret

.global __set_vector_table
__set_vector_table:
    lidt [rdi]
    ret

.global __invalidate_icache_all
__invalidate_icache_all:
    mfence
    ret

.global __flush_dcache_all
__flush_dcache_all:
    wbinvd
    ret

.global __flush_tlb
__flush_tlb:
    mov rax, cr3
    mov cr3, rax
    ret

.global __set_translation_table_base
__set_translation_table_base:
    mov cr3, rdi
    ret

.global __irq_enable
__irq_enable:
    sti
    ret

.global __irq_disable
__irq_disable:
    cli
    ret

.global __int_off
__int_off:
    pushfq
    pop rax
    cli
    ret

.global __int_on
__int_on:
    push rdi
    popfq
    ret

.global __smp_lock
__smp_lock:
    mov eax, 1
1:
    xchg eax, dword ptr [rdi]
    test eax, eax
    jz 2f
    pause
    mov eax, 1
    jmp 1b
2:
    xor eax, eax
    ret

.global __smp_unlock
__smp_unlock:
    mov dword ptr [rdi], 0
    ret
