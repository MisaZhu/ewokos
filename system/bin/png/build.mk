PNG_TEST_OBJS = $(ROOT_DIR)/bin/png/png.o

PNG_TEST = $(TARGET_DIR)/$(ROOT_DIR)/bin/png

PROGS += $(PNG_TEST)
CLEAN += $(PNG_TEST_OBJS)

$(PNG_TEST): $(PNG_TEST_OBJS)
	$(LD) -Ttext=100 $(PNG_TEST_OBJS) -o $(PNG_TEST) $(LDFLAGS) -lgraph -lx -lupng -lewokc -lc
