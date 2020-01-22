SHELL__OBJS = $(ROOT_DIR)/bin/shell/shell.o

SHELL_ = $(TARGET_DIR)/$(ROOT_DIR)/bin/shell

PROGS += $(SHELL_)
CLEAN += $(SHELL__OBJS)

$(SHELL_): $(SHELL__OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(SHELL__OBJS) $(LIB_OBJS) -o $(SHELL_) $(LDFLAGS)
