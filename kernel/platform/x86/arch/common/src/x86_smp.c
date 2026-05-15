#include <kernel/core.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <interrupt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <x86_io.h>
#include <x86_smp.h>

#ifdef get_cpu_cores
#undef get_cpu_cores
#endif

#define IA32_APIC_BASE_MSR     0x1b
#define IA32_APIC_BASE_ENABLE  0x800

#define LAPIC_ID_REG           0x020
#define LAPIC_EOI_REG          0x0b0
#define LAPIC_SVR_REG          0x0f0
#define LAPIC_ICR_LOW_REG      0x300
#define LAPIC_ICR_HIGH_REG     0x310

#define LAPIC_SVR_ENABLE       0x100
#define LAPIC_ICR_INIT         0x00000500
#define LAPIC_ICR_STARTUP      0x00000600
#define LAPIC_ICR_LEVEL        0x00004000
#define LAPIC_ICR_ASSERT       0x00004000
#define LAPIC_ICR_TRIGGER      0x00008000
#define LAPIC_ICR_DELIVERY_BUSY 0x00001000

#define PIC1_COMMAND           0x20
#define PIC1_DATA              0x21
#define PIC2_COMMAND           0xa0
#define PIC2_DATA              0xa1
#define PIC_EOI                0x20

extern uint32_t interrupt_table_start;
extern volatile uint32_t _x86_irq_raw;
extern uintptr_t x86_isr_table[256];
extern uint8_t x86_idt_table[256 * 16];

typedef struct __attribute__((packed)) {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t ist;
	uint8_t type_attr;
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t reserved;
} x86_idt_gate_t;

static void x86_idt_init(void) {
	x86_idt_gate_t *idt = (x86_idt_gate_t *)x86_idt_table;
	for (uint32_t i = 0; i < 256; ++i) {
		uintptr_t handler = x86_isr_table[i];
		idt[i].offset_low = (uint16_t)(handler & 0xffffu);
		idt[i].selector = 0x08;
		idt[i].ist = 0;
		idt[i].type_attr = 0x8e;
		idt[i].offset_mid = (uint16_t)((handler >> 16) & 0xffffu);
		idt[i].offset_high = (uint32_t)((handler >> 32) & 0xffffffffu);
		idt[i].reserved = 0;
	}
}

static inline void x86_cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx,
		uint32_t *ecx, uint32_t *edx) {
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;

	__asm__ volatile("cpuid"
		: "=a"(a), "=b"(b), "=c"(c), "=d"(d)
		: "a"(leaf), "c"(0));

	if (eax != NULL) {
		*eax = a;
	}
	if (ebx != NULL) {
		*ebx = b;
	}
	if (ecx != NULL) {
		*ecx = c;
	}
	if (edx != NULL) {
		*edx = d;
	}
}

static inline uint64_t x86_rdmsr(uint32_t msr) {
	uint32_t lo;
	uint32_t hi;

	__asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
	return ((uint64_t)hi << 32) | lo;
}

static inline void x86_wrmsr(uint32_t msr, uint64_t value) {
	uint32_t lo = (uint32_t)value;
	uint32_t hi = (uint32_t)(value >> 32);
	__asm__ volatile("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}

static inline uint32_t x86_lapic_phys_base(void) {
	uint64_t base = x86_rdmsr(IA32_APIC_BASE_MSR);
	return (uint32_t)(base & 0xfffff000u);
}

static inline uintptr_t x86_lapic_vaddr(void) {
	uint32_t phys = x86_lapic_phys_base();
	return _sys_info.mmio.v_base + (phys - _sys_info.mmio.phy_base);
}

static inline uint32_t x86_lapic_read(uint32_t reg) {
	return get32(x86_lapic_vaddr() + reg);
}

static inline void x86_lapic_write(uint32_t reg, uint32_t value) {
	put32(x86_lapic_vaddr() + reg, value);
	(void)x86_lapic_read(LAPIC_ID_REG);
}

static void x86_lapic_wait_icr(void) {
	while ((x86_lapic_read(LAPIC_ICR_LOW_REG) & LAPIC_ICR_DELIVERY_BUSY) != 0) {
	}
}

static void x86_lapic_send_ipi_raw(uint32_t apic_id, uint32_t icr_low) {
	x86_lapic_wait_icr();
	x86_lapic_write(LAPIC_ICR_HIGH_REG, apic_id << 24);
	x86_lapic_write(LAPIC_ICR_LOW_REG, icr_low);
	x86_lapic_wait_icr();
}

static void x86_lapic_init(void) {
	uint64_t apic_base = x86_rdmsr(IA32_APIC_BASE_MSR);
	if ((apic_base & IA32_APIC_BASE_ENABLE) == 0) {
		x86_wrmsr(IA32_APIC_BASE_MSR, apic_base | IA32_APIC_BASE_ENABLE);
	}

	x86_lapic_write(LAPIC_SVR_REG, LAPIC_SVR_ENABLE | X86_VECTOR_SPURIOUS);
	x86_lapic_write(LAPIC_EOI_REG, 0);
}

static void x86_pic_remap(void) {
	uint8_t mask1 = x86_inb(PIC1_DATA);
	uint8_t mask2 = x86_inb(PIC2_DATA);

	x86_outb(PIC1_COMMAND, 0x11);
	x86_io_wait();
	x86_outb(PIC2_COMMAND, 0x11);
	x86_io_wait();

	x86_outb(PIC1_DATA, 0x20);
	x86_io_wait();
	x86_outb(PIC2_DATA, 0x28);
	x86_io_wait();

	x86_outb(PIC1_DATA, 4);
	x86_io_wait();
	x86_outb(PIC2_DATA, 2);
	x86_io_wait();

	x86_outb(PIC1_DATA, 0x01);
	x86_io_wait();
	x86_outb(PIC2_DATA, 0x01);
	x86_io_wait();

	x86_outb(PIC1_DATA, mask1);
	x86_outb(PIC2_DATA, mask2);
}

static inline void x86_pic_set_mask(uint8_t irq, bool masked) {
	uint16_t port;
	uint8_t value;

	if (irq < 8) {
		port = PIC1_DATA;
	}
	else {
		port = PIC2_DATA;
		irq -= 8;
	}

	value = x86_inb(port);
	if (masked) {
		value |= (uint8_t)(1u << irq);
	}
	else {
		value &= (uint8_t)~(1u << irq);
	}
	x86_outb(port, value);
}

static inline void x86_pic_mask_all(void) {
	x86_outb(PIC1_DATA, 0xff);
	x86_outb(PIC2_DATA, 0xff);
}

void irq_init_arch(void) {
	x86_pic_remap();
	x86_pic_mask_all();
	x86_idt_init();
	set_vector_table((ewokos_addr_t)&interrupt_table_start);
	x86_lapic_init();
}

void irq_enable_arch(uint32_t irq) {
	if (irq == IRQ_TIMER0) {
		x86_pic_set_mask(0, false);
	}
}

void irq_enable_core_arch(uint32_t core, uint32_t irq) {
	(void)core;
	(void)irq;
}

void irq_disable_arch(uint32_t irq) {
	if (irq == IRQ_TIMER0) {
		x86_pic_set_mask(0, true);
	}
}

uint32_t irq_get_arch(void) {
	return _x86_irq_raw;
}

uint32_t irq_get_unified_arch(uint32_t irq_raw) {
	if (irq_raw == X86_VECTOR_TIMER) {
		return IRQ_TIMER0;
	}
	if (irq_raw == X86_VECTOR_IPI) {
		return IRQ_IPI;
	}
	return irq_raw;
}

void irq_eoi_arch(uint32_t irq_raw) {
	if (irq_raw == X86_VECTOR_IPI) {
		x86_lapic_write(LAPIC_EOI_REG, 0);
		return;
	}

	if (irq_raw >= 0x20 && irq_raw < 0x30) {
		if (irq_raw >= 0x28) {
			x86_outb(PIC2_COMMAND, PIC_EOI);
		}
		x86_outb(PIC1_COMMAND, PIC_EOI);
	}
}

void ipi_enable(uint32_t core_id) {
	(void)core_id;
}

void ipi_send(uint32_t core_id) {
	if (core_id == get_core_id()) {
		return;
	}
	x86_lapic_send_ipi_raw(core_id, X86_VECTOR_IPI);
}

void ipi_clear(uint32_t core_id) {
	(void)core_id;
}

void cpu_core_ready(uint32_t core_id) {
	(void)core_id;
	x86_idt_init();
	set_vector_table((ewokos_addr_t)&interrupt_table_start);
	x86_lapic_init();
	__irq_enable();
}

uint32_t get_cpu_cores(void) {
	uint32_t ebx = 0;
	uint32_t cores = 1;

	x86_cpuid(1, NULL, &ebx, NULL, NULL);
	cores = (ebx >> 16) & 0xff;
	if (cores == 0) {
		cores = 1;
	}
	if (cores > CPU_MAX_CORES) {
		cores = CPU_MAX_CORES;
	}
	return cores;
}

void start_core(uint32_t core_id) {
	(void)core_id;
	/* AP trampoline/startup is wired as the next x86 SMP step. */
}
