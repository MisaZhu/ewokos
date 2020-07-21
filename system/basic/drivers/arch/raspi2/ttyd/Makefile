AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

BUILD_DIR = ../../../../../../build
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

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include

TTYD_OBJS = ttyd.o

TTYD = $(TARGET_DIR)/sbin/dev/raspi2/ttyd

$(TTYD): $(TTYD_OBJS) \
		$(TARGET_DIR)/lib/libewokc.a 
	$(LD) -Ttext=100 $(TTYD_OBJS) -o $(TTYD) $(LDFLAGS) -lewokc -lc
	$(OBJDUMP) -D $(TTYD) > $(BUILD_DIR)/asm/ttyd.asm

clean:
	rm -f $(TTYD_OBJS)
