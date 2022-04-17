BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

all: 
	cd bin; make
	cd x; make
	cp -r data/* $(TARGET_DIR)/data

clean:	
	cd bin; make clean
	cd x; make clean
