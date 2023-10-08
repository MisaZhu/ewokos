include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_SCONF_OBJS = src/sconf.o

SCONF = $(TARGET_DIR)/lib/libsconf.a

$(SCONF): $(LIB_SCONF_OBJS)
	$(AR) rT $(SCONF) $(LIB_SCONF_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_SCONF_OBJS)
	rm -f $(SCONF)

