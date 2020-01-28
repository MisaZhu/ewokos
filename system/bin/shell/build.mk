SHELL__OBJS = $(ROOT_DIR)/bin/shell/shell.o

SHELL_ = $(TARGET_DIR)/$(ROOT_DIR)/bin/shell

PROGS += $(SHELL_)
CLEAN += $(SHELL__OBJS)

$(SHELL_): $(SHELL__OBJS)
	$(LD) -Ttext=100 $(SHELL__OBJS) -o $(SHELL_) $(LDFLAGS) -lewokc -lc
