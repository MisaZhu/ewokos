#include <kernel/smp/ipi.h>
#include <kernel/hw_info.h>

void ipi_enable_pix(uint32_t core_id) {
	uint32_t reg = 0;
	switch(core_id) {
	case 0:
		reg = 0x50;
		break;
	case 1:
		reg = 0x54;
		break;
	case 2:
		reg = 0x58;
		break;
	case 3:
		reg = 0x5c;
		break;
	}

	if(reg == 0)
		return;

	reg += _sys_info.mmio.v_base + _core_base_offset;
	put32(reg, 1);
}

void ipi_send_pix(uint32_t core_id) {
	uint32_t reg = 0;
	switch(core_id) {
	case 0:
		reg = 0x80;
		break;
	case 1:
		reg = 0x90;
		break;
	case 2:
		reg = 0xa0;
		break;
	case 3:
		reg = 0xb0;
		break;
	}

	if(reg == 0)
		return;

	reg += _sys_info.mmio.v_base + _core_base_offset;
	put32(reg, 1);
}

void ipi_clear_pix(uint32_t core_id) {
	uint32_t reg = 0;
	switch(core_id) {
	case 0:
		reg = 0xc0;
		break;
	case 1:
		reg = 0xd0;
		break;
	case 2:
		reg = 0xe0;
		break;
	case 3:
		reg = 0xf0;
		break;
	}

	if(reg == 0)
		return;

	reg += _sys_info.mmio.v_base + _core_base_offset;
	uint32_t v = get32(reg);
	put32(reg, v);
}