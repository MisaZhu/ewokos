AR = arm-none-eabi-ar
CXX = arm-none-eabi-g++
LD = arm-none-eabi-ld

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

# flags
CXXFLAGS = -marm \
         -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-memset \
         -fno-builtin-snprintf \
         -fno-builtin-sprintf \
         -fno-builtin-fprintf \
         -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
         -fno-rtti -fno-exceptions

#CFLAGS +=  -g
CXXFLAGS +=  -O2

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_GRAPHXX_OBJS = src/graphxx.o

LIB_X = $(TARGET_DIR)/lib/libgraphxx.a

$(LIB_X): $(LIB_GRAPHXX_OBJS) $(TARGET_DIR)/lib/libgraph.a $(TARGET_DIR)/lib/libfonts.a
	$(AR) rT $(LIB_X) $(LIB_GRAPHXX_OBJS) $(TARGET_DIR)/lib/libfonts.a $(TARGET_DIR)/lib/libgraph.a
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_GRAPHXX_OBJS)
	rm -f $(LIB_X)

