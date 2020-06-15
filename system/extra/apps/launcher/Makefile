AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

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

CFLAGS += -I $(TARGET_DIR)/include

LAUNCHER_OBJS = launcher.o

LAUNCHER = $(TARGET_DIR)/bin/launcher

$(LAUNCHER): $(LAUNCHER_OBJS) \
		$(TARGET_DIR)/lib/libewokc.a \
		$(TARGET_DIR)/lib/libconsole.a \
		$(TARGET_DIR)/lib/libgraph.a \
		$(TARGET_DIR)/lib/libx.a
	$(LD) -Ttext=100 $(LAUNCHER_OBJS) -o $(LAUNCHER) $(LDFLAGS) -lgraph -lupng -lx -lsconf -lconsole -lewokc -lc

clean:
	rm -f $(LAUNCHER_OBJS)

