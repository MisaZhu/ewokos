all:arm riscv

arm:miyoo raspi1 raspi2.3 raspi4 versatilepb

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
