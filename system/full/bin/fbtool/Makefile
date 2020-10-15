CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

# flags
CFLAGS = -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-memtool \
         -fno-builtin-snprintf \
         -fno-builtin-sprintf \
         -fno-builtin-fprintf \
				 -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -fno-exceptions \
				 -std=c99

#CFLAGS +=  -g
CFLAGS +=  -O2

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

FB_TOOL_OBJS = fbtool.o

FB_TOOL = $(TARGET_DIR)/bin/fbtool

$(FB_TOOL): $(FB_TOOL_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a  \
		$(BUILD_DIR)/lib/libgraph.a  \
		$(BUILD_DIR)/lib/libfb.a  \
		$(BUILD_DIR)/lib/libarch_vpb.a 
	$(LD) -Ttext=100 $(FB_TOOL_OBJS) -o $(FB_TOOL) $(LDFLAGS) -larch_bcm283x -larch_vpb -lfb -lgraph -lewokc -lc

clean:
	rm -f $(FB_TOOL_OBJS)

