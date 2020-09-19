CAT_OBJS = $(ROOT_DIR)/bin/cat/cat.o

CAT = $(TARGET_DIR)/$(ROOT_DIR)/bin/cat

PROGS += $(CAT)
CLEAN += $(CAT_OBJS)

$(CAT): $(CAT_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(CAT_OBJS) -o $(CAT) $(LDFLAGS) -lewokc -lc
