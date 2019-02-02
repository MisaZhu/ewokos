#include <dev/uart.h>
#include <hardware.h>
#include <mm/mmu.h>
#include <types.h>

#define GPFSEL1 0x20200004
#define GPSET0  0x2020001C
#define GPCLR0  0x20200028
#define GPPUD	   0x20200094
#define GPPUDCLK0   0x20200098

#define AUX_ENABLES	 0x20215004
#define AUX_MU_IO_REG   0x20215040
#define AUX_MU_IER_REG  0x20215044
#define AUX_MU_IIR_REG  0x20215048
#define AUX_MU_LCR_REG  0x2021504C
#define AUX_MU_MCR_REG  0x20215050
#define AUX_MU_LSR_REG  0x20215054
#define AUX_MU_MSR_REG  0x20215058
#define AUX_MU_SCRATCH  0x2021505C
#define AUX_MU_CNTL_REG 0x20215060
#define AUX_MU_STAT_REG 0x20215064
#define AUX_MU_BAUD_REG 0x20215068

#define IRQ_BASIC 0x2000B200
#define IRQ_PEND1 0x2000B204
#define IRQ_PEND2 0x2000B208
#define IRQ_FIQ_CONTROL 0x2000B210
#define IRQ_ENABLE1 0x2000B210
#define IRQ_ENABLE2 0x2000B214
#define IRQ_ENABLE_BASIC 0x2000B218
#define IRQ_DISABLE1 0x2000B21C
#define IRQ_DISABLE2 0x2000B220
#define IRQ_DISABLE_BASIC 0x2000B224

static void PUT32(uint32_t address, uint32_t value) {
	*MMIO_P2V(address) = value;
}

static uint32_t GET32(uint32_t address) {
	return *MMIO_P2V(address);
}

/*
 * delay function
 * int32_t delay: number of cycles to delay
 *
 * This just loops <delay> times in a way that the compiler
 * wont optimize away.
 */
static void delay(int32_t count) {
	__asm__ volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : : [count]"r"(count) : "cc");
}

void uartDevInit(void) {
	unsigned int ra;

	PUT32(IRQ_DISABLE1,1<<29);

	PUT32(AUX_ENABLES,1);
	PUT32(AUX_MU_IER_REG,0);
	PUT32(AUX_MU_CNTL_REG,0);
	PUT32(AUX_MU_LCR_REG,3);
	PUT32(AUX_MU_MCR_REG,0);
	PUT32(AUX_MU_IER_REG,0x5); //enable rx interrupts
	PUT32(AUX_MU_IIR_REG,0xC6);
	PUT32(AUX_MU_BAUD_REG,270);

	ra=GET32(GPFSEL1);
	ra&=~(7<<12); //gpio14
	ra|=2<<12;	//alt5
	ra&=~(7<<15); //gpio15
	ra|=2<<15;	//alt5
	PUT32(GPFSEL1,ra);

	PUT32(GPPUD,0);
	delay(150);
	PUT32(GPPUDCLK0,(1<<14)|(1<<15));
	delay(150);
	PUT32(GPPUDCLK0,0);

	PUT32(AUX_MU_CNTL_REG,3);

	PUT32(IRQ_ENABLE1,1<<29);
}


void uartTransmit(char c) {
	while(1)
	{
		if(GET32(AUX_MU_LSR_REG)&0x20) break;
	}
	PUT32(AUX_MU_IO_REG,c);
}

bool uartReadyToRecv(void) {
	unsigned int rb=GET32(AUX_MU_IIR_REG);
	return (rb & 1) != 1;
}

int uartReceive(void) {
	return GET32(AUX_MU_IO_REG);
}

