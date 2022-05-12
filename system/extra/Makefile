BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs

all: 
	cd x; make
	cp -r data/* $(TARGET_DIR)/data

clean:	
	cd x; make clean
