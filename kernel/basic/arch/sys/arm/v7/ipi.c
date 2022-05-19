#include <kernel/smp/ipi.h>
#include <kernel/hw_info.h>

#ifdef KERNEL_SMP

void ipi_enable(uint32_t core_id) {
	uint32_t reg = 0;
	switch(core_id) {
	case 0:
		reg = 0x40000050;
		break;
	case 1:
		reg = 0x40000054;
		break;
	case 2:
		reg = 0x40000058;
		break;
	case 3:
		reg = 0x4000005c;
		break;
	}

	if(reg == 0)
		return;

	reg = reg - _sys_info.mmio.phy_base + MMIO_BASE;
	put32(reg, 1);
}

void ipi_send(uint32_t core_id) {
	uint32_t reg = 0;
	switch(core_id) {
	case 0:
		reg = 0x40000080;
		break;
	case 1:
		reg = 0x40000090;
		break;
	case 2:
		reg = 0x400000a0;
		break;
	case 3:
		reg = 0x400000b0;
		break;
	}

	if(reg == 0)
		return;

	reg = reg - _sys_info.mmio.phy_base + MMIO_BASE;
	put32(reg, 1);
}

void ipi_clear(uint32_t core_id) {
	uint32_t reg = 0;
	switch(core_id) {
	case 0:
		reg = 0x400000c0;
		break;
	case 1:
		reg = 0x400000d0;
		break;
	case 2:
		reg = 0x400000e0;
		break;
	case 3:
		reg = 0x400000f0;
		break;
	}

	if(reg == 0)
		return;

	reg = reg - _sys_info.mmio.phy_base + MMIO_BASE;
	uint32_t v = get32(reg);
	put32(reg, v);
}
#endif
