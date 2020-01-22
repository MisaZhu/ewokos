ECHO_OBJS = $(ROOT_DIR)/bin/echo/echo.o

ECHO = $(TARGET_DIR)/$(ROOT_DIR)/bin/echo

PROGS += $(ECHO)
CLEAN += $(ECHO_OBJS)

$(ECHO): $(ECHO_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(ECHO_OBJS) $(LIB_OBJS) -o $(ECHO) $(LDFLAGS)
