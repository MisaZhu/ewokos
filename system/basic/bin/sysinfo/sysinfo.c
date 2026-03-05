#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>

int main(int argc, char* argv[]) {
	sys_info_t sys_info;
	sys_state_t sys_state;

	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sys_info);
	syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)&sys_state);
	uint32_t fr_mem = sys_state.mem.free / (1024*1024);
	uint32_t kfr_mem = sys_state.mem.kfree / (1024*1024);
	uint32_t phy_mem = sys_info.total_phy_mem_size / (1024*1024);
	uint32_t t_mem = sys_info.total_usable_mem_size / (1024*1024);

	printf(
		"machine            %s\n" 
		"arch               %s\n"
		"cores              %d\n"
		"phy mem size       %d MB\n"
		"usable mem size    %d MB\n"
		"kernel mem         phy:0x%08x ~ 0x%08x (%d/%d MB)\n"
		"sys_dma_base       phy:0x%08x,V:0x%08x (%d MB)\n"
		"reserved mem       phy:0x%08x ~ 0x%08x (%d MB)\n"
		"allocable mem      phy:0x%08x ~ 0x%08x (%d/%d MB)\n",
		//"mmio_base          Phy:0x%08x, V:0x%08x (%d MB)\n",
		sys_info.machine,
		sys_info.arch,
		sys_info.cores,
		phy_mem,
		t_mem,
		sys_info.phy_offset, sys_info.sys_dma.phy_base , kfr_mem, (sys_info.sys_dma.phy_base - sys_info.phy_offset)/(1024*1024),
		sys_info.sys_dma.phy_base, sys_info.sys_dma.v_base, sys_info.sys_dma.size/(1024*1024),
		sys_info.allocable_phy_mem_top, sys_info.phy_offset + sys_info.total_phy_mem_size, (sys_info.total_phy_mem_size + sys_info.phy_offset -sys_info.allocable_phy_mem_top)/(1024*1024),
		sys_info.allocable_phy_mem_base, sys_info.allocable_phy_mem_top, fr_mem,(sys_info.allocable_phy_mem_top-sys_info.allocable_phy_mem_base)/(1024*1024));
		//sys_info.mmio.phy_base, sys_info.mmio.v_base, sys_info.mmio.size/(1024*1024));

	return 0;
}

