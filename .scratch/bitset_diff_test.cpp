/* Scratch: differential test of EWOK_STL's <bitset> against the host's own
 * std::bitset.  Not part of any build; compile and run on the host with
 *
 *   c++ -std=c++14 -Wall -Wextra -pedantic -I. bitset_diff_test.cpp -o /tmp/bt
 *   /tmp/bt
 *
 * The header under test is included as ewok_bitset_ns.h, which is
 * system/basic/libs/c++/stl/include/bitset with its three quoted includes turned
 * into angle-bracket ones and `namespace std` renamed to `namespace ewok`, so
 * that both implementations can be in one translation unit without colliding.
 * Nothing else about it is altered.
 *
 * to_string() is the fingerprint compared at every step: it exposes all N bits
 * in order, so it catches a wrong shift, a wrong bit index, and leaked padding
 * in the topmost word alike.
 */

#include <bitset>
#include <cstdio>
#include <string>

#include "ewok_bitset_ns.h"

static int g_failures = 0;
static int g_checks   = 0;

/* Deterministic, so a failure reproduces without a seed to hunt for. */
static unsigned long long g_state = 88172645463325252ULL;

static unsigned long long rnd()
{
    g_state ^= g_state << 13;
    g_state ^= g_state >> 7;
    g_state ^= g_state << 17;
    return g_state;
}

static void fail( const char* what, size_t n, size_t extra )
{
    std::printf( "FAIL  N=%zu  %s  (i=%zu)\n", n, what, extra );
    ++g_failures;
}

#define SAME( expr_r, expr_g, what, n, extra )                          \
    do {                                                                \
        ++g_checks;                                                     \
        if( !((expr_r) == (expr_g)) ) fail( what, (n), (size_t)(extra) ); \
    } while( 0 )

template <size_t N>
static void compare( const std::bitset<N>& r, const ewok::bitset<N>& g,
                     const char* what )
{
    ++g_checks;
    if( r.to_string() != g.to_string() )
    {
        std::printf( "FAIL  N=%zu  %s\n        std = %s\n        ewok= %s\n",
                     N, what, r.to_string().c_str(), g.to_string().c_str() );
        ++g_failures;
    }
    SAME( r.count(), g.count(), what, N, 0 );
    SAME( r.any(),   g.any(),   what, N, 0 );
    SAME( r.none(),  g.none(),  what, N, 0 );
    SAME( r.all(),   g.all(),   what, N, 0 );
}

template <size_t N>
static void testN()
{
    /* --- default construction, and the degenerate empty bitset --- */
    {
        std::bitset<N>  r;
        ewok::bitset<N> g;
        compare( r, g, "default ctor" );
        SAME( r.size(), g.size(), "size()", N, 0 );
    }

    /* --- converting constructor across the whole 64-bit range --- */
    for( int t = 0; t < 200; ++t )
    {
        const unsigned long long v = rnd();
        std::bitset<N>  r( v );
        ewok::bitset<N> g( v );
        compare( r, g, "ctor(ull)" );

        /* Only meaningful where the value fits; the standard throws
         * overflow_error above that and ours truncates by design. */
        if( N > 0 && N <= 64 )
            SAME( r.to_ullong(), g.to_ullong(), "to_ullong", N, 0 );
    }

    /* --- set()/reset()/flip(), whole object --- */
    {
        std::bitset<N>  r( rnd() );
        ewok::bitset<N> g;
        /* Bit by bit rather than through to_ullong(): for N > 64 that throws
         * overflow_error whenever a bit above 63 is set, which it just was. */
        g.reset();
        for( size_t i = 0; i < N; ++i ) if( r.test( i ) ) g.set( i );
        compare( r, g, "random start" );

        r.set();  g.set();  compare( r, g, "set()" );
        r.flip(); g.flip(); compare( r, g, "flip() after set()" );
        r.reset(); g.reset(); compare( r, g, "reset()" );
        r.flip(); g.flip(); compare( r, g, "flip() after reset()" );

        ewok::bitset<N> n1 = ~g;
        std::bitset<N>  n2 = ~r;
        compare( n2, n1, "operator~" );
    }

    /* --- per-bit access, including the reference proxy --- */
    for( int t = 0; t < 300 && N > 0; ++t )
    {
        std::bitset<N>  r( rnd() );
        ewok::bitset<N> g;
        /* Rebuild g bit by bit from r, which exercises operator[] on both
         * sides, the reference assignment, and set(pos,val). */
        g.reset();
        for( size_t i = 0; i < N; ++i )
            if( r.test( i ) ) g.set( i );
        compare( r, g, "rebuilt via set(i)" );

        const size_t i = size_t( rnd() % N );
        const size_t j = size_t( rnd() % N );

        SAME( r.test( i ), g.test( i ), "test(i)", N, i );
        SAME( r[i], g[i], "operator[](i) const", N, i );

        /* reference = reference, including the i == j self case */
        r[i] = r[j];
        g[i] = g[j];
        compare( r, g, "ref = ref" );

        r.flip( i ); g.flip( i ); compare( r, g, "flip(i)" );
        r.reset( i ); g.reset( i ); compare( r, g, "reset(i)" );
        r.set( i, false ); g.set( i, false ); compare( r, g, "set(i,false)" );
        r.set( i, true );  g.set( i, true );  compare( r, g, "set(i,true)" );
    }

    /* --- shifts, at and beyond both ends --- */
    for( size_t pos = 0; pos <= N + 3; ++pos )
    {
        std::bitset<N>  r( rnd() );
        ewok::bitset<N> g;
        g.reset();
        for( size_t i = 0; i < N; ++i ) if( r.test( i ) ) g.set( i );

        std::bitset<N>  rl( r ), rr( r );
        ewok::bitset<N> gl( g ), gr( g );
        rl <<= pos; gl <<= pos; compare( rl, gl, "operator<<=" );
        rr >>= pos; gr >>= pos; compare( rr, gr, "operator>>=" );

        compare( r << pos, g << pos, "operator<< (non-member)" );
        compare( r >> pos, g >> pos, "operator>> (non-member)" );
    }

    /* --- combining two bitsets --- */
    for( int t = 0; t < 100; ++t )
    {
        std::bitset<N>  ra( rnd() ), rb( rnd() );
        ewok::bitset<N> ga, gb;
        ga.reset(); gb.reset();
        for( size_t i = 0; i < N; ++i )
        {
            if( ra.test( i ) ) ga.set( i );
            if( rb.test( i ) ) gb.set( i );
        }
        compare( ra, ga, "operand a" );
        compare( rb, gb, "operand b" );

        compare( ra & rb, ga & gb, "operator&" );
        compare( ra | rb, ga | gb, "operator|" );
        compare( ra ^ rb, ga ^ gb, "operator^" );

        std::bitset<N>  r1( ra ), r2( ra ), r3( ra );
        ewok::bitset<N> g1( ga ), g2( ga ), g3( ga );
        r1 &= rb; g1 &= gb; compare( r1, g1, "operator&=" );
        r2 |= rb; g2 |= gb; compare( r2, g2, "operator|=" );
        r3 ^= rb; g3 ^= gb; compare( r3, g3, "operator^=" );

        SAME( (ra == rb), (ga == gb), "operator==", N, 0 );
        SAME( (ra != rb), (ga != gb), "operator!=", N, 0 );
    }
}

int main()
{
    /* Widths chosen to straddle every boundary the word arithmetic has: zero,
     * one, a partial word, a full word, one bit either side of a full word, and
     * several words with a partial top. */
    testN<0>();
    testN<1>();
    testN<7>();
    testN<8>();       /* what SimulIDE's e-shiftreg actually uses */
    testN<31>();
    testN<32>();
    testN<33>();
    testN<63>();
    testN<64>();
    testN<65>();
    testN<127>();
    testN<128>();
    testN<129>();
    testN<200>();

    std::printf( "%d checks, %d failures\n", g_checks, g_failures );
    return g_failures ? 1 : 0;
}
