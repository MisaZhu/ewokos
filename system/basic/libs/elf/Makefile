include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_ELF_OBJS = src/elf.o

ELF = $(TARGET_DIR)/lib/libelf.a

$(ELF): $(LIB_ELF_OBJS)
	$(AR) rT $(ELF) $(LIB_ELF_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_ELF_OBJS)
	rm -f $(ELF)

