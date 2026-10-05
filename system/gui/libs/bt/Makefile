SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

# Header-only contract library: bt/bsp_bt.h is the interface every
# machine's libbsp implements (real HCI transport where the hardware
# exists, no-op stub elsewhere). The controller patchram image is
# machine-specific data and lives in that machine's own bsp sources, so
# there is no code left to archive here - just install the headers.
install:
	mkdir -p $(TARGET_DIR)/include
	cp -r include/* $(TARGET_DIR)/include

clean:
