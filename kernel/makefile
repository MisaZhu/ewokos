all:arm riscv

arm:miyoo rk3128 raspi1 raspi2.3 raspi4 versatilepb

riscv:virt nezha

miyoo:
	xmake f -p miyoo 
	xmake 

raspi1:
	xmake f -p raspi1
	xmake

raspi2.3:
	xmake f -p raspi2.3
	xmake

raspi4:
	xmake f -p raspi4
	xmake


versatilepb:
	xmake f -p versatilepb
	xmake

rk3128:
	xmake f -p rk3128
	xmake

virt:
	xmake f -p virt
	xmake

nezha:
	xmake f -p nezha
	xmake

clean:
	rm -rf build


run:raspi2.3
	xmake run

net_debug:versatilepb
	sudo qemu-system-arm  -cpu arm926 -M versatilepb -m 256M -serial mon:stdio -nic vmnet-bridged,ifname=en0 -kernel hardware/arm/versatilepb/kernel7.qemu.img -sd ../system/build/versatilepb/root.ext2	
