TEST_OBJS = $(ROOT_DIR)/bin/test/test.o

TEST = $(TARGET_DIR)/$(ROOT_DIR)/bin/test

PROGS += $(TEST)
CLEAN += $(TEST_OBJS)

$(TEST): $(TEST_OBJS)
	$(LD) -Ttext=100 $(TEST_OBJS) -o $(TEST) $(LDFLAGS) -lewokc
	$(OBJDUMP) -D $(TEST) > $(BUILD_DIR)/asm/test.asm
