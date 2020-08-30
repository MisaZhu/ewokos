CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld

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

SNAKE_OBJS = snake.o

SNAKE = $(TARGET_DIR)/bin/x/snake

$(SNAKE): $(SNAKE_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfonts.a \
		$(BUILD_DIR)/lib/libgraph.a
	$(LD) -Ttext=100 $(SNAKE_OBJS) -o $(SNAKE) $(LDFLAGS) -lgraph -lfonts -lx -lewokc -lc

clean:
	rm -f $(SNAKE_OBJS)

