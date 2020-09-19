PWD_OBJS = $(ROOT_DIR)/bin/pwd/pwd.o

PWD = $(TARGET_DIR)/$(ROOT_DIR)/bin/pwd

PROGS += $(PWD)
CLEAN += $(PWD_OBJS)

$(PWD): $(PWD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(PWD_OBJS) -o $(PWD) $(LDFLAGS) -lewokc -lc
