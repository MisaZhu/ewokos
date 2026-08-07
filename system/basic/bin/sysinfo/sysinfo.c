#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/sys.h>
#include <sysinfo.h>

static inline uint32_t addr_hi(ewokos_addr_t v) {
	return (uint32_t)(v >> 32);
}

static inline uint32_t addr_lo(ewokos_addr_t v) {
	return (uint32_t)(v & 0xffffffffu);
}

int main(int argc, char* argv[]) {
	sys_info_t sys_info;
	sys_state_t sys_state;

	sys_get_sys_info(&sys_info);
	syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)&sys_state);
	char fr_mem[32] = {0};
	get_mem_size_desc(sys_state.mem.free, fr_mem);
	char kfr_mem[32] = {0};
	get_mem_size_desc(sys_state.mem.kfree, kfr_mem);
	char kt_mem[32] = {0};
	get_mem_size_desc(sys_info.sys_dma.phy_base - sys_info.phy_offset, kt_mem);
	char phy_mem[32] = {0};
	get_mem_size_desc(sys_info.total_phy_mem_size, phy_mem);
	char t_mem[32] = {0};
	get_mem_size_desc(sys_info.total_usable_mem_size, t_mem);
	char dma_size[32] = {0};
	get_mem_size_desc(sys_info.sys_dma.size, dma_size);
	char res_mem[32] = {0};
	get_mem_size_desc(sys_info.phy_offset + sys_info.total_phy_mem_size - sys_info.allocable_phy_mem_top, res_mem);
	char allocable_mem[32] = {0};
	get_mem_size_desc(sys_info.allocable_phy_mem_top-sys_info.allocable_phy_mem_base, allocable_mem);
	char mmio_mem[32] = {0};
	get_mem_size_desc(sys_info.mmio.size, mmio_mem);

	printf(
		"machine            %s\n" 
		"arch               %s-%dk\n"
		"cores              %u\n"
		"phy mem size       %s\n"
		"usable mem size    %s\n"
		"kernel mem         phy:0x%08x%08x ~ 0x%08x%08x (%s/%s)\n"
		"sys_dma_base       phy:0x%08x%08x,V:0x%08x%08x (%s)\n"
		"reserved mem       phy:0x%08x%08x ~ 0x%08x%08x (%s)\n"
		"allocable mem      phy:0x%08x%08x ~ 0x%08x%08x (%s/%s)\n"
		"mmio               phy:0x%08x%08x,V:0x%08x%08x (%s)\n",
		sys_info.machine,
		sys_info.arch, sys_info.page_size/1024,
		sys_info.cores,
		phy_mem,
		t_mem,
		addr_hi(sys_info.phy_offset), addr_lo(sys_info.phy_offset),
		addr_hi(sys_info.sys_dma.phy_base), addr_lo(sys_info.sys_dma.phy_base),
		kfr_mem, kt_mem,
		addr_hi(sys_info.sys_dma.phy_base), addr_lo(sys_info.sys_dma.phy_base),
		addr_hi(sys_info.sys_dma.v_base), addr_lo(sys_info.sys_dma.v_base),
		dma_size,
		addr_hi(sys_info.allocable_phy_mem_top), addr_lo(sys_info.allocable_phy_mem_top),
		addr_hi(sys_info.phy_offset + sys_info.total_phy_mem_size), addr_lo(sys_info.phy_offset + sys_info.total_phy_mem_size),
		res_mem,
		addr_hi(sys_info.allocable_phy_mem_base), addr_lo(sys_info.allocable_phy_mem_base),
		addr_hi(sys_info.allocable_phy_mem_top), addr_lo(sys_info.allocable_phy_mem_top),
		fr_mem, allocable_mem,
		addr_hi(sys_info.mmio.phy_base), addr_lo(sys_info.mmio.phy_base),
		addr_hi(sys_info.mmio.v_base), addr_lo(sys_info.mmio.v_base),
		mmio_mem);

	return 0;
}
