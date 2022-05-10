all: basic_drivers
	@echo "all done."

basic_drivers:
	cd dispmand;make
	cd consoled;make
	cd dupd;make
	cd nulld;make
	cd ramfsd;make
	cd proc/state;make
	cd proc/sysinfo;make

clean:
	cd dispmand;make clean
	cd consoled;make clean
	cd dupd;make clean
	cd nulld;make clean
	cd ramfsd;make clean
	cd proc/state;make clean
	cd proc/sysinfo;make clean
