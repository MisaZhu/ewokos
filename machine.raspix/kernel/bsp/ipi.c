#include <kernel/smp/ipi.h>
#include <kernel/hw_info.h>
#include "hw_arch.h"

void ipi_enable_pix(uint32_t core_id);
void ipi_enable_pi4(uint32_t core_id);
void ipi_enable(uint32_t core_id) {
	if(_pi4)
		ipi_enable_pi4(core_id);
	else
		ipi_enable_pix(core_id);
}

void ipi_send_pix(uint32_t core_id);
void ipi_send_pi4(uint32_t core_id);
void ipi_send(uint32_t core_id) {
	if(_pi4)
		ipi_send_pi4(core_id);
	else
		ipi_send_pix(core_id);
}

void ipi_clear_pix(uint32_t core_id);
void ipi_clear_pi4(uint32_t core_id);
void ipi_clear(uint32_t core_id) {
	if(_pi4)
		ipi_clear_pi4(core_id);
	else
		ipi_clear_pix(core_id);
}