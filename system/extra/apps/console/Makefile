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
         -fno-builtin-exit -fno-builtin-stdio

#CFLAGS +=  -g
CFLAGS +=  -O2

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include

CONSOLE_OBJS = console.o

CONSOLE = $(TARGET_DIR)/bin/console

$(CONSOLE): $(CONSOLE_OBJS) \
		$(TARGET_DIR)/lib/libewokc.a \
		$(TARGET_DIR)/lib/libx++.a \
		$(TARGET_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(CONSOLE_OBJS) -o $(CONSOLE) $(LDFLAGS)  -lx++ -lgraphxx -lewokc -lc -lcxx -lsconf -lconsole

clean:
	rm -f $(CONSOLE) $(CONSOLE_OBJS)

