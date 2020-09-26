TEST_OBJS = $(ROOT_DIR)/bin/test/test.o

TEST = $(TARGET_DIR)/$(ROOT_DIR)/bin/test

PROGS += $(TEST)
CLEAN += $(TEST_OBJS)

$(TEST): $(TEST_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(TEST_OBJS) -o $(TEST) $(LDFLAGS) -larch_bcm283x -lewokc -lc
