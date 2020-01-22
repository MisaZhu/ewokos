MARIO_OBJS = $(ROOT_DIR)/bin/mario/mario.o \
		$(ROOT_DIR)/bin/mario/shell.o

MARIO = $(TARGET_DIR)/$(ROOT_DIR)/bin/mario

PROGS += $(MARIO)
CLEAN += $(MARIO_OBJS)
#CFLAGS += -DMARIO_DEBUG

$(MARIO): $(MARIO_OBJS) $(LIB_OBJS) $(LIB_MARIO_OBJS)
	$(LD) -Ttext=100 $(MARIO_OBJS) $(LIB_OBJS) $(LIB_MARIO_OBJS) -o $(MARIO) $(LDFLAGS)
