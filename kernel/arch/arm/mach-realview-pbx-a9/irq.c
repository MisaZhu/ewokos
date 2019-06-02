#include <irq.h>
#include <mm/mmu.h>
#include <timer.h>
#include <dev/basic_dev.h>

#define GIC_BASE 0x1F000000

void config_int(int int_id, int target_cpu) {
	int reg_offset, index, address;
	char priority = 0x80;
	// set int_id BIT in int ID register
	reg_offset = (int_id>>3) & 0xFFFFFFFC;
	index = int_id & 0x1F;
	address = (GIC_BASE + 0x1100) + reg_offset;
	*(int *)address |= (1 << index);
	// set int_id BYTE in processor targets register
	reg_offset = (int_id & 0xFFFFFFFC);
	index = int_id & 0x3;
	address = (GIC_BASE + 0x1400) + reg_offset + index;
	// set priority BYTE in priority register
	*(char *)address = (char)priority;
	address = (GIC_BASE + 0x1800) + reg_offset + index;
	*(char *)address = (char)(1 << target_cpu);
}

void irq_init(void) {
	config_int(36, 0); // Timer0
	config_int(44, 0); // UART0
	//config_int(45, 0); // UART1
	config_int(49, 0); // sdc handle
	config_int(52, 0); // KBD
	// set int priority mask register
	*(int *)(GIC_BASE + 0x104) = 0xFF;
	// set CPU interface control register: enable interrupts routing
	*(int *)(GIC_BASE + 0x100) = 1;
	// set distributor control register: send pending interrupts to CPUs
	*(int *)(GIC_BASE + 0x1000) = 1;
}

void irq_handle(void) {
	// read ICCIAR of CPU interface in the GIC
	int int_id = *(int *)(GIC_BASE + 0x10C);
	switch(int_id){
		case 36: timer_handle(); break; // timer interrupt
		case 44: uart_handle(); break; // UART0 interrupt
		//case 45: uart_handle(); break; // UART1 interrupt
		case 49: sdc_handle(); break; // sdc interrupt
		case 52: keyboard_handle(); break; // KBD interrupt
	}
	*(int *)(GIC_BASE + 0x110) = int_id; // write EOI
}

