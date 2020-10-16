AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)

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

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_ARCH_VPB_OBJS = src/framebuffer.o

ARCH_VPB = $(TARGET_DIR)/lib/libarch_vpb.a

$(ARCH_VPB): $(LIB_ARCH_VPB_OBJS)
	$(AR) rT $(ARCH_VPB) $(LIB_ARCH_VPB_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_ARCH_VPB_OBJS)
	rm -f $(ARCH_VPB)

