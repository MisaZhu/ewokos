SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include -I./

SHELL_OBJS = shell.o gets.o cmd.o

SHELL_PROC = $(TARGET_DIR)/bin/shell

$(SHELL_PROC): $(SHELL_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(SHELL_OBJS) -o $(SHELL_PROC) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(SHELL_OBJS)
