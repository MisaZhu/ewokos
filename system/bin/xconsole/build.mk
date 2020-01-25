XCONSOLE_OBJS = $(ROOT_DIR)/bin/xconsole/xconsole.o

XCONSOLE = $(TARGET_DIR)/$(ROOT_DIR)/bin/xconsole

PROGS += $(XCONSOLE)
CLEAN += $(XCONSOLE_OBJS)

$(XCONSOLE): $(XCONSOLE_OBJS)
	$(LD) -Ttext=100 $(XCONSOLE_OBJS) -o $(XCONSOLE) $(LDFLAGS) -lgraph -lconsole -lx -lsconf -lewokc -lc
