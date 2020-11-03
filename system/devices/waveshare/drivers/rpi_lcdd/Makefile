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

RPI_LCDD_OBJS = fbd.o

RPI_LCDD = $(TARGET_DIR)/drivers/raspix/rpi_lcdd

$(RPI_LCDD): $(RPI_LCDD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libili9486.a  \
		$(BUILD_DIR)/lib/libxpt2046.a 
	$(LD) -Ttext=100 $(RPI_LCDD_OBJS) -o $(RPI_LCDD) $(LDFLAGS) -lxpt2046 -lili9486 -larch_bcm283x -lewokc -lc

clean:
	rm -f $(RPI_LCDD_OBJS)
