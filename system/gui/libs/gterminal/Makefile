SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_gterminal_OBJS = src/gterminal.o

libgterminal = $(TARGET_DIR)/lib/libgterminal.a

$(libgterminal): $(LIB_gterminal_OBJS)
	$(AR) rcs $(libgterminal) $(LIB_gterminal_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_gterminal_OBJS)
	rm -f $(libgterminal)

