include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_EXT2_OBJS = src/ext2fs.o

EXT2 = $(TARGET_DIR)/lib/libext2.a

$(EXT2): $(LIB_EXT2_OBJS)
	$(AR) rT $(EXT2) $(LIB_EXT2_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_EXT2_OBJS)
	rm -f $(EXT2)

