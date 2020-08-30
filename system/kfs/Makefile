# tools

BUILD_DIR = ../build
KERNEL_DIR=../../kernel
FROM_DIR=../build/rootfs

all:
	gcc -o ${BUILD_DIR}/genfs   genfs.c
	${BUILD_DIR}/genfs $(FROM_DIR) > $(KERNEL_DIR)/extra/loadinit/kfs/kfs_data.c
	@echo "all done."
