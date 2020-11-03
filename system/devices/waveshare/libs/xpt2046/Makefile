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

LIB_XPT2046_OBJS = src/xpt2046.o 

XPT2046 = $(TARGET_DIR)/lib/libxpt2046.a

$(XPT2046): $(LIB_XPT2046_OBJS)
	$(AR) rT $(XPT2046) $(LIB_XPT2046_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_XPT2046_OBJS)
	rm -f $(XPT2046)


