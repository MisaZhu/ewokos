SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

# Compile with the real arch so __ARM_ARCH picks the correct atomic helper
# implementations in syscalls.c (ldrex/strex on v6+, swp only for v5, whose
# kernels keep SWP enabled; ARMv7 resets with SCTLR.SW cleared).
ifeq ($(ARCH),arm)
ifeq ($(ARCH_VER),v7)
CFLAGS += -march=armv7-a
else ifeq ($(ARCH_VER),v6)
CFLAGS += -march=armv6
else ifeq ($(ARCH_VER),v5)
CFLAGS += -march=armv5te
endif
endif
 
LIBGLOSS_OBJS = $(ARCH)/syscalls.o $(ARCH)/compat.o

LIBGLOSS = $(TARGET_DIR)/lib/libgloss.a
LIBC = $(TARGET_DIR)/lib/libc.a

all: $(LIBGLOSS) $(LIBC)

$(LIBGLOSS): $(LIBGLOSS_OBJS)
	$(AR) rcs $(LIBGLOSS) $(LIBGLOSS_OBJS)

$(LIBC): $(LIBGLOSS_OBJS)
	$(AR) rcs $(LIBC) $(LIBGLOSS_OBJS)

clean:
	rm -f $(LIBGLOSS_OBJS)
	rm -f $(LIBGLOSS)
	rm -f $(LIBC)
