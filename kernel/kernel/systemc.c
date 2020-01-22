#include <kernel/system.h>
#include <dev/timer.h>
#include <dev/actled.h>

extern void __dummy(void);
inline void _delay(uint32_t count) {
	while(count > 0) {
		__dummy();
		count--;
	}
}

inline void _delay_usec(uint64_t count) {
	uint64_t s = timer_read_sys_usec();
	uint64_t t = s + count;
	while(s < t) {
		s = timer_read_sys_usec();
	}
}

inline void _delay_msec(uint32_t count) {
	_delay_usec(count*1000);
}

inline void __attribute__((optimize("O0"))) _flush_tlb(void) {
  __asm__ volatile("MOV R6, #0");
  __asm__ volatile("MCR p15, 0, R6, c8, c7, 0");
}
