BUILD_DIR = ../build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

all: 
	cd libs; make
	cd bin; make
	cd apps; make
#	@cp -r data ${TARGET_DIR}

clean:	
	cd libs; make clean
	cd bin; make clean
	cd apps; make clean
