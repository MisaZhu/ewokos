all: basic_drivers
	@echo "all done."

basic_drivers:
	cd nulld;make
	cd ramfsd;make
	cd timerd;make
	cd netd;make
	cd proc/state;make
	cd proc/sysinfo;make

clean:
	cd nulld;make clean
	cd ramfsd;make clean
	cd timerd;make clean
	cd netd;make clean
	cd proc/state;make clean
	cd proc/sysinfo;make clean
