#include <kernel/system.h>
#include <dev/timer.h>
#include <dev/actled.h>

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
extern void __cpu_dcache_clean_flush(void);


void flush_tlb(void) {
	__cpu_dcache_clean_flush();
	__flush_tlb();
}

void set_translation_table_base(uint32_t tlb_base) {
	__set_translation_table_base(tlb_base);
	flush_tlb();
}

#ifdef KERNEL_SMP
extern uint32_t __core_id(void);
extern uint32_t __cpu_cores(void);
extern uint32_t __smp_lock(int32_t* v);
extern uint32_t __smp_unlock(int32_t* v);

uint32_t get_core_id(void) {
	return __core_id();
}

uint32_t get_cpu_cores(void) {
	return __cpu_cores();
}

void smp_lock(int32_t* v) {
	__smp_lock(v);
}

void smp_unlock(int32_t* v) {
	__smp_unlock(v);
}

#else

uint32_t get_core_id(void) {
	return 0;
}

uint32_t get_cpu_cores(void) {
	return 1;
}

void smp_lock(int32_t* v) {
	(void)v;
}

void smp_unlock(int32_t* v) {
	(void)v;
}
#endif
