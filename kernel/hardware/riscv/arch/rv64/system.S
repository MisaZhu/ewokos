#include <csr.h>

.global __core_id
__core_id:
	ret

#define ENTRY(symbol)                    \
        .globl  symbol;                  \
        .type   symbol, function;        \
symbol:
#define ENDPROC(x)

.global __flush_dcache_all
__flush_dcache_all:
	fence.i
	ret

.global __flush_tlb
__flush_tlb:
	sfence.vma
	ret

.global __set_translation_table_base
__set_translation_table_base:
	fence.i
	lui a5,0x8ffff
	slli    a5,a5,0x20
	srli    a0,a0,0xc
	or  a0,a0,a5
	csrw    satp,a0
	sfence.vma
	ret

.global __irq_enable
__irq_enable:
	csrsi   sstatus,2
	ret 

.global __irq_disable
__irq_disable:
	csrci	sstatus,2
	ret

.global __smp_lock
__smp_lock:
	ret

.global __smp_unlock
__smp_unlock:
	ret
