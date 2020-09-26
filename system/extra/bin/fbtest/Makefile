CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

# flags
CFLAGS = -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-memset \
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

FB_TEST_OBJS = fbtest.o

FB_TEST = $(TARGET_DIR)/bin/fbtest

$(FB_TEST): $(FB_TEST_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a  \
		$(BUILD_DIR)/lib/libarch_vpb.a 
	$(LD) -Ttext=100 $(FB_TEST_OBJS) -o $(FB_TEST) $(LDFLAGS) -larch_bcm283x -larch_vpb -lewokc -lc

clean:
	rm -f $(FB_TEST_OBJS)

