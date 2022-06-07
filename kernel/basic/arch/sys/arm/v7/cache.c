
#include <kernel/smp/cache.h>
#include <stdint.h>

// Cache maintenance operations for ARMv7-A
//
// See: ARMv7-A Architecture Reference Manual, Section B4.2.1
//
// NOTE: The following functions should hold all variables in CPU registers. Currently this will be
// ensured using the register keyword and maximum optimization (see uspienv/synchronize.h).
//
// The following numbers can be determined (dynamically) using CTR, CSSELR, CCSIDR and CLIDR.
// As long we use the Cortex-A7 implementation in the BCM2836 these static values will work:
//
#define SETWAY_LEVEL_SHIFT	1
#define L2_CACHE_SETS	1024
#define L2_CACHE_WAYS	8
#define L2_SETWAY_WAY_SHIFT	29	// 32-Log2(L2_CACHE_WAYS)
#define L2_CACHE_LINE_LENGTH	64
#define L2_SETWAY_SET_SHIFT	6	// Log2(L2_CACHE_LINE_LENGTH)
#define DATA_CACHE_LINE_LENGTH_MIN	64	// min(L1_DATA_CACHE_LINE_LENGTH, L2_CACHE_LINE_LENGTH)

void arm_invalidate_data_l2_caches(void)
{
// invalidate L2 unified cache
  for (register uint32_t nSet = 0; nSet < L2_CACHE_SETS; nSet++)
  {
    for (register uint32_t nWay = 0; nWay < L2_CACHE_WAYS; nWay++)
    {
      register uint32_t nSetWayLevel = nWay << L2_SETWAY_WAY_SHIFT
        | nSet << L2_SETWAY_SET_SHIFT
        | 1 << SETWAY_LEVEL_SHIFT;
      __asm volatile ("mcr p15, 0, %0, c7, c6, 2" : : "r" (nSetWayLevel) : "memory"); // DCISW
    }
  }
}

void arm_clear_data_l2_caches(void)
{
// clean L2 unified cache
  for (register uint32_t nSet = 0; nSet < L2_CACHE_SETS; nSet++)
  {
    for (register uint32_t nWay = 0; nWay < L2_CACHE_WAYS; nWay++)
    {
      register uint32_t nSetWayLevel = nWay << L2_SETWAY_WAY_SHIFT
        | nSet << L2_SETWAY_SET_SHIFT
        | 1 << SETWAY_LEVEL_SHIFT;
      __asm volatile ("mcr p15, 0, %0, c7, c10, 2" : : "r" (nSetWayLevel) : "memory"); // DCCSW
    }
  }
}