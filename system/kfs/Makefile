# tools

BUILD_DIR = build
ROOT_DIR = .

KERNEL_DIR=../../kernel
FROM_DIR=../build/rootfs

all:
	@mkdir -p $(BUILD_DIR)
	@cp -r $(FROM_DIR) $(BUILD_DIR)
	gcc -o ${BUILD_DIR}/genfs   genfs.c
	${BUILD_DIR}/genfs $(BUILD_DIR)/rootfs > $(KERNEL_DIR)/extra/loadinit/kfs/kfs_data.c
	rm -fr build
	@echo "all done."
