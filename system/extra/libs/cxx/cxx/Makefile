AR = arm-none-eabi-ar
CXX = arm-none-eabi-g++
LD = arm-none-eabi-ld

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)

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

CXXFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_CXX_OBJS = src/cxx.o

LIB_CXX = $(TARGET_DIR)/lib/libcxx.a

$(LIB_CXX): $(LIB_CXX_OBJS)
	$(AR) rT $(LIB_CXX) $(LIB_CXX_OBJS)


clean:
	rm -f $(LIB_CXX_OBJS)
	rm -f $(LIB_CXX)

