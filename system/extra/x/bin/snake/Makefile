include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

SNAKE_OBJS = snake.o

SNAKE = $(TARGET_DIR)/bin/x/snake

$(SNAKE): $(SNAKE_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libhash.a \
		$(BUILD_DIR)/lib/libgraph.a
	$(LD) -Ttext=100 $(SNAKE_OBJS) -o $(SNAKE) $(LDFLAGS) -lgraph -lttf -lfont -lhash -lx -lewokc -lc

clean:
	rm -f $(SNAKE_OBJS)

