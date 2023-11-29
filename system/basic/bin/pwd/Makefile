include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

PWD_OBJS = pwd.o

PWD = $(TARGET_DIR)/bin/pwd

$(PWD): $(PWD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(PWD_OBJS) -o $(PWD) $(LDFLAGS) -lewokc -lc_nano -lnosys

clean:
	rm -f $(PWD_OBJS)

