include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

ELFINFO_OBJS = elfinfo.o

ELFINFO = $(TARGET_DIR)/bin/elfinfo

$(ELFINFO): $(ELFINFO_OBJS) \
		$(BUILD_DIR)/lib/libelf.a \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(ELFINFO_OBJS) -o $(ELFINFO) $(LDFLAGS) -lelf -lewokc -lc_nano -lnosys

clean:
	rm -f $(ELFINFO_OBJS)

