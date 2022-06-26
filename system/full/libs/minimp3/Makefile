include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_MINIMP3_OBJS = src/minimp3.o

MINIMP3 = $(TARGET_DIR)/lib/libminimp3.a

$(MINIMP3): $(LIB_MINIMP3_OBJS)
	$(AR) rT $(MINIMP3) $(LIB_MINIMP3_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_MINIMP3_OBJS)
	rm -f $(MINIMP3)

