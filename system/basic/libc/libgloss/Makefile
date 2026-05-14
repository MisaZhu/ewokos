SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include
 
LIBGLOSS_OBJS = syscalls.o compat.o

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
