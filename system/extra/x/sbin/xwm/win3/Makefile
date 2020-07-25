AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

BUILD_DIR = ../../../../../build
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

XWM_OBJS = xwm.o 

XWM = $(TARGET_DIR)/sbin/x/xwm_win3

$(XWM): $(XWM_OBJS) \
		$(TARGET_DIR)/lib/libewokc.a \
		$(TARGET_DIR)/lib/libx.a  \
		$(TARGET_DIR)/lib/libsconf.a  \
		$(TARGET_DIR)/lib/libfonts.a  \
		$(TARGET_DIR)/lib/libgraph.a 
	$(LD) -Ttext=100 $(XWM_OBJS) -o $(XWM) $(LDFLAGS) -lx -lgraph -lfonts -lsconf -lewokc -lc

clean:
	rm -f $(XWM_OBJS)
