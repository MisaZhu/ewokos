#include <stdint.h>

#define FPEXC			cr8
#define FPEXC_EN		(1 << 30)
#define vfpreg(_vfp_) 	#_vfp_

#define fmrx(_vfp_) ({          \
    uint32_t __v;            \
    __asm("mrc p10, 7, %0, " vfpreg(_vfp_) ", cr0, 0 @ fmrx   %0, " #_vfp_    \
        : "=r" (__v) : : "cc"); \
    __v;                \
 })

#define fmxr(_vfp_,_var_)       \
    __asm("mcr p10, 7, %0, " vfpreg(_vfp_) ", cr0, 0 @ fmxr   " #_vfp_ ", %0" \
       : : "r" (_var_) : "cc")


#define isb(option) __asm__ __volatile__ ("isb " #option : : : "memory")
#define CPACC_FULL(n)		(3 << (n * 2))





static inline unsigned int get_copro_access(void)
{
    unsigned int val;
    __asm("mrc p15, 0, %0, c1, c0, 2 @ get copro access"
      : "=r" (val) : : "cc");
    return val;
}

static inline void set_copro_access(unsigned int val)
{
    __asm volatile("mcr p15, 0, %0, c1, c0, 2 @ set copro access"
      : : "r" (val) : "cc");
    isb();
}

static void vfp_enable(void *unused)
{
    uint32_t access;

    access = get_copro_access();

    /*
     * Enable full access to VFP (cp10 and cp11)
     */
    set_copro_access(access | CPACC_FULL(10) | CPACC_FULL(11));
}

void _enable_neon(void){
	vfp_enable(0);	
    uint32_t fpexc = fmrx(FPEXC);
    fmxr(FPEXC, fpexc | FPEXC_EN);
}