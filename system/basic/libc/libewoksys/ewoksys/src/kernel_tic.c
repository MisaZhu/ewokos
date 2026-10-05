#include <ewoksys/sys.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/fine_cnt.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * (a*b) >> 32 without a 64-bit divide or __int128: arm-none-eabi has no
 * hardware divide and no 128-bit type, so the fine-clock interpolation must be
 * pure multiply/shift.
 *
 * Splitting a = a_hi*2^32 + a_lo (same for b) gives
 *   (a*b)>>32 = (a_hi*b_hi)<<32 + a_hi*b_lo + a_lo*b_hi + (a_lo*b_lo)>>32
 * Every term is accumulated mod 2^64; the bits that overflow are exactly the
 * ones a 128-bit product would carry past bit 63 and this result discards.
 *
 * Note the <<32 on the a_hi*b_hi term: dropping it computes (a*b)>>64 instead,
 * which for the 32.32 reciprocals used here (nsec_q32 ~ 2^37 at the 24MHz
 * counter of the aarch64 virt target) silently evaluates to 0 for any
 * sub-second delta.
 */
static inline uint64_t mulhi64(uint64_t a, uint64_t b) {
    uint64_t a_lo = a & 0xffffffffULL, a_hi = a >> 32;
    uint64_t b_lo = b & 0xffffffffULL, b_hi = b >> 32;
    return (a_hi * b_hi << 32) + a_hi * b_lo + a_lo * b_hi + ((a_lo * b_lo) >> 32);
}

/*
 * fine_cnt_read() - the arch-specific reader for the counter this clock is
 * interpolated from - and the FINE_CNT_READABLE compile-time gate are declared in
 * <ewoksys/fine_cnt.h>. Each arch supplies the reader in fine_cnt_<arch>.c, pulled
 * in by the build the same way syscall_<arch>.S is, so this file stays arch-agnostic
 * and only consumes the two symbols.
 */

/* acquire ordering: pairs with the kernel's publication of seq/base/kernel_usec */
static inline void read_barrier(void) {
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

inline int32_t kernel_tic32(uint32_t* sec, uint32_t* usec_hi, uint32_t* usec_low) {
    if(_vsyscall_info == NULL)
        return -1;

    if(sec != NULL)
        *sec = _vsyscall_info->kernel_usec / 1000000;
    if(usec_hi != NULL)
        *usec_hi = _vsyscall_info->kernel_usec >> 32;
    if(usec_low != NULL)
        *usec_low = _vsyscall_info->kernel_usec & 0xffffffff;
    return 0;
}

inline int32_t kernel_tic(uint32_t* sec, uint64_t* usec) {
    if(_vsyscall_info == NULL)
        return -1;

    if(usec != NULL)
        *usec = _vsyscall_info->kernel_usec;
    if(sec != NULL)
        *sec = _vsyscall_info->kernel_usec / 1000000;
    return 0;
}

uint64_t kernel_tic_ms(int zone){
    return _vsyscall_info->kernel_usec / 1000;
}

/*
 * Seqlock reader for the interpolated clock. The kernel makes fine.seq odd while
 * it republishes (kernel_usec, base_cnt) on each tick; we snapshot both plus the
 * scaling factor, sample the counter, and retry if seq moved under us.
 *
 * The counter is sampled AFTER base_cnt on purpose: the kernel stores
 * kernel_usec before base_cnt, so any slack lands on the side of reporting a
 * time slightly behind real time. A deadline computed from this clock is then
 * met a hair late rather than early, which is the safe direction for a sleep.
 */
#define KERNEL_TIC_SEQ_RETRY 16

int32_t kernel_tic_nsec(uint64_t* nsec) {
    uint64_t ns;
    const volatile uint64_t* vusec;

    if(nsec == NULL)
        return -1;
    if(_vsyscall_info == NULL)
        return -1;

    /*
     * kernel_usec must be read through a volatile alias. _vsyscall_info is a
     * plain pointer, so a direct `_vsyscall_info->kernel_usec` is an ordinary
     * load the compiler may CSE against the coarse read below and hoist out of
     * the retry loop - the volatile accesses to f->seq do not invalidate it,
     * because distinct members of the same struct cannot alias each other. The
     * loop would then pair a stale usec with a freshly read base_cnt, and since
     * seq genuinely did not move during the body the mismatch passes
     * validation, putting the clock a whole tick BEHIND a previous read.
     */
    vusec = &_vsyscall_info->kernel_usec;

    ns = *vusec * 1000ULL;

#if FINE_CNT_READABLE
    {
        const volatile vsyscall_fine_clock_t* f = &_vsyscall_info->fine;
        uint64_t usec = 0;          /* only valid once `ok` is set */
        uint64_t base_cnt = 0;
        uint64_t q32 = 0;
        uint64_t cnt = 0;
        uint32_t hz = 0;
        int32_t i;
        int32_t ok = 0;

        for(i = 0; i < KERNEL_TIC_SEQ_RETRY; i++) {
            uint32_t s0 = f->seq;
            if(s0 & 1u)                 /* writer in progress */
                continue;
            read_barrier();
            usec     = *vusec;
            base_cnt = f->base_cnt;
            q32      = ((uint64_t)f->nsec_q32_hi << 32) | (uint64_t)f->nsec_q32_lo;
            hz       = f->fine_cnt_hz;
            cnt      = fine_cnt_read();
            read_barrier();
            if(f->seq == s0) {
                ok = 1;
                break;
            }
        }

        /*
         * usec and base_cnt come from the same verified iteration, so they are a
         * matched pair and the interpolation is exact.
         *
         * If the seqlock never settled, or the kernel published no counter, `ns`
         * stays the plain kernel_usec scaling. Note that this degraded value is
         * up to one tick STALE rather than merely less precise - kernel_usec is
         * the previous tick's uptime - so it can sit below an interpolated read
         * that preceded it. It is only reachable when the writer is contended
         * hard enough to defeat every retry, and proc_nsleep_precise() does not
         * depend on it (it spins on the raw counter instead), so a sleeper can
         * still not wake early.
         */
        if(ok && hz != 0 && cnt >= base_cnt)
            ns = usec * 1000ULL + mulhi64(cnt - base_cnt, q32);
    }
#endif

    *nsec = ns;
    return 0;
}

int32_t kernel_tic_fine_available(void) {
#if FINE_CNT_READABLE
    if(_vsyscall_info == NULL)
        return 0;
    return _vsyscall_info->fine.fine_cnt_hz != 0 ? 1 : 0;
#else
    return 0;
#endif
}

uint32_t kernel_tic_fine_hz(void) {
#if FINE_CNT_READABLE
    if(_vsyscall_info == NULL)
        return 0;
    return _vsyscall_info->fine.fine_cnt_hz;
#else
    return 0;
#endif
}

uint32_t kernel_tic_tick_usec(void) {
    if(_vsyscall_info == NULL)
        return 0;
    return _vsyscall_info->fine.tick_usec;
}

/*
 * Snapshot the counter's reciprocal under the seqlock. It is effectively
 * immutable once the kernel has probed it, but an unlocked read could still
 * catch the single publication that establishes it and pair a live
 * fine_cnt_hz with a reciprocal that is still zero.
 *
 * Testing q32 alone is the whole availability check: the kernel computes the
 * reciprocal as 0 exactly when it published no counter, so q32 != 0 implies
 * fine_cnt_hz != 0, and reading the two halves under the lock guarantees they
 * belong to the same value.
 */
static int32_t fine_scale(uint64_t* q32) {
#if FINE_CNT_READABLE
    const volatile vsyscall_fine_clock_t* f = &_vsyscall_info->fine;
    int32_t i;

    for(i = 0; i < KERNEL_TIC_SEQ_RETRY; i++) {
        uint32_t s0 = f->seq;
        if(s0 & 1u)                 /* writer in progress */
            continue;
        read_barrier();
        *q32 = ((uint64_t)f->nsec_q32_hi << 32) | (uint64_t)f->nsec_q32_lo;
        read_barrier();
        if(f->seq == s0)
            return (*q32 != 0) ? 0 : -1;
    }
    return -1;
#else
    (void)q32;
    return -1;
#endif
}

int32_t kernel_tic_fine_cnt(uint64_t* cnt) {
#if FINE_CNT_READABLE
    uint64_t q32 = 0;               /* only its presence matters here */

    if(cnt == NULL || _vsyscall_info == NULL)
        return -1;
    if(fine_scale(&q32) != 0)
        return -1;
    *cnt = fine_cnt_read();
    return 0;
#else
    (void)cnt;
    return -1;
#endif
}

int32_t kernel_tic_spin_until(uint64_t from_cnt, uint64_t nsec, uint32_t max_iters) {
#if FINE_CNT_READABLE
    uint32_t i;
    uint64_t q32 = 0;
    uint64_t cnt;

    if(_vsyscall_info == NULL)
        return -1;
    if(fine_scale(&q32) != 0)
        return -1;
    if(nsec == 0)
        return 0;

    for(i = 0; i < max_iters; i++) {
        cnt = fine_cnt_read();
        /*
         * from_cnt was sampled from this same counter earlier and the counter
         * only runs forward, so the subtraction cannot underflow; the check is
         * there to keep a wrapped or reset counter from producing a gigantic
         * delta that would end the wait immediately.
         *
         * Both rounding steps - the truncated reciprocal the kernel published
         * and the truncated product - under-report elapsed time, so this
         * overshoots by under one counter tick instead of returning short.
         */
        if(cnt >= from_cnt && mulhi64(cnt - from_cnt, q32) >= nsec)
            return 0;
    }
    return -1;
#else
    (void)from_cnt;
    (void)nsec;
    (void)max_iters;
    return -1;
#endif
}

#ifdef __cplusplus
}
#endif
