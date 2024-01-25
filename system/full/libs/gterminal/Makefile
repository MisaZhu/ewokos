include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_Ggterminal_OBJS = src/gterminal.o

Ggterminal = $(TARGET_DIR)/lib/libgterminal.a

$(Ggterminal): $(LIB_Ggterminal_OBJS)
	$(AR) rT $(Ggterminal) $(LIB_Ggterminal_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_Ggterminal_OBJS)
	rm -f $(Ggterminal)

