TASK = vim
TASK_OBJS = $(ARCH)/vim_util.o \
	$(ARCH)/vim_term.o \
	$(ARCH)/vim_input.o \
	$(ARCH)/vim_text.o \
	$(ARCH)/vim_cmd.o \
	$(ARCH)/vim_dcmd.o \
	$(ARCH)/vim_main.o \
	$(ARCH)/vim_syntax.o

SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	mkdir -p $(TARGET_DIR)/bin
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TARGET_DIR)/bin/$(TASK) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)