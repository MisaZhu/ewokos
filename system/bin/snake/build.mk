SNAKE_OBJS = $(ROOT_DIR)/bin/snake/snake.o

SNAKE = $(TARGET_DIR)/$(ROOT_DIR)/bin/snake

PROGS += $(SNAKE)
CLEAN += $(SNAKE_OBJS)

$(SNAKE): $(SNAKE_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(SNAKE_OBJS) $(LIB_OBJS) -o $(SNAKE) $(LDFLAGS)
