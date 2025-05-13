#include <kernel/system.h>
#include <kernel/core.h>
#include <dev/timer.h>

void __attribute__((optimize("O0"))) _delay(uint32_t count) {
	while(count > 0) {
		count--;
	}
}

void _delay_usec(uint64_t count) {
	uint64_t s = timer_read_sys_usec();
	uint64_t t = s + count;
	while(s < t) {
		s = timer_read_sys_usec();
	}
}

inline void _delay_msec(uint32_t count) {
	_delay_usec(count*1000);
}

extern void __flush_dcache_all(void);
extern void __invalidate_icache_all(void);

#ifdef KERNEL_SMP

inline void flush_dcache(void) {
	__flush_dcache_all();
}

inline void invalidate_icache_all(void) {
	__invalidate_icache_all();
}

inline void flush_tlb(void) {
	flush_dcache();
	invalidate_icache_all();
	__flush_tlb();
}

#else

inline void flush_dcache(void) { 
	__flush_dcache_all();
}

inline void flush_tlb(void) {
	flush_dcache();
	__flush_tlb();
}
#endif

inline void set_translation_table_base(ewokos_addr_t tlb_base) {
	__set_translation_table_base(tlb_base);
	flush_tlb();
}

inline void set_vector_table(ewokos_addr_t vector) {
	__set_vector_table(vector);
}


#ifdef KERNEL_SMP
static int32_t _spin = 0;
static int32_t _klock = 0;

inline void kernel_lock_init(void) {
	_spin = _klock = 0;
}

inline int32_t kernel_lock_check(void) {
	return _klock;
}

inline void kernel_lock(void) {
	mcore_lock(&_spin);
	_klock = 1;
}

inline void kernel_unlock(void) {
	_klock = 0;
	mcore_unlock(&_spin);
}
#endif

inline void halt(void) {
	while(1) {
#ifdef ARM_V6
		__asm__("MOV r0, #0; MCR p15,0,R0,c7,c0,4"); // CPU enter WFI state
#else
		__asm__("WFI");
#endif
	}
}
