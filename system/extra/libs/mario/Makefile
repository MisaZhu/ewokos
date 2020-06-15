AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld

BUILD_DIR = ../../../build
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

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_MARIO_OBJS = src/mario_bc.o \
  src/mario_json.o \
  src/mario_lex.o \
  src/mario_vm.o \
  src/compiler.o

MARIO = $(TARGET_DIR)/lib/libmario.a

$(MARIO): $(LIB_MARIO_OBJS)
	$(AR) rT $(MARIO) $(LIB_MARIO_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_MARIO_OBJS)
	rm -f $(MARIO)

