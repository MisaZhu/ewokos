SYS_DIR=../../../
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_Ggterminal_OBJS = src/gterminal.o

Ggterminal = $(TARGET_DIR)/lib/libgterminal.a

$(Ggterminal): $(LIB_Ggterminal_OBJS)
	$(AR) rcs $(Ggterminal) $(LIB_Ggterminal_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_Ggterminal_OBJS)
	rm -f $(Ggterminal)

