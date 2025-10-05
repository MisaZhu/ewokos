all: basic_drivers
	@echo "all done."

basic_drivers:
	cd nulld;make
	cd logd;make
	cd ramfsd;make
	cd timerd;make
	cd netd;make

clean:
	cd nulld;make clean
	cd logd;make clean
	cd ramfsd;make clean
	cd timerd;make clean
	cd netd;make clean
