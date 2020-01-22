PWD_OBJS = $(ROOT_DIR)/bin/pwd/pwd.o

PWD = $(TARGET_DIR)/$(ROOT_DIR)/bin/pwd

PROGS += $(PWD)
CLEAN += $(PWD_OBJS)

$(PWD): $(PWD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(PWD_OBJS) $(LIB_OBJS) -o $(PWD) $(LDFLAGS)
