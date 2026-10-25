/* Q8B live regression: the reserved vsyscall page must start with an even
 * seqlock, and precise GPU polling waits must use the 19.2 MHz counter. */
#include <ewoksys/kernel_tic.h>
#include <ewoksys/proc.h>
#include <ewoksys/sys.h>
#include <stdio.h>

static uint64_t counter(void) {
    uint64_t value;
    __asm__ volatile("isb; mrs %0, cntvct_el0" : "=r"(value) :: "memory");
    return value;
}

int main(void) {
    uint64_t hz;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(hz));
    if (!hz || !_vsyscall_info) return 1;
    printf("clock raw_hz=%llu fine_available=%d fine_hz=%u tick_us=%u\n",
           (unsigned long long)hz,kernel_tic_fine_available(),
           kernel_tic_fine_hz(),kernel_tic_tick_usec());
    volatile unsigned *fine=(void*)&_vsyscall_info->fine;
    for (unsigned i=0;i<8;i++) printf("fine[%u]=%08x\n",i,fine[i]);
    unsigned failures=0,backwards=0,even=0;
    uint64_t previous=0,max_backwards=0;
    for (unsigned i=0;i<100000;i++) {
        uint64_t now;
        kernel_tic_nsec(&now);
        if (now<previous) {
            uint64_t delta=previous-now;
            if (delta>max_backwards) max_backwards=delta;
            if (backwards<4) printf("clock backwards=%llu ns seq=%08x\n",
                                   (unsigned long long)delta,fine[0]);
            backwards++;
        }
        even+=!(fine[0]&1u);
        previous=now;
    }
    for (unsigned i=0;i<8;i++) {
        uint64_t a=counter();
        int rc=proc_nsleep_precise(10000);
        uint64_t ns=(counter()-a)*1000000000ull/hz;
        printf("precise rc=%d raw=%llu ns\n",rc,(unsigned long long)ns);
        failures+=rc!=0 || ns<10000;
    }
    failures+=!even || backwards!=0;
    printf("clock checked=100000 even=%u backwards=%u max_backwards=%llu ns failures=%u\n",
           even,backwards,(unsigned long long)max_backwards,failures);
    return failures?1:0;
}
