#include <mm/mmu.h>
#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <dev/timer.h>
#include <kernel/irq.h>

#define TIM0_BASE	(MMIO_BASE + 0x01C20000)
#define TIM0_CLK	(25*1000*1000)

#define PID12           0x0
#define TIM12           0x10
#define TIM34           0x14
#define PRD12           0x18
#define PRD34           0x1c
#define TCR				0x20
#define TGCR            0x24
#define WDTCR           0x28

/* Offsets of the 8 compare registers */
#define CMP12_0         0x60
#define CMP12_1         0x64
#define CMP12_2         0x68
#define CMP12_3         0x6c
#define CMP12_4         0x70
#define CMP12_5         0x74
#define CMP12_6         0x78
#define CMP12_7         0x7c

/* Timer register bitfields */
#define TCR_ENAMODE_DISABLE          0x0
#define TCR_ENAMODE_ONESHOT          0x1
#define TCR_ENAMODE_PERIODIC         0x2
#define TCR_ENAMODE_MASK             0x3

#define TGCR_TIMMODE_SHIFT           2
#define TGCR_TIMMODE_64BIT_GP        0x0
#define TGCR_TIMMODE_32BIT_UNCHAINED 0x1
#define TGCR_TIMMODE_64BIT_WDOG      0x2
#define TGCR_TIMMODE_32BIT_CHAINED   0x3

#define TGCR_TIM12RS_SHIFT           0
#define TGCR_TIM34RS_SHIFT           1
#define TGCR_RESET                   0x0
#define TGCR_UNRESET                 0x1
#define TGCR_RESET_MASK              0x3


#define writel(val, reg)	(*(volatile uint32_t*)(reg) = (val))
#define readl(reg)			(*(volatile uint32_t*)(reg))

static uint32_t _sys_sec_tic = 0;

static void davinci_timer_init(void){

	/* Disabled, Internal clock source */
    writel(0, TIM0_BASE + TCR);

    /* reset both timers, no pre-scaler for timer34 */
    uint32_t tgcr = 0;
    writel(tgcr, TIM0_BASE + TGCR);

    /* Set both timers to unchained 32-bit */
    tgcr = TGCR_TIMMODE_32BIT_UNCHAINED << TGCR_TIMMODE_SHIFT;
    writel(tgcr, TIM0_BASE + TGCR);

    /* Unreset timers */
    tgcr |= (TGCR_UNRESET << TGCR_TIM12RS_SHIFT) |
        (TGCR_UNRESET << TGCR_TIM34RS_SHIFT);
    writel(tgcr, TIM0_BASE + TGCR);

    /* Init both counters to zero */
    writel(0, TIM0_BASE + TIM12);
    writel(0, TIM0_BASE + TIM34);

	/* start TIM32 for system time counter*/
	writel(0, TIM0_BASE + TIM34);
	writel(TIM0_CLK, TIM0_BASE + PRD34);

	writel(TCR_ENAMODE_PERIODIC << 22, TIM0_BASE + TCR);
}

void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
	davinci_timer_init();
	(void)id;

	uint32_t tcr = readl(TIM0_BASE + TCR);
	uint32_t period = TIM0_CLK/times_per_sec;

	tcr &= ~(TCR_ENAMODE_MASK << 6);
    writel(tcr, TIM0_BASE + TCR);

	writel(0, TIM0_BASE + TIM12);
	writel(period, TIM0_BASE + PRD12);

	tcr |= TCR_ENAMODE_PERIODIC << 6;
	writel(tcr, TIM0_BASE + TCR);
}

void timer_clear_interrupt(uint32_t id) {
	(void)id;

}

/*  do fast 64bit div constant
*   we can do 64bit divided as below：
*   x / 25 = x / (4096/164) = x * 164 / 4096 = (x * 164) >> 12
*   it will save hundreds of cpu cycle
*/
static __inline uint64_t fast_div_25(uint32_t x){
    return (x*164)>>12;
}

uint64_t timer_read_sys_usec(void) { 
	static uint32_t last_counter = 0;
	uint32_t counter = readl(TIM0_BASE + TIM34);
	if(counter < last_counter){
		_sys_sec_tic += 1;
	}
	last_counter = counter;
	return _sys_sec_tic * 1000000 + fast_div_25(counter);
}
