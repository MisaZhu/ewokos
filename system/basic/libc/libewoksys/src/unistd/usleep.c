#include <unistd.h>
#include <ewoksys/proc.h>
#include <ewoksys/syscall.h>

/*
 * usleep(): "at least N microseconds" semantics, hybrid dispatch.
 *   usecs <= 200  -> precise fine-counter spin: exact N us wall-clock,
 *                    CPU cost bounded by 200us. On platforms without a
 *                    user-readable fine counter (x86, lego.ev3) precise
 *                    returns -1 and we fall through to SYS_USLEEP.
 *   usecs > 200   -> SYS_USLEEP park: tick-quantized (kernel
 *                    renew_sleep_counter decrements one tick per tick,
 *                    so N=1000 parks ~2 ticks ~1952us on virt), CPU 0%.
 *                    The overshoot is deliberate: it is the CPU-friendly
 *                    trade for medium/long waits. Callers needing sub-tick
 *                    precision above 200us should use nanosleep() /
 *                    proc_nsleep_precise() directly, which spins up to
 *                    PROC_SLEEP_SPIN_MAX_US.
 *
 * The (uint64_t)usecs * 1000ULL cast is defensive: the <=200 branch cannot
 * overflow uint32 (200*1000 = 200000), and the >200 branch bypasses the
 * multiply entirely, so sleep(N>=5) -> usleep(N*1e6) no longer truncates.
 */
int usleep(useconds_t usecs) {
    if(usecs <= 200) {
        if(proc_nsleep_precise((uint64_t)usecs * 1000ULL) == 0)
            return 0;
    }
    syscall1(SYS_USLEEP, (ewokos_addr_t)usecs);
    return 0;
}
