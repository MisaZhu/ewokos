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

LIB_ARCH_BCM283X_OBJS = src/framebuffer.o \
	src/gpio_actled.o \
	src/gpio.o \
	src/mailbox.o \
	src/mbox_actled.o \
	src/mini_uart.o \
	src/pl011_uart.o \
	src/spi.o

ARCH_BCM283X = $(TARGET_DIR)/lib/libarch_bcm283x.a

$(ARCH_BCM283X): $(LIB_ARCH_BCM283X_OBJS)
	$(AR) rT $(ARCH_BCM283X) $(LIB_ARCH_BCM283X_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_ARCH_BCM283X_OBJS)
	rm -f $(ARCH_BCM283X)


