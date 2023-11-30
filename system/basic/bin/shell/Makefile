include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include -I./

SHELL_OBJS = shell.o gets.o cmd.o

SHELL_PROC = $(TARGET_DIR)/bin/shell

$(SHELL_PROC): $(SHELL_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(SHELL_OBJS) -o $(SHELL_PROC) $(LDFLAGS) -lewokc -lc_nano -lnosys

clean:
	rm -f $(SHELL_OBJS)
