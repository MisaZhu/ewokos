ifeq ($(ARCH),)
export ARCH=aarch64
endif

ifeq ($(HW),)
export HW=virt
endif

ifeq ($(EWOKOS_ROOT_DIR),)
EWOKOS_ROOT_DIR = ../../..
endif

SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system

#SDK_DIR= /usr/local/ewokos/$(ARCH)
#ROOTFS_DIR= /usr/local/ewokos/$(ARCH)/$(HW)

SDK_DIR= $(SYS_ROOT_DIR)/build/$(HW)
BUILD_DIR = $(SDK_DIR)

include $(SYS_ROOT_DIR)/platform/$(ARCH)/make.rule

LDFLAGS = -L$(SDK_DIR)/lib
CFLAGS += -I include -I $(SDK_DIR)/include
CXXFLAGS += -I include -I $(SDK_DIR)/include
