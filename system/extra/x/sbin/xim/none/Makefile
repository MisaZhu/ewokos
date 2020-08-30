CXX = arm-none-eabi-g++
LD = arm-none-eabi-ld

BUILD_DIR = ../../../../../build
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

#CXXFLAGS +=  -g
CXXFLAGS +=  -O2

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

XIM_OBJS = xim.o

XIM = $(TARGET_DIR)/sbin/x/xim_none

$(XIM): $(XIM_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XIM_OBJS) -o $(XIM) $(LDFLAGS) -lewokc -lc -lcxx

clean:
	rm -f $(XIM_OBJS)

