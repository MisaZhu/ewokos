AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

# flags
CFLAGS = -Wstrict-prototypes \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-memset \
         -fno-builtin-snprintf \
         -fno-builtin-sprintf \
         -fno-builtin-fprintf \
				 -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

#CFLAGS +=  -g
CFLAGS +=  -O2

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

XJOYSTICKD_OBJS = xjoystickd.o 

XJOYSTICKD = $(TARGET_DIR)/sbin/x/xjoystickd

$(XJOYSTICKD): $(XJOYSTICKD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a 
	$(LD) -Ttext=100 $(XJOYSTICKD_OBJS) -o $(XJOYSTICKD) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(XJOYSTICKD_OBJS)
