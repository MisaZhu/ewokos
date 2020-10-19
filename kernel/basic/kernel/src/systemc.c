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

