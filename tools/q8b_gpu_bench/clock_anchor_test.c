#include <kernel/fine_clock.h>
#include <assert.h>
#include <stdio.h>

static void check_frequency(uint32_t hz,uint32_t period) {
    assert(fine_clock_period_usec(hz)==period);
    uint64_t q32=(0x100000000ull*1000000000ull)/hz;
    uint64_t previous=0;
    /* Exercise every fractional-us phase over tick boundaries, using the
       real counter-to-usec formula and an independent 128-bit ns oracle. */
    for (unsigned i=0;i<100000;i++) {
        uint64_t sampled=(uint64_t)hz*3600u+i;
        uint64_t usec=(uint64_t)((__uint128_t)sampled*1000000u/hz);
        uint64_t base=sampled+24u;
        fine_clock_anchor(hz,period,&usec,&base);
        uint64_t read=sampled+96u;
        uint64_t ns=usec*1000u+(uint64_t)((__uint128_t)(read-base)*q32>>32);
        uint64_t expected=(uint64_t)((__uint128_t)read*1000000000u/hz);
        assert(ns<=expected && expected-ns<=1u);
        assert(ns>=previous);
        previous=ns;
    }
}

int main(void) {
    check_frequency(19200000u,5u);
    check_frequency(24000000u,1u);
    check_frequency(1000000u,1u);
    assert(fine_clock_period_usec(0)==0);
    assert(fine_clock_period_usec(32768u)==0);
    uint64_t usec=1000000u,base=240000000u;
    fine_clock_anchor(24000000u,1u,&usec,&base);
    assert(usec==1000000u && base==240000000u); /* different epoch */
    base=24000000u-1u;
    fine_clock_anchor(24000000u,1u,&usec,&base);
    assert(usec==1000000u && base==24000000u-1u); /* approximate BSP */
    puts("clock anchor: 300000 fractional/integer clock samples passed");
    return 0;
}
