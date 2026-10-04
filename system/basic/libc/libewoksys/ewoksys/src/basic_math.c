#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>

#ifdef __cplusplus
extern "C" {
#endif

static uint32_t _r_state = 0x13579abc;
inline uint32_t random_u32(void) {
    uint64_t usec;
    uint32_t x;

    kernel_tic(NULL, &usec);
    /*
     * Counter-based PRNG (splitmix32 style). The state advances by the golden
     * ratio each call and the kernel tick is folded in only as extra entropy.
     *
     * The old scheme returned usec * mask, which degenerates when several
     * calls land inside one tick (x86: 1024Hz tick vs a ~331fps repaint): the
     * usec factor is then constant and the outputs form a pure LCG mod 2^32,
     * whose weak low bits make `r % to` collapse onto a visible lattice
     * (regular rows/columns in xDemo). A well-mixed counter keeps `r % to`
     * uniform regardless of timer granularity.
     */
    _r_state += 0x9e3779b9u + (uint32_t)(usec ^ (usec >> 32));
    x = _r_state;
    x ^= x >> 16;
    x *= 0x21f0aaadu;
    x ^= x >> 15;
    x *= 0x735a2d97u;
    x ^= x >> 15;
    return x;
}

inline uint32_t random_to(uint32_t to) {
    /* to == 0 would be a divide-by-zero (#DE) fault; a zero-width range has a
       single possible result anyway, so return 0. Callers routinely pass a
       computed range that collapses to 0 (e.g. a window auto-size before the
       display size is known). */
    if(to == 0)
        return 0;
    uint32_t r = random_u32();	
    return r % to;
}

inline uint32_t abs_32(int32_t v) {
    return v < 0 ? -v:v;
}

#ifdef __cplusplus
}
#endif
