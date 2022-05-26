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

extern void __set_translation_table_base(uint32_t);
extern void __flush_tlb(void);
extern void __invalidate_dcache_all(void);
extern void __flush_dcache_all(void);


inline void flush_tlb(void) {
	//__cpu_dcache_clean_flush();
	//__invalidate_dcache_all();
	__flush_dcache_all();
	__flush_tlb();
}

inline void flush_dcache(void) {
	__flush_dcache_all();
}

inline void set_translation_table_base(uint32_t tlb_base) {
	__set_translation_table_base(tlb_base);
	flush_tlb();
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
#ifdef KERNEL_SMP
		__asm__("WFI");
#else
		__asm__("MOV r0, #0; MCR p15,0,R0,c7,c0,4"); // CPU enter WFI state
#endif
	}
}

