#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <bsp/pcie.h>
#define HOST_MAX_FUNCTIONS 32
#define BRIDGE_GRANULE 0x100000ULL
#define PCIE4_MEM32_BUS 0x40000000ULL
#define PCIE4_MEM32_SIZE 0x10000000ULL
#define PCI_PRIMARY_BUS 0x18
static uint32_t cfg32(unsigned c,unsigned bus,unsigned dev,unsigned fn,unsigned reg) {
 (void)c;(void)fn;(void)reg;
 return bus==1 ? 0x00050201 : ((dev+2)<<16)|((dev+2)<<8)|2;
}
static uint64_t bar_to_phys(unsigned c,uint64_t addr) { return (c==1 ? 0x490000000ULL:0x410000000ULL)+addr-0x40000000; }
static uint64_t resource_align(uint64_t value, uint64_t alignment) {
	return (value + alignment - 1) & ~(alignment - 1);
}

static int plan_bus_resources(q8b_pcie_device_t *devices, int count,
		unsigned bus, unsigned depth, uint64_t *next,
		uint32_t windows[HOST_MAX_FUNCTIONS]) {
	if (depth >= 8)
		return -1;
	for (int i = 0; i < count; i++) {
		q8b_pcie_device_t *d = &devices[i];
		if (d->bus != bus)
			continue;
		for (unsigned bar = 0; bar < 6; bar++) {
			uint64_t size = d->bar_size[bar];
			if (!size)
				continue;
			uint64_t addr = resource_align(*next, size);
			if (addr + size > PCIE4_MEM32_BUS + PCIE4_MEM32_SIZE)
				return -1;
			d->bar[bar] = addr;
			d->bar_phys[bar] = bar_to_phys(d->controller, addr);
			*next = addr + size;
		}
	}
	for (int i = 0; i < count; i++) {
		q8b_pcie_device_t *d = &devices[i];
		if (d->bus != bus || (d->header_type & 0x7f) != 1)
			continue;
		uint32_t buses = cfg32(d->controller, bus, d->device,
				d->function, PCI_PRIMARY_BUS);
		unsigned secondary = (buses >> 8) & 0xff;
		unsigned subordinate = (buses >> 16) & 0xff;
		if (secondary <= bus || secondary > subordinate || subordinate >= 8)
			return -1;
		uint64_t begin = resource_align(*next, BRIDGE_GRANULE);
		*next = begin;
		if (plan_bus_resources(devices, count, secondary, depth + 1,
				next, windows))
			return -1;
		*next = resource_align(*next, BRIDGE_GRANULE);
		windows[i] = *next > begin ?
				(uint32_t)(((begin >> 16) & 0xfff0) | ((*next - 1) & 0xfff00000)) :
				0x0000fff0; /* base > limit: no child resources */
	}
	return *next <= PCIE4_MEM32_BUS + PCIE4_MEM32_SIZE ? 0 : -1;
}

int main(void) {
 q8b_pcie_device_t d[7]={0}; uint32_t w[32]={0}; uint64_t n=PCIE4_MEM32_BUS;
 d[0].bus=1;d[0].header_type=1;d[0].bar_size[0]=0x10000;
 for(int i=1;i<=3;i++){d[i].bus=2;d[i].device=i;d[i].header_type=1;d[i].bar_size[0]=0x10000;}
 for(int i=4;i<6;i++){d[i].bus=5;d[i].function=i-4;d[i].bar_size[0]=0x10000;d[i].bar_size[2]=0x80000;d[i].bar_size[4]=0x100000;}
 d[6].bus=3;d[6].bar_size[0]=0x200000;
 for(int i=0;i<7;i++)d[i].controller=1;
 assert(!plan_bus_resources(d,7,1,0,&n,w));
 assert(w[1]!=0xfff0 && w[2]==0xfff0 && w[3]!=0xfff0);
 for(int i=0;i<7;i++)for(int b=0;b<6;b++)if(d[i].bar_size[b]) {
 assert(d[i].bar[b]%d[i].bar_size[b]==0);
 for(int j=0;j<=i;j++)for(int k=0;k<6;k++)if(d[j].bar_size[k] && (j<i || k<b))assert(d[i].bar[b]>=d[j].bar[k]+d[j].bar_size[k] || d[j].bar[k]>=d[i].bar[b]+d[i].bar_size[b]);
 }
 n=PCIE4_MEM32_BUS;d[4].bar_size[4]=0x10000000;assert(plan_bus_resources(d,7,1,0,&n,w)<0);
 puts("resource plan: hierarchy, peer windows, alignment, overlap, exhaustion PASS");
}
